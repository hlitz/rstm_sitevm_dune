/**
 *  Copyright (C) 2013
 *  Heiner Litz, Stanford University
 */

/**
 * Hardware TM Implementation using ZSIM, we only define entry points for
 * the simulator, plus some debugging and stack tracing stuff that is 
 * disabled per default 
 */

#include "../profiling.hpp"
#include "algs.hpp"
#include "RedoRAWUtils.hpp"
#include <iostream>
#include <vector>
#include <execinfo.h>
#include <map>
#include <set>
#include <string>
#include "stm/lib_sitevm.h"

using namespace std;
using stm::TxThread;

sitevm_seg_t* sit_segment;
//sit_seg* sit_segment_promo;
//sit_malloc* sit_segment_malloc;
sitevm_malloc* sitevm_segment_malloc;


//Some debugging facilities
const bool DEBUG_BACKTRACE = false;//true;
const bool BENCH = true;
const bool MVCC = false;
//const uint64_t PROMO_LIST_SIZE = 1024;

//uint64_t* promo_list[MAX_SITE_THREADS];
//uint64_t promo_list_ptr[MAX_SITE_THREADS];
/**
 *  Declare the functions that we're going to implement, so that we can avoid
 *  circular dependencies.
 */
namespace {
  struct SITE_VM
  {
    static TM_FASTCALL bool begin(TxThread*);
    static TM_FASTCALL void site_update();
    static TM_FASTCALL void site_commit();
    static TM_FASTCALL void* read_ro(STM_READ_SIG(,,));
    static TM_FASTCALL void* read_rw(STM_READ_SIG(,,));
    static TM_FASTCALL void* read_ro_promo(STM_READ_SIG(,,));
    static TM_FASTCALL void* read_rw_promo(STM_READ_SIG(,,));
    static TM_FASTCALL void write_ro(STM_WRITE_SIG(,,,));
    static TM_FASTCALL void write_rw(STM_WRITE_SIG(,,,));
    static TM_FASTCALL void commit_ro(TxThread*);
    static TM_FASTCALL void commit_rw(TxThread*);

    static stm::scope_t* rollback(STM_ROLLBACK_SIG(,,));
    static bool irrevoc(TxThread*);
    static void onSwitchTo();
    static NOINLINE void validate(TxThread*);
  };

  /* Fine grained exponential backoff in Software */
inline uint64_t rdtsc()
{
    uint32_t lo, hi;
    __asm__ __volatile__ (
      "xorl %%eax, %%eax\n"
      "cpuid\n"
      "rdtscp\n"
      : "=a" (lo), "=d" (hi)
      :
      : "%ebx", "%ecx" );
    return (uint64_t)hi << 32 | lo;
}

  void  inline promo_inc(char* addr){
    /*  uint64_t seg_offset = ((uint64_t)addr-(uint64_t)sit_segment->base())>>PROMO_GRANULARITY;
    uint64_t promo_offset = (uint64_t)sit_segment->base()+sit_segment->size() + seg_offset;
    //std::cout << hex << "base " <<(uint64_t)sit_segment->base() << std::endl;
    (*(uint64_t*)promo_offset)++;
    //std::cout << *(uint64_t*)promo_offset << std::endl;
*/  
}
  
  /**
   *  SITE_VM begin:
   */
  bool
  SITE_VM::begin(TxThread* tx)
  {
    //promo_list_ptr[sit_thread::sit_gettid()] = 0x0UL;
    //std::cout << "Begin TRX"<< tx->id << std::endl;
    tx->txn++;
    // uint64_t begintime, endtime;
    tx->allocator.onTxBegin();
    //std::cout << "Starting TRX " << tx->id << std::endl;
    //sit_segment->clear_promoted_reads();
    //sit_segment->update();
    //sitevm_update(sit_segment);
    //std::cout << "Started TRX " << sit_thread::sit_gettid() << std::endl;
    
    //XBEGIN;
    /*    uint64_t wait = TMstart();
    if(wait>1){
      begintime = rdtsc();
      endtime = begintime;
      while(begintime+wait>endtime && begintime+wait>begintime){
	endtime = rdtsc();
      }
      } */
    OnFirstWrite(tx, read_rw, read_rw_promo, write_rw, commit_rw);
    return false;
  }

