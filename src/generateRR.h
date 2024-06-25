trng::lcg64 generator;
std::default_random_engine generator1;
trng::uniform01_dist<float> val;
std::uniform_real_distribution<double> distribution(0.0, 1.0);

#ifdef COVARIANCE

#define BFS_ITERATION_INDEX uint64_t

class RRSelector: public hclib::Selector<2, IMMpckt> {
        std::map<SRC, EDGE*> *G;
        std::map<SRC, std::set<TAG>*> *visited;
        std::queue<IMMpckt>*currentFrontier;
        std::queue<IMMpckt>*nextFrontier;
        std::map<BFS_ITERATION_INDEX,std::vector<DST>*> *RRsets;
        std::map<uint64_t, uint64_t> *IMMvisited;

        void process(IMMpckt appPkt, int sender_rank) {
            if(appPkt.type == ROOT_V) {
                appPkt.type = NO_ROOT_V;
                #ifdef PREFIX
                    auto itr = G->begin();
                    std::advance(itr, appPkt.dest_vertex);
                    appPkt.dest_vertex = (*itr).first;
                #endif
            }
            nextFrontier->push(appPkt);
        }

        void ACK(IMMpckt appPkt, int sender_rank) {
            RRsets->find(appPkt.bfs_iteration_index)->second->push_back(appPkt.dest_vertex);
        }

public:
    RRSelector(std::map<SRC, EDGE*> *_G, std::map<SRC, std::set<TAG>*> *_visited, 
    std::queue<IMMpckt>*_currentFrontier, std::queue<IMMpckt>*_nextFrontier, std::map<BFS_ITERATION_INDEX,std::vector<DST>*>*_RRsets,
        std::map<uint64_t, uint64_t> *_IMMvisited): 
            hclib::Selector<2, IMMpckt>(true), G(_G), visited(_visited), 
            currentFrontier(_currentFrontier), nextFrontier(_nextFrontier), RRsets(_RRsets), IMMvisited(_IMMvisited) {
        mb[0].process = [this](IMMpckt appPkt, int sender_rank) { this->process(appPkt, sender_rank); };
        mb[1].process = [this](IMMpckt appPkt, int sender_rank) { this->ACK(appPkt, sender_rank); };
    }
};

