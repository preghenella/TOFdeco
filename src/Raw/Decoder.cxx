#include "Decoder.h"
#include <iostream>
#include <chrono>
#include <boost/format.hpp>

namespace tof {
namespace data {
namespace raw {

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
  
  void
  Decoder::print(std::string what)
  {
    if (mVerbose)
      std::cout << boost::format("%08x") % mUnion->Data << " " << what << std::endl;
  }
  
  bool
  Decoder::next()
  {
    /** search for DRM Global header **/
    while (true) {
      if (((char *)mUnion - mBuffer) >= mSize)
	return true;
      if (mUnion->Word.WordType == 4 && mUnion->Word.SlotID == 1)
	break;
      mUnion++;
    }
    return false;
  }

  void
  Decoder::clear()
  {
    mSummary.DRMGlobalHeader  = {0x0};
    mSummary.DRMStatusHeader1 = {0x0};
    mSummary.DRMStatusHeader2 = {0x0};
    mSummary.DRMStatusHeader3 = {0x0};
    mSummary.DRMStatusHeader4 = {0x0};
    //    mSummary.DRMStatusHeader5 = {0x0};
    mSummary.DRMGlobalTrailer  = {0x0};
    for (int itrm = 0; itrm < 10; itrm++) {
      mSummary.TRMGlobalHeader[itrm]  = {0x0};
      mSummary.TRMGlobalTrailer[itrm] = {0x0};
      for (int ichain = 0; ichain < 2; ichain++) {
	mSummary.TRMChainHeader[itrm][ichain]  = {0x0};
	mSummary.TRMChainTrailer[itrm][ichain] = {0x0};
	for (int itdc = 0; itdc < 15; itdc++) {
	  mSummary.nTDCUnpackedHits[itrm][ichain][itdc] = 0;
	}}}
  }
  