  void
  SITE_VM::site_update(){
    //std::cout << "--------------------------------site update in sitevm" << std::endl;
    // sit_segment->update();
    //sitevm_update(sit_segment);
  }

  void
  SITE_VM::site_commit(){
    //std::cout << "--------------------------------site commit in sitevm" << std::endl;
    //sit_segment->commit();
    //sitevm_commit(sit_segment);
    sitevm_commit_and_update(sit_segment);
  }

  /**
   *  SITE_VM commit (read-only):
   */
  void
  SITE_VM::commit_ro(TxThread* tx)
  {
    //    cout << "Commit Ro TRX"<< tx->id << endl;
    //std::cout << "committing" << std::endl;
    /*   if(!TMrocommit()){
      //std::cout << "comit ro abort" << std::endl;
      tx->allocator.onTxAbort(); 
      tx->tmabort(tx);
      }*/
    //std::cout << "Commit RO TRX "<< sit_thread::sit_gettid() << endl;
    
    //int result = sit_segment->commit();
    int result = sitevm_commit_and_update(sit_segment);
    //    int result = sit_segment->commit();
    //   std::cout << "Committing TRX " << sit_thread::sit_gettid() << " result " << result << std::endl;
     if(result ==0){
       //       std::cout << "comit ro" << sit_thread::sit_gettid()<< std::endl;
       tx->allocator.onTxCommit();
       //sit_thread::stat_commit();
       OnReadOnlyCommit(tx);
    }
    else{
      //std::cout << "comit ro abort"<< sit_thread::sit_gettid() << std::endl;
      tx->allocator.onTxAbort(); 
      //sit_thread::stat_abort();
      tx->tmabort(tx);
    }

  }

  /**
   *  SITE_VM commit (writing context):
   *
   */
  void
  SITE_VM::commit_rw(TxThread* tx)
  {
    //    cout << "Commit RW TRX"<< tx->id << endl;
    //cout << "Commit RW TRX"<< sit_thread::sit_gettid() << endl;
    //    std::cout << "committing" << std::endl;
    /*bool res = TMcommit(); 
    if(!res) { 
      //std::cout << "comit rw abort" << std::endl;
      tx->allocator.onTxAbort(); 
      tx->tmabort(tx);
      }*/
    //int result = sit_segment->commit();
    //std::cout << "c and u tid: "<< tx->id << std::endl;
    int result = sitevm_commit_and_update(sit_segment);
    //std::cout << "Committing TRX " << sit_thread::sit_gettid() << " result " << result << std::endl;
    if(result ==0){
      //std::cout << "comit ro" << sit_thread::sit_gettid()<< std::endl;
      tx->allocator.onTxCommit();
      //sit_thread::stat_commit();
      OnReadWriteCommit(tx, read_rw, read_rw_promo, write_rw, commit_rw);
	 //OnReadOnlyCommit(tx);
    }
    else{
      //      std::cout << "comit rw abort" << sit_thread::sit_gettid()<< std::endl;
      tx->allocator.onTxAbort(); 
      //sit_thread::stat_abort();
      tx->tmabort(tx);
    }
    //   XEND;
    //std::cout << "comit rw" << std::endl;
    //tx->allocator.onTxCommit(); 
    //   OnReadWriteCommit(tx, read_rw, read_rw_promo, write_rw, commit_rw);
  }