void GENERATE_RRR(GRAPH *graph, std::map<SRC, std::set<TAG>*> *visited, std::vector<int>* root_vertex, uint64_t offset,
        std::map<BFS_ITERATION_INDEX,std::vector<DST>*> *RRsets, std::map<uint64_t, uint64_t> *IMMvisited) {
    
    std::map<SRC, EDGE*> *G = graph->G;
    std::queue<IMMpckt>*currentFrontier = new std::queue<IMMpckt>;
    std::queue<IMMpckt>*nextFrontier = new std::queue<IMMpckt>;
    for(int i = 0; i < root_vertex->size(); i++) {
        IMMpckt sendpckt;
        sendpckt.origin_pe = MYTHREAD;
        sendpckt.dest_vertex = (*root_vertex)[i];
        sendpckt.bfs_iteration_index = i+offset;
        std::vector<DST> *v = new std::vector<DST>;
        RRsets->insert(std::make_pair(sendpckt.bfs_iteration_index, v));
        sendpckt.type = ROOT_V;
        currentFrontier->push(sendpckt);
    }
    uint64_t sum = currentFrontier->size();
    while(sum > 0) {
        RRSelector* rrselector = new RRSelector(G, visited, currentFrontier, nextFrontier, RRsets, IMMvisited);
        hclib::finish([=]() {
            while(!currentFrontier->empty()) {
                IMMpckt appPkt = currentFrontier->front();
                TAG dw;
                dw.first = appPkt.bfs_iteration_index;
                dw.second = appPkt.origin_pe;
                currentFrontier->pop();
                if(appPkt.type == ROOT_V) {
                    IMMpckt sendpckt;
                    sendpckt.origin_pe = appPkt.origin_pe;
                    sendpckt.bfs_iteration_index = appPkt.bfs_iteration_index;
                    sendpckt.type = ROOT_V;
                    #ifdef PREFIX
                        std::pair<uint64_t, uint64_t> *peAndOffsetItr = new std::pair<uint64_t, uint64_t>;
                        whichVertice(appPkt.dest_vertex, graph->_PREFIX_MAPPING, peAndOffsetItr);
                        sendpckt.dest_vertex = peAndOffsetItr->second;
                        rrselector->send(0, sendpckt, (peAndOffsetItr->first) % THREADS);
                        #ifdef TRACE
                            if(graph->cfg->STATE == ESTIMATE_THETA) (graph->cfg->TRACE_SENDS[0])++;
                            else if(graph->cfg->STATE == FINAL) (graph->cfg->TRACE_SENDS[1])++;
                        #endif
                    #else
                        sendpckt.dest_vertex = appPkt.dest_vertex;
                        rrselector->send(0, sendpckt, (sendpckt.dest_vertex) % THREADS);
                        #ifdef TRACE
                            if(graph->cfg->STATE == ESTIMATE_THETA) (graph->cfg->TRACE_SENDS[0])++;
                            else if(graph->cfg->STATE == FINAL) (graph->cfg->TRACE_SENDS[1])++;
                        #endif
                    #endif
                }
                else {
                    bool shouldSend = false;
                    if(visited->find(appPkt.dest_vertex) == visited->end()) {
                        std::set<TAG> *vv = new std::set<TAG>;
                        vv->insert(std::make_pair(appPkt.bfs_iteration_index, appPkt.origin_pe));
                        visited->insert(std::make_pair(appPkt.dest_vertex, vv));
                        shouldSend = true;
                    }
                    else if((visited->find(appPkt.dest_vertex)->second)->find(dw) == (visited->find(appPkt.dest_vertex)->second)->end()) {
                        (visited->find(appPkt.dest_vertex)->second)->insert(dw);
                        shouldSend = true;
                    }
                    if(shouldSend) {
                        // initiate the ack to original pe to construct RR set
                        IMMpckt ACKpkt;
                        ACKpkt.origin_pe = appPkt.origin_pe;
                        ACKpkt.dest_vertex = appPkt.dest_vertex;
                        ACKpkt.bfs_iteration_index = appPkt.bfs_iteration_index;
                        rrselector->send(1, ACKpkt, (ACKpkt.origin_pe) % THREADS);

                        if(IMMvisited->find(appPkt.dest_vertex) == IMMvisited->end()) {
                            IMMvisited->insert(std::make_pair(appPkt.dest_vertex, 1));
                        }
                        else {
                            (IMMvisited->find(appPkt.dest_vertex)->second)++;
                        }
                        EDGE *destAndWeight = G->find(appPkt.dest_vertex)->second;
                        if(graph->cfg->type == IC) {
                            for(auto itr1 = destAndWeight->begin(); itr1 != destAndWeight->end(); itr1++) {
                                if(itr1->second >= val(generator)) {
                                    IMMpckt sendpckt;
                                    sendpckt.origin_pe = appPkt.origin_pe;
                                    sendpckt.dest_vertex = (itr1->first);
                                    sendpckt.bfs_iteration_index = appPkt.bfs_iteration_index;
                                    sendpckt.type = NO_ROOT_V;
                                    rrselector->send(0, sendpckt, (sendpckt.dest_vertex) % THREADS);
                                    #ifdef TRACE
                                        if(graph->cfg->STATE == ESTIMATE_THETA) (graph->cfg->TRACE_SENDS[0])++;
                                        else if(graph->cfg->STATE == FINAL) (graph->cfg->TRACE_SENDS[1])++;
                                    #endif
                                }
                            }
                        }
                        else if (graph->cfg->type == LT) {
                            float threshold = distribution(generator1);
                            for(auto itr1 = destAndWeight->begin(); itr1 != destAndWeight->end(); itr1++) {
                                threshold -= itr1->second;
                                if(threshold > 0) continue;
                                IMMpckt sendpckt;
                                sendpckt.origin_pe = appPkt.origin_pe;
                                sendpckt.dest_vertex = (itr1->first);
                                sendpckt.bfs_iteration_index = appPkt.bfs_iteration_index;
                                sendpckt.type = NO_ROOT_V;
                                rrselector->send(0, sendpckt, (sendpckt.dest_vertex) % THREADS);
                                #ifdef TRACE
                                    if(graph->cfg->STATE == ESTIMATE_THETA) (graph->cfg->TRACE_SENDS[0])++;
                                    else if(graph->cfg->STATE == FINAL) (graph->cfg->TRACE_SENDS[1])++;
                                #endif
                                break;
                            }
                        }
                    }
                }
            }
            rrselector->done(0);
        });
        lgp_barrier();
        delete rrselector;
        int64_t tot_sum = nextFrontier->size() > 0 ? 1:0;
        lgp_barrier();
        sum = lgp_reduce_max_l(tot_sum);
        std::queue<IMMpckt>*tmp = currentFrontier;
        currentFrontier = nextFrontier;
        nextFrontier = tmp;
        lgp_barrier();
    }
    delete currentFrontier;
    delete nextFrontier;
}

