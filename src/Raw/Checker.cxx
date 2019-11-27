#include "Checker.h"
#include <iostream>
#include <chrono>

namespace tof {
namespace data {
namespace raw {

  bool
  Checker::reset()
  {
#ifdef CHECK_VERBOSE
    if (mVerbose) {
      std::cout << "-------- RESET CHECKER COUNTERS ---------------------------------"
		<< std::endl;
    }
#endif
    mCounter = 0;
    mDRMCounters = {0};
    for (int itrm = 0; itrm < 10; ++itrm) {
      mTRMCounters[itrm] = {0};
      for (int ichain = 0; ichain < 2; ++ichain) {
	mTRMChainCounters[itrm][ichain] = {0};
      }
    }
    return false;
  }

  bool
  Checker::print()
  {
    char chname[2] = {'a', 'b'};

    std::cout << "-------- SUMMARY COUNTERS ---------------------------------"
	      << "-----------------------------------------------------------"
	      << std::endl;
    printf("        | %d events \n", mCounter);
    if (mCounter == 0) {
      std::cout << "-----------------------------------------------------------"
		<< "-----------------------------------------------------------"
		<< std::endl;
      return false;
    }
    std::cout << "-----------------------------------------------------------"
	      << "-----------------------------------------------------------"
	      << std::endl;
    printf("    DRM ");
    float drmheaders = 100. * (float)mDRMCounters.Headers / (float)mCounter;
    printf("| \033%sheaders: %5.1f %%\033[0m ", drmheaders < 100. ? "[1;31m" : "[0m", drmheaders);
    if (mDRMCounters.Headers == 0) {
      std::cout << "-----------------------------------------------------------"
		<< "-----------------------------------------------------------"
		<< std::endl;
      return false;
    }
    float cbit   = 100. * (float)mDRMCounters.CBit / float(mDRMCounters.Headers);
    printf("|    \033%sCbit: %5.1f %%\033[0m ", cbit > 0. ? "[1;31m" : "[0m", cbit);
    float fault  = 100. * (float)mDRMCounters.Fault / float(mDRMCounters.Headers);
    printf("|   \033%sfault: %5.1f %%\033[0m ", fault > 0. ? "[1;31m" : "[0m", cbit);
    float rtobit = 100. * (float)mDRMCounters.RTOBit / float(mDRMCounters.Headers);
    printf("|  \033%sRTObit: %5.1f %%\033[0m ", rtobit > 0. ? "[1;31m" : "[0m", cbit);
    printf("\n");
    //      std::cout << "-----------------------------------------------------------" << std::endl;
    //      printf("    LTM | headers: %5.1f %% \n", 0.);
    for (int itrm = 0; itrm < 10; ++itrm) {
      std::cout << "-----------------------------------------------------------"
		<< "-----------------------------------------------------------"
		<< std::endl;
      printf(" %2d TRM ", itrm+3);
      float trmheaders = 100. * (float)mTRMCounters[itrm].Headers / float(mDRMCounters.Headers);
      printf("| \033%sheaders: %5.1f %%\033[0m ", trmheaders < 100. ? "[1;31m" : "[0m", trmheaders);
      if (mTRMCounters[itrm].Headers == 0.) {
	printf("\n");
	continue;
      }
      float empty   = 100. * (float)mTRMCounters[itrm].Empty   / (float)mTRMCounters[itrm].Headers;
      printf("|   \033%sempty: %5.1f %%\033[0m ", empty > 0. ? "[1;31m" : "[0m", empty);
      float evCount = 100. * (float)mTRMCounters[itrm].EventCounterMismatch / (float)mTRMCounters[itrm].Headers;
      printf("| \033%sevCount: %5.1f %%\033[0m ", evCount > 0. ? "[1;31m" : "[0m", evCount);
      float ebit = 100. * (float)mTRMCounters[itrm].EBit / (float)mTRMCounters[itrm].Headers;
      printf("|    \033%sEbit: %5.1f %%\033[0m ", ebit > 0. ? "[1;31m" : "[0m", ebit);
      printf(" \n");
      for (int ichain = 0; ichain < 2; ++ichain) {
	printf("      %c ", chname[ichain]);
	float chainheaders = 100. * (float)mTRMChainCounters[itrm][ichain].Headers / (float)mTRMCounters[itrm].Headers;
	printf("| \033%sheaders: %5.1f %%\033[0m ", chainheaders < 100. ? "[1;31m" : "[0m", chainheaders);
	if (mTRMChainCounters[itrm][ichain].Headers == 0) {
	  printf("\n");
	  continue;
	}
	float status = 100. * mTRMChainCounters[itrm][ichain].BadStatus / (float)mTRMChainCounters[itrm][ichain].Headers;
	printf("|  \033%sstatus: %5.1f %%\033[0m ", status > 0. ? "[1;31m" : "[0m", status);	
	float bcid = 100. * mTRMChainCounters[itrm][ichain].BunchIDMismatch / (float)mTRMChainCounters[itrm][ichain].Headers;
	printf("|    \033%sbcID: %5.1f %%\033[0m ", bcid > 0. ? "[1;31m" : "[0m", bcid);	
	float tdcerr = 100. * mTRMChainCounters[itrm][ichain].TDCerror / (float)mTRMChainCounters[itrm][ichain].Headers;
	printf("|  \033%sTDCerr: %5.1f %%\033[0m ", tdcerr > 0. ? "[1;31m" : "[0m", tdcerr);	
	printf("\n");
      }
    }
      
    std::cout << "-----------------------------------------------------------"
	      << "-----------------------------------------------------------"
	      << std::endl;

    return false;
  }
  
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

