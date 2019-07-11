#include "Decoder.h"
#include <iostream>
#include <chrono>
#include <boost/format.hpp>

#define VERBOSE

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

  void
  Decoder::print(std::string what)
  {
    if (mVerbose)
      std::cout << boost::format("%08x") % mUnion->Data << " " << what << std::endl;
  }
  
  bool
  Decoder::next()
  {
    if (((char *)mUnion - mBuffer) >= mSize) return true;
    if (mUnion->Word.WordType != 1) {
      print("[ERROR]");
      return true;
    }    
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

    unsigned int nWords = 1;

    auto BunchID = mUnion->CrateHeader.BunchID;
    auto EventCounter = mUnion->CrateHeader.EventCounter;
    auto DRMID = mUnion->CrateHeader.DRMID;
#ifdef VERBOSE
    print(str(boost::format("Crate header (DRMID=%d, EventCounter=%d, BunchID=%d)") % DRMID % EventCounter % BunchID));
#endif
    mUnion++; nWords++;

    auto OrbitID = mUnion->CrateOrbit.OrbitID;
#ifdef VERBOSE
    print(str(boost::format("Crate orbit (OrbitID=%d)") % OrbitID));
#endif
    mUnion++; nWords++;

    /** loop over Crate payload **/
    while (true) {
      
      /** Crate trailer detected **/
      if (mUnion->Word.WordType == 1) {
#ifdef VERBOSE
	print("Crate trailer");
#endif
	mUnion++; nWords++;
	
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

      /** Frame header detected **/
      auto NumberOfHits = mUnion->FrameHeader.NumberOfHits;
      auto FrameID = mUnion->FrameHeader.FrameID;
      auto TRMID = mUnion->FrameHeader.TRMID;
#ifdef VERBOSE
      print(str(boost::format("Frame header (TRMID=%d, FrameID=%d, NumberOfHits=%d)") % TRMID % FrameID % NumberOfHits));
#endif
      mUnion++; nWords++;
      /** loop over frame payload **/
      for (int ihit = 0; ihit < NumberOfHits; ihit++) {
	print("Packed hit");
	mUnion++; nWords++;
      }
      
    }

    return false;
  }
  
}}}
