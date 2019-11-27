#include "Decoder.h"
#include <iostream>
#include <chrono>

namespace tof {
namespace data {
namespace raw {

  bool
  Decoder::init()
  {
#ifdef DECODE_VERBOSE
    if (mVerbose) {
      std::cout << "-------- INITIALISE DECODER BUFFER ---------------------------------"
		<< " | " << mSize << " bytes"
		<< std::endl;
    }
#endif
    if (mBuffer) {
      std::cout << "Warning: a buffer was already allocated, cleaning" << std::endl;
      delete [] mBuffer;
    }
    mBuffer = new char[mSize];
    return false;
  }
  
  bool
  Decoder::open(std::string name)
  {
    if (mFile.is_open()) {
      std::cout << "Warning: a file was already open, closing" << std::endl;
      mFile.close();
    }
    mFile.open(name.c_str(), std::fstream::in | std::fstream::binary);
    if (!mFile.is_open()) {
      std::cerr << "Cannot open " << name << std::endl;
      return true;
    }
    return false;
  }

  bool
  Decoder::close()
  {
    if (mFile.is_open()) {
      mFile.close();
      return false;
    }
    return true;
  }
  
  bool
  Decoder::load(std::string name)
  {
    if (open(name)) return true;
    mFile.seekg(0, mFile.end);
    mSize = mFile.tellg();
    mFile.seekg(0);
    mBuffer = new char[mSize];
    mFile.read(mBuffer, mSize);
    close();
    return false;
  }

  bool
  Decoder::read()
  {
    if (!mFile.is_open()) {
      std::cout << "Warning: no file is open" << std::endl;      
      return true;
    }
    mFile.read(mBuffer, mSize);
    mPointer = (uint32_t *)mBuffer;
    if (!mFile) {
      std::cout << "Nothing else to read" << std::endl;
      return true; 
    }
#ifdef DECODE_VERBOSE
    if (mVerbose) {
      std::cout << "-------- READ CRU PAGE ---------------------------------------------"
		<< " | " << mSize << " bytes"
		<< std::endl;
    }
#endif
    mPageCounter++;
    return false;
  }
  
  void
  Decoder::clear()
  {
    mSummary.DRMCommonHeader  = 0x0;
    mSummary.DRMOrbitHeader   = 0x0;
    mSummary.DRMGlobalHeader  = 0x0;
    mSummary.DRMStatusHeader1 = 0x0;
    mSummary.DRMStatusHeader2 = 0x0;
    mSummary.DRMStatusHeader3 = 0x0;
    mSummary.DRMStatusHeader4 = 0x0;
    mSummary.DRMStatusHeader5 = 0x0;
    mSummary.DRMGlobalTrailer = 0x0;
    mSummary.faultFlags = 0x0;
    for (int itrm = 0; itrm < 10; itrm++) {
      mSummary.TRMGlobalHeader[itrm]  = 0x0;
      mSummary.TRMGlobalTrailer[itrm] = 0x0;
      mSummary.TRMempty[itrm] = true;
      for (int ichain = 0; ichain < 2; ichain++) {
	mSummary.TRMChainHeader[itrm][ichain]  = 0x0;
	mSummary.TRMChainTrailer[itrm][ichain] = 0x0;
	mSummary.TDCerror[itrm][ichain] = false;
	for (int itdc = 0; itdc < 15; itdc++) {
	  mSummary.nTDCUnpackedHits[itrm][ichain][itdc] = 0;
	}}}
  }

  inline void
  Decoder::next32()
  {
    mPointer += mSkip;
    mSkip = (mSkip + 2) % 4;
    mByteCounter += 4;
  }

  inline void
  Decoder::next128()
  {
    mPointer += 4;
    mRDH = reinterpret_cast<RDH_t *>(mPointer);
  }
  
