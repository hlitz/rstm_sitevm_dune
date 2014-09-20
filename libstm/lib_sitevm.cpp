#include "stm/lib_sitevm.h"
//#include <stdio.h>
//#include <iostream>
#include <pthread.h>
#include <execinfo.h>

void print_backtrace(){
  void *array[100];
  size_t size;
  char **strings;
    
  printf("$$$$$$$$$$ backtrace %i $$$$$$$$\n", size);
  size = backtrace (array, 100);
  strings = backtrace_symbols (array, size);
  //std::cout << " ------------------ addr " << addr << std::endl;
  int i;
  for(i =0; i< size; i++){
    printf("%s\n",strings[i]);
    //    std::cout << strings[i] << std::endl;
    //std::string str(strings[i]);
    /*   if(uint32_t pos = str.find(filename)!=std::string::npos){
      pos = str.find("[0x", pos);
      uint32_t posend = str.find("]", pos);
      std::string substr = str.substr(pos+1, posend-1);
      //std::cout << " char " << strings[i] << " extract " << substr << endl;
      result = std::stoi(substr, nullptr, 16);
      //std::cout << " char " << strings[i] << " extract " << substr << " int "<< hex << result << std::endl;
      break;
      }*/
  }
  printf("\n$$$$$$$$$$$$$$$$$$$$$$$\n");
}

//#define INSERT_MALLOC asm("mov $666, %rcx\n\t" " movl $1027, %ecx\n\t"  "xchg %rcx, %rcx");
void sitemallocinit(sitevm_seg_t* seg){
  //  printf("lib_sitevm.cpp call malloc init\n");
  sitevm_malloc::init_sitevm_malloc(seg);
}

void* sitemalloc(size_t size){
  void* address;
  address = sitevm_malloc::smalloc(size);
  if((uint64_t)address<0x30000000000 || (uint64_t)address>0x30040000000){
    static  int allocs = 0;
    printf("LIBSITE MALLOC %p tid %i alloc nur %i\n",address, pthread_self(), allocs++);
    print_backtrace();
  }
  return address;
}

void* sitecalloc(size_t num, size_t size){
  void * ptr = sitevm_malloc::scalloc(num, size);
  if((uint64_t)ptr<0x30000000000){
    printf("LIBSITE CALLOC %p\n",ptr);
  }  
//  printf( "APP calloc: %x\n", (unsigned long long)ptr);
  return ptr;
}

void sitefree(void * ptr){
  if((uint64_t)ptr<0x30000000000 || (uint64_t)ptr>0x30040000000){
    std::cout << "freeing small address " << std::hex << (uint64_t)ptr << std::endl;
    print_backtrace();
    }
  
  //printf("LIBSITE FREE %p tid %i\n", ptr, pthread_self());
  sitevm_malloc::sfree(ptr);
  //printf("APP FREE %p", ptr);
  //free(ptr);
}
/*
void siteaddconstraint(void* src, void* dest){
  printf("add constraint");
}


__attribute__ ((noinline)) unsigned long long TMpromotedread(unsigned long long addr){
  //printf("prmoted read this should not be shown!\n");
  return 0;
  }

unsigned long long tm_read_promo(unsigned long long* addr){
  TMpromotedread((unsigned long long)addr); 
  return *addr;
} 


*/
