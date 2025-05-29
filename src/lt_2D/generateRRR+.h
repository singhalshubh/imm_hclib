trng::lcg64 generator;
trng::uniform01_dist<WEIGHT> val;

class IMMpckt {
    public: 
        TAG tag;
        VERTEX vertex;
};

#define ROOT_V 0
#define NO_ROOT_V 1

class RRSelector: public hclib::Selector<2, IMMpckt> {
        GRAPH *graph;
        std::queue<IMMpckt>*currentFrontier;
        std::queue<IMMpckt>*nextFrontier;
        std::unordered_map<VERTEX, std::set<TAG>*> *_LOCALE_visited;
        std::unordered_map<VERTEX, uint64_t> *_IMM_visited_count;
        std::vector<std::vector<VERTEX>> *_LOCALE_RRsets;
        int *phase;
        uint64_t *BATCH_SIZE;

        inline void insertIntoIMMCount(IMMpckt appPkt) {
            std::unordered_map<VERTEX, uint64_t>::iterator it = _IMM_visited_count->find(appPkt.vertex);
            if(it == _IMM_visited_count->end()) {
                _IMM_visited_count->insert(std::make_pair(appPkt.vertex, 1));
            }
            else {
                (it->second)++;
            }
        }   

        void process(IMMpckt appPkt, int sender_rank) {
            std::unordered_map<VERTEX, std::set<TAG>*>::iterator it = _LOCALE_visited->find(appPkt.vertex);
            if(it == _LOCALE_visited->end()) {
                std::set<TAG>* s = new std::set<TAG>;
                s->insert(appPkt.tag);
                _LOCALE_visited->insert(std::make_pair(appPkt.vertex, s));
                insertIntoIMMCount(appPkt);
                nextFrontier->push(appPkt);
                send(1, appPkt, (appPkt.tag/(*BATCH_SIZE)));
            }
            else if(it->second->find(appPkt.tag) == it->second->end()) {
                it->second->insert(appPkt.tag);
                insertIntoIMMCount(appPkt);
                nextFrontier->push(appPkt);
                send(1, appPkt, (appPkt.tag/(*BATCH_SIZE)));
            }           
        }

        void ACK(IMMpckt appPkt, int sender_rank) {
            _LOCALE_RRsets->at(appPkt.tag%(*BATCH_SIZE)).push_back(appPkt.vertex);
        }

        public : void DO_ITR_LEVEL_ASYNC() {
            while(!currentFrontier->empty()) {
                IMMpckt appPkt = currentFrontier->front();
                currentFrontier->pop();
                if(*phase == ROOT_V) {
                    send(0, appPkt, appPkt.vertex % THREADS);
                }
                else {
                    #ifdef DEBUG
                        assert(*phase == NO_ROOT_V);
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
        std::queue<IMMpckt>*_nextFrontier,std::unordered_map<VERTEX, std::set<TAG>*> *LOCALE_visited, int *_phase,
        uint64_t *_BATCH_SIZE, std::vector<std::vector<VERTEX>> *LOCALE_RRsets,  
        std::unordered_map<VERTEX, uint64_t> *IMM_visited_count): 
            hclib::Selector<2, IMMpckt>(true), graph(_graph), currentFrontier(_currentFrontier), 
            nextFrontier(_nextFrontier), _LOCALE_visited(LOCALE_visited), phase(_phase), _LOCALE_RRsets(LOCALE_RRsets), 
            BATCH_SIZE(_BATCH_SIZE), _IMM_visited_count(IMM_visited_count) {
        mb[0].process = [this](IMMpckt appPkt, int sender_rank) { this->process(appPkt, sender_rank); };
        mb[1].process = [this](IMMpckt appPkt, int sender_rank) { this->ACK(appPkt, sender_rank); };
    }
};

class GENERATE_RRR {
    private:
        std::queue<IMMpckt>*currentFrontier;
        std::queue<IMMpckt>*nextFrontier;
        std::unordered_map<VERTEX, std::set<TAG>*> *_LOCALE_visited;
    public:
        std::vector<std::vector<VERTEX>> *_LOCALE_RRsets = NULL;
        std::unordered_map<VERTEX, uint64_t> *_IMM_visited_count = NULL;

    public:
        GENERATE_RRR() {
            currentFrontier = new std::queue<IMMpckt>;
            nextFrontier = new std::queue<IMMpckt>;
        }

        void INIT(uint64_t BATCH_SIZE) {
            _LOCALE_visited = new std::unordered_map<VERTEX, std::set<TAG>*>;
            if(_IMM_visited_count == NULL) {
                _IMM_visited_count = new std::unordered_map<VERTEX, uint64_t>;
            }
            _LOCALE_RRsets = new std::vector<std::vector<VERTEX>>(BATCH_SIZE);
        }

        inline void FILL(GRAPH *G, uint64_t BATCH_SIZE) {
            trng::uniform_int_dist start(0, G->global_num_nodes);
            for(uint64_t tracker = 0; tracker < BATCH_SIZE; tracker++) {
                IMMpckt sendpckt;
                sendpckt.tag = MYTHREAD*BATCH_SIZE + tracker;
                sendpckt.vertex = start(generator);
                currentFrontier->push(sendpckt);
            }
        }

        void PERFORM_GENERATERR(GRAPH *G, uint64_t BATCH_SIZE, uint64_t offset) {
            FILL(G, BATCH_SIZE);
            int phase = ROOT_V; // phase ->0 indicates that phase 0 is simple exchange phase.
            uint64_t OR_VAL = 1;
            while(OR_VAL == 1) {
                RRSelector rrselector(G, currentFrontier, nextFrontier, _LOCALE_visited, &phase, 
                    &BATCH_SIZE, _LOCALE_RRsets, _IMM_visited_count);
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
            }
            #ifdef DEBUG
                // uint64_t local_size = 0;
                // for(auto vec: *_LOCALE_RRsets) {
                //     //fprintf(stderr, "%ld\n", vec.size());
                //     local_size += vec.size();
                // }
                // uint64_t global_size = lgp_reduce_add_l(local_size);
                // T0_fprintf(stderr, "Average RRset length: %ld\n", global_size/(BATCH_SIZE*THREADS));
            #endif
        }

        inline void CLEAR_GENERATERR() {
            #ifdef DEBUG
                assert(nextFrontier->empty() == true);
                assert(currentFrontier->empty() == true);
            #endif
            delete _LOCALE_visited;
        }

        void DELETE_GENERATERR() {
            delete nextFrontier;
            delete currentFrontier;
            delete _IMM_visited_count;
        }
};
 