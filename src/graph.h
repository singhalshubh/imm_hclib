#include "renumbering.h"

#define DST uint64_t
#define SRC uint64_t
#define WEIGHT double

#define EDGE std::map<DST, WEIGHT>
#define TAG std::pair<uint64_t, uint64_t>

trng::lcg64 g_generator;
trng::uniform01_dist<float> g_val;

#define STORE 1
#define NO_STORE 0

#define ROOT_V 1
#define NO_ROOT_V 0

struct fileAppPacket {
    uint64_t src;
    uint64_t dst;
    double weight;
    bool type;
};

class IMMpckt {
    public: 
        uint64_t origin_pe;
        uint64_t dest_vertex;
        uint64_t bfs_iteration_index;
        bool type;
};

class FileSelector: public hclib::Selector<1, fileAppPacket> {
    std::map<SRC, EDGE*> *adjacencyMap;
    CONFIGURATION *cfg;
    std::map<SRC, std::set<TAG>*> *visited = new std::map<SRC, std::set<TAG>*>;

    void process(fileAppPacket appPkt, int sender_rank) {
        if(appPkt.type == STORE) {
            if(adjacencyMap->find(appPkt.src) != adjacencyMap->end()) {
                EDGE *e = adjacencyMap->find(appPkt.src)->second;
                if(e->find(appPkt.dst) != e->end()) {
                    e->find(appPkt.dst)->second += appPkt.weight;
                }
                else {
                    e->insert(std::make_pair(appPkt.dst, appPkt.weight));
                }
            }
            else {
                EDGE* e = new EDGE;
                e->insert(std::make_pair(appPkt.dst, appPkt.weight));
                adjacencyMap->insert(std::make_pair(appPkt.src, e));
            }
        }
        else {
            if(cfg->undirected == false) {
                if(adjacencyMap->find(appPkt.dst) == adjacencyMap->end()) {
                    EDGE* e = new EDGE;
                    adjacencyMap->insert(std::make_pair(appPkt.dst, e));
                }
            }
            else {
                if(adjacencyMap->find(appPkt.dst) != adjacencyMap->end()) {
                    EDGE *e = adjacencyMap->find(appPkt.dst)->second;
                    if(e->find(appPkt.src) != e->end()) {
                        e->find(appPkt.src)->second += appPkt.weight;
                    }
                    else {
                        e->insert(std::make_pair(appPkt.src, appPkt.weight));
                    }
                }
                else {
                    EDGE* e = new EDGE;
                    e->insert(std::make_pair(appPkt.src, appPkt.weight));
                    adjacencyMap->insert(std::make_pair(appPkt.dst, e));
                }
            }
        }
    }

public:
    FileSelector(std::map<uint64_t, EDGE*> *_adjacencyMap, CONFIGURATION *_cfg): 
            hclib::Selector<1, fileAppPacket>(true), adjacencyMap(_adjacencyMap), cfg(_cfg) {
        mb[0].process = [this](fileAppPacket appPkt, int sender_rank) { this->process(appPkt, sender_rank); };
    }
};

class GRAPH {
    public:
        std::map<SRC, EDGE*> *G;
        uint64_t total_num_nodes;
        #ifdef PREFIX
            std::vector<uint64_t> *_PREFIX_MAPPING;
        #else
            std::map<SRC, EDGE*> *_G;
            std::map<uint64_t, uint64_t> *globalToUserVerticeMap;
        #endif
        #ifdef COVARIANCE
            std::map<uint64_t, uint64_t> *global2localcolidx;
            std::map<uint64_t, uint64_t> *local2globalcolidx;
        #endif
        CONFIGURATION *cfg;


        void ALLOCATE_GRAPH();
        void DEALLOCATE_GRAPH();
        void READFILE();
        void LOAD_GRAPH();
        void STATS_OF_FILE();
        void generateWeightsLC();
};

void GRAPH::ALLOCATE_GRAPH() {
    this->G = new std::map<SRC, EDGE*>; 
    #ifdef PREFIX
        this->_PREFIX_MAPPING = new std::vector<uint64_t>(THREADS, 0);
    #else
        this->globalToUserVerticeMap = new std::map<uint64_t, uint64_t>;
        this->_G = new std::map<SRC, EDGE*>;
    #endif
    #ifdef COVARIANCE
        global2localcolidx = new std::map<uint64_t, uint64_t>;
        local2globalcolidx = new std::map<uint64_t, uint64_t>;
    #endif
}