  bool
  Decoder::decode()
  {
#ifdef VERBOSE
    if (mVerbose)
      std::cout << "-------- START DECODE EVENT ----------------------------------------" << std::endl;    
#endif
    auto start = std::chrono::high_resolution_clock::now();
    
    clear();
    unsigned int nWords = 1;
    
    mSummary.DRMGlobalHeader = mUnion->DRMGlobalHeader;
#ifdef VERBOSE
    print("DRM Global Header");
#endif
    mUnion++; nWords++;
    mSummary.DRMStatusHeader1 = mUnion->DRMStatusHeader1;
#ifdef VERBOSE
    print("DRM Status Header 1");
#endif
    mUnion++; nWords++;
    mSummary.DRMStatusHeader2 = mUnion->DRMStatusHeader2;
#ifdef VERBOSE
    print("DRM Status Header 2");
#endif
    mUnion++; nWords++;
    mSummary.DRMStatusHeader3 = mUnion->DRMStatusHeader3;
#ifdef VERBOSE
    print("DRM Status Header 3");
#endif
    mUnion++; nWords++;
    mSummary.DRMStatusHeader4 = mUnion->DRMStatusHeader4;
#ifdef VERBOSE
    print("DRM Status Header 4");
#endif
    mUnion++; nWords++;
    //    mSummary.DRMStatusHeader5 = mUnion->DRMStatusHeader5;
#ifdef VERBOSE
    print("DRM Status Header 5");
#endif
    mUnion++; nWords++;

    /** loop over DRM payload **/
    while (true) {

      /** LTM global header detected **/
      if (mUnion->Word.WordType == 4 && mUnion->Word.SlotID == 2) {

#ifdef VERBOSE
	print("LTM Global Header");
#endif
	mUnion++; nWords++;

	/** loop over LTM payload **/
	while (true) {

	  /** TRM global trailer detected **/
	  if (mUnion->Word.WordType == 5 && mUnion->Word.SlotID == 2) {
#ifdef VERBOSE
	    print("LTM Global Trailer");
#endif
	    mUnion++; nWords++;
	    break;
	  }

#ifdef VERBOSE
	  print("LTM data");
#endif
	  mUnion++; nWords++;
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
	mUnion++; nWords++;
	
	/** loop over TRM payload **/
	while (true) {

	  /** TRM chain-A header detected **/
	  if (mUnion->Word.WordType == 0 && mUnion->Word.SlotID == islot) {
	    int ichain = 0;
	    mSummary.TRMChainHeader[itrm][ichain] = mUnion->TRMChainHeader;
#ifdef VERBOSE
	    print("TRM Chain-A Header");
#endif
	    mUnion++; nWords++;

	    /** loop over TRM chain-A payload **/
	    while (true) {

	      /** TDC hit detected **/
	      if (mUnion->Word.WordType & 0x8) {
		int itdc = mUnion->TDCUnpackedHit.TDCID;
		int ihit = mSummary.nTDCUnpackedHits[itrm][ichain][itdc];
		mSummary.TDCUnpackedHit[itrm][ichain][itdc][ihit] = mUnion->TDCUnpackedHit;
		mSummary.nTDCUnpackedHits[itrm][ichain][itdc]++;
#ifdef VERBOSE
		print("TDC hit");
#endif
		mUnion++; nWords++;
		continue;
	      }
	      
	      /** TDC error detected **/
	      if (mUnion->Word.WordType == 6) {
#ifdef VERBOSE
		print("TDC error");
#endif
		mUnion++; nWords++;
		continue;
	      }

	      /** TRM chain-A trailer detected **/
	      if (mUnion->Word.WordType == 1) {
		mSummary.TRMChainTrailer[itrm][ichain] = mUnion->TRMChainTrailer;
#ifdef VERBOSE
		print("TRM Chain-A Trailer");
#endif
		mUnion++; nWords++;
		break;
	      }
		
#ifdef VERBOSE
	      print("[ERROR] breaking TRM Chain-A decode stream");
#endif
	      mUnion++; nWords++;
	      break;
	      
	    }} /** end of loop over TRM chain-A payload **/	    
	  
	  /** TRM chain-B header detected **/
	  if (mUnion->Word.WordType == 2 && mUnion->Word.SlotID == islot) {
	    int ichain = 1;
	    mSummary.TRMChainHeader[itrm][ichain] = mUnion->TRMChainHeader;
#ifdef VERBOSE
	    print("TRM Chain-B Header");
#endif
	    mUnion++; nWords++;

	    /** loop over TRM chain-B payload **/
	    while (true) {

	      /** TDC hit detected **/
	      if (mUnion->Word.WordType & 0x8) {
		int itdc = mUnion->TDCUnpackedHit.TDCID;
		int ihit = mSummary.nTDCUnpackedHits[itrm][ichain][itdc];
		mSummary.TDCUnpackedHit[itrm][ichain][itdc][ihit] = mUnion->TDCUnpackedHit;
		mSummary.nTDCUnpackedHits[itrm][ichain][itdc]++;
#ifdef VERBOSE
		print("TDC hit");
#endif
		mUnion++; nWords++;
		continue;
	      }
	      
	      /** TDC error detected **/
	      if (mUnion->Word.WordType == 6) {
#ifdef VERBOSE
		print("TDC error");
#endif
		mUnion++; nWords++;
		continue;
	      }

	      /** TRM chain-B trailer detected **/
	      if (mUnion->Word.WordType == 3) {
		mSummary.TRMChainTrailer[itrm][ichain] = mUnion->TRMChainTrailer;
#ifdef VERBOSE
		print("TRM Chain-B Trailer");
#endif
		mUnion++; nWords++;
		break;
	      }
		
#ifdef VERBOSE
	      print("[ERROR] breaking TRM Chain-B decode stream");
#endif
	      mUnion++; nWords++;
	      break;
	      
	    }} /** end of loop over TRM chain-A payload **/	    
	  
	  /** TRM global trailer detected **/
	  if (mUnion->Word.WordType == 5 && mUnion->Word.SlotID == 0xf) {
	    mSummary.TRMGlobalTrailer[itrm] = mUnion->TRMGlobalTrailer;
#ifdef VERBOSE
	    print("TRM Global Trailer");
#endif
	    mUnion++; nWords++;
	    
	    /** filler detected **/
	    if (mUnion->Word.WordType == 7) {
#ifdef VERBOSE
	      print("Filler");
#endif
	      mUnion++; nWords++;
	    }
	    
	    break;
	  }
	  
#ifdef VERBOSE
	  print("[ERROR] breaking TRM decode stream");
#endif
	  mUnion++; nWords++;
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
	break;
      }
      
#ifdef VERBOSE
      print("[ERROR] trying to recover DRM decode stream");
#endif
      mUnion++; nWords++;
      
    } /** end of loop over DRM payload **/

    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;

    mIntegratedBytes += nWords * 4;
    mIntegratedTime += elapsed.count();
    
#ifdef VERBOSE
    if (mVerbose)
      std::cout << "-------- END DECODE EVENT ------------------------------------------"
		<< " | " << nWords << " words"
		<< " | " << 1.e3  * elapsed.count() << " ms"
		<< " | " << 1.e-6 * mIntegratedBytes / mIntegratedTime << " MB/s (average)"
		<< std::endl;
#endif
    
    return false;
  }

}}}
