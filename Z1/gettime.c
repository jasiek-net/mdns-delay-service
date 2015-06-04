#include <sys/time.h>
#include <inttypes.h>
#define __STDC_FORMAT_MACROS

uint64_t gettime() {
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec*1000000ull+tv.tv_usec;
}
