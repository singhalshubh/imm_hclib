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
#define DEBUG
#include "../lt/utility.h"
#include "../lt/configuration.h"
#include "../lt/graph.h"

#include "generateRRR+.h"
#include "selectseeds_2D.h"
#include "selectseedsIMM.h"

#include "profile.h"

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

        double tt,tt1,generateRR_time = 0, selectSeeds_time = 0;
        tt = wall_seconds();
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
        std::set<VERTEX> *influencers = new std::set<VERTEX>;
        std::map<VERTEX, std::map<VERTEX,uint64_t>* > *COCCUR;
        COCCUR = new std::map<VERTEX, std::map<VERTEX, uint64_t>*>;  
        INIT_COCCUR(COCCUR, g->global_num_nodes);
        for(int tracker = 1; tracker < std::log2(g->global_num_nodes); ++tracker) {
            ssize_t thetaPrime = ThetaPrime(tracker, epsilonPrime, l, cfg->k, g->global_num_nodes)/THREADS + 1;
            size_t delta = thetaPrime - thetaPrimePrevious;
            /*
                Clear the output
            */
            influencers->clear();
            
            /*
                do generateRR
            */

            sample->INIT(delta);
            #ifdef DEBUG
                T0_fprintf(stderr, "Delta/PE: %ld\n", delta);
            #endif
            tt1 = wall_seconds();
            sample->PERFORM_GENERATERR(g, delta, thetaPrimePrevious);  
            sample->CLEAR_GENERATERR();

            thetaPrimePrevious += delta;
            generateRR_time += wall_seconds() - tt1;
            #ifdef DEBUG
                T0_fprintf(stderr, "[ESTIMATE]Time taken to generate RR sets in sampling: %8.3lf seconds\n", wall_seconds() - tt1);
                //measureMemoryPerPE(g, sample->_LOCALE_RRsets, sample->_IMM_visited_count, COCCUR, tracker);
            #endif
            tt1 = wall_seconds();
            /*
                Perform selectseeds2D
            */

            double rr_covered = PERFORM_IMM<std::vector<uint64_t>>(g, sample->_LOCALE_RRsets, sample->_IMM_visited_count, influencers, cfg->k, COCCUR, -1, -1, 0);
            
            delete sample->_LOCALE_RRsets;
            double f = double(rr_covered/(THREADS*thetaPrimePrevious));
            selectSeeds_time += wall_seconds() - tt1;
            #ifdef DEBUG
                T0_fprintf(stderr, "[ESTIMATE]Time taken to select seeds in sampling: %8.3lf seconds\n", wall_seconds() - tt1);
                T0_fprintf(stderr, "Fraction covered: %f\n", f);
            #endif

            /*
                Check the condition
            */

            if (f >= std::pow(2, -(tracker))) {
                LB = (g->global_num_nodes * f) / (1 + epsilonPrime);
                break;
            }
        }
        size_t thetaLocal = Theta(cfg->epsilon, l, cfg->k, LB, g->global_num_nodes)/THREADS + 1;
        if (thetaLocal > thetaPrimePrevious) {
            #ifdef DEBUG
                T0_fprintf(stderr, "\nThetaFinal/PE: %ld\n", thetaLocal - thetaPrimePrevious);
                T0_fprintf(stderr, "final,STEP 2: Generate RR final\n");
            #endif
            tt1 = wall_seconds();
            size_t final_delta = thetaLocal - thetaPrimePrevious;
            sample->INIT(final_delta);
            sample->PERFORM_GENERATERR(g, final_delta, thetaPrimePrevious);
            sample->CLEAR_GENERATERR();
            thetaPrimePrevious += final_delta;
            generateRR_time += wall_seconds() - tt1;
            #ifdef DEBUG
                T0_fprintf(stderr, "Final, Time taken to generate RR sets: %8.3lf seconds\n", wall_seconds() - tt1);
                T0_fprintf(stderr, "final, STEP 3: Select Seeds\n");
            #endif  
            tt1 = wall_seconds();
            influencers->clear();
            double rr_covered = PERFORM_IMM<std::vector<uint64_t>>(g, sample->_LOCALE_RRsets, sample->_IMM_visited_count, influencers, cfg->k, COCCUR, -1, -1, 1);
            double f = double(rr_covered/(THREADS*thetaPrimePrevious));
            // delete sample->_LOCALE_RRsets;
            selectSeeds_time += wall_seconds() - tt1;
            #ifdef DEBUG
                T0_fprintf(stderr, "Final, Time taken to select seeds: %8.3lf seconds\n", wall_seconds() - tt1);
                T0_fprintf(stderr, "Fraction covered: %f\n", f);    
            #endif
        }
        // _measureNNZ(COCCUR, 1);

        //measureMemoryPerPE(g, COCCUR, sample->_IMM_visited_count, sample->_LOCALE_RRsets, 0);
        
        sample->DELETE_GENERATERR();
        delete sample;
        if(MYTHREAD == 0) {
            FILE *fp = fopen(cfg->timefileName, "a");
            fprintf(fp, "%8.3lf\n", selectSeeds_time);
            fclose(fp);
        }
        T0_fprintf(stderr, "#RRsets total/pe: %ld\n", thetaPrimePrevious);
        T0_fprintf(stderr, "Total Time: %8.3lf seconds\n", wall_seconds() - tt);
        T0_fprintf(stderr, "Total Time(generateRR): %8.3lf seconds\n", generateRR_time);
        T0_fprintf(stderr, "Total Time(selectseeds): %8.3lf seconds\n", selectSeeds_time);
        
        if(influencers->size() > 0) {
            std::ofstream fp;
            fp.open(cfg->outputfileName, std::ios::app);
            for(auto inf: *influencers) {
                fp << inf + 1 << "\n";
            }
            fp.close();
        }
        delete influencers;
        g->DEALLOCATE_GRAPH();
        delete g;
        delete cfg;
    });
    lgp_finalize();
    return EXIT_SUCCESS;
}