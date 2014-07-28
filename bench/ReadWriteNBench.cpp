/**
 *  Copyright (C) 2011
 *  University of Rochester Department of Computer Science
 *    and
 *  Lehigh University Department of Computer Science and Engineering
 *
 * License: Modified BSD
 *          Please see the file LICENSE.RSTM for licensing information
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

int* matrix;

/**
 *  Step 3:
 *    Declare an instance of the data type, and provide init, test, and verify
 *    functions
 */

/*** Initialize an array that we can use for our MCAS test */
void bench_init()
{
    matrix = (int*)sitemalloc(CFG.elements*1000 * sizeof(int));
    std::cout << "allocated Matrix " << (uint64_t)matrix << " to " << ((uint64_t)matrix)+CFG.elements*sizeof(int)*8*1000 << " size " << CFG.elements * sizeof(int)*1000 << std::endl;
}

static uint64_t sum;

/*** Run a bunch of random transactions */
int bench_test(uintptr_t id, uint32_t* seed)
{
    // cache the seed locally so we can restore it on abort
    //
    // NB: volatile needed because using a non-volatile local in conjunction
    //     with a setjmp-longjmp control transfer is undefined, and gcc won't
    //     allow it with -Wall -Werror.
    volatile uint32_t local_seed = *seed;

    uint32_t val = rand_r(seed) % (CFG.elements*1000);
    
      uint32_t act = rand_r(seed) % 100;

      if (act < CFG.lookpct) {
	//std::cout << "lookup " << id << std::endl;
	int temp =0;
	TM_BEGIN(atomic) {
	  for(int i=0; i<100;i++){
	  for(uint o=0; o<CFG.ops; o++){
	    //temp += TM_READ(matrix[val+o]);
	    temp += matrix[val+o];
	  }
	  }
	} TM_END;
	sum = temp;//TM_WRITE(matrix[val], temp);
      }
      else{


	TM_BEGIN(atomic) {
	  /*for(uint o=0; o<CFG.ops; o++){
	    TM_READ(matrix[val+o]);
	    }*/
	  for(uint o=0; o<CFG.ops; o++){
	    //TM_WRITE(matrix[val], TM_READ(matrix[val])+1);
	    matrix[val]++;
	  }
	} TM_END;
	
      }
	
      /*
        int snapshot[1024];
        uint32_t loc[1024];
        for (uint32_t i = 0; i < CFG.ops; ++i) {
            loc[i] = rand_r((uint32_t*)&local_seed) % CFG.elements;
            snapshot[i] = TM_READ(matrix[loc[i]]);
        }
        for (uint32_t i = 0; i < CFG.ops; ++i) {
            TM_WRITE(matrix[loc[i]], 1 + snapshot[i]);
        }
	} TM_END;*/
      //*seed = local_seed;
    return 0;
}

/*** Ensure the final state of the benchmark satisfies all invariants */
bool bench_verify() { 
  std::cout << sum << std::endl;
return true; }

/**
 *  Step 4:
 *    Include the code that has the main() function, and the code for creating
 *    threads and calling the three above-named functions.  Don't forget to
 *    provide an arg reparser.
 */

/*** Deal with special names that map to different M values */
void bench_reparse()
{
    if      (CFG.bmname == "")          CFG.bmname   = "ReadWriteN";
}
