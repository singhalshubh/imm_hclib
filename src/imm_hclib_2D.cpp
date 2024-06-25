#include <math.h>
#include <shmem.h>
extern "C" {
#include <spmat.h>
}
#include <std_options.h>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <unordered_map>
#include <queue>
#include <fstream>
#include <sys/stat.h>
#include <sys/time.h>
#include "selector.h"
#include <endian.h>
#include <ctime> 
#include <cstdlib> 
#include <random>
#include <sys/time.h>
#include <cmath>
#include <cstddef>
#include <limits>
#include "trng/lcg64.hpp"
#include "trng/uniform01_dist.hpp"
#include "trng/uniform_int_dist.hpp"
#include <chrono>
#include <utility>
#include <memory>
#include <sstream>

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#include <machine/endian.h>
#endif
//#define DEBUG
#define PREFIX
// #define TRACE
// #define IMM_SELECTOR
#define COVARIANCE 

#include "configuration.h"
#include "utility.h"
#include "graph.h"
#include "generateRR.h"
#ifdef COVARIANCE
    #include "selectseeds_2D.h"
#else
    #include "selectseeds.h"
#endif

int main (int argc, char* argv[]) {
    static long lock = 0;
    const char *deps[] = { "system", "bale_actor" };
    hclib::launch(deps, 2, [=] {
        CONFIGURATION *cfg = new CONFIGURATION;
        cfg->GET_ARGS_FROM_CMD(argc, argv);
        
        GRAPH *g = new GRAPH;
        g->cfg = cfg;
        g->LOAD_GRAPH();

        struct timeval tt,rr,tt1,rr1,generateRR_time = {0}, selectSeeds_time = {0};
        gettimeofday(&tt, NULL);
        #ifdef DEBUG
            T0_fprintf(stderr, "STEP 1: Sampling\n");
        #endif
        generator.seed(0UL);
        generator.split(2, 1);
        generator.split(THREADS, MYTHREAD);
        double l = 1.0;
        l = l * (1 + 1 / std::log2(g->total_num_nodes));
        double epsilonPrime =  1.4142135623730951 * cfg->epsilon;
        double LB = 0;
        size_t thetaPrimePrevious = 0;
        #ifdef COVARIANCE
            std::map<uint64_t, uint64_t> *_IMMvisited = new std::map<uint64_t, uint64_t>;
            //std::vector<std::vector<uint64_t>> COVAR(g->total_num_nodes, std::vector<uint64_t>(g->G->size(), 0));
            std::unordered_map<DST, std::unordered_map<SRC,uint64_t>* > *COVAR = new std::unordered_map<DST, std::unordered_map<SRC,uint64_t>*>;  
            for(int ik = 0; ik < g->total_num_nodes; ik++) {
                std::unordered_map<SRC,uint64_t> * temp = new std::unordered_map<SRC,uint64_t>  ;
                COVAR->insert(std::make_pair(ik, temp));
            }
            _defineMappings(g);
        #else
            std::map<SRC, std::set<TAG>*> *visited = new std::map<SRC, std::set<TAG>*>;
        #endif
        #ifdef TRACE
            cfg->STATE = ESTIMATE_THETA;
        #endif
        for(int tracker = 1; tracker < std::log2(g->total_num_nodes); ++tracker) {
            #ifdef COVARIANCE
                std::map<BFS_ITERATION_INDEX,std::vector<DST>*> *RRsets = new std::map<BFS_ITERATION_INDEX,std::vector<DST>*>;
                std::map<SRC, std::set<TAG>*> *visited = new std::map<SRC, std::set<TAG>*>;
            #endif
            ssize_t thetaPrime = ThetaPrime(tracker, epsilonPrime, l, cfg->k, g->total_num_nodes)/THREADS + 1;
            size_t delta = thetaPrime - thetaPrimePrevious;

            std::vector<int> *root_vertex = new std::vector<int>;

            #ifdef DEBUG
                T0_fprintf(stderr, "Delta/PE: %ld\n", delta);
                gettimeofday(&tt1, NULL);
            #endif
            #ifdef PREFIX
                trng::uniform_int_dist start(1, g->total_num_nodes+1);
            #else
                trng::uniform_int_dist start(0, g->total_num_nodes);
            #endif
            for(uint64_t tracker1 = 0; tracker1 < delta; tracker1++) {
                root_vertex->push_back(start(generator));
            }
            #ifdef COVARIANCE
                GENERATE_RRR(g, visited, root_vertex, thetaPrimePrevious, RRsets, _IMMvisited);
            #else
                GENERATE_RRR(g, visited, root_vertex, thetaPrimePrevious);
            #endif
            lgp_barrier();
            delete root_vertex;
            #ifdef COVARIANCE
                delete visited;
            #endif
            thetaPrimePrevious += delta;
            
            #ifdef DEBUG
                gettimeofday(&rr1, NULL);
                timersub(&rr1, &tt1, &rr1);
                timeradd(&rr1, &generateRR_time, &generateRR_time);
                T0_fprintf(stderr, "[ESTIMATE]Time taken to generate RR sets in sampling:%8.3lf seconds\n", rr1.tv_sec + (double) rr1.tv_usec/(double)1000000);
            #endif
            #ifdef DEBUG
                gettimeofday(&tt1, NULL);
            #endif
            std::set<uint64_t> *influencers = new std::set<uint64_t>;
            #ifdef COVARIANCE
                double rr_covered = PERFORM_IMM(g, RRsets, _IMMvisited, influencers, COVAR);
            #else
                double rr_covered = PERFORM_IMM(g, visited, influencers);
            #endif
            lgp_barrier();
            delete influencers;
            #ifdef COVARIANCE
                delete RRsets;
            #endif
            double f = double(rr_covered/(THREADS*thetaPrimePrevious));
            
            #ifdef DEBUG
                gettimeofday(&rr1, NULL);
                timersub(&rr1, &tt1, &rr1);
                timeradd(&rr1, &selectSeeds_time, &selectSeeds_time);
                T0_fprintf(stderr, "[ESTIMATE]Time taken to select seeds in sampling: %8.3lf seconds\n", rr1.tv_sec + (double) rr1.tv_usec/(double)1000000);
                T0_fprintf(stderr, "Fraction covered: %f\n", f);
            #endif
            
            if (f >= std::pow(2, -(tracker))) {
                LB = (g->total_num_nodes * f) / (1 + epsilonPrime);
                break;
            }
        }
        #ifdef TRACE
            cfg->STATE = FINAL;
        #endif
        size_t thetaLocal = Theta(cfg->epsilon, l, cfg->k, LB, g->total_num_nodes)/THREADS + 1;
        #ifdef DEBUG
            T0_fprintf(stderr, "\nThetaFinal/PE: %ld\n", thetaLocal - thetaPrimePrevious);
            T0_fprintf(stderr, "final,STEP 2: Generate RR final\n");
            gettimeofday(&tt1, NULL);
        #endif
        #ifdef COVARIANCE
            std::map<BFS_ITERATION_INDEX,std::vector<DST>*> *RRsets = new std::map<BFS_ITERATION_INDEX,std::vector<DST>*>;
            std::map<SRC, std::set<TAG>*> *visited = new std::map<SRC, std::set<TAG>*>;
        #endif
        if (thetaLocal > thetaPrimePrevious) {
            size_t final_delta = thetaLocal - thetaPrimePrevious;
            std::vector<int> *root_vertex = new std::vector<int>;
            #ifdef PREFIX
                trng::uniform_int_dist start(1, g->total_num_nodes+1);
            #else
                trng::uniform_int_dist start(0, g->total_num_nodes);
            #endif
            for(uint64_t tracker1 = 0; tracker1 < final_delta; tracker1++) {
                root_vertex->push_back(start(generator));
            }
            #ifdef COVARIANCE
                GENERATE_RRR(g, visited, root_vertex, thetaPrimePrevious, RRsets, _IMMvisited);
            #else
                GENERATE_RRR(g, visited, root_vertex, thetaPrimePrevious);
            #endif
            delete root_vertex;
        }
        #ifdef DEBUG
            gettimeofday(&rr1, NULL);
            timersub(&rr1, &tt1, &rr1);
            timeradd(&rr1, &generateRR_time, &generateRR_time);
            T0_fprintf(stderr, "Time taken to select generate RRR sets: %8.3lf seconds\n", rr1.tv_sec + (double) rr1.tv_usec/(double)1000000);
            T0_fprintf(stderr, "final,STEP 3: Select Seeds\n");
            gettimeofday(&tt1, NULL);
        #endif   
        
        std::set<uint64_t> *influencers = new std::set<uint64_t>;
        #ifdef COVARIANCE
            double rr_covered = PERFORM_IMM(g, RRsets, _IMMvisited, influencers, COVAR);
        #else
            double rr_covered = PERFORM_IMM(g, visited, influencers);
        #endif
        lgp_barrier();
        delete visited;
        #ifdef COVARIANCE
            delete _IMMvisited;
            delete RRsets;
            delete COVAR;
        #endif
        #ifdef DEBUG
            gettimeofday(&rr1, NULL);
            timersub(&rr1, &tt1, &rr1);
            timeradd(&rr1, &selectSeeds_time, &selectSeeds_time);
            T0_fprintf(stderr, "Time taken to select seeds: %8.3lf seconds\n", rr1.tv_sec + (double) rr1.tv_usec/(double)1000000);
        #endif
        gettimeofday(&rr, NULL);
        timersub(&rr, &tt, &rr);
        T0_fprintf(stderr, "%lf", rr.tv_sec + (double) rr.tv_usec/(double)1000000);
        #ifdef DEBUG
            T0_fprintf(stderr, "Total Time(generateRR): %8.3lf seconds\n", generateRR_time.tv_sec + (double) generateRR_time.tv_usec/(double)1000000);
            T0_fprintf(stderr, "Total Time(selectseeds): %8.3lf seconds\n", selectSeeds_time.tv_sec + (double) selectSeeds_time.tv_usec/(double)1000000);
        #endif
        
        #ifdef TRACE
            uint64_t numOfSends = lgp_reduce_add_l(cfg->TRACE_SENDS[0]);
            T0_fprintf(stderr, "Number of sends in generateRR(Estimate Phase): %ld\n", numOfSends);
            numOfSends = lgp_reduce_add_l(cfg->TRACE_SENDS[1]);
            T0_fprintf(stderr, "Number of sends in generateRR(Final Phase): %ld\n", numOfSends);
            numOfSends = lgp_reduce_add_l(cfg->TRACE_SENDS[2]);
            T0_fprintf(stderr, "Number of sends in SelectSeeds(Estimate Phase): %ld\n", numOfSends);
            numOfSends = lgp_reduce_add_l(cfg->TRACE_SENDS[3]);
            T0_fprintf(stderr, "Number of sends in SelectSeeds(Final Phase): %ld\n", numOfSends);
        #endif
        #ifdef PREFIX
            if(influencers->size() > 0) {
                std::ofstream fp;
                fp.open(cfg->outputfileName, std::ios::app);
                for(auto inf: *influencers) {
                    fp << inf+1 << "\n";
                }
                fp.close();
            }
        #else
            if(influencers->size() > 0) {
                std::ofstream fp;
                fp.open(cfg->outputfileName, std::ios::app);
                for(auto inf: *influencers) {
                    fp << g->globalToUserVerticeMap->find(inf)->second << "\n";
                }
                fp.close();
            }
        #endif
        lgp_barrier();
        delete influencers;
        delete g;
        delete cfg;
        lgp_finalize();
    });
    return EXIT_SUCCESS;
}