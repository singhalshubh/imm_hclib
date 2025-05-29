uint64_t PERFORM_IMM(GRAPH *G, CUSTOMAP<VERTEX, std::set<TAG>*> *visited, 
    std::set<VERTEX> *influencers, uint64_t k) {
    
    /*  dest vertex obtained is global, we follow global here as well.
    */

    uint64_t num_nodes = G->global_num_nodes;
    std::set<TAG> *tags = new std::set<TAG>;
    std::set<TAG> *cur_tags = new std::set<TAG>;
    CUSTOMAP<VERTEX, uint64_t> *IMMvisited = new CUSTOMAP<VERTEX, uint64_t>;
    for(auto v: *visited) {
        IMMvisited->insert(std::make_pair(v.first, v.second->size()));
    }
    uint64_t max_coverage = 0;
    while(k > 0) {
        uint64_t local_max_size = 0;
        VERTEX dest_vertex;
        // for(auto itr: *IMMvisited) {
        //     fprintf(stderr, "%d, %d\n", itr.first, itr.second);
        // }
        // fprintf(stderr, "\n");
        for(auto itr: *IMMvisited) {
            if(local_max_size < (itr.second)) {
                local_max_size = (itr.second);
                dest_vertex = itr.first;
            }
        }
        uint64_t global_max_size = lgp_reduce_max_l(local_max_size);
        if(global_max_size == 0) {
            break;
        }
        //T0_fprintf(stderr, "Max tag size: %ld\n", global_max_size);
        max_coverage += global_max_size;
        VERTEX infl = std::numeric_limits<int64_t>::max(); // Note int64_t!
        if(global_max_size == local_max_size) {
            infl = dest_vertex;
        }
        // needs local->global conversion!
        VERTEX global_min_vertice = lgp_reduce_min_l(infl);
        if(global_max_size == local_max_size && dest_vertex == global_min_vertice) {
            influencers->insert(global_min_vertice);
        }
        uint64_t sizeOfArray = global_max_size + 1;
        uint64_t *broadAr = (uint64_t *) shmem_malloc(sizeOfArray* sizeof(uint64_t));
        uint64_t *dest = (uint64_t *) shmem_malloc(sizeOfArray* sizeof(uint64_t));
        if(global_max_size == local_max_size && dest_vertex == global_min_vertice) {
            uint64_t tracker = 0;
            set_difference(visited->find(dest_vertex)->second->begin(), 
                    visited->find(dest_vertex)->second->end(), tags->begin(), tags->end(), cur_tags);
            broadAr[tracker] = cur_tags->size()+1; 
            tracker++;
            for(auto ii: *(cur_tags)) {
                broadAr[tracker] = ii;
                tracker = tracker + 1;
            }
            #ifdef DEBUG
                assert(tracker <= sizeOfArray);
            #endif
        }
        lgp_barrier(); 
        shmem_broadcast64(dest, broadAr, sizeOfArray, G->GMapper->to_host(global_min_vertice), 0, 0, THREADS, NULL);
        if(global_max_size == local_max_size && dest_vertex == global_min_vertice) {
            for(uint64_t tracker = 1; tracker < broadAr[0]; tracker++) {
                cur_tags->insert(broadAr[tracker]);
                tags->insert(broadAr[tracker]);
            }
        }
        else {
            for(uint64_t tracker = 1; tracker < dest[0]; tracker++) {
                cur_tags->insert(dest[tracker]);
                tags->insert(dest[tracker]);
            }
        }
        lgp_barrier();
        shmem_free(broadAr);
        shmem_free(dest);

        CUSTOMAP<VERTEX, std::set<TAG>*> :: iterator it;
        for(it = visited->begin(); it != visited->end(); it++) {
            uint64_t *measure =  (uint64_t *) &(IMMvisited->find(it->first)->second);
            if(*measure == 0) continue;
            uint64_t *intersection_size = new uint64_t;
            set_intersection (it->second->begin(), it->second->end(), 
                    cur_tags->begin(), cur_tags->end(), intersection_size );
            *measure = *measure - *intersection_size;
            #ifdef DEBUG
                assert((*measure) >= 0);
            #endif
            delete intersection_size;
        }
        cur_tags->clear();    
        k--;
    }
    delete tags;
    delete IMMvisited;
    delete cur_tags;
    return max_coverage;
}