    /** increment check counter **/
    mCounter++;
    
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

    /** increment DRM header counter **/
    mDRMCounters.Headers++;
      
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
    
    /** check DRM CBit **/
    if (GET_DRMSTATUSHEADER1_CBIT(summary.DRMStatusHeader1)) {
      status = true;
      summary.faultFlags |= 1;
      mDRMCounters.CBit++;
#ifdef CHECK_VERBOSE
      if (mVerbose) {
	printf(" DRM CBit is on \n");
      }
#endif	
    }
      
    /** check DRM FaultID **/
    if (GET_DRMSTATUSHEADER2_FAULTID(summary.DRMStatusHeader2)) {
      status = true;
      summary.faultFlags |= 1;
      mDRMCounters.Fault++;
#ifdef CHECK_VERBOSE
      if (mVerbose) {
	printf(" DRM FaultID: %x \n", GET_DRMSTATUSHEADER2_FAULTID(summary.DRMStatusHeader2));
      }
#endif	
    }
      
    /** check DRM RTOBit **/
    if (GET_DRMSTATUSHEADER2_RTOBIT(summary.DRMStatusHeader2)) {
      status = true;
      summary.faultFlags |= 1;
      mDRMCounters.RTOBit++;
#ifdef CHECK_VERBOSE
      if (mVerbose) {
	printf(" DRM RTOBit is on \n");
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

      /** increment TRM header counter **/
      mTRMCounters[itrm].Headers++;

      /** check TRM empty flag **/
      if (summary.TRMempty[itrm])
	mTRMCounters[itrm].Empty++;
      
      /** check TRM EventCounter **/
      uint32_t EventCounter = GET_TRM_EVENTNUMBER(summary.TRMGlobalHeader[itrm]);
      if (EventCounter != LocalEventCounter % 1024) {
	status = true;
	summary.faultFlags |= trmFaultBit;
	mTRMCounters[itrm].EventCounterMismatch++;
#ifdef CHECK_VERBOSE
	if (mVerbose) {
	  printf(" TRM EventCounter / DRM LocalEventCounter mismatch: %d / %d (SlotID=%d) \n", EventCounter, LocalEventCounter, SlotID);
	}
#endif
	continue;
      }

      /** check TRM EBit **/
      if (GET_TRMGLOBALHEADER_EBIT(summary.TRMGlobalHeader[itrm])) {
	status = true;
	summary.faultFlags |= trmFaultBit;
	mTRMCounters[itrm].EBit++;
#ifdef CHECK_VERBOSE
	if (mVerbose) {
	  printf(" TRM EBit is on (SlotID=%d) \n", SlotID);
	}
#endif	
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

	/** increment TRM Chain header counter **/
	mTRMChainCounters[itrm][ichain].Headers++;

	/** check TDC errors **/
	if (summary.TDCerror[itrm][ichain]) {
	  status = true;
	  summary.faultFlags |= chainFaultBit;
	  mTRMChainCounters[itrm][ichain].TDCerror++;
#ifdef CHECK_VERBOSE
	  if (mVerbose) {
	    printf(" TDC error detected (SlotID=%d, chain=%d) \n", SlotID, ichain);
	  }
#endif	  
	}
	
	/** check TRM Chain EventCounter **/
	auto EventCounter = GET_TRMCHAIN_EVENTCOUNTER(summary.TRMChainTrailer[itrm][ichain]);
	if (EventCounter != LocalEventCounter) {
	  status = true;
	  summary.faultFlags |= chainFaultBit;
	  mTRMChainCounters[itrm][ichain].EventCounterMismatch++;
#ifdef CHECK_VERBOSE
	  if (mVerbose) {
	    printf(" TRM Chain EventCounter / DRM LocalEventCounter mismatch: %d / %d (SlotID=%d, chain=%d) \n", EventCounter, EventCounter, SlotID, ichain);
	  }
#endif
	}
      
	/** check TRM Chain Status **/
        auto Status = GET_TRMCHAIN_STATUS(summary.TRMChainTrailer[itrm][ichain]);
	if (Status != 0) {
	  status = true;
	  summary.faultFlags |= chainFaultBit;
	  mTRMChainCounters[itrm][ichain].BadStatus++;
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
	  mTRMChainCounters[itrm][ichain].BunchIDMismatch++;
#ifdef CHECK_VERBOSE
	  if (mVerbose) {
	    printf(" TRM Chain BunchID / DRM L0BCID mismatch: %d / %d (SlotID=%d, chain=%d) \n", BunchID, L0BCID, SlotID, ichain);
	  }
#endif	  
	}

	
      } /** end of loop over TRM chains **/
    } /** end of loop over TRMs **/


#ifdef CHECK_VERBOSE
    if (mVerbose) {
      std::cout << "-------- END CHECK EVENT -------------------------------------------" << std::endl;    
    }
#endif


    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;

    mIntegratedTime += elapsed.count();
    
    return status;
  }
  
}}}