  /**
   *  SITE_VM read (read-only transaction)
   * For promoted reads, SI-TM only
   */
  void*
  SITE_VM::read_ro_promo(STM_READ_SIG(tx,addr,))
  {
    //uint64_t * list = promo_list[sit_thread::sit_gettid()];
    //*(list+promo_list_ptr[sit_thread::sit_gettid()]) = (uint64_t)addr;
    //promo_list_ptr[sit_thread::sit_gettid()]++;
    //assert(promo_list_ptr[sit_thread::sit_gettid()]<PROMO_LIST_SIZE);
    //promo_inc((char*)addr);
    //determine segment offset 
    //(*((char*)sit_segment + (offset>>PROMO_GRANULARITY)))++;
    //sit_segment->add_promoted_read((uint64_t)addr);
    return *addr;
    /*    uint64_t data = 0;
    TMpromotedread((uint64_t)addr);
    bool res = TMaddrset((uint64_t)addr, (uint64_t)&data, 0);
    if(!res) { 
      tx->tmabort(tx);
    }
    return (void*)data;*/
  }

  void* __attribute__ ((noinline))
  SITE_VM::read_ro(STM_READ_SIG(tx,addr,))
  {
    //promo_inc((char*)addr);
    //    cout << "read ro " << sit_thread::sit_gettid() << endl;
    return *addr;
    /*
    uint64_t data;
    uint64_t codeline = 0;
    void *array[4];
    if(DEBUG_BACKTRACE){
      print_bt((uint64_t)addr);
      backtrace(array, 4);
      if(BENCH)
	codeline = (uint64_t) array[2];
      else
	codeline = (uint64_t) array[3];	
      if(codeline == 0){
	print_bt((uint64_t)addr);
	assert(0);
      }
    }
    bool res = TMaddrset((uint64_t)addr, (uint64_t)&data, codeline);
    if(!res) {
      //debug stuff
      if(MVCC){
	print_bt((uint64_t)addr);
	assert(0);
      }
      tx->allocator.onTxAbort();
      tx->tmabort(tx);
    }
    return (void*)data;*/
  }

  /**
   *  SITE_VM read (writing transaction)
   */
  void* //__attribute__ ((noinline))
  SITE_VM::read_rw_promo(STM_READ_SIG(tx,addr,mask))
  {
    //    std::cout << "promo rw : " << addr << std::endl;
    /*uint64_t * list = promo_list[sit_thread::sit_gettid()];
    *(list+promo_list_ptr[sit_thread::sit_gettid()]) = (uint64_t)addr;
    promo_list_ptr[sit_thread::sit_gettid()]++;
    //std::cout << promo_list_ptr[sit_thread::sit_gettid()] << " " << PROMO_LIST_SIZE << " as " << (promo_list_ptr[sit_thread::sit_gettid()]<PROMO_LIST_SIZE)<<std::endl;
    assert(promo_list_ptr[sit_thread::sit_gettid()]<PROMO_LIST_SIZE);*/
    //sit_segment->add_promoted_read((uint64_t)addr);
    return *addr;
    /*
    uint64_t data = 0;
    TMpromotedread((uint64_t)addr);
    bool res = TMaddrset((uint64_t)addr, (uint64_t)&data, 0);
    if(!res) { 
      tx->allocator.onTxAbort(); 
      tx->tmabort(tx);
    }
    return (void*)data;//read_rw(tx,addr);*/
  }
  
  void* __attribute__ ((noinline))
  SITE_VM::read_rw(STM_READ_SIG(tx,addr,mask))
  {

    return *addr;
    /*
    uint64_t data;
    uint64_t codeline = 0;
    void *array[4];
    if(DEBUG_BACKTRACE){
      print_bt((uint64_t)addr);
      backtrace(array, 4);
     if(BENCH)
       codeline = (uint64_t) array[2];
     else
       codeline = (uint64_t) array[3];
     if(codeline == 0){
       print_bt((uint64_t)addr);
       assert(0);
     }
    }
    bool res = TMaddrset((uint64_t)addr, (uint64_t)&data, codeline);
    if(!res) { 
      tx->allocator.onTxAbort(); 
      if(MVCC){
	print_bt((uint64_t)addr);
	assert(0);
      }
      tx->tmabort(tx);
    }
    return (void*)data;*/
  }


