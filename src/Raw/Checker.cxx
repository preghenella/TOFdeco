#include "Checker.h"
#include <iostream>
#include <chrono>

namespace tof {
namespace data {
namespace raw {

  bool
  Checker::check(tof::data::raw::Summary_t &summary)
  {
    bool status = false;

    auto start = std::chrono::high_resolution_clock::now();

    
#ifdef CHECK_VERBOSE
    if (mVerbose) {
      std::cout << "-------- START CHECK EVENT -----------------------------------------" << std::endl;    
    }
#endif

    /** check DRM Global Header **/
    if (summary.DRMGlobalHeader == 0x0) {
      status = true;
      summary.faultFlags |= 1;
#ifdef CHECK_VERBOSE
      if (mVerbose) {
	printf(" Missing DRM Global Header \n");
      }
#endif
      return status;
    }
    
    /** check DRM Global Trailer **/
    if (summary.DRMGlobalTrailer == 0x0) {
      status = true;
      summary.faultFlags |= 1;
#ifdef CHECK_VERBOSE
      if (mVerbose) {
	printf(" Missing DRM Global Trailer \n");
      }
#endif
      return status;
    }

    /** get DRM relevant data **/
    uint32_t ParticipatingSlotID = GET_DRM_PARTICIPATINGSLOTID(summary.DRMStatusHeader1);
    uint32_t SlotEnableMask      = GET_DRM_SLOTENABLEMASK(summary.DRMStatusHeader2);
    uint32_t L0BCID              = GET_DRM_L0BCID(summary.DRMStatusHeader3);
    uint32_t LocalEventCounter   = GET_DRM_LOCALEVENTCOUNTER(summary.DRMGlobalTrailer);
    
    if (ParticipatingSlotID != SlotEnableMask) {
#ifdef CHECK_VERBOSE
      if (mVerbose) {
	printf(" Warning: enable/participating mask differ: %03x/%03x \n", SlotEnableMask, ParticipatingSlotID);
      }
#endif
    }
    
    /** loop over TRMs **/
    for (int itrm = 0; itrm < 10; ++itrm) {
      uint32_t SlotID = itrm + 3;
      uint32_t trmFaultBit = 1 << (1 + itrm * 3);
      
      /** check participating TRM **/
      if (!(ParticipatingSlotID & 1 << (itrm + 1))) {
	if (summary.TRMGlobalHeader[itrm] != 0x0) {
	  status = true;
#ifdef CHECK_VERBOSE
	if (mVerbose) {
	  printf(" Non-participating header found (SlotID=%d) \n", SlotID);	
	}
#endif
	}
	summary.faultFlags |= trmFaultBit;
	continue;
      }
      
      /** check TRM Global Header **/
      if (summary.TRMGlobalHeader[itrm] == 0x0) {
	status = true;
	summary.faultFlags |= trmFaultBit;
#ifdef CHECK_VERBOSE
	if (mVerbose) {
	  printf(" Missing TRM Header (SlotID=%d) \n", SlotID);
	}
#endif
	continue;
      }

      /** check TRM Global Trailer **/
      if (summary.TRMGlobalTrailer[itrm] == 0x0) {
	status = true;
	summary.faultFlags |= trmFaultBit;
#ifdef CHECK_VERBOSE
	if (mVerbose) {
	  printf(" Missing TRM Trailer (SlotID=%d) \n", SlotID);
	}
#endif
	continue;
      }

      /** check TRM EventCounter **/
      uint32_t EventCounter = GET_TRM_EVENTNUMBER(summary.TRMGlobalHeader[itrm]);
      if (EventCounter != LocalEventCounter % 1024) {
	status = true;
	summary.faultFlags |= trmFaultBit;
#ifdef CHECK_VERBOSE
	if (mVerbose) {
	  printf(" TRM EventCounter / DRM LocalEventCounter mismatch: %d / %d (SlotID=%d) \n", EventCounter, LocalEventCounter, SlotID);
	}
#endif
	continue;
      }
      
      /** loop over TRM chains **/
      for (int ichain = 0; ichain < 2; ichain++) {
	uint32_t chainFaultBit = trmFaultBit << (ichain + 1);
	
 	/** check TRM Chain Header **/
	if (summary.TRMChainHeader[itrm][ichain] == 0x0) {
	  status = true;
	  summary.faultFlags |= chainFaultBit;
#ifdef CHECK_VERBOSE
	  if (mVerbose) {
	    printf(" Missing TRM Chain Header (SlotID=%d, chain=%d) \n", SlotID, ichain);
	  }
#endif
	  continue;
	}

 	/** check TRM Chain Trailer **/
	if (summary.TRMChainTrailer[itrm][ichain] == 0x0) {
	  status = true;
	  summary.faultFlags |= chainFaultBit;
#ifdef CHECK_VERBOSE
	  if (mVerbose) {
	    printf(" Missing TRM Chain Trailer (SlotID=%d, chain=%d) \n", SlotID, ichain);
	  }
#endif
	  continue;
	}

	/** check TRM Chain EventCounter **/
	auto EventCounter = GET_TRMCHAIN_EVENTCOUNTER(summary.TRMChainTrailer[itrm][ichain]);
	if (EventCounter != LocalEventCounter) {
	  status = true;
	  summary.faultFlags |= chainFaultBit;
#ifdef CHECK_VERBOSE
	  if (mVerbose) {
	    printf(" TRM Chain EventCounter / DRM LocalEventCounter mismatch: %d / %d (SlotID=%d, chain=%d) \n", EventCounter, EventCounter, SlotID, ichain);
	  }
#endif
	  continue;
	}
      
	/** check TRM Chain Status **/
        auto Status = GET_TRMCHAIN_STATUS(summary.TRMChainTrailer[itrm][ichain]);
	if (Status != 0) {
	  status = true;
	  summary.faultFlags |= chainFaultBit;
#ifdef CHECK_VERBOSE
	  if (mVerbose) {
	    printf(" TRM Chain bad Status: %d (SlotID=%d, chain=%d) \n", Status, SlotID, ichain);
	  }
#endif	  
	}

	/** check TRM Chain BunchID **/
	uint32_t BunchID = GET_TRMCHAIN_BUNCHID(summary.TRMChainHeader[itrm][ichain]);
	if (BunchID != L0BCID) {
	  status = true;
	  summary.faultFlags |= chainFaultBit;
#ifdef CHECK_VERBOSE
	  if (mVerbose) {
	    printf(" TRM Chain BunchID / DRM L0BCID mismatch: %d / %d (SlotID=%d, chain=%d) \n", BunchID, L0BCID, SlotID, ichain);
	  }
#endif	  
	}

	
      } /** end of loop over TRM chains **/
    } /** end of loop over TRMs **/

    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;

    mIntegratedTime += elapsed.count();
    
    return status;
  }
  
}}}