void GRAPH::READFILE() {
    #ifdef PREFIX
        FileSelector* fileSelector = new FileSelector(this->G, this->cfg);
    #else
        FileSelector* fileSelector = new FileSelector(this->_G, this->cfg);
    #endif
    hclib::finish([=]() {
        struct stat stats;
        std::ifstream file(this->cfg->fileName);
        if (!file.is_open()) { 
            report("ERROR_OPEN_FILE"); 
        }
        std::string line;
        stat(this->cfg->fileName, & stats);

        uint64_t bytes = stats.st_size / THREADS;       
        uint64_t rem_bytes = stats.st_size % THREADS;
        uint64_t start, end;
        if(MYTHREAD < rem_bytes) {
            start = MYTHREAD*(bytes + 1);
            end = start + bytes + 1;
        }
        else {
            start = MYTHREAD*bytes + rem_bytes;
            end = start + bytes;
        }

        file.seekg(start);
        if (MYTHREAD != 0) {                                     
            file.seekg(start - 1);
            getline(file, line); 
            if (line[0] != '\n') start += line.size();         
        } 

        while (start < end && start < stats.st_size) {
            getline(file, line);
            start += line.size() + 1;
            if (line[0] == '#') continue;
            fileAppPacket pckt;
            std::stringstream ss(line);
            if(this->cfg->weighted) {
                ss >> pckt.dst >> pckt.src >> pckt.weight;
            }
            else {
                ss >> pckt.dst >> pckt.src;
                pckt.weight = g_val(g_generator);
            }
            #ifdef COVARIANCE
                pckt.dst--;
                pckt.src--;
            #endif
            pckt.type = STORE;
            fileSelector->send(0, pckt, pckt.src % THREADS);
            pckt.type = NO_STORE;
            if(!(this->cfg->weighted)) {
                pckt.weight = g_val(g_generator);
            }
            fileSelector->send(0, pckt, pckt.dst % THREADS);
        }
        file.close();
        fileSelector->done(0);
    });
} 

void GRAPH::LOAD_GRAPH() {
    CONFIGURATION *cfg = this->cfg;
    this->ALLOCATE_GRAPH();
    this->READFILE();
    if(cfg->type == LT && cfg->weighted == false) {this->generateWeightsLC();}
    this->STATS_OF_FILE();
}

void GRAPH::STATS_OF_FILE() {
    #ifdef PREFIX
        uint64_t num_nodes = init_renumbering(_PREFIX_MAPPING, G->size());
        //T0_fprintf(stderr, "Total Number of Nodes in G: %llu\n", num_nodes);
    #else
        std::map<uint64_t, uint64_t> *userToGlobalIdMap = new std::map<uint64_t, uint64_t>;
        uint64_t local_nodes = _G->size();
        uint64_t num_nodes = lgp_reduce_add_l(local_nodes);
        //T0_fprintf(stderr, "Total Number of Nodes in G: %llu\n", num_nodes);
        _sortRenumbering(_G, total_num_nodes, G, globalToUserVerticeMap, userToGlobalIdMap);
        delete _G;
        delete userToGlobalIdMap;
    #endif
    
    if(cfg->k >= num_nodes) {
        report("[REPORT_INFLUENCER_ERROR_CODE]:Please provide reasonable number of influencers, default = V(G)/2");
    }
    total_num_nodes = num_nodes;
    uint64_t local_edges = 0;
    for(auto x: *G) {
        local_edges += (x.second)->size();
    }
    uint64_t num_edges = 0;
    num_edges = lgp_reduce_add_l(local_edges);
    //T0_fprintf(stderr, "Total Number of Edges in G: %llu\n", num_edges);
}
    
void GRAPH::generateWeightsLC() {
    for(auto u: *G) {
        double total_weight = g_val(g_generator);
        for(auto edge: *(u.second)) {
            total_weight += edge.second; 
        }
        for(auto edge = u.second->begin(); edge != u.second->end(); edge++) {
            edge->second /= total_weight; 
        }
    }
}