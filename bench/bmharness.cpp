/**
 *  Copyright (C) 2011
 *  University of Rochester Department of Computer Science
 *    and
 *  Lehigh University Department of Computer Science and Engineering
 *
 * License: Modified BSD
 *          Please see the file LICENSE.RSTM for licensing information
 */

#ifndef BMHARNESS_HPP__
#define BMHARNESS_HPP__

#include <cstdlib>
#include <iostream>
#include <signal.h>
#include <pthread.h>
#include <api/api.hpp>
#include <common/platform.hpp>
#include <common/locks.hpp>
#include "bmconfig.hpp"
#include "sitevm/sitevm.h"
//#include "sitevm/sitevm_malloc.h"
//#include "sit_thread.h"

using std::string;
using std::cout;

Config::Config() :
    bmname(""),
    duration(1),
    execute(0),
    threads(1),
    nops_after_tx(0),
    elements(256),
    lookpct(34),
    inspct(66),
    sets(1),
    ops(1),
    time(0),
    running(true),
    txcount(0)
{
}

Config CFG TM_ALIGN(64);

namespace
{

  /**
   * Print benchmark configuration output
   */
  void dump_csv()
  {
      // csv output
      std::cout << "csv"
                << ", ALG=" << TM_GET_ALGNAME()
                << ", B=" << CFG.bmname     << ", R=" << CFG.lookpct
                << ", d=" << CFG.duration   << ", p=" << CFG.threads
                << ", X=" << CFG.execute    << ", m=" << CFG.elements
                << ", S=" << CFG.sets       << ", O=" << CFG.ops
                << ", txns=" << CFG.txcount << ", time=" << CFG.time
                << ", throughput="
                << (1000000000LL * CFG.txcount) / (CFG.time)
                << std::endl;
  }

  /**
   *  Print usage
   */
  void usage()
  {
      std::cerr << "Usage: CounterBench -C <stm algorithm> [flags]\n";
      std::cerr << "    -d: number of seconds to time (default 1)\n";
      std::cerr << "    -X: execute fixed tx count, not for a duration\n";
      std::cerr << "    -p: number of threads (default 1)\n";
      std::cerr << "    -N: nops between transactions (default 0)\n";
      std::cerr << "    -R: % lookup txns (remainder split ins/rmv)\n";
      std::cerr << "    -m: range of keys in data set\n";
      std::cerr << "    -B: name of benchmark\n";
      std::cerr << "    -S: number of sets to build (default 1)\n";
      std::cerr << "    -O: operations per transaction (default 1)\n";
      std::cerr << "    -h: print help (this message)\n\n";
  }

/**
 *  Parse command line arguments
 */
void
parseargs(int argc, char** argv)
{
    // parse the command-line options
    int opt;
    while ((opt = getopt(argc, argv, "N:d:p:hX:B:m:R:S:O:")) != -1) {
        switch(opt) {
          case 'd': CFG.duration      = strtol(optarg, NULL, 10); break;
          case 'p': CFG.threads       = strtol(optarg, NULL, 10); break;
          case 'N': CFG.nops_after_tx = strtol(optarg, NULL, 10); break;
          case 'X': CFG.execute       = strtol(optarg, NULL, 10); break;
          case 'B': CFG.bmname        = std::string(optarg); break;
          case 'm': CFG.elements      = strtol(optarg, NULL, 10); break;
          case 'S': CFG.sets          = strtol(optarg, NULL, 10); break;
          case 'O': CFG.ops           = strtol(optarg, NULL, 10); break;
          case 'R':
            CFG.lookpct = strtol(optarg, NULL, 10);
            CFG.inspct = (100 - CFG.lookpct)/2 + strtol(optarg, NULL, 10);
            break;
          case 'h':
            usage();
        }
    }
}

/**
 *  Run some nops between transactions, to simulate some time being spent on
 *  computation
 */
void
nontxnwork()
{
    if (CFG.nops_after_tx)
        for (uint32_t i = 0; i < CFG.nops_after_tx; i++)
            spin64();
}

/*** Signal handler to end a test */
extern "C" void catch_SIGALRM(int) {
  std::cout << "alarm!! "<<std::endl;
    CFG.running = false;
}

/**
 *  Support a few lightweight barriers
 */
//  uint32_t* barriers;
void
barrier(uint32_t which)
{
  
  //  static volatile bool initialized = false;
  /*if(!initialized){
    barriers = (uint32_t*)malloc(16*sizeof(uint32_t));
    initialized = true;
  }*/
    static volatile uint32_t barriers[16] = {0};
    CFENCE;
    fai32(&barriers[which]);
    while (barriers[which] != CFG.threads) { }
    CFENCE;
}

/*** Run a timed or fixed-count experiment */
int32_t
run(uintptr_t id)
{

    // create a transactional context (repeat calls from thread 0 are OK)
    TM_THREAD_INIT();
    //printf("thread init \n");
    int32_t inserts = 0;
    // wait until all threads created, then set alarm and read timer
    barrier(0);
    //    printf("after barrier\n");
    //sit_thread::sit_thread_barrier_wait(0);
        if (id == 0) {
        if (!CFG.execute) {
            signal(SIGALRM, catch_SIGALRM);
            alarm(CFG.duration);
        }
        CFG.time = getElapsedTime();
    }

    // wait until read of start timer finishes, then start transactios
    barrier(1);
    //sit_thread::sit_thread_barrier_wait(1);
    uint32_t count = 0;
    uint32_t seed = id; // not everyone needs a seed, but we have to support it
    if (!CFG.execute) {
        // run txns until alarm fires
        while (CFG.running) {
            inserts += bench_test(id, &seed);

	    //if(seed==66) inserts++;
	    //else if(seed==77) inserts--;
            ++count;
            nontxnwork(); // some nontx work between txns?
        }
    }
    else {
      // run fixed number of txns
        for (uint32_t e = 0; e < CFG.execute; e++) {
	  //
	  //	  printf("e %i tid %i\n", e, id);
	  //	  std::cout << "executing tid " << e  << " TID " << std::endl;
            inserts += bench_test(id, &seed);
	    //	    if(e%10000==0) std::cout << "10 k iters id : "<< sit_thread::sit_gettid() << " e: " << e << std::endl;
	    //if(seed==66) inserts++;
	    //else if(seed==77) inserts--;
            ++count;
            nontxnwork(); // some nontx work between txns?
        }
    }
    //    std::cout << "going to sleep"<< std::endl;
    //sleep(1);
    //bench_test(id, &seed);
    //std::cout <<"awake " <<std::endl;
    // wait until all txns finish, then get time
    barrier(2);
    //sit_thread::sit_thread_barrier_wait(7);
    //bench_update();
    //bench_update();
    if (id == 0)
        CFG.time = getElapsedTime() - CFG.time;

    // add this thread's count to an accumulator
    faa32(&CFG.txcount, count);
    //    std::cout << "in run() per thread inserts : " << pthread_self() << std::dec << " : " << inserts << std::endl;
    return inserts;
}

/**
 *  pthread wrapper for running the experiments
 *
 *  NB: noinline prevents this from getting inlined into main (and prevents
 *      run from being inlined there as well. This eliminates an
 *      _ITM_initializeProcess ordering problem if there's a transaction
 *      lexically scoped inside of main.
 */
NOINLINE
void*
run_wrapper(void* i)
{
  
    int64_t inserts = run((uintptr_t)i);
    //bench_verify();
    printf("shutting down thread\n");
    //TM_THREAD_SHUTDOWN();
    return (void*)inserts;// NULL;
}
}

