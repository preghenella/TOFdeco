#include "Decoder.h"
#include <iostream>
#include <boost/format.hpp>

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

  void
  Decoder::print(std::string what)
  {
    if (mVerbose)
      std::cout << boost::format("%08x") % mUnion.Data << " " << what << std::endl;
  }
  
  bool
  Decoder::next()
  {
    if (mFile.eof()) return true;
    mFile.read(reinterpret_cast<char *>(&mUnion.Data), sizeof(mUnion.Data));
    mWordType = mUnion.Word.WordType;
    return false;
  }
  
  bool
  Decoder::decode()
  {
    next();
    if (mWordType != 1) {
      print("[ERROR]");
      return true;
    }
    
    if (mVerbose)
      std::cout << "-------- START DECODE EVENT ----------------------------------------" << std::endl;
    
    auto BunchID = mUnion.CrateHeader.BunchID;
    auto EventCounter = mUnion.CrateHeader.EventCounter;
    auto DRMID = mUnion.CrateHeader.DRMID;
    print(str(boost::format("Crate header (DRMID=%d, EventCounter=%d, BunchID=%d)")
	      % DRMID % EventCounter % BunchID));
    next();

    /** loop over Crate payload **/
    while (true) {

      /** Crate trailer detected **/
      if (mWordType == 1) {
	print("Crate trailer");
	return false;
      }

      /** Frame header detected **/
      auto NumberOfHits = mUnion.FrameHeader.NumberOfHits;
      auto FrameID = mUnion.FrameHeader.FrameID;
      auto TRMID = mUnion.FrameHeader.TRMID;
      print(str(boost::format("Frame header (TRMID=%d, FrameID=%d, NumberOfHits=%d)")
		% TRMID % FrameID % NumberOfHits));
      next();
      /** loop over frame payload **/
      for (int ihit = 0; ihit < NumberOfHits; ihit++) {
	print("Packed hit");
	next();
      }
      
    }

    return false;
  }
  
}}}