  /**
   *  SITE_VM write (read-only context)
   */
  void
  SITE_VM::write_ro(STM_WRITE_SIG(tx,addr,val,mask))
  {
    //cout << "write ro " << endl;
    /*
    uint64_t codeline = 0;
    bool res = TMaddwset((uint64_t)addr, (uint64_t)val, codeline);
    if(!res) { 
      tx->allocator.onTxAbort(); 
      if(MVCC){
	print_bt((uint64_t)addr);
	assert(0);
      }
      tx->tmabort(tx);
      }*/
    //promo_inc((char*)addr);
    OnFirstWrite(tx, read_rw, read_rw_promo, write_rw, commit_rw);
    *addr = val;
  }
    
  /**
   *  SITE_VM write (writing context)
   */
  void
  SITE_VM::write_rw(STM_WRITE_SIG(tx,addr,val,mask))
  {   
    //    cout << "write rw " << sit_thread::sit_gettid() << " address " << addr << " data " << val << endl;
    //cout << "write rw " << endl;

    /*    uint64_t codeline = 0;
    bool res = TMaddwset((uint64_t)addr, (uint64_t)val, codeline);
    if(!res) { 
      tx->allocator.onTxAbort(); 
      if(MVCC){
	print_bt((uint64_t)addr);
	assert(0);
      }
     tx->tmabort(tx);
     }*/
    //promo_inc((char*)addr);
    *addr = val;
  }

  /**
   *  SITE_VM unwinder:
   */
  stm::scope_t*
  SITE_VM::rollback(STM_ROLLBACK_SIG(tx, except, len))
  {
      PreRollback(tx); 
      return PostRollback(tx, read_ro, write_ro, commit_ro);
  }

  /**
   *  SITE_VM in-flight irrevocability:
   */
  bool
  SITE_VM::irrevoc(TxThread*)
  {
      return false;
  }

  /**
   *  SITE_VM validation
   */
  void
  SITE_VM::validate(TxThread* tx)
  {
    std::cout << "validate!"<< std::endl;
  }