/**
 *  Main routine: parse args, set up the TM system, prep the benchmark, run
 *  the experiments, verify results, print results, and shut down the system
 */
int main(int argc, char** argv) {
  parseargs(argc, argv);
  bench_reparse();
  TM_SYS_INIT();
  //std::cout << "-------------------------------setting numthreads " << std::endl;
  //sit_thread::sit_thread_set_numThread(CFG.threads);
  TM_THREAD_INIT();
  bench_init();

  void* args[256];
  pthread_t tid[256];
  int inserts = 0;
  // set up configuration structs for the threads we'll create
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  for (uintptr_t i = 0; i < CFG.threads; i++)
    args[i] = reinterpret_cast<void*>(i);

  
  struct timespec start, finish;
  double elapsed;
  
  //clock_gettime(CLOCK_MONOTONIC, &start);
  
  //sit_thread::sit_thread_set_numThread(CFG.threads);
    // actually create the threads
  for (uint32_t j = 1; j < CFG.threads; j++){
  // sit_thread::sit_thread_create(&run_wrapper, args[j]);
    pthread_create(&tid[j], &attr, &run_wrapper, args[j]);
  }
  // all of the other threads should be queued up, waiting to run the
  // benchmark, but they can't until this thread starts the benchmark
  // too...
  inserts += (intptr_t)run_wrapper(args[0]);
  //std::cout << " retval tid : "<< sit_thread::sit_gettid() << " : " <<inserts << std::endl;
  
  // Don't call any more transactional functions, because run_wrapper called
  // shutdown.

  // everyone should be done.  Join all threads so we don't leave anything
  // hanging around
  for (uint32_t k = 1; k < CFG.threads; k++){
    void* retval;
    pthread_join(tid[k], &retval);
    //sit_thread::sit_thread_join(k, &retval);
    inserts += (intptr_t)retval;
    //std::cout << "retval "<< sit_thread::sit_gettid() << " : "  << (intptr_t)retval << std::endl;
  }
  //clock_gettime(CLOCK_MONOTONIC, &finish);
    
  elapsed = (finish.tv_sec - start.tv_sec);
  elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
  std::cout << "time: " << elapsed <<  std::endl;
    
  bool v = bench_verify();
  std::cout << "Verification: " << (v ? "Passed" : "Failed") << "\n";
  std::cout << "Sum of inserts/deletions " << inserts << std::endl;

  dump_csv();

  // And call sys shutdown stuff
  TM_SYS_SHUTDOWN();
  return 0;
}

#endif // BMHARNESS_HPP__
