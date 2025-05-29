#define WEIGHT double
#define VERTEX uint64_t

/*Graph for IMM Actor algorithms arte restricted to lie between [0..n] for memory scalability. Otherwise,
Our programs can hold the nodes upto their maximum label of the vertice!*/

#define EDGE std::pair<VERTEX, WEIGHT>
#define EDGELIST std::vector<EDGE>
#define TAG uint64_t

#define BINARY_SEARCH

trng::lcg64 g_generator;
trng::uniform01_dist<WEIGHT> g_val;

// #define ASSERT 

#include "mapper.h"
#include "user.h"

class GRAPH {
    public:
        std::vector<EDGELIST> *G;
        uint64_t global_num_nodes;
        uint64_t global_num_edges;
        Mapper *GMapper;

        uint64_t local_num_nodes;
        uint64_t local_num_edges;        
        CONFIGURATION *cfg;
        uint64_t global_num_blocks_;
        uint64_t block_num_edges_;

        void ALLOCATE_GRAPH();
        void MAPPER_ALLOCATION();
        void SQUISH(EDGELIST *);
        void LOAD_GRAPH();
        void GENERATE_GRAPH();
        void READ_GRAPH();
        void DEALLOCATE_GRAPH();
       
        void STATS_OF_FILE();
        void MAX_DEGREE_GRAPH();
        void generateWeightsLC();
        #ifdef ASSERT
            void CHECK_FORMAT();
        #endif
};

void GRAPH::MAPPER_ALLOCATION() {
    /* PE 0 will determine the maximum label in the file */
    uint64_t MAX_LABEL = 0;
    if(cfg->scale_ == -1 && cfg->degree_ == -1) {
        struct stat stats;
        std::ifstream file(cfg->fileName);
        if (!file.is_open()) { 
            report("ERROR_OPEN_FILE"); 
        }
        std::string line;
        stat(cfg->fileName, & stats);

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
            if (line[0] == '#' || line[0] == '%') continue;
            std::stringstream ss(line);
            VERTEX a,b;
            ss >> a >> b;
            if(MAX_LABEL < std::max(a-1, b-1)) {
                MAX_LABEL = std::max(a-1, b-1);   
            }
        }
        global_num_nodes = lgp_reduce_max_l(MAX_LABEL) + 1;
        file.close();
    }

    switch(cfg->m_type) {
        case MapperType::Cyclic :
            GMapper = new CyclicMapper; 
            T0_fprintf(stderr, "Cyclic Mapping\n");
            reinterpret_cast<CyclicMapper*>(GMapper); break;
        case MapperType::Range :
            GMapper =  new RangeMapper(global_num_nodes); 
            T0_fprintf(stderr, "Range Mapping\n");
            reinterpret_cast<RangeMapper*>(GMapper); break;
        case MapperType::XOR :
            GMapper = new XORMapper(global_num_nodes);
            T0_fprintf(stderr, "XOR Mapping\n");
            reinterpret_cast<XORMapper*>(GMapper); break;
    }
}

void GRAPH::ALLOCATE_GRAPH() {
    if(cfg->scale_ != -1 && cfg->degree_ != -1) {
        this->global_num_nodes = 1L << cfg->scale_;
        this->global_num_edges = this->global_num_nodes * cfg->degree_;
        int block_scale = cfg->scale_/2;
        this->block_num_edges_ = (1L << block_scale) * cfg->degree_;
        this->global_num_blocks_ = (1L << (cfg->scale_ - block_scale));
    }
    MAPPER_ALLOCATION();
    this->G = new std::vector<EDGELIST>(global_num_nodes/THREADS + THREADS);
    #ifdef ASSERT
        assert(this->global_num_nodes > 0);
        if(cfg->k >= global_num_nodes/2) {
            T0_fprintf(stderr, "Choose the correct value for k, less than V/2");
            exit(-1);
        }
    #endif
    T0_fprintf(stderr, "Total Number of Nodes in G: %llu\n", global_num_nodes);
}

void GRAPH::SQUISH(EDGELIST *el) {   
    /* ZERO weighted edges are not allowed! */
    auto move_pointer = el->begin();
    auto start_pointer = el->begin();
    WEIGHT total_weight = el->begin()->second;
    move_pointer++;
    while(move_pointer < el->end()) {
        if(move_pointer->first == start_pointer->first) {
            total_weight += move_pointer->second;
            move_pointer->first = 0;
            move_pointer->second = 0;
        }
        else {
            start_pointer->second = total_weight;
            start_pointer = move_pointer;
            total_weight = move_pointer->second;
        }
        move_pointer++;
    }
    std::sort(el->begin(), el->end());
    uint64_t pos = 0;
    for(auto x: *el) {
        if(x.first == 0 && x.second == 0) {
            pos++;
        }
        else {
            break;
        }
    }
    el->erase(el->begin(), el->begin()+pos);
}


