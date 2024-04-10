#define BATCH_SIZE 1000

typedef struct count_pkt_t {
  uint64_t node;
  uint64_t count;
} count_pkt;

typedef struct matrix_pkt_t {
  uint64_t row;
  uint64_t col;
  uint64_t count;
} matrix_pkt;

typedef struct matrix_elem_t {
  uint64_t row;
  uint64_t col;
} matrix_elem;

class matrixhandler: public hclib::Selector<1, matrix_pkt> {
public:
  matrixhandler(std::unordered_map<DST, std::unordered_map<SRC,uint64_t>* > *COVAR, std::map<uint64_t, uint64_t> *g2lidx)
      : COVAR_(COVAR), g2lidx_(g2lidx){
    mb[0].process = [this] (matrix_pkt pkt, int sender_pe) {
      this->put(pkt, sender_pe);
    };
  }

private:
  std::unordered_map<DST, std::unordered_map<SRC,uint64_t>* > *COVAR_;
  std::map<uint64_t, uint64_t> *g2lidx_;
  
  void put(matrix_pkt pkt, int sender_pe) {
    
    uint64_t local_col = (*g2lidx_)[pkt.col];
    std::unordered_map<SRC,uint64_t> *itr = COVAR_->find(pkt.row)->second;
    if(itr->find(local_col) !=  itr->end()) {
      itr->find(local_col)->second += pkt.count;
    }
    else {
      itr->insert(std::make_pair(local_col, pkt.count));
    }
    
    // #ifdef DEBUG
    //   assert(pkt.row != pkt.col); // should not be diagonal elements
    //   assert(pkt.row < COVAR_->size());
    //   assert(pkt.col < COVAR_->size());
    //   if(local_col >= (*COVAR_)[0].size()) {
    //     std::cout << MYTHREAD << " " << pkt.row << " " << pkt.col << " " << local_col << std::endl;
    //   }
    //   assert(local_col < (*COVAR_)[0].size());
    // #endif
  }
};

void flush_messages(std::vector<matrix_elem> &pkt_vec, uint64_t &curr_value, matrixhandler* mh) {
  std::sort(pkt_vec.begin(), pkt_vec.end(), [](const matrix_elem& a, const matrix_elem& b) {
    return std::tie(a.row, a.col) < std::tie(b.row, b.col);
  });

  matrix_pkt curr_pkt{pkt_vec[0].row, pkt_vec[0].col, 1};

  for (int i = 1; i < pkt_vec.size(); i++) {
    if ((pkt_vec[i].row == curr_pkt.row) && (pkt_vec[i].col == curr_pkt.col)) {
      curr_pkt.count++;
    } else {
      // send curr_pkt to destination
      mh->send(0, curr_pkt, pe(curr_pkt.col));

      // make an inverse pkt of the curr_pkt and send it to dest
      matrix_pkt inv_pkt{curr_pkt.col, curr_pkt.row, curr_pkt.count};
      mh->send(0, inv_pkt, pe(inv_pkt.col));

      // update curr_pkt
      curr_pkt.row = pkt_vec[i].row;
      curr_pkt.col = pkt_vec[i].col;
      curr_pkt.count = 1;
    }
  }

  pkt_vec.clear();
  curr_value = 0;
}


uint64_t PERFORM_IMM(GRAPH *g, std::map<BFS_ITERATION_INDEX, std::vector<DST>*> *RRsets, 
    std::map<uint64_t, uint64_t> *_IMMvisited, std::set<uint64_t> *influencers, std::unordered_map<DST, std::unordered_map<SRC,uint64_t>*> *COVAR) {
    uint64_t local_chunk_size = g->global2localcolidx->size();
    std::vector<uint64_t> *count = new std::vector<uint64_t>;
    count->resize(local_chunk_size, 0);
  
    for(auto x: *(_IMMvisited)) {
      (*count)[g->global2localcolidx->find(x.first)->second] = x.second;
    }
    
    matrixhandler* mh = new matrixhandler(COVAR, g->global2localcolidx);
    hclib::finish([=]() {
        mh->start();
        std::vector<matrix_elem> pkt_vec;
        pkt_vec.reserve(BATCH_SIZE + 100); 
        uint64_t cutoff_value = BATCH_SIZE;
        uint64_t curr_value = 0;
        for (auto &sett : (*RRsets)) { 
        for (int i = 0; i < sett.second->size(); i++) {
            uint64_t node_i = (*(sett.second))[i];
            for (int j = i+1; j < sett.second->size(); j++) {
              uint64_t node_j = (*(sett.second))[j];
              pkt_vec.push_back(matrix_elem{node_i, node_j});
              curr_value++;
            }
        }
        if (curr_value >= cutoff_value) {
            flush_messages(pkt_vec, curr_value, mh);
        }
        }
        flush_messages(pkt_vec, curr_value, mh);
        mh->done(0);
    });
    shmem_barrier_all();
    delete mh;
    uint64_t k = g->cfg->k;
    uint64_t max_coverage = 0;
    for (int i = 0; i < k; i++) {
        uint64_t local_max_size = 0;
        uint64_t dest_vertex;
        for(uint64_t tracker = 0; tracker < count->size(); tracker++) {
            if(local_max_size < (*count)[tracker]) {
                local_max_size = (*count)[tracker];
                dest_vertex = (*(g->local2globalcolidx))[tracker];
            }
        }
        uint64_t global_max_size = lgp_reduce_max_l(local_max_size);
        if(global_max_size == 0) {
            break;
        }
        max_coverage += global_max_size;
        uint64_t infl = std::numeric_limits<int64_t>::max();
        if(global_max_size == local_max_size) {
            infl = dest_vertex;
        }
        uint64_t curr_influencer = lgp_reduce_min_l(infl);
        lgp_barrier();
        if (pe(curr_influencer) == MYTHREAD) {
          influencers->insert(curr_influencer);
          (*count)[g->global2localcolidx->find(curr_influencer)->second] = 0;
        } 
        std::unordered_map<SRC,uint64_t> *ht = COVAR->find(curr_influencer)->second;
        for(auto g: *(ht)) {
          (*count)[g.first] = (*count)[g.first] - g.second; 
        }
    }
    return max_coverage;
}

void _defineMappings(GRAPH *g) {
    uint64_t idx = 0;
    for (uint64_t node = 0; node < g->total_num_nodes; node++) {
      if (pe(node) == MYTHREAD) {
        g->global2localcolidx->insert(std::make_pair(node, idx));
        idx++;
      }
    }
    for (auto x: *(g->global2localcolidx)) {
      g->local2globalcolidx->insert(std::make_pair(x.second, x.first));
    }
}