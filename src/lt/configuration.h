enum class genType {
    RMAT = 0,
    UNIFORM = 1
};

enum class MapperType {
    Cyclic = 1,
    Range = 2,
    XOR = 3
};

class CONFIGURATION {
    public:
        char fileName[200];
        char outputfileName[200];
        char timefileName[200];
        double epsilon;
        uint64_t k;
        MapperType m_type;
        bool undirected = false;
        genType TYPE = genType::RMAT;
        int scale_ = -1;
        int degree_ = -1;
        bool weighted = false;
        void GET_ARGS_FROM_CMD(int argc, char* argv[]) {
            int opt;
            while( (opt = getopt(argc, argv, "ugwcrxe:k:o:s:d:f:t:")) != -1 ) {
                switch(opt) {
                    case 't': sscanf(optarg,"%s" , timefileName); break;
                    case 'e': sscanf(optarg,"%lf" , &(epsilon)); break;
                    case 'k': sscanf(optarg,"%ld", &(k)); break;
                    case 'o': sscanf(optarg,"%s" , outputfileName); break;
                    case 'u': undirected = true; break;
                    case 'g': TYPE = genType::UNIFORM; break;
                    case 's': sscanf(optarg,"%d" , &(scale_)); break;
                    case 'd': sscanf(optarg,"%d" , &(degree_)); break;
                    case 'f': sscanf(optarg,"%s" , fileName); break;
                    case 'w': weighted = true; break;
                    case 'c': m_type = MapperType::Cyclic; break;
                    case 'r': m_type = MapperType::Range; break;
                    case 'x': m_type = MapperType::XOR; break;
                    default:  break;
                }
            }
            T0_fprintf(stderr, "Application: IMM (Only for LT), Number of influencers: %ld, epsilon = %f, output file: %s, Is un-directed: %d, Is-weighted: %d, scale: %d, degree: %d, file: %s\n\n", k, epsilon, outputfileName, undirected, weighted, scale_, degree_, fileName);
        }
};