  /**
   *  Switch to SITE_VM:
   *
   */
  void
  SITE_VM::onSwitchTo()
  {
    std::cout << "Switch to SITE_VM"  << std::endl;
    //sit_segment = new sit_seg(SIT_SEG_SIZE, "segment 1");
    sit_segment = sitevm_seg_create(NULL, SITEVM_SEG_SIZE);
    sitemallocinit(sit_segment);
    
    /*for(int i =0; i< MAX_SITE_THREADS; i++){
      promo_list[i] = (uint64_t*)malloc(sizeof(uint64_t*)*PROMO_LIST_SIZE);
      }*/

    /*for(uint32_t i =0; i<(1024*1024*16) ; i++){
      //cout << "i " << i << " addr : " <<  (uint64_t*)&(((char*)sit_segment->segment->segment)[i]) << std::endl; 
      ((uint64_t*)sit_segment->segment->segment)[i] = 0;
      }
    sit_segment->commit();
    sit_segment->update();*/
    //std::cout << "seg created" << std::endl;
    
    //sit_segment_promo = new sit_seg(SIT_SEG_SIZE<<PROMO_GRANULARITY, "promo segment 1");
    //std::cout << "promo created" << std::endl;
    //    sitevm_segment_malloc = new sitevm_malloc(sit_segment, "malllocseg");//(sit_malloc**)malloc(sizeof(sit_malloc*)*MAX_SITE_THREADS);
    
/*for(int t=0; t<MAX_SITE_THREADS; t++){
      string name = "malloc-";
      name.append(std::to_string(t));
      sitevm_segment_malloc[t] = new sit_malloc(sit_segment, name.c_str());
      sitevm_segment_malloc[t]->update();
      } */
    //sit_segment->clear_promoted_reads();
    
    std::cout << "Switched to SITE-VM" << std::endl;
    return;

 unsigned long FAULTS = 1000;
    unsigned long avg = 0;
    unsigned long* vec = (uint64_t*)sit_segment->begin;
    unsigned long ubegin, cbegin, uend, cend;
    unsigned long rbegin, rend;
    for(int uu=0; uu<10; uu++){
      avg = 0;
      ubegin = rdtsc();
      //sit_segment->update();//TM_BEGIN(); 

      //sitevm_update(sit_segment);
      uend = rdtsc();
      for(int i=0; i<FAULTS; i++){
	rbegin = rdtsc();
	//(*(uint64_t*)(sit_segment->base()+i*4096))++;
	(*(vec+i*4096))=7;
	rend = rdtsc();
	avg += rend-rbegin;
	//      std::cout << "pagefault time " << end-begin << " writing addr " << (uint64_t)(sit_segment->base()+i*4096) << std::endl;
      }
      cbegin = rdtsc();
      //sit_segment->commit();//TM_END();
      cend = rdtsc();
      std::cout << "pagefault time " << std::dec << avg/FAULTS << "\t" << avg << "\t"  << cend-cbegin << "\t" << uend-ubegin << std::endl;
      
    }
    //    assert(0);
    



    /*
    sit_segment->update();
    for(int i=0; i<1000; i++){
      
      sit_segment->update();
      uint64_t begin = rdtsc();
      (*(uint64_t*)(sit_segment->base()+i*4096))++;
      uint64_t end = rdtsc();
      avg += end-begin;
      //      std::cout << "pagefault time " << end-begin << " writing addr " << (uint64_t)(sit_segment->base()+i*4096) << std::endl;
    }
    uint64_t begin = rdtsc();
    sit_segment->commit();
    uint64_t end = rdtsc();
    
    std::cout << "--pagefault time " << avg/1000 << " commit time " << end-begin << std::endl;
    */
    //sit_segment_promo->update();
    //std::cout << "Calling malloc" << std::endl;
    //int** toRet = (int**)sitevm_segment_malloc->malloc<int**>(sizeof(int*));
//  std::cout << "Calling commit" << std::endl;
    /*  int commit_result = sitevm_segment_malloc->commit();
    if (commit_result){
      std::cout << "ERROR: sequential use of seg_malloc failed to commit" << std::endl;
    }
    std::cout << "Returning " << toRet << " segment starts " <<
      sit_segment->segment->segment << std::endl;

    *toRet = NULL;
    int commit_result_seg = sit_segment->commit();
    //sit_segment_promo->commit();
    if (commit_result_seg){
      std::cout << "ERROR: sequential use of segment failed to commit" << std::endl;
    }

    */
    //sit_segment->reset(new sit_seg((1 << 12) * NUM_OF_PAGES, "segment 1"));
    //sitevm_segment_malloc->reset(new sit_malloc(sit_segment->get(), "malloc 1"));

    //sit_seg sit_segment(1, "segment 1");
    //sit_malloc sitevm_segment_malloc(&sit_segment, "malloc 1");
    //Create a shared counter and initialize it to 0
    //sit_segment->update();
    //sit_segment_promo->update();
    //sitevm_segment_malloc->update();
  }

}

namespace stm {
  /**
   *  SITE_VM initialization
   */
  template<>
  void initTM<SITE_VM>()
  {
      // set the name
      stms[SITE_VM].name      = "SITE_VM";

      // set the pointers
      stms[SITE_VM].site_update = ::SITE_VM::site_update;
      stms[SITE_VM].site_commit = ::SITE_VM::site_commit;
      stms[SITE_VM].begin     = ::SITE_VM::begin;
      stms[SITE_VM].commit    = ::SITE_VM::commit_rw;//o;
      stms[SITE_VM].read      = ::SITE_VM::read_rw;//o;      
      stms[SITE_VM].read_promo= ::SITE_VM::read_rw_promo;//o
      stms[SITE_VM].write     = ::SITE_VM::write_rw;//o;
      stms[SITE_VM].rollback  = ::SITE_VM::rollback;
      stms[SITE_VM].irrevoc   = ::SITE_VM::irrevoc;
      stms[SITE_VM].switcher  = ::SITE_VM::onSwitchTo;
      stms[SITE_VM].privatization_safe = true;
  }
}

