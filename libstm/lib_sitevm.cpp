#include "stm/lib_sitevm.h"
#include <stdio.h>
#include <iostream>

//#define INSERT_MALLOC asm("mov $666, %rcx\n\t" " movl $1027, %ecx\n\t"  "xchg %rcx, %rcx");

void* hcmalloc(size_t size){
  //std::cout << "allocating SITE-VM memory "<< std::endl;
  void* address;

  for(int retries = 0; ; retries++){
    //std::cout << "doing update " << std::endl;
    sit_segment_malloc->update();
    //std::cout << "done update" <<std::endl;
    //const size_t sz = 4;
    address = sit_segment_malloc->malloc<void*>(size);
    //std::cout << "done malloc" <<std::endl;
    
    int commit_result = sit_segment_malloc->commit();
    //std::cout << "malloc commit result " << commit_result << std::endl;
    if (commit_result == -EAGAIN){
      //Retry. malloc's cannot be "fixed up" because the returned value
      //may not be valid.
      //std::cout <<"RETRY allocate " << std::endl;
      continue;
    } else if (commit_result == 0){
      break;
    } else {
      //Any other error message should be reported.
      printf("Error in malloc %s", strerror(commit_result));
      exit(1);
    }
  }
  return address;
}

void* hccalloc(size_t num, size_t size){
  void * ptr = hcmalloc(num*size);
  //  printf( "APP calloc: %x\n", (unsigned long long)ptr);
  return ptr;
}

void hcfree(void * ptr){
  //printf("APP FREE %p", ptr);
  //free(ptr);
}

void hcaddconstraint(void* src, void* dest){
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


