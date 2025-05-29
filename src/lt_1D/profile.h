uint64_t _graphMemory(GRAPH *g) {
    uint64_t numberOfSources = g->G->size();
    uint64_t numberOfEdges = 0;
    for(auto x: *(g->G)) {
        numberOfEdges += x.size();
    }
    uint64_t memoryGraph = (numberOfEdges*sizeof(EDGE)) + numberOfSources*sizeof(VERTEX);
    return memoryGraph;
}


uint64_t _visitedMemory(CUSTOMAP<VERTEX, std::set<TAG>*> *visited) {
    uint64_t numberOfSources = visited->size();
    uint64_t numberOfTags = 0;
    for(auto x: *visited) {
        numberOfTags += x.second->size();
    }
    uint64_t memory = (numberOfTags*sizeof(TAG)) + numberOfSources*sizeof(VERTEX);
    return memory;
}

static long lock;
static long iteration = 0;

void measureMemoryPerPE(GRAPH *g, CUSTOMAP<VERTEX, std::set<TAG>*> *visited, int file_key = iteration) {
    iteration++;
    uint64_t p = _graphMemory(g);
    uint64_t q = _visitedMemory(visited);
    fprintf(stderr, "SIZE: %ld bytes of TAGS\n", lgp_reduce_add_l(q));
    shmem_set_lock(&lock);
    std::string file_name = "memory-profile";
    file_name += std::to_string(file_key);
    file_name += ".txt";
    FILE *fp = fopen(file_name.c_str(), "a");
    fprintf(fp, "%ld, %ld, %ld\n", MYTHREAD, p, q);
    fclose(fp);
    shmem_clear_lock(&lock);
}