static long lock;
void _measureNNZ(std::map<VERTEX, std::map<VERTEX, uint64_t>*> *COCCUR, uint64_t condition) {
    uint64_t local_size = 0;
    for(auto cor: *COCCUR) {
        local_size += cor.second->size();
    }
    // if(condition == 1) {
    //     shmem_set_lock(&lock);
    //     FILE *fp = fopen("debug.txt", "a");
    //     fprintf(fp, "%ld, %ld\n", MYTHREAD, local_size);
    //     fclose(fp);
    //     shmem_clear_lock(&lock);
    // }
    uint64_t max_size = lgp_reduce_max_l(local_size);
    uint64_t global_size = lgp_reduce_add_l(local_size);
    T0_fprintf(stderr, "(max, total) nnz in C: %ld, %ld\n", max_size, global_size); 
}

double adjust_time = 0;
double select_time = 0;

template<typename T>
uint64_t PERFORM_IMM(GRAPH *g, std::vector<std::vector<VERTEX>> *RRsets, 
    std::unordered_map<uint64_t, uint64_t> *_IMMvisited, std::set<uint64_t> *influencers, uint64_t k,
    std::map<VERTEX, std::map<VERTEX, uint64_t>*> *COCCUR, uint64_t p_r, uint64_t p_c, uint64_t condition) {

    /********************CONSTRUCTION OF TAGS count and MATRIX array*******************/
    double t1 = wall_seconds();
    T *TAGS_COUNT = new T;
    _CONSTRUCT(g, RRsets, _IMMvisited, influencers, k, COCCUR, TAGS_COUNT, p_r, p_c);
    adjust_time += wall_seconds() - t1;
    
    #ifdef DEBUG
    //_measureNNZ(COCCUR, condition);
    #endif

    /********************FIND THE TOP K influencers*******************/
    t1 = wall_seconds();
    uint64_t max_coverage = 0;
    for (int i = 0; i < k; i++) {
        
        /***** All-Reduce Integer based ****/
        uint64_t local_max_size = 0;
        uint64_t dest_vertex;
        _MAX_LOCAL(g, TAGS_COUNT, &local_max_size, &dest_vertex);
        uint64_t global_max_size = lgp_reduce_max_l(local_max_size);
        #ifdef DEBUG
            //T0_fprintf(stderr, "Max tag size: %ld\n", global_max_size);
        #endif
        if(global_max_size == 0) {
            break;
        }
        max_coverage += global_max_size;
        uint64_t infl = std::numeric_limits<int64_t>::max();
        if(global_max_size == local_max_size) {
            infl = dest_vertex;
        }
        uint64_t curr_influencer = lgp_reduce_min_l(infl);
        #ifdef DEBUG
            //T0_fprintf(stderr, "curr_inf: %ld\n", curr_influencer);
        #endif
        /***** Delete if needed ****/ 
        _DELETION(g, RRsets, _IMMvisited, influencers, k, COCCUR, TAGS_COUNT, p_r, p_c, curr_influencer);
    }
    select_time += wall_seconds() - t1;

    T0_fprintf(stderr, "[Time until now] in matrixGen: %8.3lf seconds\n", adjust_time);
    T0_fprintf(stderr, "[Time until now] in k loops: %8.3lf seconds\n", select_time);
    delete TAGS_COUNT;
    return max_coverage;
}