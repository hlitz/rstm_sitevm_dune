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

#include "List.hpp"
#include <time.h>

#define XBEGIN asm(" movl $1028, %ecx\n\t"  "xchg %rcx, %rcx")
#define XEND asm(" movl $1029, %ecx\n\t"  "xchg %rcx, %rcx")

uint64_t lsum[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint64_t rsum [32]= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint64_t isum[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

inline uint64_t rdtsc()
{
    uint32_t lo, hi;
    __asm__ __volatile__ (
      "xorl %%eax, %%eax\n"
      "cpuid\n"
      "rdtsc\n"
      : "=a" (lo), "=d" (hi)
      :
      : "%ebx", "%ecx" );
    return (uint64_t)hi << 32 | lo;
}


/**
 *  Step 3:
 *    Declare an instance of the data type, and provide init, test, and verify
 *    functions
 */

/*** the list we will manipulate in the experiment */
List* SET;
int elems [32];
int ielems [32];
int relems [32];
int startelems = 0;

/*** Initialize the counter */
void bench_init()
{
  //    SET = new List();
  SET = (List*)sitemalloc(sizeof(List));
 
  TM_BEGIN(atomic){//_FAST_INITIALIZATION();
 
  new (SET) List();

  for(int i=0;i<32;i++){
    elems[i] = 0;
    ielems[i] = 0;
    relems[i] = 0;
  }
  std::cout << "malloced " << std::endl;

  // warm up the datastructure
  //
  // NB: if we switch to CGL, we can initialize without transactions
    for (uint32_t w = 0; w < CFG.elements; w+=2){
      //startelems++;
      //std::cout << "insert el" << w <<" "<< CFG.elements << std::endl;
      SET->insert(w TM_PARAM);
    }
  }TM_END;//_FAST_INITIALIZATION();
  std::cout << "start elems  " << std::dec << startelems << std::endl;
  
}

void bench_update(){
  TM_BEGIN(atomic){

    SET->update( TM_PARAM_ALONE);
  }TM_END;
}
 
 
/*** Run a bunch of increment transactions */
int bench_test(uintptr_t id, uint32_t* seed)
{
  //   std::cout << "id " << id << " " << std::endl;
  //TM_BEGIN(atomic){
  long tid = id;

    for(uint o=0; o<CFG.ops; o++){
      uint32_t val = rand_r(seed) % CFG.elements;
      uint32_t act = rand_r(seed) % 100;
      //printf("insp %i\n", CFG.inspct);
      bool res = false;
      if (act < CFG.lookpct) {
	uint64_t begin = rdtsc();
	//std::cout << "up" << std::endl;
	TM_BEGIN(atomic) {
	  //	val = 2000;
	  //SET->lookup(val /*TM_PARAM*/);
	//val = 1999;
	SET->lookup(val TM_PARAM);
	} TM_END;
	lsum[tid] += (rdtsc()-begin);

      }
      else if (act < CFG.inspct) {
	//bool res = false;
	uint64_t begin = rdtsc();
	//std::cout << "----------------START INSERT ---------- tid " << tid << " val " << val <<std::endl;
	TM_BEGIN(atomic) {
	  //SET->update(TM_PARAM_ALONE);
	  res = SET->insert(val TM_PARAM);
	} TM_END;
	isum[tid] += (rdtsc()-begin);

	if(res){
	  //std::cout << "------------------------------------------------------------------------------insert el " << val << " id " << id << std::endl; 
	  elems[id]++;
	  ielems[id]++;
	  *seed = 66;
	  //std::cout << "----------------END INSERT ---------- tid " << tid <<std::endl;
	  return 1;
	}
	//std::cout << "----------------END INSERT ---------- tid " << tid <<std::endl;
      }
      else {
	//bool res =false;
	uint64_t begin = rdtsc();
	//std::cout << "----------------START REMOVE ---------- tid " << tid << " val " << val <<std::endl;
	TM_BEGIN(atomic) {
	  //SET->update(TM_PARAM_ALONE);
	  res = SET->remove(val TM_PARAM);
	} TM_END;

	rsum[tid] += (rdtsc()-begin);

	if(res){
	  //	  std::cout << "-------------------------------------------------------------------------------remove el " << val << " id " << id << std::endl; 
	  elems[id]--;
	  relems[id]++;
	  *seed = 77;
	  //std::cout << "----------------END REMOVE ---------- tid " << tid <<std::endl;
	  return -1;
	}
	//std::cout << "----------------END REMOVE ---------- tid " << tid <<std::endl;
      }
    }
    return 0;
    //}
//TM_END;
}

/*** Ensure the final state of the benchmark satisfies all invariants */
bool bench_verify() { 
  int sum = 0;
  int srelems = 0;
  int sielems = 0;
  for(int i=0; i<16; i++){
    sum += elems[i];
    srelems += relems[i];
    sielems += ielems[i];
    //std::cout << "tid " << i << " : " << elems[i] << std::endl; 
  }
  //std::cout << "relems " << srelems << " ielems " << sielems << std::endl;
  //std::cout << "sum " << sum << " sum + startelems " << sum+startelems << std::endl;
  for(int t=0; t<8; t++){
    //std::cout << "Thread "<<t<< " lookup time: " << lsum[t] << " insert " << isum[t] << " remove " << rsum[t] << std::endl;
  }
  bool ret = false;
  TM_BEGIN(atomic){
    ret = SET->isSane();
  } TM_END; 
  TM_BEGIN(atomic){
    ret = SET->isSane();
  } TM_END; 
  TM_BEGIN(atomic){
    ret = SET->isSane();
  } TM_END; 
  return ret;
}

/**
 *  Step 4:
 *    Include the code that has the main() function, and the code for creating
 *    threads and calling the three above-named functions.  Don't forget to
 *    provide an arg reparser.
 */

/*** Deal with special names that map to different M values */
void bench_reparse()
{
    if      (CFG.bmname == "")          CFG.bmname   = "List";
    else if (CFG.bmname == "List")      CFG.elements = 256;
}
