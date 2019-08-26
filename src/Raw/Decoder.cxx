#include "Decoder.h"
#include <iostream>
#include <chrono>
#include <boost/format.hpp>

namespace tof {
namespace data {
namespace raw {

  bool
  Decoder::init()
  {
#ifdef VERBOSE
    if (mVerbose)
      std::cout << "-------- INITIALISE DECODER BUFFER ---------------------------------"
		<< " | " << mSize << " bytes"
		<< std::endl;
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
    mUnion = reinterpret_cast<Union_t *>(mBuffer);
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
#ifdef VERBOSE
    if (mVerbose)
      std::cout << "-------- READ CRU PAGE ---------------------------------------------"
		<< " | " << mSize << " bytes"
		<< std::endl;
#endif
    mMemoryCounter = 0;
    mPageCounter++;
    return false;
  }
  
  void
  Decoder::print(std::string what)
  {
    if (mVerbose)
      std::cout << boost::format("%08x") % mUnion->Data << " " << what << std::endl;
  }
  
  void
  Decoder::clear()
  {
    mSummary.DRMCommonHeader  = {0x0};
    mSummary.DRMOrbitHeader   = {0x0};
    mSummary.DRMGlobalHeader  = {0x0};
    mSummary.DRMStatusHeader1 = {0x0};
    mSummary.DRMStatusHeader2 = {0x0};
    mSummary.DRMStatusHeader3 = {0x0};
    mSummary.DRMStatusHeader4 = {0x0};
    mSummary.DRMStatusHeader5 = {0x0};
    mSummary.DRMGlobalTrailer  = {0x0};
    mSummary.DRMFaultFlag = false;
    for (int itrm = 0; itrm < 10; itrm++) {
      mSummary.TRMGlobalHeader[itrm]  = {0x0};
      mSummary.TRMGlobalTrailer[itrm] = {0x0};
      mSummary.TRMempty[itrm] = true;
      mSummary.TRMFaultFlag[itrm] = false;
      mSummary.nTRMSpiderHits[itrm] = 0;
      for (int ichain = 0; ichain < 2; ichain++) {
	mSummary.TRMChainHeader[itrm][ichain]  = {0x0};
	mSummary.TRMChainTrailer[itrm][ichain] = {0x0};
	mSummary.TRMChainFaultFlag[itrm][ichain] = false;
	for (int itdc = 0; itdc < 15; itdc++) {
	  mSummary.nTDCUnpackedHits[itrm][ichain][itdc] = 0;
	}}}
  }

  inline void
  Decoder::next32()
  {
    mPointer += mSkip;
    mMemoryCounter += mSkip * 4;
    mByteCounter += 4;
    mSkip = (mSkip + 2) % 4;
    mUnion = reinterpret_cast<Union_t *>(mPointer);
  }

  inline void
  Decoder::next128()
  {
    mPointer += 4;
    mMemoryCounter += 16;
    mRDH = reinterpret_cast<RDH_t *>(mPointer);
  }
  
  void
  Decoder::printRDH(std::string what)
  {
    if (mVerbose) {
      for (int i = 3; i >= 0; --i)
	std::cout << boost::format("%08x") % mRDH->Data[i];
      std::cout << " " << what << std::endl;
    }
  }
  
  bool
  Decoder::decodeRDH()
  {
    mRDH = reinterpret_cast<RDH_t *>(mPointer);

#ifdef VERBOSE
    boost::format fmt;
    if (mVerbose)
      std::cout << "-------- START DECODE RDH ------------------------------------------"
		<< " | " << mPageCounter << " pages"
		<< std::endl;    
#endif

    mSummary.RDHWord0 = mRDH->Word0;
#ifdef VERBOSE
    uint32_t BlockLength = mRDH->Word0.BlockLength;
    uint32_t PacketCounter = mRDH->Word0.PacketCounter;
    uint32_t HeaderSize = mRDH->Word0.HeaderSize;
    uint32_t MemorySize = mRDH->Word0.MemorySize;
    fmt = boost::format("RDH Word0 (MemorySize=%d, PacketCounter=%d)") % MemorySize % PacketCounter;
    printRDH(fmt.str());
#endif
    next128();

    mSummary.RDHWord1 = mRDH->Word1;
#ifdef VERBOSE
    uint32_t TrgOrbit = mRDH->Word1.TrgOrbit;
    uint32_t HbOrbit = mRDH->Word1.HbOrbit;
    fmt = boost::format("RDH Word1 (TrgOrbit=%d, HbOrbit=%d)") % TrgOrbit % HbOrbit;
    printRDH(fmt.str());
#endif
    next128();

    mSummary.RDHWord2 = mRDH->Word2;
#ifdef VERBOSE
    uint32_t TrgBC = mRDH->Word2.TrgBC;
    uint32_t HbBC = mRDH->Word2.HbBC;
    uint32_t TrgType = mRDH->Word2.TrgType;
    fmt = boost::format("RDH Word2 (TrgBC=%d, HbBC=%d, TrgType=%d)") % TrgBC % HbBC % TrgType;
    printRDH(fmt.str());
#endif
    next128();

    mSummary.RDHWord3 = mRDH->Word3;
#ifdef VERBOSE
    printRDH("RDH Word3");
#endif
    next128();

    return false;
  }
  
