#include "Decoder.h"
#include <iostream>
#include <chrono>

namespace tof {
namespace data {
namespace compressed {

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
    if (mFile.is_open())
      mFile.close();
    return false;
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
  Decoder::next()
  {
    if (((char *)mUnion - mBuffer) >= mSize) return true;
    if (mUnion->Word.WordType != 1) {
      printf(" %08x [ERROR] \n", mUnion->Data);
      return true;
    }    
    return false;
  }

  void
  Decoder::clear()
  {
    mSummary.CrateHeader  = {0x0};
    mSummary.CrateOrbit   = {0x0};
    mSummary.CrateTrailer  = {0x0};
    mSummary.nHits = 0;
  }
  
  bool
  Decoder::decode()
  {
#ifdef DECODE_VERBOSE
    if (mVerbose)
      std::cout << "-------- START DECODE EVENT ----------------------------------------" << std::endl;
#endif
    auto start = std::chrono::high_resolution_clock::now();

    clear();
    mByteCounter = 0;

    mSummary.CrateHeader = mUnion->CrateHeader;
#ifdef DECODE_VERBOSE
    auto BunchID = mUnion->CrateHeader.BunchID;
    auto EventCounter = mUnion->CrateHeader.EventCounter;
    auto DRMID = mUnion->CrateHeader.DRMID;
    printf(" %08x Crate header (DRMID=%d, EventCounter=%d, BunchID=%d) \n", mUnion->Data, DRMID, EventCounter, BunchID);
#endif
    mUnion++; mByteCounter += 4;

    mSummary.CrateOrbit = mUnion->CrateOrbit;
#ifdef DECODE_VERBOSE
    auto OrbitID = mUnion->CrateOrbit.OrbitID;
    printf(" %08x Crate orbit (OrbitID=%d) \n", mUnion->Data, OrbitID);
#endif
    mUnion++; mByteCounter += 4;

    /** loop over Crate payload **/
    while (true) {
      
      /** Crate trailer detected **/
      if (mUnion->Word.WordType == 1) {
	mSummary.CrateTrailer = mUnion->CrateTrailer;
#ifdef DECODE_VERBOSE
	printf(" %08x Crate trailer \n", mUnion->Data);
#endif
	mUnion++; mByteCounter += 4;
	
	auto finish = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = finish - start;
	
	mIntegratedBytes += mByteCounter;
	mIntegratedTime += elapsed.count();
	
#ifdef DECODE_VERBOSE
	if (mVerbose)
	  std::cout << "-------- END DECODE EVENT ------------------------------------------"
		    << " | " << mByteCounter << " bytes"
		    << " | " << 1.e3  * elapsed.count() << " ms"
		    << " | " << 1.e-6 * mIntegratedBytes / mIntegratedTime << " MB/s (average)"
		    << std::endl;
#endif
	
	return false;
      }

      /** Frame header detected **/
      auto NumberOfHits = mUnion->FrameHeader.NumberOfHits;
      auto FrameHeader = mUnion->FrameHeader;
#ifdef DECODE_VERBOSE
      auto TRMID = mUnion->FrameHeader.TRMID;
      auto FrameID = mUnion->FrameHeader.FrameID;
      printf(" %08x Frame header (TRMID=%d, FrameID=%d, NumberOfHits=%d) \n", mUnion->Data, TRMID, FrameID, NumberOfHits);
#endif
      mUnion++; mByteCounter += 4;
      /** loop over frame payload **/
      for (int ihit = 0; ihit < NumberOfHits; ihit++) {
	mSummary.FrameHeader[mSummary.nHits] = FrameHeader;
	mSummary.PackedHit[mSummary.nHits] = mUnion->PackedHit;
	mSummary.nHits++;
#ifdef DECODE_VERBOSE
	auto Chain = mUnion->PackedHit.Chain;
	auto TDCID = mUnion->PackedHit.TDCID;
	auto Channel = mUnion->PackedHit.Channel;
	auto Time = mUnion->PackedHit.Time;
	auto TOT = mUnion->PackedHit.TOT;
	printf(" %08x Packed hit (Chain=%d, TDCID=%d, Channel=%d, Time=%d, TOT=%d) \n", mUnion->Data, Chain, TDCID, Channel, Time, TOT);
#endif
	mUnion++; mByteCounter += 4;
      }
      
    }

    return false;
  }
  
}}}
