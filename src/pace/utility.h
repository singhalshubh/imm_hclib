#define PP std::pair<uint64_t, uint64_t>

inline double logBinomial(size_t n, size_t k) {
  return n * log(n) - k * log(k) - (n - k) * log(n - k);
}

inline double generateRandomNumber(uint64_t upper_limit) {
    return (double)rand() / RAND_MAX;
}

ssize_t ThetaPrime(ssize_t x, double epsilonPrime, double l, size_t k, size_t num_nodes) {
  k = std::min(k, num_nodes/2);
  return (2 + 2. / 3. * epsilonPrime) *
         (l * std::log(num_nodes) + logBinomial(num_nodes, k) +
          std::log(std::log2(num_nodes))) *
         std::pow(2.0, x) / (epsilonPrime * epsilonPrime);
}

inline size_t Theta(double epsilon, double l, size_t k, double LB,
                    size_t num_nodes) {
  if (LB == 0) return 0;

  k = std::min(k, num_nodes/2);
  double term1 = 0.6321205588285577;  // 1 - 1/e
  double alpha = sqrt(l * std::log(num_nodes) + std::log(2));
  double beta = sqrt(term1 * (logBinomial(num_nodes, k) +
                              l * std::log(num_nodes) + std::log(2)));
  double lamdaStar = 2 * num_nodes * (term1 * alpha + beta) *
                     (term1 * alpha + beta) * pow(epsilon, -2);
  return lamdaStar / LB;
}

/*SYSTEM DESIGN*/

std::vector <std::string> split(std::string & line, char delim, uint64_t size = 0) {
  uint64_t ndx = 0, start = 0, end = 0;
  std::vector <std::string> tokens(size);

  for ( ; end < line.length(); end ++)  {
    if ( (line[end] == delim) || (line[end] == '\n') ) {
       tokens[ndx] = line.substr(start, end - start);
       start = end + 1;
       ndx ++;
  } }

  tokens[size - 1] = line.substr(start, end - start);
  return tokens;
}

bool less (const PP& lhs, const PP& rhs) { 
  return lhs.first<rhs.first || ((rhs.first==lhs.first) && lhs.second<rhs.second); 
}

template <class InputIterator1, class InputIterator2, class OutputIterator>
  void set_difference (InputIterator1 first1, InputIterator1 last1,
                                 InputIterator2 first2, InputIterator2 last2,
                                 OutputIterator *result)
{
  while (first1!=last1 && first2!=last2)
  {
    if (less(*first1, *first2)) { result->insert(*first1); ++first1; }
    else if (less(*first2, *first1)) ++first2;
    else { ++first1; ++first2; }
  }
  while (first1!=last1) {
    result->insert(*first1);++first1;
  }
}

template <class InputIterator1, class InputIterator2>
  void set_intersection (InputIterator1 first1, InputIterator1 last1,
                                   InputIterator2 first2, InputIterator2 last2,
                                   uint64_t *result)
{
  *result = 0;
  while (first1!=last1 && first2!=last2)
  {
    if (less(*first1, *first2)) ++first1;
    else if (less(*first2, *first1)) ++first2;
    else {
      (*result)++;++first1; ++first2;
    }
  }
}

void report(std::string error_msg) {
    fprintf(stderr, "%s\n", error_msg.c_str());
    exit(-1);
}

uint64_t pe(uint64_t node) {
  return static_cast<uint64_t>(node % THREADS);
}