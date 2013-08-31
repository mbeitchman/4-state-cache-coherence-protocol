#include "MESI_SMPCache.h"

////////////////////////////////////////////////////////////////////////////
// This file as it is given to you *DOES NOT* implement the 4-state MESI 
// protocol yet. It has been directly copied from the provided MSI_SMPCache
// implementation (with things renamed to refer to the MESI_SMPCache class.
//
// It is up to you to modify this and MESI_SMPCache.h so they implement the
// 4th 'exclusive' state correctly.
//
// This file is created for you as a convenience and to help us all have 
// the same filenames.
/////////////////////////////////////////////////////////////////////////////


MESI_SMPCache::MESI_SMPCache(int cpuid, 
                           std::vector<SMPCache * > * cacheVector,
                           int csize, 
                           int cassoc, 
                           int cbsize, 
                           int caddressable, 
                           const char * repPol, 
                           bool cskew) : 
                             SMPCache(cpuid,cacheVector){
  
  CacheGeneric<MESI_SMPCacheState> *c = 
    CacheGeneric<MESI_SMPCacheState>::create(csize, 
                                            cassoc, 
                                            cbsize, 
                                            caddressable, 
                                            repPol, 
                                            cskew);
  cache = (CacheGeneric<StateGeneric<> >*)c; 

}

void MESI_SMPCache::fillLine(uint32_t addr, uint32_t msi_state){

  //this gets the state of whatever line this address maps to 
  MESI_SMPCacheState *st = (MESI_SMPCacheState *)cache->findLine2Replace(addr); 

  if(st==0){
    /*No state*/
    return;
  }

  /*Set the tags to the tags for the newly cached block*/
  st->setTag(cache->calcTag(addr));

  /*Set the state of the block to the msi_state passed in*/
  st->changeStateTo((MESIState_t)msi_state);
  return;
    
}
  

MESI_SMPCache::RemoteReadService MESI_SMPCache::readRemoteAction(uint32_t addr){

  /*This method implements snoop behavior on all the other 
   *caches that this cache might be interacting with*/

  /*Loop over the other caches in the simulation*/
  std::vector<SMPCache * >::iterator cacheIter;
  std::vector<SMPCache * >::iterator lastCacheIter;
  for(cacheIter = this->getCacheVector()->begin(), 
      lastCacheIter = this->getCacheVector()->end(); 
      cacheIter != lastCacheIter; 
      cacheIter++){

    /*Get a pointer to the other cache*/
    MESI_SMPCache *otherCache = (MESI_SMPCache*)*cacheIter; 
    if(otherCache->getCPUId() == this->getCPUId()){

      /*We don't want to snoop our own access*/
      continue;

    }

    /*Get the state of the block this addr maps to in the other cache*/      
    MESI_SMPCacheState* otherState = 
      (MESI_SMPCacheState *)otherCache->cache->findLine(addr);

    /*If otherState == NULL here, the tags didn't match, so the
     *other cache didn't have this line cached*/
    if(otherState){
      /*The tags matched -- need to do snoop actions*/

      /*Other cache has recently written the line*/
      if(otherState->getState() == MESI_MODIFIED || otherState->getState() == MESI_EXCLUSIVE){
    
        /*Modified transitions to Shared on a remote Read*/ 
        otherState->changeStateTo(MESI_SHARED);

        /*Return a Remote Read Service indicating that 
         *1)The line was not shared (the false param)
         *2)The line was provided by otherCache, as only it had it cached
        */
        return MESI_SMPCache::RemoteReadService(false,true);

      /*Other cache has recently read the line*/
      }else if(otherState->getState() == MESI_SHARED){  
        
        /*Return a Remote Read Service indicating that 
         *1)The line was shared (the true param)
         *2)The line was provided by otherCache 
        */
        return MESI_SMPCache::RemoteReadService(true,true);

      /*Line was cached, but invalid*/
      }else if(otherState->getState() == MESI_INVALID){ 

        /*Do Nothing*/
        /* marc: do i need to count num invalid and only load exclusive if numinvalid == 0 ???? */

      }

    }/*Else: Tag didn't match. Nothing to do for this cache*/

  }/*Done with other caches*/

  /*If all other caches were MESI_INVALID*/
  return MESI_SMPCache::RemoteReadService(false,false);
}