  bool
  Decoder::decode()
  {

#ifdef VERBOSE
    boost::format fmt;
#endif

    /** check if we have memory to decode **/
    if (mMemoryCounter >= mSummary.RDHWord0.MemorySize) {
#ifdef VERBOSE
      if (mVerbose) std::cout << "Warning: decode request exceeds memory size" << std::endl;
#endif
      return true;
    }

#ifdef VERBOSE
    if (mVerbose)
      std::cout << "-------- START DECODE EVENT ----------------------------------------" << std::endl;    
#endif

    auto start = std::chrono::high_resolution_clock::now();
    mUnion = reinterpret_cast<Union_t *>(mPointer);
    mByteCounter = 0;
    mSkip = 1;

    clear();

    
    /** check DRM Common Header **/
    if (!IS_DRM_COMMON_HEADER(*mPointer)) {
#ifdef VERBOSE
      print("[ERROR] fatal error");
#endif
      return true;
    }
      
    mSummary.DRMCommonHeader = mUnion->DRMCommonHeader;
#ifdef VERBOSE
    uint32_t Payload = mUnion->DRMCommonHeader.Payload;
    fmt = boost::format("DRM Common Header     (Payload=%d)") % Payload;
    print(fmt.str());
#endif
    next32();
    mSummary.DRMOrbitHeader = mUnion->DRMOrbitHeader;
#ifdef VERBOSE
    uint32_t Orbit = mUnion->DRMOrbitHeader.Orbit;
    fmt = boost::format("DRM Orbit Header      (Orbit=%d)") % Orbit;
    print(fmt.str());
#endif
    next32();    

    /** check DRM Global Header **/
    if (!IS_DRM_GLOBAL_HEADER(*mPointer)) {
#ifdef VERBOSE
      print("[ERROR] fatal error");
#endif
      return true;
    }

    mSummary.DRMGlobalHeader = mUnion->DRMGlobalHeader;
#ifdef VERBOSE
    uint32_t DRMID = mUnion->DRMGlobalHeader.DRMID;
    fmt = boost::format("DRM Global Header     (DRMID=%d)") % DRMID;
    print(fmt.str());
#endif
    next32();
    mSummary.DRMStatusHeader1 = mUnion->DRMStatusHeader1;
#ifdef VERBOSE
    uint32_t ParticipatingSlotID = mUnion->DRMStatusHeader1.ParticipatingSlotID;
    uint32_t CBit = mUnion->DRMStatusHeader1.CBit;
    uint32_t DRMhSize = mUnion->DRMStatusHeader1.DRMhSize;
    fmt = boost::format("DRM Status Header 1   (ParticipatingSlotID=0x%03x, CBit=%d, DRMhSize=%d)") % ParticipatingSlotID % CBit % DRMhSize;
    print(fmt.str());
#endif
    next32();
    mSummary.DRMStatusHeader2 = mUnion->DRMStatusHeader2;
#ifdef VERBOSE
    uint32_t SlotEnableMask = mUnion->DRMStatusHeader2.SlotEnableMask;
    uint32_t FaultID = mUnion->DRMStatusHeader2.FaultID;
    uint32_t RTOBit = mUnion->DRMStatusHeader2.RTOBit;
    fmt = boost::format("DRM Status Header 2   (SlotEnableMask=0x%03x, FaultID=%d, RTOBit=%d)") % SlotEnableMask % FaultID % RTOBit;
    print(fmt.str());
#endif
    next32();
    mSummary.DRMStatusHeader3 = mUnion->DRMStatusHeader3;
#ifdef VERBOSE
    uint32_t L0BCID = mUnion->DRMStatusHeader3.L0BCID;
    uint32_t RunTimeInfo = mUnion->DRMStatusHeader3.RunTimeInfo;
    fmt = boost::format("DRM Status Header 3   (L0BCID=%d, RunTimeInfo=0x%03x)") % L0BCID % RunTimeInfo;
    print(fmt.str());
#endif
    next32();
    mSummary.DRMStatusHeader4 = mUnion->DRMStatusHeader4;
#ifdef VERBOSE
    print("DRM Status Header 4");
#endif
    next32();
    mSummary.DRMStatusHeader5 = mUnion->DRMStatusHeader5;
#ifdef VERBOSE
    print("DRM Status Header 5");
#endif
    next32();

    /** loop over DRM payload **/
    while (true) {

      /** LTM global header detected **/
      if (IS_LTM_GLOBAL_HEADER(*mPointer)) {
	
#ifdef VERBOSE
	print("LTM Global Header");
#endif
	next32();

	/** loop over LTM payload **/
	while (true) {

	  /** LTM global trailer detected **/
	  if (IS_LTM_GLOBAL_TRAILER(*mPointer)) {
#ifdef VERBOSE
	    print("LTM Global Trailer");
#endif
	    next32();
	    break;
	  }

#ifdef VERBOSE
	  print("LTM data");
#endif
	  next32();
	}
	
      }
      
      /** TRM global header detected **/
      if (IS_TRM_GLOBAL_HEADER(*mPointer) && GET_TRM_SLOTID(*mPointer) > 2) {
	uint32_t SlotID = GET_TRM_SLOTID(*mPointer);
	int itrm = SlotID - 3;
	mSummary.TRMGlobalHeader[itrm] = mUnion->TRMGlobalHeader;
#ifdef VERBOSE
	uint32_t EventWords = mUnion->TRMGlobalHeader.EventWords;
	uint32_t EventNumber = mUnion->TRMGlobalHeader.EventNumber;
	uint32_t EBit = mUnion->TRMGlobalHeader.EBit;
	fmt = boost::format("TRM Global Header     (SlotID=%d, EventWords=%d, EventNumber=%d, EBit=%01x)") % SlotID % EventWords % EventNumber % EBit;
	print(fmt.str());
#endif
	next32();
	
	/** loop over TRM payload **/
	while (true) {

	  /** TRM chain-A header detected **/
	  if (IS_TRM_CHAINA_HEADER(*mPointer) && GET_TRM_SLOTID(*mPointer) == SlotID) {
	    int ichain = 0;
	    mSummary.TRMChainHeader[itrm][ichain] = mUnion->TRMChainHeader;
#ifdef VERBOSE
	    uint32_t BunchID = mUnion->TRMChainHeader.BunchID;
	    fmt = boost::format("TRM Chain-A Header    (SlotID=%d, BunchID=%d)") % SlotID % BunchID;
	    print(fmt.str());
#endif
	    next32();

	    /** loop over TRM chain-A payload **/
	    while (true) {
	      
	      /** TDC hit detected **/
	      if (IS_TDC_HIT(*mPointer)) {
                mSummary.TRMempty[itrm] = false;
                int itdc = mUnion->TDCUnpackedHit.TDCID;
		int ihit = mSummary.nTDCUnpackedHits[itrm][ichain][itdc];
		mSummary.TDCUnpackedHit[itrm][ichain][itdc][ihit] = mUnion->TDCUnpackedHit;
		mSummary.nTDCUnpackedHits[itrm][ichain][itdc]++;
#ifdef VERBOSE
		uint32_t HitTime = mUnion->TDCUnpackedHit.HitTime;
		uint32_t Chan = mUnion->TDCUnpackedHit.Chan;
		uint32_t TDCID = mUnion->TDCUnpackedHit.TDCID;
		uint32_t EBit = mUnion->TDCUnpackedHit.EBit;
		uint32_t PSBits = mUnion->TDCUnpackedHit.PSBits;
		fmt = boost::format("TDC Hit               (HitTime=%d, Chan=%d, TDCID=%d, EBit=%d, PSBits=%d") % HitTime % Chan % TDCID % EBit % PSBits;
#endif
		next32();
		continue;
	      }
	      
	      /** TDC error detected **/
	      if (IS_TDC_ERROR(*mPointer)) {
#ifdef VERBOSE
		print("TDC error");
#endif
		next32();
		continue;
	      }
	      
	      /** TRM chain-A trailer detected **/
	      if (IS_TRM_CHAINA_TRAILER(*mPointer)) {
		mSummary.TRMChainTrailer[itrm][ichain] = mUnion->TRMChainTrailer;
#ifdef VERBOSE
		uint32_t EventCounter = mUnion->TRMChainTrailer.EventCounter;
		fmt = boost::format("TRM Chain-A Trailer   (SlotID=%d, EventCounter=%d)") % SlotID % EventCounter;
		print(fmt.str());
#endif
		next32();
		break;
	      }
	      
#ifdef VERBOSE
	      print("[ERROR] breaking TRM Chain-A decode stream");
#endif
	      next32();
	      break;
	      
	    }} /** end of loop over TRM chain-A payload **/	    
	  
	  /** TRM chain-B header detected **/
	  if (IS_TRM_CHAINB_HEADER(*mPointer) && GET_TRM_SLOTID(*mPointer) == SlotID) {
	    int ichain = 1;
	    mSummary.TRMChainHeader[itrm][ichain] = mUnion->TRMChainHeader;
#ifdef VERBOSE
	    uint32_t BunchID = mUnion->TRMChainHeader.BunchID;
	    fmt = boost::format("TRM Chain-B Header    (SlotID=%d, BunchID=%d)") % SlotID % BunchID;
	    print(fmt.str());
#endif
	    next32();
	    
	    /** loop over TRM chain-B payload **/
	    while (true) {
	      
	      /** TDC hit detected **/
	      if (IS_TDC_HIT(*mPointer)) {
                mSummary.TRMempty[itrm] = false;
                int itdc = mUnion->TDCUnpackedHit.TDCID;
		int ihit = mSummary.nTDCUnpackedHits[itrm][ichain][itdc];
		mSummary.TDCUnpackedHit[itrm][ichain][itdc][ihit] = mUnion->TDCUnpackedHit;
		mSummary.nTDCUnpackedHits[itrm][ichain][itdc]++;
#ifdef VERBOSE
		uint32_t HitTime = mUnion->TDCUnpackedHit.HitTime;
		uint32_t Chan = mUnion->TDCUnpackedHit.Chan;
		uint32_t TDCID = mUnion->TDCUnpackedHit.TDCID;
		uint32_t EBit = mUnion->TDCUnpackedHit.EBit;
		uint32_t PSBits = mUnion->TDCUnpackedHit.PSBits;
		fmt = boost::format("TDC Hit               (HitTime=%d, Chan=%d, TDCID=%d, EBit=%d, PSBits=%d") % HitTime % Chan % TDCID % EBit % PSBits;
		print(fmt.str());
#endif
		next32();
		continue;
	      }
	      
	      /** TDC error detected **/
	      if (IS_TDC_ERROR(*mPointer)) {
#ifdef VERBOSE
		print("TDC error");
#endif
		next32();
		continue;
	      }
	      
	      /** TRM chain-B trailer detected **/
	      if (IS_TRM_CHAINB_TRAILER(*mPointer)) {
		mSummary.TRMChainTrailer[itrm][ichain] = mUnion->TRMChainTrailer;
#ifdef VERBOSE
		uint32_t EventCounter = mUnion->TRMChainTrailer.EventCounter;
		fmt = boost::format("TRM Chain-B Trailer   (SlotID=%d, EventCounter=%d)") % SlotID % EventCounter;
		print(fmt.str());
#endif
		next32();
		break;
	      }
	      
#ifdef VERBOSE
	      print("[ERROR] breaking TRM Chain-B decode stream");
#endif
	      next32();
	      break;
	      
	    }} /** end of loop over TRM chain-A payload **/	    
	  
	  /** TRM global trailer detected **/
	  if (IS_TRM_GLOBAL_TRAILER(*mPointer)) {
	    mSummary.TRMGlobalTrailer[itrm] = mUnion->TRMGlobalTrailer;
#ifdef VERBOSE
	    uint32_t EventCRC = mUnion->TRMGlobalTrailer.EventCRC;
	    uint32_t LBit = mUnion->TRMGlobalTrailer.LBit;
	    fmt = boost::format("TRM Global Trailer    (SlotID=%d, EventCRC=%d, LBit=%d)") % SlotID % EventCRC % LBit;
	    print(fmt.str());
#endif
	    next32();
	    
 	    /** filler detected **/
	    if (IS_FILLER(*mPointer)) {
#ifdef VERBOSE
	      print("Filler");
#endif
	      next32();
	    }
	    
	    break;
	  }
	  
#ifdef VERBOSE
	  print("[ERROR] breaking TRM decode stream");
#endif
	  next32();
	  break;
	  
	} /** end of loop over TRM payload **/
	
	continue;
      }
      
      /** DRM global trailer detected **/
      if (IS_DRM_GLOBAL_TRAILER(*mPointer)) {
	mSummary.DRMGlobalTrailer = mUnion->DRMGlobalTrailer;
#ifdef VERBOSE
	uint32_t LocalEventCounter = mUnion->DRMGlobalTrailer.LocalEventCounter;
	fmt = boost::format("DRM Global Trailer    (LocalEventCounter=%d)") % LocalEventCounter;
	print(fmt.str());
#endif
	next32();
	
	/** filler detected **/
	if (IS_FILLER(*mPointer)) {
#ifdef VERBOSE
	  print("Filler");
#endif
	  next32();
	}
	
	break;
      }
      
#ifdef VERBOSE
      print("[ERROR] trying to recover DRM decode stream");
#endif
      next32();
      
    } /** end of loop over DRM payload **/
    
    /** run SPIDER **/
    //    spider();
    
    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    
    mIntegratedBytes += mByteCounter;
    mIntegratedTime += elapsed.count();
    
#ifdef VERBOSE
    if (mVerbose)
      std::cout << "-------- END DECODE EVENT ------------------------------------------"
		<< " | " << mByteCounter << " bytes"
		<< " | " << 1.e3  * elapsed.count() << " ms"
		<< " | " << 1.e-6 * mIntegratedBytes / mIntegratedTime << " MB/s (average)"
		<< std::endl;
#endif

    return false;
  }

