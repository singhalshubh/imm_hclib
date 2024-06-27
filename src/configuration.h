#define THREADS shmem_n_pes()
#define MYTHREAD shmem_my_pe()

#define IC 0
#define LT 1

#define ESTIMATE_THETA 0
#define FINAL 1

class CONFIGURATION {
    public:
        char fileName[200];
        char timefileName[200];
        char outputfileName[200];
        char diffusion_model[200];
        double epsilon;
        uint64_t k;
        bool type;
        bool undirected = false;
        bool weighted = false;
        uint64_t TRACE_SENDS[4] = {0};
        bool STATE;
        void GET_ARGS_FROM_CMD(int argc, char* argv[]) {
            int opt;
            while( (opt = getopt(argc, argv, "huwf:e:k:o:d:t:")) != -1 ) {
                switch(opt) {
                    case 'h': fprintf(stderr, "[HELP]: imm-collective -f <filename> -e <epsilon> -k <#ofinfluencers> -o outputfilename -d <IC/LT> -u<1 for unweighted>, -w<1 for weighted>"); break;
                    case 'f': sscanf(optarg,"%s" , fileName); break;
                    case 'e': sscanf(optarg,"%lf" , &(epsilon)); break;
                    case 'k': sscanf(optarg,"%ld", &(k)); break;
                    case 'd':   sscanf(optarg, "%s", diffusion_model); 
                                if(std::strcmp(diffusion_model, "IC") == 0) type = IC;
                                if(std::strcmp(diffusion_model, "LT") == 0) type = LT;
                                break;
                    case 'o': sscanf(optarg,"%s" , outputfileName); break;
                    case 't': sscanf(optarg,"%s" , timefileName); break;
                    case 'u': undirected = true; break;
                    case 'w': weighted = true; break;
                    default:  break;
                }
            }
            #ifdef DEBUG
                T0_fprintf(stderr, "Application: IMM, Filename: %s, number of influencers: %ld, epsilon = %f, output file: %s, Model: %s, Is un-directed: %d, Is weighted: %d\n\n", fileName, k, epsilon, outputfileName, diffusion_model, undirected, weighted);
            #endif
        }
};