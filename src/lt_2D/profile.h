uint64_t _graphMemory(GRAPH *g) {
    uint64_t numberOfSources = g->G->size();
    uint64_t numberOfEdges = 0;
    for(auto x: *(g->G)) {
        numberOfEdges += x.size();
    }
    uint64_t memoryGraph = (numberOfEdges*sizeof(EDGE)) + numberOfSources*sizeof(VERTEX);
    return memoryGraph;
}

uint64_t _rrsetsMemory(std::vector<std::vector<VERTEX>> *_LOCALE_RRsets) {
    uint64_t memory = 0;
    for(auto x: *_LOCALE_RRsets) {
        memory += x.size();
    }
    return memory*sizeof(VERTEX);
}

uint64_t _visitedMemory(std::unordered_map<VERTEX, uint64_t> *_IMM_visited_count) {
    uint64_t memory = _IMM_visited_count->size();
    return memory*sizeof(VERTEX)*2;
}

uint64_t _coccurMemory(std::map<VERTEX, std::map<VERTEX,uint64_t>* > *COCCUR) {
    uint64_t numberOfSources = COCCUR->size();
    uint64_t numberOfEdges = 0;
    for(auto x: *(COCCUR)) {
        numberOfEdges += x.second->size();
    }
    uint64_t memory = (numberOfEdges*sizeof(uint64_t)*2) + numberOfSources*sizeof(VERTEX);
    return memory;
}

void measureMemoryPerPE(GRAPH *g, 
    std::map<VERTEX, std::map<VERTEX,uint64_t>* > *COCCUR, 
    std::unordered_map<VERTEX, uint64_t> *_IMM_visited_count, std::vector<std::vector<VERTEX>> *_LOCALE_RRsets,
    uint64_t file_key) {
    
    uint64_t p = _graphMemory(g);
    uint64_t q = _visitedMemory(_IMM_visited_count);
    uint64_t s = _coccurMemory(COCCUR);
    uint64_t t = _coccurMemory(COCCUR);
    fprintf(stderr, "SIZE: %ld bytes of COCCUR\n", lgp_reduce_add_l(s));
    shmem_set_lock(&lock);
    std::string file_name = "memory-profile-2D";
    file_name += std::to_string(file_key);
    file_name += ".txt";
    FILE *fp = fopen(file_name.c_str(), "a");
    fprintf(fp, "%ld, %ld, %ld, %ld\n", MYTHREAD, p, q, s);
    fclose(fp);
    shmem_clear_lock(&lock);
}