  bool
  Decoder::decodeRDH()
  {
    mRDH = reinterpret_cast<RDH_t *>(mPointer);

#ifdef DECODE_VERBOSE
    if (mVerbose) {
      std::cout << "-------- START DECODE RDH ------------------------------------------"
		<< " | " << mPageCounter << " pages"
		<< std::endl;    
    }
#endif

    mSummary.RDHWord0 = mRDH->Word0;
#ifdef DECODE_VERBOSE
    if (mVerbose) {
      uint32_t BlockLength = mRDH->Word0.BlockLength;
      uint32_t PacketCounter = mRDH->Word0.PacketCounter;
      uint32_t HeaderSize = mRDH->Word0.HeaderSize;
      uint32_t MemorySize = mRDH->Word0.MemorySize;
      printf(" %08x%08x%08x%08x RDH Word0 (MemorySize=%d, PacketCounter=%d) \n", mRDH->Data[3], mRDH->Data[2], mRDH->Data[1], mRDH->Data[0],
	     MemorySize, PacketCounter);
    }
#endif
    next128();

    mSummary.RDHWord1 = mRDH->Word1;
#ifdef DECODE_VERBOSE
    if (mVerbose) {
      uint32_t TrgOrbit = mRDH->Word1.TrgOrbit;
      uint32_t HbOrbit = mRDH->Word1.HbOrbit;
      printf(" %08x%08x%08x%08x RDH Word1 (TrgOrbit=%d, HbOrbit=%d) \n", mRDH->Data[3], mRDH->Data[2], mRDH->Data[1], mRDH->Data[0],
	     TrgOrbit, HbOrbit);
    }
#endif
    next128();

    mSummary.RDHWord2 = mRDH->Word2;
#ifdef DECODE_VERBOSE
    if (mVerbose) {
      uint32_t TrgBC = mRDH->Word2.TrgBC;
      uint32_t HbBC = mRDH->Word2.HbBC;
      uint32_t TrgType = mRDH->Word2.TrgType;
      printf(" %08x%08x%08x%08x RDH Word2 (TrgBC=%d, HbBC=%d, TrgType=%d) \n", mRDH->Data[3], mRDH->Data[2], mRDH->Data[1], mRDH->Data[0],
	     TrgBC, HbBC, TrgType);
    }
#endif
    next128();

    mSummary.RDHWord3 = mRDH->Word3;
#ifdef DECODE_VERBOSE
    if (mVerbose) {
      printf(" %08x%08x%08x%08x RDH Word3 \n", mRDH->Data[3], mRDH->Data[2], mRDH->Data[1], mRDH->Data[0]);
    }
#endif
    next128();

    return false;
  }
  