void GRAPH::LOAD_GRAPH() {
    unsigned long seed_start = MYTHREAD;
    g_generator.seed(seed_start);
    g_generator.split(THREADS, MYTHREAD);
    
    CONFIGURATION *cfg = this->cfg;
    this->ALLOCATE_GRAPH();
    if(cfg->scale_ == -1) {
        T0_fprintf(stderr, "Reading the graph file\n");
        this->READ_GRAPH();
    }
    else {
        T0_fprintf(stderr, "Generating the graph\n");
        this->GENERATE_GRAPH();
    }
    T0_fprintf(stderr, "Adjusting weights\n");
    if(cfg->weighted == false) {
        this->generateWeightsLC();   
    }
    for(auto el = G->begin(); el != G->end(); el++) {
        #ifdef BINARY_SEARCH
            WEIGHT total_weight = 0.0;
            for(auto edge = el->begin(); edge != el->end(); edge++) {
                total_weight += edge->second;
                (*edge).second = total_weight;
            }
        #endif
    }
    this->STATS_OF_FILE();
    #ifdef ASSERT
        this->CHECK_FORMAT();
    #endif
}

void GRAPH::MAX_DEGREE_GRAPH() {
    uint64_t max_degree = 0;
    for(auto x: *G) {
        max_degree = std::max(max_degree, x.size());
    }
    uint64_t glb_max_degree = lgp_reduce_max_l(max_degree);
    T0_fprintf(stderr, "Graph Info: Max-degree: %ld\n", glb_max_degree);
}

void GRAPH::generateWeightsLC() {
    for(auto el = G->begin(); el != G->end(); el++) {
        WEIGHT total_weight = g_val(g_generator);
        for(auto edge: *el) {
            total_weight += edge.second; 
        }
        for(auto edge = el->begin(); edge != el->end(); edge++) {
            (*edge).second /= total_weight; 
        }
    }
}

void GRAPH::DEALLOCATE_GRAPH() {
    delete G;
}

void GRAPH::STATS_OF_FILE() {
    this->local_num_nodes = G->size();
    uint64_t local_edges = 0;
    for(auto x: *G) {
        local_edges += x.size();
    }
    uint64_t num_edges = lgp_reduce_add_l(local_edges);
    this->local_num_edges = num_edges;
    global_num_edges = num_edges;
    T0_fprintf(stderr, "Total Number of Edges in G: %llu\n", num_edges);
    T0_fprintf(stderr, "Graph Info: AVG-degree: %ld\n", num_edges / global_num_nodes);
    MAX_DEGREE_GRAPH();
}

#ifdef ASSERT 
    void GRAPH::CHECK_FORMAT() {
        // Every vertice should be from 0..n-1 from the user
        for(auto x: *G) {
            for(auto edge: x) {
                ASSERT_WITH_MESSAGE(edge.first < global_num_nodes, std::to_string(edge.first));
            }
        }
    }
#endif

struct fileAppPacket {
    VERTEX src;
    VERTEX dst;
    WEIGHT weight;
};

class FileSelector: public hclib::Selector<1, fileAppPacket> {
    GRAPH *g;
    std::shared_ptr<Mapper> GMapper;

    void process(fileAppPacket appPkt, int sender_rank) {
        std::vector<EDGELIST> *G = g->G;
        VERTEX local_src_ID = g->GMapper->to_local(appPkt.src);
        #ifdef ASSERT
            assert(G->size() > local_src_ID);
        #endif
        (*G)[local_src_ID].push_back(std::make_pair(appPkt.dst, appPkt.weight));
        // if((*G)[local_src_ID].size() > MAX_DEGREE) {
        //     g->SQUISH(&((*G)[local_src_ID]));
        // }
    }

public:
    FileSelector(GRAPH *_g): 
            hclib::Selector<1, fileAppPacket>(true), g(_g) {
        mb[0].process = [this](fileAppPacket appPkt, int sender_rank) { this->process(appPkt, sender_rank); };
    }
};