  bool
  Decoder::check()
  {
    bool status = false;

    auto start = std::chrono::high_resolution_clock::now();

    
#ifdef VERBOSE
    boost::format fmt;
#endif

#ifdef VERBOSE
    if (mVerbose)
      std::cout << "-------- START CHECK EVENT -----------------------------------------" << std::endl;    
#endif

    /** check DRM Global Header **/
    if (mSummary.DRMGlobalHeader.raw() == 0) {
      status = true;
      mSummary.DRMFaultFlag = true;
#ifdef VERBOSE
      fmt = boost::format("Missing DRM Global Header");
      if (mVerbose) std::cout << fmt << std::endl;
#endif
      return status;
    }
    
    /** check DRM Global Trailer **/
    if (mSummary.DRMGlobalTrailer.raw() == 0) {
      status = true;
      mSummary.DRMFaultFlag = true;
#ifdef VERBOSE
      fmt = boost::format("Missing DRM Global Trailer");
      if (mVerbose) std::cout << fmt << std::endl;
#endif
      return status;
    }

    /** get DRM relevant data **/
    uint32_t ParticipatingSlotID = mSummary.DRMStatusHeader1.ParticipatingSlotID;
    uint32_t SlotEnableMask      = mSummary.DRMStatusHeader2.SlotEnableMask;
    uint32_t L0BCID              = mSummary.DRMStatusHeader3.L0BCID;
    uint32_t LocalEventCounter   = mSummary.DRMGlobalTrailer.LocalEventCounter;
    
    if (ParticipatingSlotID != SlotEnableMask) {
#ifdef VERBOSE
      fmt = boost::format("Warning: enable/participating mask differ: %03x/%03x") % SlotEnableMask % ParticipatingSlotID;
      if (mVerbose) std::cout << fmt << std::endl;
#endif
    }
    
    /** loop over TRMs **/
    for (int itrm = 0; itrm < 10; ++itrm) {
      uint32_t SlotID = itrm + 3;
	
      /** check participating TRM **/
      if (!(ParticipatingSlotID & 1 << (itrm + 1))) {
	mSummary.TRMFaultFlag[itrm] = true;
	if (mSummary.TRMGlobalHeader[itrm].raw() != 0) {
	  status = true;
#ifdef VERBOSE
	  printf("trm%02d: non-participating header found \n", SlotID);	
#endif
	}
	continue;
      }
      
      /** check TRM Global Header **/
      if (mSummary.TRMGlobalHeader[itrm].raw() == 0) {
	status = true;
	mSummary.TRMFaultFlag[itrm] = true;
#ifdef VERBOSE
	fmt = boost::format("Missing TRM Header (SlotID=%d)") % SlotID;
	if (mVerbose) std::cout << fmt << std::endl;
#endif
	continue;
      }

      /** check TRM Global Trailer **/
      if (mSummary.TRMGlobalTrailer[itrm].raw() == 0) {
	status = true;
	mSummary.TRMFaultFlag[itrm] = true;
#ifdef VERBOSE
	fmt = boost::format("Missing TRM Trailer (SlotID=%d)") % SlotID;
	if (mVerbose) std::cout << fmt << std::endl;
#endif
	continue;
      }

      /** check TRM EventCounter **/
#if 0
      uint32_t EventCounter = mSummary.TRMGlobalTrailer[itrm].EventCounter;
      if (EventCounter != LocalEventCounter) {
	status = true;
	mSummary.TRMFalutFlag[itrm] = true;
#ifdef VERBOSE
	fmt = boost::format("TRM EventCounter / DRM LocalEventCounter mismatch: %d / %d (SlotID=%d)") % EventCounter % LocalEventCounter % SlotID;
	if (mVerbose) std::cout << fmt << std::endl;
#endif
	continue;
      }
#endif
      
      /** loop over TRM chains **/
      for (int ichain = 0; ichain < 2; ichain++) {
	
 	/** check TRM Chain Header **/
	if (mSummary.TRMChainHeader[itrm][ichain].raw() == 0) {
	  status = true;
	  mSummary.TRMChainFaultFlag[itrm][ichain] = true;
#ifdef VERBOSE
	  fmt = boost::format("Missing TRM Chain Header (SlotID=%d, chain=%d)") % SlotID % ichain;
	  if (mVerbose) std::cout << fmt << std::endl;
#endif
	  continue;
	}

 	/** check TRM Chain Trailer **/
	if (mSummary.TRMChainTrailer[itrm][ichain].raw() == 0) {
	  status = true;
	  mSummary.TRMChainFaultFlag[itrm][ichain] = true;
#ifdef VERBOSE
	  fmt = boost::format("Missing TRM Chain Trailer (SlotID=%d, chain=%d)") % SlotID % ichain;
	  if (mVerbose) std::cout << fmt << std::endl;
#endif
	  continue;
	}

	/** check TRM Chain EventCounter **/
	uint32_t EventCounter = mSummary.TRMChainTrailer[itrm][ichain].EventCounter;
	if (EventCounter != LocalEventCounter) {
	  status = true;
	  mSummary.TRMChainFaultFlag[itrm][ichain] = true;
#ifdef VERBOSE
	  fmt = boost::format("TRM Chain EventCounter / DRM LocalEventCounter mismatch: %d / %d (SlotID=%d, chain=%d)") % EventCounter % EventCounter % SlotID % ichain;
	  if (mVerbose) std::cout << fmt << std::endl;
#endif
	  continue;
	}
      
	/** check TRM Chain Status **/
	uint32_t Status = mSummary.TRMChainTrailer[itrm][ichain].Status;
	if (Status != 0) {
	  status = true;
	  mSummary.TRMChainFaultFlag[itrm][ichain] = true;
#ifdef VERBOSE
	  fmt = boost::format("TRM Chain bad Status: %d (SlotID=%d, chain=%d)") % Status % SlotID % ichain;
	  if (mVerbose) std::cout << fmt << std::endl;
#endif	  
	}

	/** check TRM Chain BunchID **/
	uint32_t BunchID = mSummary.TRMChainHeader[itrm][ichain].BunchID;
	if (BunchID != L0BCID) {
	  status = true;
	  mSummary.TRMChainFaultFlag[itrm][ichain] = true;
#ifdef VERBOSE
	  fmt = boost::format("TRM Chain BunchID / DRM L0BCID mismatch: %d / %d (SlotID=%d, chain=%d)") % BunchID % L0BCID % SlotID % ichain;
	  if (mVerbose) std::cout << fmt << std::endl;
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

