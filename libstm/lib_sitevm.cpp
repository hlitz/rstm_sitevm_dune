#include "stm/lib_sitevm.h"
#include <stdio.h>
#include <iostream>

//#define INSERT_MALLOC asm("mov $666, %rcx\n\t" " movl $1027, %ecx\n\t"  "xchg %rcx, %rcx");

void* sitemalloc(size_t size){
  void* address;
  address = sitevm_segment_malloc->malloc(size);
  return address;
}

void* sitecalloc(size_t num, size_t size){
  void * ptr = sitemalloc(num*size);
  //  printf( "APP calloc: %x\n", (unsigned long long)ptr);
  return ptr;
}

void sitefree(void * ptr){
  //printf("APP FREE %p", ptr);
  //free(ptr);
}

void siteaddconstraint(void* src, void* dest){
  printf("add constraint");
}


__attribute__ ((noinline)) unsigned long long TMpromotedread(unsigned long long addr){
  printf("prmoted read this should not be shown!\n");
  return 0;
  }

unsigned long long tm_read_promo(unsigned long long* addr){
  TMpromotedread((unsigned long long)addr); 
  return *addr;
} 


