/*
This module implements the global index ranging from [0..num_nodes-1]
This ensures that if we select any random value from [0..num_nodes-1], we would
obtain the corresponding unique vertice, which is the primary aim of indexing vertice to
be uniquely identified when pciked randomly. 
Note: We chose unique numbering of vertices as desrired choice and not lexicographical
order of vertices as requirement of application is only limited to: "pick a vertice uniquely at random",
under IC/LT model of probability!
Author: Shubhendra Pal Singhal (Georgia Institute of Technology)
*/
#ifdef PREFIX

class reIndexSelector: public hclib::Selector<1, uint64_t> {
    
    std::vector<uint64_t> *tot_nodes;
    
    void process(uint64_t appPkt, int sender_rank) {
        (*tot_nodes)[sender_rank] = appPkt;
    }

  public:
    reIndexSelector(std::vector<uint64_t> *_tot_nodes): 
            hclib::Selector<1, uint64_t>(true), tot_nodes(_tot_nodes) {
        mb[0].process = [this](uint64_t appPkt, int sender_rank) { this->process(appPkt, sender_rank); };
    }
};

void whichVertice(int rank, std::vector<uint64_t> *tot_nodes, std::pair<uint64_t, uint64_t> *pp) {
    auto lower = std::lower_bound(tot_nodes->begin(), tot_nodes->end(), rank);
    pp->first = std::distance(tot_nodes->begin(), lower);
    pp->second = pp->first != 0? (rank - (*tot_nodes)[pp->first - 1] - 1):(rank - 1);
    #ifdef MACRO_DEBUG
        fprintf(stderr, "R: %d, %ld, %ld\n", rank, pp->first, pp->second);
    #endif
}

uint64_t init_renumbering(std::vector<uint64_t> *tot_nodes, uint64_t size) {

    reIndexSelector* rei = new reIndexSelector(tot_nodes);
    hclib::finish([=]() { 
        if(size > 0) {
            for(int i = 0; i < THREADS; i++) {
                uint64_t pckt = size;
                rei->send(0, pckt, i);
            }
        }
        rei->done(0);
    });
    lgp_barrier();
    for(int tracker = 1; tracker < THREADS; tracker++) {
        (*tot_nodes)[tracker] += (*tot_nodes)[tracker-1]; 
    }
    return (*tot_nodes)[THREADS-1];
}

/*Sorting renumbering module*/

#else

#define NO_STORE_ADJ 0
#define STORE_ADJ 1

struct _sortRenumberingPacket {
    uint64_t src;
    uint64_t dst;
    double weight;
    bool type;
};

class _sortRenumberingSelector: public hclib::Selector<1, _sortRenumberingPacket> {

    std::map<uint64_t, std::map<uint64_t, double>*> *adjacencyMap;

    void process(_sortRenumberingPacket appPkt, int sender_rank) {
        if(appPkt.type == STORE_ADJ) {
            if(adjacencyMap->find(appPkt.src) != adjacencyMap->end()) {
                std::map<uint64_t, double> *destAndWeight = adjacencyMap->find(appPkt.src)->second;
                if(destAndWeight->find(appPkt.dst) != destAndWeight->end()) {
                    destAndWeight->find(appPkt.dst)->second += appPkt.weight;
                }
                else {
                    destAndWeight->insert(std::make_pair(appPkt.dst, appPkt.weight));
                }
            }
            else {
                std::map<uint64_t, double>* adj = new std::map<uint64_t, double>;
                adj->insert(std::make_pair(appPkt.dst, appPkt.weight));
                adjacencyMap->insert(std::make_pair(appPkt.src, adj));
            }
        }
        else if(appPkt.type == NO_STORE_ADJ) {
            if(adjacencyMap->find(appPkt.src) == adjacencyMap->end()) {
                std::map<uint64_t, double>* adj = new std::map<uint64_t, double>;
                adjacencyMap->insert(std::make_pair(appPkt.src, adj));
            }
        }
    }

  public:
    _sortRenumberingSelector( std::map<uint64_t, std::map<uint64_t, double>*> *_adjacencyMap): 
            hclib::Selector<1, _sortRenumberingPacket>(true), adjacencyMap(_adjacencyMap)  {
        mb[0].process = [this](_sortRenumberingPacket appPkt, int sender_rank) { this->process(appPkt, sender_rank); };
    }
};

void _sortRenumbering(std::map<uint64_t, std::map<uint64_t, double>*> *oldadjacencyMap, uint64_t num_nodes,
                        std::map<uint64_t, std::map<uint64_t, double>*> *adjacencyMap, 
                        std::map<uint64_t, uint64_t> *globalToUserVerticeMap, std::map<uint64_t, uint64_t> *userToGlobalIdMap) {
    struct timeval tm, tm1;
    gettimeofday(&tm, NULL);
    uint64_t k = 0;
    std::map<uint64_t, std::map<uint64_t, double>*> *tempadjacencyMap = new std::map<uint64_t, std::map<uint64_t, double>*>;
    for(auto itr: *oldadjacencyMap) {
        std::map<uint64_t, double>* destAndWeight = new std::map<uint64_t, double>;
        for(auto ii: *(itr.second)) {
            destAndWeight->insert(std::make_pair(ii.first, ii.second));
        }
        tempadjacencyMap->insert(std::make_pair(itr.first, destAndWeight));
    }
    lgp_barrier();
    while(k < num_nodes) {
        uint64_t local_min_vertice = std::numeric_limits<int64_t>::max();
        for(auto itr: *oldadjacencyMap) {
            if(local_min_vertice > (itr.first)) {
                local_min_vertice = (itr.first);
            }
        }
        uint64_t global_min_vertice = lgp_reduce_min_l(local_min_vertice);
        if(global_min_vertice == std::numeric_limits<int64_t>::max()) {
            break;
        }
        oldadjacencyMap->erase(global_min_vertice);
        userToGlobalIdMap->insert(std::make_pair(global_min_vertice, k));
        if(k%THREADS == MYTHREAD) {
            // Store the global to local conversion
            globalToUserVerticeMap->insert(std::make_pair(k, global_min_vertice));
        }
        k++;
        lgp_barrier();
    }
    lgp_barrier();

    _sortRenumberingSelector *srs = new _sortRenumberingSelector(adjacencyMap); 
    hclib::finish([=]() {
        for(auto itr: *tempadjacencyMap) {
            if(itr.second->empty()) {
                _sortRenumberingPacket sendpckt;
                sendpckt.src = userToGlobalIdMap->find(itr.first)->second;
                sendpckt.type = NO_STORE_ADJ;
                srs->send(0, sendpckt, (sendpckt.src)%THREADS);
            }
            else {
                for(auto ngbr: *(itr.second)) {
                    _sortRenumberingPacket sendpckt;
                    sendpckt.src = userToGlobalIdMap->find(itr.first)->second;
                    sendpckt.dst = userToGlobalIdMap->find(ngbr.first)->second;
                    sendpckt.weight = ngbr.second;
                    sendpckt.type = STORE_ADJ;
                    srs->send(0, sendpckt, (sendpckt.src)%THREADS);
                }
            }
        }     
        srs->done(0);
    });
    lgp_barrier();
    delete srs;
    delete tempadjacencyMap;
    lgp_barrier();
    gettimeofday(&tm1, NULL);
    timersub(&tm1, &tm, &tm1);
    shmem_barrier_all();
    #ifdef DEBUG
        T0_fprintf(stderr,"Time for sorting and making global ids:  %8.3lf seconds\n", tm1.tv_sec + (double)tm1.tv_usec/(double)1000000);
    #endif
}

#endif