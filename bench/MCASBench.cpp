/**
 *  Copyright (C) 2011
 *  University of Rochester Department of Computer Science
 *    and
 *  Lehigh University Department of Computer Science and Engineering
 *
 * License: Modified BSD
 *          Please see the file LICENSE.RSTM/ for licensing information
 */

#include <stm/config.h>
#if defined(STM_CPU_SPARC)
#include <sys/types.h>
#endif

/**
 *  Step 1:
 *    Include the configuration code for the harness, and the API code.
 */
#include <iostream>
#include <api/api.hpp>
#include "bmconfig.hpp"

/**
 *  We provide the option to build the entire benchmark in a single
 *  source. The bmconfig.hpp include defines all of the important functions
 *  that are implemented in this file, and bmharness.cpp defines the
 *  execution infrastructure.
 */
#ifdef SINGLE_SOURCE_BUILD
#include "bmharness.cpp"
#endif

/**
 *  Step 2:
 *    Declare the data type that will be stress tested via this benchmark.
 *    Also provide any functions that will be needed to manipulate the data
 *    type.  Take care to avoid unnecessary indirection.
 */

uint64_t* matrix;

/**
 *  Step 3:
 *    Declare an instance of the data type, and provide init, test, and verify
 *    functions
 */

/*** Initialize an array that we can use for our MCAS test */
void bench_init()
{
    matrix = (uint64_t*)sitemalloc(10000 * sizeof(uint64_t));
    printf("matrix addr %p\n", matrix);
    TM_BEGIN(atomic){
    for(int i =0; i<10000; i++){ 
      uint64_t initval = 0x0LL;;
      TM_WRITE(matrix[i], initval);
      //matrix[i] = initval;
    }
    }TM_END;
    //  printf("MCAS initialized to 7770000\n");
}

void bench_update(){
   TM_BEGIN(atomic) {
        for (uint64_t i = 0; i < CFG.ops; ++i) {
	  //            uint64_t loc = rand_r((uint32_t*)&local_seed) % CFG.elements;
	    //printf("read matrix %llx\n", TM_READ(matrix[loc]));
	  //if(TM_READ(matrix[0]) == 0xCAFEBABEDEADBEAFLL)
	  TM_WRITE(matrix[0], TM_READ(matrix[0]));
	  
        }
    } TM_END;
 
}

/*** Run a bunch of random transactions */
int bench_test(uintptr_t, uint32_t* seed)
{
    // cache the seed locally so we can restore it on abort
    //
    // NB: volatile needed because using a non-volatile local in conjunction
    //     with a setjmp-longjmp control transfer is undefined, and gcc won't
    //     allow it with -Wall -Werror.
    volatile uint64_t local_seed = *seed;
    uint64_t loc[100];
    for (uint64_t i = 0; i < CFG.ops; ++i) {
      loc[i] = rand_r((uint32_t*)&local_seed) % CFG.elements;
    }
    TM_BEGIN(atomic) {
        for (uint64_t i = 0; i < CFG.ops; ++i) {
	  //            uint64_t loc = rand_r((uint32_t*)&local_seed) % CFG.elements;
	    //printf("read matrix %llx\n", TM_READ(matrix[loc]));
	    TM_WRITE(matrix[loc[i]], 1+TM_READ(matrix[loc[i]]));
	    
        }
    } TM_END;
    //    std::cout << "written to i " << loc[0]+8 << " tid " << sit_thread::sit_gettid() << std::endl;
    *seed = local_seed;
    return 0;
}

/*** Ensure the final state of the benchmark satisfies all invariants */
bool bench_verify() { 
  uint sum =0;
  //sleep(1);
  TM_BEGIN(atomic) {
  for(uint i =0; i< CFG.elements; i++){
    sum += matrix[i];
    //    if(matrix[i]!=0) std::cout << "matrix i: "<<(i+8) << " val " << matrix[i] <<std::endl;
  }
  }TM_END;
  std::cout << "sum : " << sum << std::endl;
  return sum == (CFG.execute*CFG.ops*CFG.threads);}//true; }

/**
 *  Step 4:
 *    Include the code that has the main() function, and the code for creating
 *    threads and calling the three above-named functions.  Don't forget to
 *    provide an arg reparser.
 */

/*** Deal with special names that map to different M values */
void bench_reparse()
{
    if      (CFG.bmname == "")          CFG.bmname   = "MCAS";
}
