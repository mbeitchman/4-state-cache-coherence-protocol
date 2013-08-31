#include "CacheCore.h"
#include "SMPCache.h"
#include "MESI_SMPCacheState.h"
#include <vector>

////////////////////////////////////////////////////////////////////////////
// This file as it is given to you *DOES NOT* implement the 4-state MESI 
// protocol yet. It has been directly copied from the provided MSI_SMPCache
// implementation (with things renamed to refer to the MESI_SMPCache class.
//
// It is up to you to modify this and MESI_SMPCache.cpp so they implement
// the 4th 'exclusive' state correctly.
//
// This file is created for you as a convenience and to help us all have 
// the same filenames.
/////////////////////////////////////////////////////////////////////////////


class MESI_SMPCache : public SMPCache{

public:

  /*First we define a couple of helper classes that 
   * carry data between methods in our main class*/
  class RemoteReadService{
  public:
    bool isShared;
    bool providedData;
  
    RemoteReadService(bool shrd, bool prov){
      isShared = shrd;
      providedData = prov;
    }
  
  };
  
  class InvalidateReply{
  /*This class isn't used, but i left it here so you
   * could add to it if you need to communicate
   * between caches in Invalidate Replies*/
  public:
    uint32_t empty;
  
    InvalidateReply(bool EMPTY){
      empty = EMPTY;
    }
  
  };

  //FIELDS
  //This cache's ID
  int CPUId;


  //METHODS
  //Constructor
  MESI_SMPCache(int cpuid, 
               std::vector<SMPCache * > * cacheVector,
               int csize,
               int cassoc,
               int cbsize,
               int caddressable,
               const char * repPol,
               bool cskew);

  //Readline performs a read, and uses readRemoteAction to 
  //check for data in other caches
  virtual void readLine(uint32_t rdPC, uint32_t addr);//SMPCache Interface Function
  virtual MESI_SMPCache::RemoteReadService readRemoteAction(uint32_t addr);

  //Writeline performs a write, and uses writeRemoteAction
  //to check for data in other caches
  virtual void writeLine(uint32_t wrPC, uint32_t addr);//SMPCache Interface Function
  virtual MESI_SMPCache::InvalidateReply writeRemoteAction(uint32_t addr);
 
  //Fill line touches cache state, bringing addr's block in, and setting its state to mesi_state 
  virtual void fillLine(uint32_t addr, uint32_t mesi_state);//SMPCache Interface Function

  virtual char *Identify();
 
  //Dump the stats for this cache to outFile
  //void dumpStatsToFile(FILE* outFile);

  //Destructor
  ~MESI_SMPCache();
};


