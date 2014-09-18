#ifndef SITE_VM_LIB
#define SITE_VM_LIB

#include <stdlib.h>
#include "sitevm/sitevm.h"
//#include "sit_seg.h"
#include "sitevm/sitevm_malloc.h"
//#include "sit_thread.h"
#define MAX_SITE_THREADS 64
#define SITEVM_SEG_SIZE 1024*1024*1024

extern sitevm_seg_t* sit_segment;
//extern sit_seg* sit_segment_promo;
//extern sitevm_malloc* sitevm_segment_malloc;

extern "C" {
  void* sitemalloc (size_t size);
  void sitefree (void * ptr);
  void* sitecalloc (size_t num, size_t size);
  void sitemallocinit(sitevm_seg_t* seg);

  /*
  void siteaddconstraint (void* src, void* dest);
  // __attribute__ ((noinline)) unsigned long long TMpromotedread(unsigned long long addr);
  __attribute__ ((noinline)) unsigned long long tm_read_promo(unsigned long long* addr);
#define TM_READ_PROMOTED(x) tm_read_promo((unsigned long long*)&x)
 */ 
}

#endif
