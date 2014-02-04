#ifndef SITE_VM_LIB
#define SITE_VM_LIB

#include <stdlib.h>
#include "sit_seg.h"
#include "sit_malloc.h"
#include "sit_thread.h"


extern sit_seg* sit_segment;
extern sit_seg* sit_segment_promo;
extern sit_malloc* sit_segment_malloc;

extern "C" {
  void* hcmalloc (size_t size);
  void hcfree (void * ptr);
  void* hccalloc (size_t num, size_t size);
  void hcaddconstraint (void* src, void* dest);
  // __attribute__ ((noinline)) unsigned long long TMpromotedread(unsigned long long addr);
  __attribute__ ((noinline)) unsigned long long tm_read_promo(unsigned long long* addr);
#define TM_READ_PROMOTED(x) tm_read_promo((unsigned long long*)&x)

}

#endif
