#include <cstdio>
#include <algorithm>
#include <sys/time.h>

static int collatz(const long upper, const int threads)
{
  // compute sequence lengths
  int maxlen = 0;
  #pragma omp parallel for num_threads(threads)\
              default(none) reduction(max:maxlen)
  for (long i = 1; i <= upper; i += 2) {
    long val = i;
    int len = 1;
    while (val != 1) {
      len++;
      if ((val % 2) == 0) {
        val = val / 2;  // even
      } else {
        val = 3 * val + 1;  // odd
      }
    }
    maxlen = std::max(maxlen, len);
  }

  return maxlen;
}

int main(int argc, char *argv[])
{
  printf("Collatz v1.2\n");

  // check command line
  if (argc != 3) {fprintf(stderr, "USAGE: %s upper_bound thread_count\n", argv[0]); exit(-1);}
  const long upper = atol(argv[1]);
  if (upper < 5) {fprintf(stderr, "ERROR: upper_bound must be at least 5\n"); exit(-1);}
  if ((upper % 2) != 1) {fprintf(stderr, "ERROR: upper_bound must be an odd number\n"); exit(-1);}
  printf("upper bound: %ld\n", upper);
  const int threads = atoi(argv[2]);
  if (threads < 1) {fprintf(stderr, "ERROR: thread_count must be at least 1\n"); exit(-1);}
  printf("thread count: %d\n", threads);

  // start time
  timeval start, end;
  gettimeofday(&start, NULL);

  // execute timed code
  const int maxlen = collatz(upper,threads);

  // end time
  gettimeofday(&end, NULL);
  const double runtime = end.tv_sec - start.tv_sec + (end.tv_usec - start.tv_usec) / 1000000.0;
  printf("compute time: %.4f s\n", runtime);

  // print result
  printf("longest sequence: %d elements\n", maxlen);

  return 0;
}