#else
class RRSelector: public hclib::Selector<1, IMMpckt> {
        std::map<SRC, EDGE*> *G;
        std::map<SRC, std::set<TAG>*> *visited;
        std::queue<IMMpckt>*currentFrontier;
        std::queue<IMMpckt>*nextFrontier;

        void process(IMMpckt appPkt, int sender_rank) {
            if(appPkt.type == ROOT_V) {
                appPkt.type = NO_ROOT_V;
                #ifdef PREFIX
                    auto itr = G->begin();
                    std::advance(itr, appPkt.dest_vertex);
                    appPkt.dest_vertex = (*itr).first;
                #endif
            }
            nextFrontier->push(appPkt);
        }

public:
    RRSelector(std::map<SRC, EDGE*> *_G, std::map<SRC, std::set<TAG>*> *_visited, 
    std::queue<IMMpckt>*_currentFrontier, std::queue<IMMpckt>*_nextFrontier): 
            hclib::Selector<1, IMMpckt>(true), G(_G), visited(_visited), 
            currentFrontier(_currentFrontier), nextFrontier(_nextFrontier) {
        mb[0].process = [this](IMMpckt appPkt, int sender_rank) { this->process(appPkt, sender_rank); };
    }
};

void GENERATE_RRR(GRAPH *graph, std::map<SRC, std::set<TAG>*> *visited, std::vector<int>* root_vertex, uint64_t offset) {
    
    std::map<SRC, EDGE*> *G = graph->G;
    std::queue<IMMpckt>*currentFrontier = new std::queue<IMMpckt>;
    std::queue<IMMpckt>*nextFrontier = new std::queue<IMMpckt>;
    for(int i = 0; i < root_vertex->size(); i++) {
        IMMpckt sendpckt;
        sendpckt.origin_pe = MYTHREAD;
        sendpckt.dest_vertex = (*root_vertex)[i];
        sendpckt.bfs_iteration_index = i+offset;
        sendpckt.type = ROOT_V;
        currentFrontier->push(sendpckt);
    }
    uint64_t sum = currentFrontier->size();
    while(sum > 0) {
        RRSelector* rrselector = new RRSelector(G, visited, currentFrontier, nextFrontier);
        hclib::finish([=]() {
            while(!currentFrontier->empty()) {
                IMMpckt appPkt = currentFrontier->front();
                TAG dw;
                dw.first = appPkt.bfs_iteration_index;
                dw.second = appPkt.origin_pe;
                currentFrontier->pop();
                if(appPkt.type == ROOT_V) {
                    IMMpckt sendpckt;
                    sendpckt.origin_pe = appPkt.origin_pe;
                    sendpckt.bfs_iteration_index = appPkt.bfs_iteration_index;
                    sendpckt.type = ROOT_V;
                    #ifdef PREFIX
                        std::pair<uint64_t, uint64_t> *peAndOffsetItr = new std::pair<uint64_t, uint64_t>;
                        whichVertice(appPkt.dest_vertex, graph->_PREFIX_MAPPING, peAndOffsetItr);
                        sendpckt.dest_vertex = peAndOffsetItr->second;
                        rrselector->send(0, sendpckt, (peAndOffsetItr->first) % THREADS);
                        #ifdef TRACE
                            if(graph->cfg->STATE == ESTIMATE_THETA) (graph->cfg->TRACE_SENDS[0])++;
                            else if(graph->cfg->STATE == FINAL) (graph->cfg->TRACE_SENDS[1])++;
                        #endif
                    #else
                        sendpckt.dest_vertex = appPkt.dest_vertex;
                        rrselector->send(0, sendpckt, (sendpckt.dest_vertex) % THREADS);
                        #ifdef TRACE
                            if(graph->cfg->STATE == ESTIMATE_THETA) (graph->cfg->TRACE_SENDS[0])++;
                            else if(graph->cfg->STATE == FINAL) (graph->cfg->TRACE_SENDS[1])++;
                        #endif
                    #endif
                }
                else {
                    bool shouldSend = false;
                    if(visited->find(appPkt.dest_vertex) == visited->end()) {
                        std::set<TAG> *vv = new std::set<TAG>;
                        vv->insert(std::make_pair(appPkt.bfs_iteration_index, appPkt.origin_pe));
                        visited->insert(std::make_pair(appPkt.dest_vertex, vv));
                        shouldSend = true;
                    }
                    else if((visited->find(appPkt.dest_vertex)->second)->find(dw) == (visited->find(appPkt.dest_vertex)->second)->end()) {
                        (visited->find(appPkt.dest_vertex)->second)->insert(dw);
                        shouldSend = true;
                    }
                    if(shouldSend) {
                        EDGE *destAndWeight = G->find(appPkt.dest_vertex)->second;
                        if(graph->cfg->type == IC) {
                            for(auto itr1 = destAndWeight->begin(); itr1 != destAndWeight->end(); itr1++) {
                                if(itr1->second >= val(generator)) {
                                    IMMpckt sendpckt;
                                    sendpckt.origin_pe = appPkt.origin_pe;
                                    sendpckt.dest_vertex = (itr1->first);
                                    sendpckt.bfs_iteration_index = appPkt.bfs_iteration_index;
                                    sendpckt.type = NO_ROOT_V;
                                    rrselector->send(0, sendpckt, (sendpckt.dest_vertex) % THREADS);
                                    #ifdef TRACE
                                        if(graph->cfg->STATE == ESTIMATE_THETA) (graph->cfg->TRACE_SENDS[0])++;
                                        else if(graph->cfg->STATE == FINAL) (graph->cfg->TRACE_SENDS[1])++;
                                    #endif
                                }
                            }
                        }
                        else if (graph->cfg->type == LT) {
                            float threshold = distribution(generator1);
                            for(auto itr1 = destAndWeight->begin(); itr1 != destAndWeight->end(); itr1++) {
                                threshold -= itr1->second;
                                if(threshold > 0) continue;
                                IMMpckt sendpckt;
                                sendpckt.origin_pe = appPkt.origin_pe;
                                sendpckt.dest_vertex = (itr1->first);
                                sendpckt.bfs_iteration_index = appPkt.bfs_iteration_index;
                                sendpckt.type = NO_ROOT_V;
                                rrselector->send(0, sendpckt, (sendpckt.dest_vertex) % THREADS);
                                #ifdef TRACE
                                    if(graph->cfg->STATE == ESTIMATE_THETA) (graph->cfg->TRACE_SENDS[0])++;
                                    else if(graph->cfg->STATE == FINAL) (graph->cfg->TRACE_SENDS[1])++;
                                #endif
                                break;
                            }
                        }
                    }
                }
            }
            rrselector->done(0);
        });
        lgp_barrier();
        delete rrselector;
        int64_t tot_sum = nextFrontier->size() > 0 ? 1:0;
        lgp_barrier();
        sum = lgp_reduce_max_l(tot_sum);
        std::queue<IMMpckt>*tmp = currentFrontier;
        currentFrontier = nextFrontier;
        nextFrontier = tmp;
        lgp_barrier();
    }
    delete currentFrontier;
    delete nextFrontier;
}
#endif