  bool
  Decoder::decode()
  {

    /** check if we have memory to decode **/
    if ((char *)mPointer - mBuffer >= mSummary.RDHWord0.MemorySize) {
#ifdef DECODE_VERBOSE
      if (mVerbose) {
	std::cout << "Warning: decode request exceeds memory size" << std::endl;
      }
#endif
      return true;
    }

#ifdef DECODE_VERBOSE
    if (mVerbose) {
      std::cout << "-------- START DECODE EVENT ----------------------------------------" << std::endl;    
    }
#endif

    /** init decoder **/
    auto start = std::chrono::high_resolution_clock::now();
    mByteCounter = 0;
    mSkip = 1;
    clear();
    
    /** check DRM Common Header **/
    if (!IS_DRM_COMMON_HEADER(*mPointer)) {
#ifdef DECODE_VERBOSE
      printf(" %08x [ERROR] fatal error \n", *mPointer);
#endif
      return true;
    }
    mSummary.DRMCommonHeader = *mPointer;
#ifdef DECODE_VERBOSE
    if (mVerbose) {
      auto DRMCommonHeader = reinterpret_cast<DRMCommonHeader_t *>(mPointer);
      auto Payload = DRMCommonHeader->Payload;
      printf(" %08x DRM Common Header     (Payload=%d) \n", *mPointer, Payload);
    }
#endif
    next32();

    /** DRM Orbit Header **/
    mSummary.DRMOrbitHeader = *mPointer;
#ifdef DECODE_VERBOSE
    if (mVerbose) {
      auto DRMOrbitHeader = reinterpret_cast<DRMOrbitHeader_t *>(mPointer);
      auto Orbit = DRMOrbitHeader->Orbit;
      printf(" %08x DRM Orbit Header      (Orbit=%d) \n", *mPointer, Orbit);
    }
#endif
    next32();    

    /** check DRM Global Header **/
    if (!IS_DRM_GLOBAL_HEADER(*mPointer)) {
#ifdef DECODE_VERBOSE
      printf(" %08x [ERROR] fatal error \n", *mPointer);
#endif
      return true;
    }
    mSummary.DRMGlobalHeader = *mPointer;
#ifdef DECODE_VERBOSE
    if (mVerbose) {
      auto DRMGlobalHeader = reinterpret_cast<DRMGlobalHeader_t *>(mPointer);
      auto DRMID = DRMGlobalHeader->DRMID;
      printf(" %08x DRM Global Header     (DRMID=%d) \n", *mPointer, DRMID);
    }
#endif
#ifdef ALLOW_DRMID
    if (mDRM != -1 && GET_DRM_DRMID(*mPointer) != mDRM)
      return true;
#endif
    next32();

    /** DRM Status Header 1 **/
    mSummary.DRMStatusHeader1 = *mPointer;
#ifdef DECODE_VERBOSE
    if (mVerbose) {
      auto DRMStatusHeader1 = reinterpret_cast<DRMStatusHeader1_t *>(mPointer);
      auto ParticipatingSlotID = DRMStatusHeader1->ParticipatingSlotID;
      auto CBit = DRMStatusHeader1->CBit;
      auto DRMhSize = DRMStatusHeader1->DRMhSize;
      printf(" %08x DRM Status Header 1   (ParticipatingSlotID=0x%03x, CBit=%d, DRMhSize=%d) \n", *mPointer, ParticipatingSlotID, CBit, DRMhSize);
    }
#endif
    next32();

    /** DRM Status Header 2 **/
    mSummary.DRMStatusHeader2 = *mPointer;
#ifdef DECODE_VERBOSE
    if (mVerbose) {
      auto DRMStatusHeader2 = reinterpret_cast<DRMStatusHeader2_t *>(mPointer);
      auto SlotEnableMask = DRMStatusHeader2->SlotEnableMask;
      auto FaultID = DRMStatusHeader2->FaultID;
      auto RTOBit = DRMStatusHeader2->RTOBit;
      printf(" %08x DRM Status Header 2   (SlotEnableMask=0x%03x, FaultID=%d, RTOBit=%d) \n", *mPointer, SlotEnableMask, FaultID, RTOBit);
    }
#endif
    next32();

    /** DRM Status Header 3 **/
    mSummary.DRMStatusHeader3 = *mPointer;
#ifdef DECODE_VERBOSE
    if (mVerbose) {
      auto DRMStatusHeader3 = reinterpret_cast<DRMStatusHeader3_t *>(mPointer);
      auto L0BCID = DRMStatusHeader3->L0BCID;
      auto RunTimeInfo = DRMStatusHeader3->RunTimeInfo;
      printf(" %08x DRM Status Header 3   (L0BCID=%d, RunTimeInfo=0x%03x) \n", *mPointer, L0BCID, RunTimeInfo);
    }
#endif
    next32();

    /** DRM Status Header 4 **/
    mSummary.DRMStatusHeader4 = *mPointer;
#ifdef DECODE_VERBOSE
    if (mVerbose) {
      printf(" %08x DRM Status Header 4 \n", *mPointer);
    }
#endif
    next32();

    /** DRM Status Header 5 **/
    mSummary.DRMStatusHeader5 = *mPointer;
#ifdef DECODE_VERBOSE
    if (mVerbose) {
      printf(" %08x DRM Status Header 5 \n", *mPointer);
    }
#endif
    next32();

    /** loop over DRM payload **/
    while (true) {

      /** LTM global header detected **/
      if (IS_LTM_GLOBAL_HEADER(*mPointer)) {
	
#ifdef DECODE_VERBOSE
	if (mVerbose) {
	  printf(" %08x LTM Global Header \n", *mPointer);
	}
#endif
	next32();

	/** loop over LTM payload **/
	while (true) {

	  /** LTM global trailer detected **/
	  if (IS_LTM_GLOBAL_TRAILER(*mPointer)) {
#ifdef DECODE_VERBOSE
	    if (mVerbose) {
	      printf(" %08x LTM Global Trailer \n", *mPointer);
	    }
#endif
	    next32();
	    break;
	  }

#ifdef DECODE_VERBOSE
	  if (mVerbose) {
	    printf(" %08x LTM data \n", *mPointer);
	  }
#endif
	  next32();
	}
	
      }
      
      /** TRM global header detected **/
      if (IS_TRM_GLOBAL_HEADER(*mPointer) && GET_TRM_SLOTID(*mPointer) > 2) {
	uint32_t SlotID = GET_TRM_SLOTID(*mPointer);
	int itrm = SlotID - 3;
	mSummary.TRMGlobalHeader[itrm] = *mPointer;
#ifdef DECODE_VERBOSE
	if (mVerbose) {
	  auto TRMGlobalHeader = reinterpret_cast<TRMGlobalHeader_t *>(mPointer);
	  auto EventWords = TRMGlobalHeader->EventWords;
	  auto EventNumber = TRMGlobalHeader->EventNumber;
	  auto EBit = TRMGlobalHeader->EBit;
	  printf(" %08x TRM Global Header     (SlotID=%d, EventWords=%d, EventNumber=%d, EBit=%01x) \n", *mPointer, SlotID, EventWords, EventNumber, EBit);
	}
#endif
	next32();
	
	/** loop over TRM payload **/
	while (true) {

	  /** TRM chain-A header detected **/
	  if (IS_TRM_CHAINA_HEADER(*mPointer) && GET_TRM_SLOTID(*mPointer) == SlotID) {
	    int ichain = 0;
	    mSummary.TRMChainHeader[itrm][ichain] = *mPointer;
#ifdef DECODE_VERBOSE
	    if (mVerbose) {
	      auto TRMChainHeader = reinterpret_cast<TRMChainHeader_t *>(mPointer);
	      auto BunchID = TRMChainHeader->BunchID;
	      printf(" %08x TRM Chain-A Header    (SlotID=%d, BunchID=%d) \n", *mPointer, SlotID, BunchID);
	    }
#endif
	    next32();

	    /** loop over TRM chain-A payload **/
	    while (true) {
	      
	      /** TDC hit detected **/
	      if (IS_TDC_HIT(*mPointer)) {
                mSummary.TRMempty[itrm] = false;
                auto itdc = GET_TDCHIT_TDCID(*mPointer);
		auto ihit = mSummary.nTDCUnpackedHits[itrm][ichain][itdc];
		mSummary.TDCUnpackedHit[itrm][ichain][itdc][ihit] = *mPointer;
		mSummary.nTDCUnpackedHits[itrm][ichain][itdc]++;
#ifdef DECODE_VERBOSE
		if (mVerbose) {
		  auto TDCUnpackedHit = reinterpret_cast<TDCUnpackedHit_t *>(mPointer);
		  auto HitTime = TDCUnpackedHit->HitTime;
		  auto Chan = TDCUnpackedHit->Chan;
		  auto TDCID = TDCUnpackedHit->TDCID;
		  auto EBit = TDCUnpackedHit->EBit;
		  auto PSBits = TDCUnpackedHit->PSBits;
		  printf(" %08x TDC Hit               (HitTime=%d, Chan=%d, TDCID=%d, EBit=%d, PSBits=%d \n", *mPointer, HitTime, Chan, TDCID, EBit, PSBits);
		}
#endif
		next32();
		continue;
	      }
	      
	      /** TDC error detected **/
	      if (IS_TDC_ERROR(*mPointer)) {
		mSummary.TDCerror[itrm][ichain] = true;
#ifdef DECODE_VERBOSE
		if (mVerbose) {
		  printf(" %08x TDC error \n", *mPointer);
		}
#endif
		next32();
		continue;
	      }
	      
	      /** TRM chain-A trailer detected **/
	      if (IS_TRM_CHAINA_TRAILER(*mPointer)) {
		mSummary.TRMChainTrailer[itrm][ichain] = *mPointer;
#ifdef DECODE_VERBOSE
		if (mVerbose) {
		  auto TRMChainTrailer = reinterpret_cast<TRMChainTrailer_t *>(mPointer);
		  auto EventCounter = TRMChainTrailer->EventCounter;
		  printf(" %08x TRM Chain-A Trailer   (SlotID=%d, EventCounter=%d) \n", *mPointer, SlotID, EventCounter);
		}
#endif
		next32();
		break;
	      }
	      
#ifdef DECODE_VERBOSE
	      if (mVerbose) {
		printf(" %08x [ERROR] breaking TRM Chain-A decode stream \n", *mPointer);
	      }
#endif
	      next32();
	      break;
	      
	    }} /** end of loop over TRM chain-A payload **/	    
	  
	  /** TRM chain-B header detected **/
	  if (IS_TRM_CHAINB_HEADER(*mPointer) && GET_TRM_SLOTID(*mPointer) == SlotID) {
	    int ichain = 1;
	    mSummary.TRMChainHeader[itrm][ichain] = *mPointer;
#ifdef DECODE_VERBOSE
	    if (mVerbose) {
	      auto TRMChainHeader = reinterpret_cast<TRMChainHeader_t *>(mPointer);
	      auto BunchID = TRMChainHeader->BunchID;
	      printf(" %08x TRM Chain-B Header    (SlotID=%d, BunchID=%d) \n", *mPointer, SlotID, BunchID);
	    }
#endif
	    next32();
	    
	    /** loop over TRM chain-B payload **/
	    while (true) {
	      
	      /** TDC hit detected **/
	      if (IS_TDC_HIT(*mPointer)) {
                mSummary.TRMempty[itrm] = false;
                auto itdc = GET_TDCHIT_TDCID(*mPointer);
		auto ihit = mSummary.nTDCUnpackedHits[itrm][ichain][itdc];
		mSummary.TDCUnpackedHit[itrm][ichain][itdc][ihit] = *mPointer;
		mSummary.nTDCUnpackedHits[itrm][ichain][itdc]++;
#ifdef DECODE_VERBOSE
		if (mVerbose) {
		  auto TDCUnpackedHit = reinterpret_cast<TDCUnpackedHit_t *>(mPointer);
		  auto HitTime = TDCUnpackedHit->HitTime;
		  auto Chan = TDCUnpackedHit->Chan;
		  auto TDCID = TDCUnpackedHit->TDCID;
		  auto EBit = TDCUnpackedHit->EBit;
		  auto PSBits = TDCUnpackedHit->PSBits;
		  printf(" %08x TDC Hit               (HitTime=%d, Chan=%d, TDCID=%d, EBit=%d, PSBits=%d \n", *mPointer, HitTime, Chan, TDCID, EBit, PSBits);
		}
#endif
		next32();
		continue;
	      }
	      
	      /** TDC error detected **/
	      if (IS_TDC_ERROR(*mPointer)) {
		mSummary.TDCerror[itrm][ichain] = true;
#ifdef DECODE_VERBOSE
		if (mVerbose) {
		  printf(" %08x TDC error \n", *mPointer);
		}
#endif
		next32();
		continue;
	      }
	      
	      /** TRM chain-B trailer detected **/
	      if (IS_TRM_CHAINB_TRAILER(*mPointer)) {
		mSummary.TRMChainTrailer[itrm][ichain] = *mPointer;
#ifdef DECODE_VERBOSE
		if (mVerbose) {
		  auto TRMChainTrailer = reinterpret_cast<TRMChainTrailer_t *>(mPointer);
		  auto EventCounter = TRMChainTrailer->EventCounter;
		  printf(" %08x TRM Chain-B Trailer   (SlotID=%d, EventCounter=%d) \n", *mPointer, SlotID, EventCounter);
		}
#endif
		next32();
		break;
	      }
	      
#ifdef DECODE_VERBOSE
	      if (mVerbose) {
		printf(" %08x [ERROR] breaking TRM Chain-B decode stream \n", *mPointer);
	      }
#endif
	      next32();
	      break;
	      
	    }} /** end of loop over TRM chain-A payload **/	    
	  
	  /** TRM global trailer detected **/
	  if (IS_TRM_GLOBAL_TRAILER(*mPointer)) {
	    mSummary.TRMGlobalTrailer[itrm] = *mPointer;
#ifdef DECODE_VERBOSE
	    if (mVerbose) {
	      auto TRMGlobalTrailer = reinterpret_cast<TRMGlobalTrailer_t *>(mPointer);
	      auto EventCRC = TRMGlobalTrailer->EventCRC;
	      auto LBit = TRMGlobalTrailer->LBit;
	      printf(" %08x TRM Global Trailer    (SlotID=%d, EventCRC=%d, LBit=%d) \n", *mPointer, SlotID, EventCRC, LBit);
	    }
#endif
	    next32();
	    
 	    /** filler detected **/
	    if (IS_FILLER(*mPointer)) {
#ifdef DECODE_VERBOSE
	      if (mVerbose) {
		printf(" %08x Filler \n", *mPointer);
	      }
#endif
	      next32();
	    }
	    
	    break;
	  }
	  
#ifdef DECODE_VERBOSE
	  if (mVerbose) {
	    printf(" %08x [ERROR] breaking TRM decode stream \n", *mPointer);
	  }
#endif
	  next32();
	  break;
	  
	} /** end of loop over TRM payload **/
	
	continue;
      }
      
      /** DRM global trailer detected **/
      if (IS_DRM_GLOBAL_TRAILER(*mPointer)) {
	mSummary.DRMGlobalTrailer = *mPointer;
#ifdef DECODE_VERBOSE
	if (mVerbose) {
	  auto DRMGlobalTrailer = reinterpret_cast<DRMGlobalTrailer_t *>(mPointer);
	  auto LocalEventCounter = DRMGlobalTrailer->LocalEventCounter;
	  printf(" %08x DRM Global Trailer    (LocalEventCounter=%d) \n", *mPointer, LocalEventCounter);
	}
#endif
	next32();
	
	/** filler detected **/
	if (IS_FILLER(*mPointer)) {
#ifdef DECODE_VERBOSE
	  if (mVerbose) {
	    printf(" %08x Filler \n", *mPointer);
	  }
#endif
	  next32();
	}
	
	break;
      }
      
#ifdef DECODE_VERBOSE
      if (mVerbose) {
	printf(" %08x [ERROR] trying to recover DRM decode stream \n", *mPointer);
      }
#endif
      next32();
      
    } /** end of loop over DRM payload **/
    
    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    
    mIntegratedBytes += mByteCounter;
    mIntegratedTime += elapsed.count();
    
#ifdef DECODE_VERBOSE
    if (mVerbose) {
      std::cout << "-------- END DECODE EVENT ------------------------------------------"
		<< " | " << mByteCounter << " bytes"
		<< " | " << 1.e3  * elapsed.count() << " ms"
		<< " | " << 1.e-6 * mIntegratedBytes / mIntegratedTime << " MB/s (average)"
		<< std::endl;
    }
#endif

    return false;
  }

}}}

