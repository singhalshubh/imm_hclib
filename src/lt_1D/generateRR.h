trng::lcg64 generator;
trng::uniform01_dist<WEIGHT> val;

class IMMpckt {
    public: 
        TAG tag;
        VERTEX vertex;
};

#define ROOT_V 0
#define NO_ROOT_V 1

#define VISITED

class RRSelector: public hclib::Selector<1, IMMpckt> {
        GRAPH *graph;
        std::queue<IMMpckt>*currentFrontier;
        std::queue<IMMpckt>*nextFrontier;
        CUSTOMAP<VERTEX, std::set<TAG>*> *_LOCALE_visited;
        int *phase;

        void process(IMMpckt appPkt, int sender_rank) {            
            #ifdef VISITED
                nextFrontier->push(appPkt);
            #else
                CUSTOMAP<VERTEX, std::set<TAG>*>::iterator it = _LOCALE_visited->find(appPkt.vertex);
                if(it == _LOCALE_visited->end()) {
                    std::set<TAG>* s = new std::set<TAG>;
                    s->insert(appPkt.tag);
                    _LOCALE_visited->insert(std::make_pair(appPkt.vertex, s));
                    nextFrontier->push(appPkt);
                }
                else if(it->second->find(appPkt.tag) == it->second->end()) {
                    it->second->insert(appPkt.tag);
                    nextFrontier->push(appPkt);
                }
            #endif
        }

        public : void DO_ITR_LEVEL_ASYNC() {
            while(!currentFrontier->empty()) {
                IMMpckt appPkt = currentFrontier->front();
                currentFrontier->pop();
                if(*phase == ROOT_V) {
                    send(0, appPkt, graph->GMapper->to_host(appPkt.vertex));
                }
                else {
                    #ifdef VISITED
                        bool shouldSend = false;
                        CUSTOMAP<VERTEX, std::set<TAG>*>::iterator it = _LOCALE_visited->find(appPkt.vertex);
                        if(it == _LOCALE_visited->end()) {
                            std::set<TAG>* s = new std::set<TAG>;
                            s->insert(appPkt.tag);
                            _LOCALE_visited->insert(std::make_pair(appPkt.vertex, s));
                            shouldSend = true;
                        }
                        else if(it->second->find(appPkt.tag) == it->second->end()) {
                            it->second->insert(appPkt.tag);
                            shouldSend = true;
                        }
                        if(shouldSend == false) continue;
                    #endif
                    VERTEX u_local = graph->GMapper->to_local(appPkt.vertex);
                    if(graph->G->at(u_local).empty()) continue;
                    WEIGHT threshold = val(generator);
                    bool invalid = false;
                    #ifdef BINARY_SEARCH
                        VERTEX _selected = binarySearch(graph->G->at(u_local), threshold, &invalid);
                    #else
                        VERTEX _selected = linearSearch(graph->G->at(u_local), threshold, &invalid);
                    #endif

                    if(invalid == true) continue;
                    else {
                        appPkt.vertex = _selected;
                        send(0, appPkt, graph->GMapper->to_host(appPkt.vertex));
                    }
                }
            }
        }

public:
    RRSelector(GRAPH *_graph, std::queue<IMMpckt>*_currentFrontier, 
        std::queue<IMMpckt>*_nextFrontier,CUSTOMAP<VERTEX, std::set<TAG>*> *LOCALE_visited, int *_phase): 
            hclib::Selector<1, IMMpckt>(true), graph(_graph), currentFrontier(_currentFrontier), 
            nextFrontier(_nextFrontier), _LOCALE_visited(LOCALE_visited), phase(_phase) {
        mb[0].process = [this](IMMpckt appPkt, int sender_rank) { this->process(appPkt, sender_rank); };
    }
};

class GENERATE_RRR {
    private:
        std::queue<IMMpckt>*currentFrontier;
        std::queue<IMMpckt>*nextFrontier;
        CUSTOMAP<VERTEX, std::set<TAG>*> *_LOCALE_visited;

    public:
        GENERATE_RRR() {
            currentFrontier = new std::queue<IMMpckt>;
            nextFrontier = new std::queue<IMMpckt>;
            _LOCALE_visited = new CUSTOMAP<VERTEX, std::set<TAG>*>;
        }

        inline void FILL(GRAPH *G, uint64_t BATCH_SIZE) {
            /*  dest vertex is chosen at ranmdomly and therefore has to conform with application IMM standards, 
                which makes it global. We will continue supporting global vertex which should not interfere
                with AGL in any sense.
            */
            trng::uniform_int_dist start(0, G->global_num_nodes);
            for(uint64_t tracker = 0; tracker < BATCH_SIZE; tracker++) {
                IMMpckt sendpckt;
                sendpckt.tag = MYTHREAD*BATCH_SIZE + tracker;
                sendpckt.vertex = start(generator);
                currentFrontier->push(sendpckt);
            }
        }

        inline void FLUSH_AND_CLEAR(uint64_t offset, CUSTOMAP<VERTEX, std::set<TAG>*> * visited) {
            for(auto vertex_tags: *_LOCALE_visited) {
                CUSTOMAP<VERTEX, std::set<TAG>*>::iterator it = visited->find(vertex_tags.first);
                std::set<TAG>*s;
                if(it == visited->end()) {
                    s = new std::set<TAG>;
                    set_COPY(vertex_tags.second->begin(), vertex_tags.second->end(), s, offset);
                    visited->insert(std::make_pair(vertex_tags.first, s));
                }
                else {
                    set_COPY(vertex_tags.second->begin(), vertex_tags.second->end(), it->second, offset);
                }
                vertex_tags.second->clear();
            }
        }

        void PERFORM_GENERATERR(GRAPH *G, CUSTOMAP<VERTEX, std::set<TAG>*> *visited, uint64_t BATCH_SIZE, uint64_t offset) {
            FILL(G, BATCH_SIZE);
            int phase = ROOT_V; // phase ->0 indicates that phase 0 is simple exchange phase.
            uint64_t OR_VAL = 1;
            while(OR_VAL == 1) {
                RRSelector rrselector(G, currentFrontier, nextFrontier, _LOCALE_visited, &phase);
                hclib::finish([&rrselector] {
                    rrselector.DO_ITR_LEVEL_ASYNC();
                    rrselector.done(0);
                });
                #ifdef DEBUG
                    // uint64_t tot_size = lgp_reduce_add_l(nextFrontier->size());
                    // T0_fprintf(stderr, "Size of next Frontier (total for all pes): %ld\n", tot_size);
                #endif
                uint64_t val = nextFrontier->size() > 0 ? 1:0;
                OR_VAL = lgp_reduce_max_l(val);
                std::swap(currentFrontier, nextFrontier);
                phase = NO_ROOT_V;
                //if(MYTHREAD == 0)
                //rrselector.print_profiling("grr");
            }
            FLUSH_AND_CLEAR(offset, visited);
        }

        inline void CLEAR_GENERATERR() {
            #ifdef DEBUG
                assert(nextFrontier->empty() == true);
                assert(currentFrontier->empty() == true);
            #endif
        }

        void DELETE_GENERATERR() {
            delete nextFrontier;
            delete currentFrontier;
            delete _LOCALE_visited; 
        }
};