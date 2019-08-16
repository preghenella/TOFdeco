#include "Decoder.h"
#include <iostream>
#include <chrono>
#include <boost/format.hpp>

#define VERBOSE

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
    mPointer = mBuffer;
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
    for (int itrm = 0; itrm < 10; itrm++) {
      mSummary.TRMGlobalHeader[itrm]  = {0x0};
      mSummary.TRMGlobalTrailer[itrm] = {0x0};
      mSummary.TRMempty[itrm] = true;
      mSummary.nTRMSpiderHits[itrm] = 0;
      for (int ichain = 0; ichain < 2; ichain++) {
	mSummary.TRMChainHeader[itrm][ichain]  = {0x0};
	mSummary.TRMChainTrailer[itrm][ichain] = {0x0};
	for (int itdc = 0; itdc < 15; itdc++) {
	  mSummary.nTDCUnpackedHits[itrm][ichain][itdc] = 0;
	}}}
  }

  void
  Decoder::next32()
  {
    mPointer += mSkip;
    mByteCounter += 4;
    mSkip = (mSkip + 8) % 16;
    mUnion = reinterpret_cast<Union_t *>(mPointer);
  }

  void
  Decoder::next128()
  {
    mPointer += 16;
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

    uint32_t BlockLength = mRDH->Word0.BlockLength;
    uint32_t PacketCounter = mRDH->Word0.PacketCounter;
    uint32_t HeaderSize = mRDH->Word0.HeaderSize;
    boost::format fmt = boost::format("RDH Word0 (HeaderSize=%d, PacketCounter=%d)") % HeaderSize % PacketCounter;
#ifdef VERBOSE
    if (mVerbose)
      printRDH(fmt.str());
#endif
    next128();

    uint32_t TrgOrbit = mRDH->Word1.TrgOrbit;
    uint32_t HbOrbit = mRDH->Word1.HbOrbit;
    fmt = boost::format("RDH Word1 (TrgOrbit=%d, HbOrbit=%d)") % TrgOrbit % HbOrbit;
#ifdef VERBOSE
    if (mVerbose)
      printRDH(fmt.str());
#endif
    next128();

    uint32_t TrgBC = mRDH->Word2.TrgBC;
    uint32_t HbBC = mRDH->Word2.HbBC;
    uint32_t TrgType = mRDH->Word2.TrgType;
    fmt = boost::format("RDH Word2 (TrgBC=%d, HbBC=%d, TrgType=%d)") % TrgBC % HbBC % TrgType;
#ifdef VERBOSE
    if (mVerbose)
      printRDH(fmt.str());
#endif
    next128();

#ifdef VERBOSE
    if (mVerbose)
      printRDH("RDH Word3");
#endif
    next128();

    return false;
  }
  
  bool
  Decoder::decode()
  {
#ifdef VERBOSE
    if (mVerbose)
      std::cout << "-------- START DECODE EVENT ----------------------------------------" << std::endl;    
#endif
    auto start = std::chrono::high_resolution_clock::now();
    
    mUnion = reinterpret_cast<Union_t *>(mPointer);
    mByteCounter = 0;
    mSkip = 4;

    clear();
    
    /** DRM Common Header not detected **/
    if (mUnion->Word.WordType != 4) {
#ifdef VERBOSE
      print("[ERROR] fatal error");
#endif
      return true;
    }
      
    mSummary.DRMCommonHeader = mUnion->DRMCommonHeader;
#ifdef VERBOSE
    uint32_t Payload = mUnion->DRMCommonHeader.Payload;
    fmt = boost::format("DRM Common Header (Payload=%d)") % Payload;
    print(fmt.str());
#endif
    next32();
    mSummary.DRMOrbitHeader = mUnion->DRMOrbitHeader;
#ifdef VERBOSE
    uint32_t Orbit = mUnion->DRMOrbitHeader.Orbit;
    fmt = boost::format("DRM Orbit Header (Orbit=%d)") % Orbit;
    print(fmt.str());
#endif
    next32();    
    mSummary.DRMGlobalHeader = mUnion->DRMGlobalHeader;
#ifdef VERBOSE
    uint32_t DRMID = mUnion->DRMGlobalHeader.DRMID;
    fmt = boost::format("DRM Global Header (DRMID=%d)") % DRMID;
    print(fmt.str());
#endif
    next32();
    mSummary.DRMStatusHeader1 = mUnion->DRMStatusHeader1;
#ifdef VERBOSE
    print("DRM Status Header 1");
#endif
    next32();
    mSummary.DRMStatusHeader2 = mUnion->DRMStatusHeader2;
#ifdef VERBOSE
    print("DRM Status Header 2");
#endif
    next32();
    mSummary.DRMStatusHeader3 = mUnion->DRMStatusHeader3;
#ifdef VERBOSE
    print("DRM Status Header 3");
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
      if (mUnion->Word.WordType == 4 && mUnion->Word.SlotID == 2) {

#ifdef VERBOSE
	print("LTM Global Header");
#endif
	next32();

	/** loop over LTM payload **/
	while (true) {

	  /** LTM global trailer detected **/
	  if (mUnion->Word.WordType == 5 && mUnion->Word.SlotID == 2) {
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
      if (mUnion->Word.WordType == 4 && mUnion->Word.SlotID > 2) {
	int islot = mUnion->Word.SlotID;
	int itrm = islot - 3;
	mSummary.TRMGlobalHeader[itrm] = mUnion->TRMGlobalHeader;
#ifdef VERBOSE
	print("TRM Global Header");
#endif
	next32();
	
	/** loop over TRM payload **/
	while (true) {

	  /** TRM chain-A header detected **/
	  if (mUnion->Word.WordType == 0 && mUnion->Word.SlotID == islot) {
	    int ichain = 0;
	    mSummary.TRMChainHeader[itrm][ichain] = mUnion->TRMChainHeader;
#ifdef VERBOSE
	    print("TRM Chain-A Header");
#endif
	    next32();

	    /** loop over TRM chain-A payload **/
	    while (true) {

	      /** TDC hit detected **/
	      if (mUnion->Word.WordType & 0x8) {
                mSummary.TRMempty[itrm] = false;
                int itdc = mUnion->TDCUnpackedHit.TDCID;
		int ihit = mSummary.nTDCUnpackedHits[itrm][ichain][itdc];
		mSummary.TDCUnpackedHit[itrm][ichain][itdc][ihit] = mUnion->TDCUnpackedHit;
		mSummary.nTDCUnpackedHits[itrm][ichain][itdc]++;
#ifdef VERBOSE
		print("TDC hit");
#endif
		next32();
		continue;
	      }
	      
	      /** TDC error detected **/
	      if (mUnion->Word.WordType == 6) {
#ifdef VERBOSE
		print("TDC error");
#endif
		next32();
		continue;
	      }

	      /** TRM chain-A trailer detected **/
	      if (mUnion->Word.WordType == 1) {
		mSummary.TRMChainTrailer[itrm][ichain] = mUnion->TRMChainTrailer;
#ifdef VERBOSE
		print("TRM Chain-A Trailer");
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
	  if (mUnion->Word.WordType == 2 && mUnion->Word.SlotID == islot) {
	    int ichain = 1;
	    mSummary.TRMChainHeader[itrm][ichain] = mUnion->TRMChainHeader;
#ifdef VERBOSE
	    print("TRM Chain-B Header");
#endif
	    next32();

	    /** loop over TRM chain-B payload **/
	    while (true) {

	      /** TDC hit detected **/
	      if (mUnion->Word.WordType & 0x8) {
                mSummary.TRMempty[itrm] = false;
                int itdc = mUnion->TDCUnpackedHit.TDCID;
		int ihit = mSummary.nTDCUnpackedHits[itrm][ichain][itdc];
		mSummary.TDCUnpackedHit[itrm][ichain][itdc][ihit] = mUnion->TDCUnpackedHit;
		mSummary.nTDCUnpackedHits[itrm][ichain][itdc]++;
#ifdef VERBOSE
		print("TDC hit");
#endif
		next32();
		continue;
	      }
	      
	      /** TDC error detected **/
	      if (mUnion->Word.WordType == 6) {
#ifdef VERBOSE
		print("TDC error");
#endif
		next32();
		continue;
	      }

	      /** TRM chain-B trailer detected **/
	      if (mUnion->Word.WordType == 3) {
		mSummary.TRMChainTrailer[itrm][ichain] = mUnion->TRMChainTrailer;
#ifdef VERBOSE
		print("TRM Chain-B Trailer");
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
	  if (mUnion->Word.WordType == 5 && (mUnion->Word.SlotID & 0x3) == 0x3) {
	    mSummary.TRMGlobalTrailer[itrm] = mUnion->TRMGlobalTrailer;
#ifdef VERBOSE
	    print("TRM Global Trailer");
#endif
	    next32();
	    
 	    /** filler detected **/
	    if (mUnion->Word.WordType == 7) {
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
      if (mUnion->Word.WordType == 5 && mUnion->Word.SlotID == 1) {
	mSummary.DRMGlobalTrailer = mUnion->DRMGlobalTrailer;
#ifdef VERBOSE
	print("DRM Global Trailer");
#endif
	next32();

	/** filler detected **/
	if (mUnion->Word.WordType == 7) {
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
    spider();
    
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

  void
  Decoder::spider()
  {

    /** loop over TRMs **/
    for (int itrm = 0; itrm < 10; itrm++) {

      /** check if TRM is empty **/
      if (mSummary.TRMempty[itrm])
	continue;

      /** loop over TRM chains **/
      for (int ichain = 0; ichain < 2; ++ichain) {
	
	/** loop over TDCs **/
	for (int itdc = 0; itdc < 15; ++itdc) {
	  
          auto nhits = mSummary.nTDCUnpackedHits[itrm][ichain][itdc];
          if (nhits == 0)
            continue;

          /** loop over hits **/
          for (int ihit = 0; ihit < nhits; ++ihit) {

            auto lhit = mSummary.TDCUnpackedHit[itrm][ichain][itdc][ihit];
            if (lhit.PSBits != 0x1)
              continue; // must be a leading hit
            auto tot = 0;
	    
            // check next hits for packing
            for (int jhit = ihit + 1; jhit < nhits; ++jhit) {
              auto thit = mSummary.TDCUnpackedHit[itrm][ichain][itdc][jhit];
              if (thit.PSBits == 0x2 && thit.Chan == lhit.Chan) { // must be a trailing hit from same channel
                tot = thit.HitTime - lhit.HitTime; // compute TOT
                lhit.PSBits = 0x0;                 // mark as used
                break;
              }
            }

	    auto phit = mSummary.nTRMSpiderHits[itrm];
            mSummary.TRMSpiderHit[itrm][phit].Chain = ichain;
            mSummary.TRMSpiderHit[itrm][phit].TDCID = itdc;
            mSummary.TRMSpiderHit[itrm][phit].Chan = lhit.Chan;
            mSummary.TRMSpiderHit[itrm][phit].HitTime = lhit.HitTime;
            mSummary.TRMSpiderHit[itrm][phit].TOTWidth = tot;
            mSummary.TRMSpiderHit[itrm][phit].EBit = lhit.EBit;
            mSummary.nTRMSpiderHits[itrm]++;
	    
	  }}}}
  }
  
}}}

