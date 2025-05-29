#include <math.h>
#include <shmem.h>
extern "C" {
#include <spmat.h>
}
#include <std_options.h>
#include <string>
#include <set>
#include <map>
#include <unordered_map>
#include <vector>
#include <queue>
#include <fstream>
#include <sys/stat.h>
#include <sys/time.h>

//#define ENABLE_TCOMM_PROFILING
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
#include <stdint.h>

//#include "../tsl/sparse_map.h"
// #include <sparsehash/sparse_hash_map>
#define CUSTOMAP std::map

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#include <machine/endian.h>
#endif
#define DEBUG

#include "../lt/utility.h"
#include "../lt/configuration.h"
#include "../lt/graph.h"
#include "profile.h"
//#include "../async-RR/generateRR.h"
#include "generateRR.h"
#include "selectseeds.h"

int main (int argc, char* argv[]) {
    static long lock = 0;
    const char *deps[] = { "system", "bale_actor" };
    hclib::launch(deps, 2, [=] {
        /* MASTER: IMM configuration parameters */

        /*########## Generate and Build Graph ##############*/
        /*#################################################*/

        CONFIGURATION *cfg = new CONFIGURATION;
        cfg->GET_ARGS_FROM_CMD(argc, argv);

        GRAPH *g = new GRAPH;
        g->cfg = cfg;
        g->LOAD_GRAPH();

        /*#################################################*/
        /*############# IMM Math and time init ####################*/
        /*#################################################*/

        struct timeval tt,rr,tt1,rr1,generateRR_time = {0}, selectSeeds_time = {0};
        gettimeofday(&tt, NULL);
        #ifdef DEBUG
            T0_fprintf(stderr, "STEP 1: Sampling\n");
        #endif

        generator.seed(0UL);
        generator.split(2, 1);
        generator.split(THREADS, MYTHREAD);
        double l = 1.0;
        l = l * (1 + 1 / std::log2(g->global_num_nodes));
        double epsilonPrime =  1.4142135623730951 * cfg->epsilon;
        double LB = 0;
        size_t thetaPrimePrevious = 0;

        /*#################################################*/
        /*############# DATA Structures init ###############*/
        /*#################################################*/

        GENERATE_RRR *sample = new GENERATE_RRR();
        CUSTOMAP<VERTEX, std::set<TAG>*> *visited = new CUSTOMAP<VERTEX, std::set<TAG>*>;
        std::set<VERTEX> *influencers = new std::set<VERTEX>;

        for(int tracker = 1; tracker < std::log2(g->global_num_nodes); ++tracker) {
            ssize_t thetaPrime = ThetaPrime(tracker, epsilonPrime, l, cfg->k, g->global_num_nodes)/THREADS + 1;
            size_t delta = thetaPrime - thetaPrimePrevious;
            influencers->clear();
            #ifdef DEBUG
                T0_fprintf(stderr, "Delta/PE: %ld\n", delta);
            #endif
            gettimeofday(&tt1, NULL);
            sample->PERFORM_GENERATERR(g, visited, delta, thetaPrimePrevious);  
            sample->CLEAR_GENERATERR();

            thetaPrimePrevious += delta;
            gettimeofday(&rr1, NULL);
            timersub(&rr1, &tt1, &rr1);
            timeradd(&rr1, &generateRR_time, &generateRR_time);
            #ifdef DEBUG
                T0_fprintf(stderr, "[ESTIMATE]Time taken to generate RR sets in sampling:%8.3lf seconds\n", rr1.tv_sec + (double) rr1.tv_usec/(double)1000000);
            #endif
            
            gettimeofday(&tt1, NULL);
            double rr_covered = PERFORM_IMM(g, visited, influencers, cfg->k);
            double f = double(rr_covered/(THREADS*thetaPrimePrevious));
            
            gettimeofday(&rr1, NULL);
            timersub(&rr1, &tt1, &rr1);
            timeradd(&rr1, &selectSeeds_time, &selectSeeds_time);

            #ifdef DEBUG
                T0_fprintf(stderr, "[ESTIMATE]Time taken to select seeds in sampling: %8.3lf seconds\n", rr1.tv_sec + (double) rr1.tv_usec/(double)1000000);
                T0_fprintf(stderr, "Fraction covered: %f\n", f);
            #endif
            if (f >= std::pow(2, -(tracker))) {
                LB = (g->global_num_nodes * f) / (1 + epsilonPrime);
                break;
            }
        }
        size_t thetaLocal = Theta(cfg->epsilon, l, cfg->k, LB, g->global_num_nodes)/THREADS + 1;
        #ifdef DEBUG
            T0_fprintf(stderr, "\nThetaFinal/PE: %ld\n", thetaLocal - thetaPrimePrevious);
            T0_fprintf(stderr, "final,STEP 2: Generate RR final\n");
        #endif
        gettimeofday(&tt1, NULL);
        if (thetaLocal > thetaPrimePrevious) {
            size_t final_delta = thetaLocal - thetaPrimePrevious;
            sample->PERFORM_GENERATERR(g, visited, final_delta, thetaPrimePrevious);
            sample->CLEAR_GENERATERR();
            gettimeofday(&rr1, NULL);
            timersub(&rr1, &tt1, &rr1);
            timeradd(&rr1, &generateRR_time, &generateRR_time);
            #ifdef DEBUG
                T0_fprintf(stderr, "Time taken to select generate RRR sets: %8.3lf seconds\n", rr1.tv_sec + (double) rr1.tv_usec/(double)1000000);
                T0_fprintf(stderr, "final,STEP 3: Select Seeds\n");    
            #endif
            gettimeofday(&tt1, NULL);   
            influencers->clear();
            double rr_covered = PERFORM_IMM(g, visited, influencers, cfg->k);
            gettimeofday(&rr1, NULL);
            timersub(&rr1, &tt1, &rr1);
            timeradd(&rr1, &selectSeeds_time, &selectSeeds_time);
            #ifdef DEBUG
                T0_fprintf(stderr, "Time taken to select seeds: %8.3lf seconds\n", rr1.tv_sec + (double) rr1.tv_usec/(double)1000000);
            #endif
        }

        gettimeofday(&rr, NULL);
        timersub(&rr, &tt, &rr);
        if(MYTHREAD == 0) {
            FILE *fp = fopen(cfg->timefileName, "a");
            fprintf(fp, "%8.3lf\n", generateRR_time.tv_sec + (double) generateRR_time.tv_usec/(double)1000000);
            fclose(fp);
        }
        T0_fprintf(stderr, "Total Time: %8.3lf seconds\n", rr.tv_sec + (double) rr.tv_usec/(double)1000000);
        T0_fprintf(stderr, "Total Time(generateRR): %8.3lf seconds\n", generateRR_time.tv_sec + (double) generateRR_time.tv_usec/(double)1000000);
        T0_fprintf(stderr, "Total Time(selectseeds): %8.3lf seconds\n", selectSeeds_time.tv_sec + (double) selectSeeds_time.tv_usec/(double)1000000);
    
        if(influencers->size() > 0) {
            std::ofstream fp;
            fp.open(cfg->outputfileName, std::ios::app);
            for(auto inf: *influencers) {
                fp << inf + 1 << "\n";
            }
            fp.close();
        }
        // #ifdef DEBUG
        //     measureMemoryPerPE(g, visited);
        // #endif

        lgp_barrier();
        delete influencers;
        delete visited;
        sample->DELETE_GENERATERR();
        delete sample;
        g->DEALLOCATE_GRAPH();
        delete g;
        delete cfg;
    });
    lgp_finalize();
    return EXIT_SUCCESS;
}