void MESI_SMPCache::readLine(uint32_t rdPC, uint32_t addr){
  /*
   *This method implements actions taken on a read access to address addr
   *at instruction rdPC
  */

  /*Get the state of the line to which this address maps*/
  MESI_SMPCacheState *st = 
    (MESI_SMPCacheState *)cache->findLine(addr);    
  
  /*Read Miss - tags didn't match, or line is invalid*/
  if(!st || (st && !(st->isValid())) ){

    /*Update event counter for read misses*/
    numReadMisses++;

    if(st){

      /*Tag matched, but state was invalid*/
      numReadOnInvalidMisses++;

    }

    /*Make the other caches snoop this access 
     *and get a remote read service object describing what happened.
     *This is effectively putting the access on the bus.
    */
    MESI_SMPCache::RemoteReadService rrs = readRemoteAction(addr);
    numReadRequestsSent++;
    
    if(rrs.providedData){

      /*If it was shared or modified elsewhere,
       *the line was provided by another cache.
       *Update these counters to reflect that
      */
      numReadMissesServicedByOthers++;

      if(rrs.isShared){
        numReadMissesServicedByShared++;
      }else{
        numReadMissesServicedByModified++;
      }

      /*Fill the line as shared*/
      fillLine(addr,MESI_SHARED);

    }
    else
    {
      /* the line does not exist in another cache so fill it from memory as exclusive */
      fillLine(addr,MESI_EXCLUSIVE);
    } 

  }else{

    /*Read Hit - any state but Invalid*/
    numReadHits++; 
    return; 

  }

}


MESI_SMPCache::InvalidateReply MESI_SMPCache::writeRemoteAction(uint32_t addr){
    
    /*This method implements snoop behavior on all the other 
     *caches that this cache might be interacting with*/
    
    bool empty = true;

    /*Loop over all other caches*/
    std::vector<SMPCache * >::iterator cacheIter;
    std::vector<SMPCache * >::iterator lastCacheIter;
    for(cacheIter = this->getCacheVector()->begin(), 
        lastCacheIter = this->getCacheVector()->end(); 
        cacheIter != lastCacheIter; 
        cacheIter++){


      
      MESI_SMPCache *otherCache = (MESI_SMPCache*)*cacheIter; 
      if(otherCache->getCPUId() == this->getCPUId()){
        /*We don't snoop ourselves*/
        continue;
      }

      /*Get the line from the current other cache*/
      MESI_SMPCacheState* otherState = 
        (MESI_SMPCacheState *)otherCache->cache->findLine(addr);

      /*if it is cached by otherCache*/
      if(otherState && otherState->isValid()){

          /*Invalidate the line, because we're writing*/
          otherState->invalidate();
 
          /*The reply contains data, so "empty" is false*/
          empty = false;

      }

    }/*done with other caches*/

    /*Empty=true indicates that no other cache 
    *had the line or there were no other caches
    * 
    *This data in this object is not used as is, 
    *but it might be useful if you plan to extend 
    *this simulator, so i left it in.
    */
    return MESI_SMPCache::InvalidateReply(empty);
}


void MESI_SMPCache::writeLine(uint32_t wrPC, uint32_t addr){
  /*This method implements actions taken when instruction wrPC
   *writes to memory location addr*/

  /*Find the line to which this address maps*/ 
  MESI_SMPCacheState * st = (MESI_SMPCacheState *)cache->findLine(addr);    
   
  /*
   *If the tags didn't match, or the line was invalid, it is a 
   *write miss
   */ 
  if(!st || (st && !(st->isValid())) ){


    numWriteMisses++;
    
    if(st){

      /*We're writing to an invalid line*/
      numWriteOnInvalidMisses++;

    }
 
    /*
     * Let the other caches snoop this write access and update their
     * state accordingly.  This action is effectively putting the write
     * on the bus.
     */ 
    MESI_SMPCache::InvalidateReply inv_ack = writeRemoteAction(addr);
    numInvalidatesSent++;

    /*Fill the line with the new written block*/
    fillLine(addr,MESI_MODIFIED);

    return;

  }
  else if(st->getState() == MESI_EXCLUSIVE)
  {
      numWriteHits++;
      // change state to modified
      // no other cache has a copy so we don't need to update state of other 
      // caches
      st->changeStateTo(MESI_MODIFIED);
      return;
  }
  else if(st->getState() == MESI_SHARED)
  {
    /* If the block is shared and we're writing, still count it as a cache hit,
     * but we need to invalidate all of the sharers' copies and upgrade to Modified 
     * in order to write.
     */
    numWriteHits++;

    /*Write-on-shared Coherence Misses*/
    numWriteOnSharedMisses++;

    /*Let the other sharers snoop this write, and invalidate themselves*/
    MESI_SMPCache::InvalidateReply inv_ack = writeRemoteAction(addr);
    numInvalidatesSent++;

    /*Change the state of the line to Modified to reflect the write*/
    st->changeStateTo(MESI_MODIFIED);
    return;

  }
  else
  { //Write Hit

    /*Already have it writable: No coherence action required!*/
    numWriteHits++;

    return;

  }

}

char *MESI_SMPCache::Identify(){
  return (char *)"MESI Cache Coherence";
}

MESI_SMPCache::~MESI_SMPCache(){

}

extern "C" SMPCache *Create(int num, std::vector<SMPCache*> *cvec, int csize, int casso, int bs, int addrble, const char *repl, bool skw){

  return new MESI_SMPCache(num,cvec,csize,casso,bs,addrble,repl,skw);

}
