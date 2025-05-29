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
  matrixhandler(std::map<VERTEX, std::map<VERTEX,uint64_t>* > *COCCUR, GRAPH *_g, uint64_t *_pulls)
      : COCCUR_(COCCUR), g(_g), PULLS(_pulls){
    mb[0].process = [this] (matrix_pkt pkt, int sender_pe) {
      this->put(pkt, sender_pe);
    };
  }

private:
  std::map<VERTEX, std::map<VERTEX,uint64_t>* > *COCCUR_;
  GRAPH *g;
  uint64_t *PULLS;
  
  void put(matrix_pkt pkt, int sender_pe) {
      (*PULLS)++;
      uint64_t local_col = g->GMapper->to_local(pkt.col);
      if(COCCUR_->find(pkt.row) == COCCUR_->end()) {
          std::map<VERTEX,uint64_t> *itr = new std::map<VERTEX,uint64_t>;
          itr->insert(std::make_pair(local_col, pkt.count));
          COCCUR_->insert(std::make_pair(pkt.row, itr));
      }
      else {
        std::map<VERTEX, uint64_t> *itr = COCCUR_->find(pkt.row)->second;
        if(itr->find(local_col) !=  itr->end()) {
          itr->find(local_col)->second += pkt.count;
        }
        else {
          itr->insert(std::make_pair(local_col, pkt.count));
        }
      }
  }
};

uint64_t PUSHS = 0;
uint64_t PULLS = 0;

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
      mh->send(0, curr_pkt, curr_pkt.col%THREADS);
      PUSHS++;

      // make an inverse pkt of the curr_pkt and send it to dest
      matrix_pkt inv_pkt{curr_pkt.col, curr_pkt.row, curr_pkt.count};
      mh->send(0, inv_pkt, inv_pkt.col%THREADS);
      PUSHS++;

      // update curr_pkt
      curr_pkt.row = pkt_vec[i].row;
      curr_pkt.col = pkt_vec[i].col;
      curr_pkt.count = 1;
    }
  }

  pkt_vec.clear();
  curr_value = 0;
}

void INIT_COCCUR(std::map<VERTEX, std::map<VERTEX, uint64_t>*> *COCCUR,
  uint64_t global_num_nodes) {
  // for(uint64_t ik = 0; ik < global_num_nodes; ik++) {
  //     std::map<VERTEX, uint64_t> *temp = new std::map<VERTEX, uint64_t>;
  //     COCCUR->insert(std::make_pair(ik, temp));
  // }
}

template<typename T>
inline void _CONSTRUCT(GRAPH *g, std::vector<std::vector<VERTEX>> *RRsets, 
    std::unordered_map<uint64_t, uint64_t> *_IMMvisited, std::set<uint64_t> *influencers, uint64_t k,
    std::map<VERTEX, std::map<VERTEX, uint64_t>*> *COCCUR, T *count,
     uint64_t, uint64_t) {

      count->resize(g->G->size(), 0);
      for(auto x: *(_IMMvisited)) {
        (*count)[g->GMapper->to_local(x.first)] = x.second;
      }

      if(RRsets->size() > 0) {
        double t1 = wall_seconds();
        matrixhandler* mh = new matrixhandler(COCCUR, g, &PULLS);
        //mh->profile_conveyors = true;
        hclib::finish([=]() {
            mh->start();
            std::vector<matrix_elem> pkt_vec;
            pkt_vec.reserve(BATCH_SIZE + 100); 
            uint64_t cutoff_value = BATCH_SIZE;
            uint64_t curr_value = 0;
            for (auto vec : (*RRsets)) {// std::vector<uint64_t> set 
              for (int i = 0; i < vec.size(); i++) {
                  uint64_t node_i = vec[i];
                  for (int j = i+1; j < vec.size(); j++) {
                    uint64_t node_j = vec[j];
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
        //mh->print_profiling("construction of coccur");
        delete mh;
        // uint64_t max_load = lgp_reduce_max_l(PUSHS);
        // uint64_t mean_load = lgp_reduce_add_l(PUSHS)/THREADS;
        // uint64_t max_load_re = lgp_reduce_max_l(PULLS);
        // uint64_t mean_load_re = lgp_reduce_add_l(PULLS)/THREADS;
        // // FILE *f_load_file = fopen("load-imbalance.txt", "a");
        // double load_imb = (double) (max_load - mean_load) / (double) max_load;
        // // T0_fprintf(f_load_file, "%lf\n", load_imb);
        // // fclose(f_load_file);
        // uint64_t pushes_global = lgp_reduce_add_l(PUSHS);
        // T0_fprintf(stderr, "[[Time]] in matrix construction: %lf, tot-pushs: %ld, pushs/PE: %ld\n", wall_seconds() - t1, pushes_global, PUSHS);
        // T0_fprintf(stderr, "max_pushes: %ld\n", max_load);
        // T0_fprintf(stderr, "avg_pushes: %ld\n", mean_load);
        // T0_fprintf(stderr, "max_recv: %ld\n", max_load_re);
        // T0_fprintf(stderr, "avg_recv: %ld\n", mean_load_re);
      }
}

template<typename T>
inline void _DELETION(GRAPH *g, std::vector<std::vector<VERTEX>> *RRsets, 
    std::unordered_map<uint64_t, uint64_t> *_IMMvisited, std::set<uint64_t> *influencers, uint64_t k,
    std::map<VERTEX, std::map<VERTEX, uint64_t>*> *COCCUR, T *count,
     uint64_t, uint64_t, VERTEX curr_influencer) {
  if (curr_influencer%THREADS == MYTHREAD) {
    influencers->insert(curr_influencer);
    (*count)[g->GMapper->to_local(curr_influencer)] = 0;
  } 
  if(COCCUR->find(curr_influencer) != COCCUR->end()) {
    std::map<VERTEX, uint64_t> *ht = COCCUR->find(curr_influencer)->second;
    for(auto g1: *(ht)) {
      if((*count)[g1.first] > g1.second) {
        (*count)[g1.first] = (*count)[g1.first] - g1.second;
      }
      else {
        (*count)[g1.first] = 0;
      }
    }
  }
}

template<typename T>
inline void _MAX_LOCAL(GRAPH *g, T *count, uint64_t *local_max_size, uint64_t *dest_vertex) {
  for(uint64_t tracker = 0; tracker < count->size(); tracker++) {
      if(*local_max_size < (*count)[tracker]) {
          *local_max_size = (*count)[tracker];
          *dest_vertex = g->GMapper->to_global(tracker);
      }
  }
}