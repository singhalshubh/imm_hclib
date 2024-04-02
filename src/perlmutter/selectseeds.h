#ifdef IMM_SELECTOR

class IMMSelector: public hclib::Selector<1, IMMpckt> {

    std::set<uint64_t> *influencers;
    std::map<uint64_t, uint64_t> *IMMvisited;
    std::map<uint64_t, std::set<PP>*> *visited;
    std::set<PP> *tags;
    std::set<PP> *cur_tags;
    
    void process(IMMpckt appPkt, int sender_rank) {
        cur_tags->insert(std::make_pair(appPkt.bfs_iteration_index, appPkt.origin_pe));
        tags->insert(std::make_pair(appPkt.bfs_iteration_index, appPkt.origin_pe));
    }

  public:
    IMMSelector(std::map<uint64_t, uint64_t> *_IMMvisited, std::map<uint64_t, std::set<PP>*> *_visited,
             std::set<uint64_t> *_influencers, std::set<PP> *_tags, std::set<PP> *_cur_tags): 
            hclib::Selector<1, IMMpckt>(true), IMMvisited(_IMMvisited), visited(_visited), influencers(_influencers), tags(_tags), cur_tags(_cur_tags) {
        mb[0].process = [this](IMMpckt appPkt, int sender_rank) { this->process(appPkt, sender_rank); };
    }
};

#endif

uint64_t PERFORM_IMM(GRAPH *g, std::map<uint64_t, std::set<TAG>*> *visited, 
    std::set<uint64_t> *influencers) {
    
    uint64_t num_nodes = g->total_num_nodes;
    uint64_t k = g->cfg->k;
    std::set<TAG> *tags = new std::set<TAG>;
    std::set<TAG> *cur_tags = new std::set<TAG>;
    std::map<uint64_t, uint64_t> *IMMvisited = new std::map<uint64_t, uint64_t>;
    for(auto v: *visited) {
        IMMvisited->insert(std::make_pair(v.first, v.second->size()));
    }
    uint64_t max_coverage = 0;
    while(k > 0) {
        uint64_t local_max_size = 0;
        uint64_t dest_vertex;
        // for(auto itr: *IMMvisited) {
        //     fprintf(stderr, "%d, %d\n", itr.first, itr.second);
        // }
        for(auto itr: *IMMvisited) {
            if(local_max_size < (itr.second)) {
                local_max_size = (itr.second);
                dest_vertex = itr.first;
            }
        }
        uint64_t global_max_size = lgp_reduce_max_l(local_max_size);
        if(global_max_size == 0) {
            return max_coverage;
        }
        max_coverage += global_max_size;
        uint64_t infl = std::numeric_limits<int64_t>::max(); // Note int64_t!
        if(global_max_size == local_max_size) {
            infl = dest_vertex;
        }
        // needs local->global conversion!
        uint64_t global_min_vertice = lgp_reduce_min_l(infl);
        if(global_max_size == local_max_size && dest_vertex == global_min_vertice) {
            influencers->insert(global_min_vertice);
        }
        lgp_barrier();
        #ifdef IMM_SELECTOR
            IMMSelector* immselector = new IMMSelector(IMMvisited, visited, influencers, tags, cur_tags);
            hclib::finish([=]() {
                if(global_max_size == local_max_size && dest_vertex == global_min_vertice) {
                    set_difference(visited->find(dest_vertex)->second->begin(), 
                        visited->find(dest_vertex)->second->end(), tags->begin(), tags->end(), cur_tags);
                    IMMpckt sendpckt;
                    for(auto ii: *(cur_tags)) {
                        sendpckt.bfs_iteration_index = ii.first;
                        sendpckt.origin_pe = ii.second;
                        #ifdef TRACE
                            if(g->cfg->STATE == ESTIMATE_THETA) g->cfg->TRACE_SENDS[2] += (THREADS);
                            else if(g->cfg->STATE == FINAL) g->cfg->TRACE_SENDS[3] += (THREADS);
                        #endif
                        for(int pe = 0; pe < THREADS; pe++) {
                            immselector->send(0, sendpckt, pe);
                        }
                    }
                }       
                immselector->done(0);
            });
            lgp_barrier();
            delete immselector;
        #else
            uint64_t sizeOfArray = 2*global_max_size + 1;
            uint64_t *broadAr = (uint64_t *) shmem_malloc(sizeOfArray* sizeof(uint64_t));
            uint64_t *dest = (uint64_t *) shmem_malloc(sizeOfArray* sizeof(uint64_t));
            if(global_max_size == local_max_size && dest_vertex == global_min_vertice) {
                uint64_t tracker = 0;
                set_difference(visited->find(dest_vertex)->second->begin(), 
                        visited->find(dest_vertex)->second->end(), tags->begin(), tags->end(), cur_tags);
                broadAr[tracker] = 2*cur_tags->size()+1; 
                //fprintf(stderr, "%d, %d, %d\n", global_max_size, cur_tags->size(), sizeOfArray);
                tracker++;
                for(auto ii: *(cur_tags)) {
                    broadAr[tracker] = ii.first;
                    broadAr[tracker+1] = ii.second;
                    tracker = tracker + 2;
                }
                assert(tracker <= sizeOfArray);
            }
            static long pSync[SHMEM_BCAST_SYNC_SIZE];
            for (int i = 0; i < SHMEM_BCAST_SYNC_SIZE; i++)
                pSync[i] = SHMEM_SYNC_VALUE;
            lgp_barrier();
            shmem_broadcast64(dest, broadAr, sizeOfArray, global_min_vertice%THREADS, 0, 0, THREADS, pSync);
            //T0_fprintf(stderr, "Destination:%d", dest[0]);
            if(global_max_size == local_max_size && dest_vertex == global_min_vertice) {
                for(uint64_t tracker = 1; tracker < broadAr[0]; tracker=tracker+2) {
                    cur_tags->insert(std::make_pair(broadAr[tracker], broadAr[tracker+1]));
                    tags->insert(std::make_pair(broadAr[tracker], broadAr[tracker+1]));
                }
            }
            else {
                for(uint64_t tracker = 1; tracker < dest[0]; tracker=tracker+2) {
                    cur_tags->insert(std::make_pair(dest[tracker], dest[tracker+1]));
                    tags->insert(std::make_pair(dest[tracker], dest[tracker+1]));
                }
            }
            lgp_barrier();
            shmem_free(broadAr);
            shmem_free(dest);
        #endif
        std::map<uint64_t, std::set<TAG>*> :: iterator it;
        for(it = visited->begin(); it != visited->end(); it++) {
            uint64_t *measure = &(IMMvisited->find(it->first)->second);
            if(*measure == 0) continue;
            uint64_t *intersection_size = new uint64_t;
            set_intersection (it->second->begin(), it->second->end(), 
                    cur_tags->begin(), cur_tags->end(), intersection_size );
            *measure = *measure - *intersection_size;
            assert((*measure) >= 0);
            delete intersection_size;
        }
        cur_tags->clear();    
        k--;
    }
    lgp_barrier();
    delete tags;
    delete IMMvisited;
    return max_coverage;
}