void GRAPH::GENERATE_GRAPH() {
    FileSelector* genSelector = new FileSelector(this);
    hclib::finish([=]() {
        if(cfg->TYPE == genType::RMAT) {
            const float A = 0.57f, B = 0.19f, C = 0.19f;
            std::mt19937 rng;
            std::uniform_real_distribution<float> udist(0, 1.0f);
            for (size_t block = MYTHREAD; block < global_num_blocks_; block += THREADS) {
                rng.seed(kRandSeed + block);
                for (size_t m = 0; m < block_num_edges_; m++) {
                    VERTEX src = 0;
                    VERTEX dst = 0;
                    for (uint64_t depth=0; depth < cfg->scale_; depth++) {
                        float rand_point = udist(rng);
                        src = src << 1;
                        dst = dst << 1;
                        if (rand_point < A+B) {
                            if (rand_point > A) {
                                dst++;
                            }
                        } 
                        else {
                            src++;
                            if (rand_point > A+B+C) {
                                dst++;
                            }
                        }
                    }
                    fileAppPacket pckt;
                    pckt.weight = g_val(g_generator);
                    pckt.dst = src;
                    pckt.src = dst;
                    if(pckt.dst == pckt.src) continue;
                    genSelector->send(0, pckt, GMapper->to_host(pckt.src));
                    if(cfg->undirected) {
                        std::swap(pckt.dst, pckt.src);
                        genSelector->send(0, pckt, GMapper->to_host(pckt.src));
                    }
                }
            }
        }
        else {
            std::mt19937 rng;
            for (size_t block = MYTHREAD; block < global_num_blocks_; block += THREADS) {
                rng.seed(kRandSeed + block);
                std::uniform_int_distribution<VERTEX> udist(0, global_num_nodes-1);
                for (size_t m = 0; m < block_num_edges_; m++) {
                    VERTEX u = udist(rng);
                    VERTEX v = udist(rng);
                    fileAppPacket pckt;
                    pckt.weight = g_val(g_generator);
                    pckt.dst = u;
                    pckt.src = v;
                    if(pckt.dst == pckt.src) continue;
                    genSelector->send(0, pckt, GMapper->to_host(pckt.src));
                    if(cfg->undirected) {
                        std::swap(pckt.dst, pckt.src);
                        genSelector->send(0, pckt, GMapper->to_host(pckt.src));
                    }
                }
            }
        }
        genSelector->done(0);
    });
    delete genSelector;
    for(auto el: *G) {
        //std::sort(el.begin(), el.end());
        //this->SQUISH(&(el));
    }
}

void GRAPH::READ_GRAPH() {
    FileSelector* fileSelector = new FileSelector(this);
    hclib::finish([=]() {
        struct stat stats;
        std::ifstream file(cfg->fileName);
        if (!file.is_open()) { 
            report("ERROR_OPEN_FILE"); 
        }
        std::string line;
        stat(cfg->fileName, & stats);

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
            if (line[0] == '#' || line[0] == '%') continue;
            fileAppPacket pckt;
            std::stringstream ss(line);
            if(cfg->weighted == true) {
                ss >> pckt.dst >> pckt.src >> pckt.weight;
                if(strstr(cfg->fileName, "GAP") != NULL) {
                    #ifdef ASSERT
                        if(pckt.dst != pckt.src) {
                            std::string trace_msg = "weight: ";
                            trace_msg += std::to_string(pckt.weight);
                            ASSERT_WITH_MESSAGE(pckt.weight < 256, trace_msg);
                        }
                    #endif
                    pckt.weight = (double) pckt.weight/(256.0);   
                }
            }
            else {
                ss >> pckt.dst >> pckt.src;
                pckt.weight = g_val(g_generator);
            }
            if(pckt.dst == pckt.src) continue; /*MTX first line automatically gets dropped!*/ 
            pckt.dst--;
            pckt.src--;
            fileSelector->send(0, pckt, GMapper->to_host(pckt.src));
            if(cfg->undirected) {
                std::swap(pckt.dst, pckt.src);
                pckt.weight = g_val(g_generator);
                fileSelector->send(0, pckt, GMapper->to_host(pckt.src));
            }
        }
        file.close();
        fileSelector->done(0);
    });
    delete fileSelector;
    for(auto el: *G) {
        std::sort(el.begin(), el.end());
        //this->SQUISH(&(el));
    }
}

VERTEX binarySearch(const EDGELIST &prefixSum, WEIGHT target, bool *invalid) {
    if (prefixSum.empty()) { 
        *invalid = true;
        return 0;
    }
    uint64_t low = 0;
    uint64_t high = prefixSum.size() - 1;
    while (low < high) {  
        uint64_t mid = low + (high - low) / 2;
        if (prefixSum[mid].second < target) {
            low = mid + 1;
        } 
        else {
            high = mid; 
        }
    }
    if (prefixSum[low].second >= target) {
        return prefixSum[low].first;
    }
    *invalid = true;
    return 0;
}

VERTEX linearSearch(EDGELIST &array, WEIGHT target, bool *invalid) {
    double total_weight = target;
    for(int i = 0; i < array.size(); i++) {
        total_weight -= array[i].second;
        if(total_weight > 0) { 
            continue;
        }
        return array[i].first;
    }
    *invalid = true;
    return 0;
}