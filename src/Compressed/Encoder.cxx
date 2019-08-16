#include "Encoder.h"
#include <iostream>
#include <chrono>
#include <boost/format.hpp>

namespace tof {
namespace data {
namespace compressed {

  bool
  Encoder::open(std::string name)
  {
    if (mFile.is_open()) {
      std::cout << "Warning: a file was already open, closing" << std::endl;
      mFile.close();
    }
    mFile.open(name.c_str(), std::fstream::out | std::fstream::binary);
    if (!mFile.is_open()) {
      std::cerr << "Cannot open " << name << std::endl;
      return true;
    }
    return false;
  }

  bool
  Encoder::flush()
  {
#ifdef VERBOSE
    if (mVerbose)
      std::cout << "-------- FLUSH ENCODER BUFFER --------------------------------------"
		<< " | " << mByteCounter << " bytes"
		<< std::endl;
#endif
    mFile.write(mBuffer, mByteCounter);
    mPointer = mBuffer;
    mByteCounter = 0;
    return false;
  }
  
  bool
  Encoder::close()
  {
    if (mFile.is_open())
      mFile.close();
    return false;
  }
  
  bool
  Encoder::init()
  {
#ifdef VERBOSE
    if (mVerbose)
      std::cout << "-------- INITIALISE ENCODER BUFFER ---------------------------------"
		<< " | " << mSize << " bytes"
		<< std::endl;
#endif
    if (mBuffer) {
      std::cout << "Warning: a buffer was already allocated, cleaning" << std::endl;
      delete [] mBuffer;
    }
    mBuffer = new char[mSize];
    mPointer = mBuffer;
    mByteCounter = 0;
    return false;
  }
  
  void
  Encoder::next32()
  {
    mPointer += 4;
    mByteCounter += 4;
    mUnion = reinterpret_cast<Union_t *>(mPointer);
  }

  bool
  Encoder::encode(const tof::data::raw::Summary_t &summary)
  {

#ifdef VERBOSE
    if (mVerbose)
      std::cout << "-------- START ENCODE EVENT ----------------------------------------" << std::endl;
#endif
    auto start = std::chrono::high_resolution_clock::now();	
    
    mUnion = reinterpret_cast<Union_t *>(mPointer);

    // crate header
    mUnion->CrateHeader = {0x0};
    mUnion->CrateHeader.MustBeOne = 1;
    mUnion->CrateHeader.DRMID = summary.DRMGlobalHeader.DRMID;
    mUnion->CrateHeader.EventCounter = summary.DRMGlobalTrailer.LocalEventCounter;
    mUnion->CrateHeader.BunchID = summary.DRMStatusHeader3.L0BCID;
#ifdef VERBOSE
    if (mVerbose) {
      auto BunchID = mUnion->CrateHeader.BunchID;
      auto EventCounter = mUnion->CrateHeader.EventCounter;
      auto DRMID = mUnion->CrateHeader.DRMID;
      std::cout << boost::format("%08x") % mUnion->Data
      		<< " "
		<< boost::format("Crate header (DRMID=%d, EventCounter=%d, BunchID=%d)") % DRMID % EventCounter % BunchID
		<< std::endl;
    }
#endif
    next32();
    
    // crate orbit
    mUnion->CrateOrbit = {0x0};
    mUnion->CrateOrbit.OrbitID = summary.DRMOrbitHeader.Orbit;
#ifdef VERBOSE
    if (mVerbose) {
      auto OrbitID = mUnion->CrateOrbit.OrbitID;
      std::cout << boost::format("%08x") % mUnion->Data
      		<< " "
		<< boost::format("Crate orbit (OrbitID=%d)") % OrbitID
		<< std::endl;
    }
#endif
    next32();
    
    /** loop over TRMs **/

    unsigned char nPackedHits[256] = {0};
    PackedHit_t PackedHit[256][256];
    for (int itrm = 0; itrm < 10; itrm++) {

      /** check if TRM is empty **/
      if (summary.nTRMSpiderHits[itrm] == 0)
	continue;

      unsigned char firstFilledFrame = 255;
      unsigned char lastFilledFrame = 0;

      /** loop over hits **/
      for (int ihit = 0; ihit < summary.nTRMSpiderHits[itrm]; ++ihit) {
	
	auto hit = summary.TRMSpiderHit[itrm][ihit];
	auto iframe = hit.HitTime >> 13;
	auto phit = nPackedHits[iframe];
	PackedHit[iframe][phit].Chain = hit.Chain;
	PackedHit[iframe][phit].TDCID = hit.TDCID;
	PackedHit[iframe][phit].Channel = hit.Chan;
	PackedHit[iframe][phit].Time = hit.HitTime;
	PackedHit[iframe][phit].TOT = hit.TOTWidth;
	nPackedHits[iframe]++;
	
	if (iframe < firstFilledFrame)
	  firstFilledFrame = iframe;
	if (iframe > lastFilledFrame)
	  lastFilledFrame = iframe;
      }
        
      /** loop over frames **/
      for (int iframe = firstFilledFrame; iframe < lastFilledFrame + 1;
           iframe++) {

        /** check if frame is empty **/
        if (nPackedHits[iframe] == 0)
          continue;

        // frame header
	mUnion->FrameHeader = {0x0};
	mUnion->FrameHeader.MustBeZero = 0;
	mUnion->FrameHeader.TRMID = itrm + 3;
	mUnion->FrameHeader.FrameID = iframe;
        mUnion->FrameHeader.NumberOfHits = nPackedHits[iframe];
#ifdef VERBOSE
	if (mVerbose) {
	  auto NumberOfHits = mUnion->FrameHeader.NumberOfHits;
	  auto FrameID = mUnion->FrameHeader.FrameID;
	  auto TRMID = mUnion->FrameHeader.TRMID;
	  std::cout << boost::format("%08x") % mUnion->Data
		    << " "
		    << boost::format("Frame header (TRMID=%d, FrameID=%d, NumberOfHits=%d)") % TRMID % FrameID % NumberOfHits
		    << std::endl;
	}
#endif
	next32();
	
	// packed hits
        for (int ihit = 0; ihit < nPackedHits[iframe]; ++ihit) {
          mUnion->PackedHit = PackedHit[iframe][ihit];
#ifdef VERBOSE
	  if (mVerbose) {
            auto Chain = mUnion->PackedHit.Chain;
            auto TDCID = mUnion->PackedHit.TDCID;
            auto Channel = mUnion->PackedHit.Channel;
            auto Time = mUnion->PackedHit.Time;
            auto TOT = mUnion->PackedHit.TOT;
            std::cout << boost::format("%08x") % mUnion->Data << " "
                      << boost::format("Packed hit (Chain=%d, TDCID=%d, "
                                       "Channel=%d, Time=%d, TOT=%d)") %
                             Chain % TDCID % Channel % Time % TOT
                      << std::endl;
          }
#endif
	  next32();
        }

        nPackedHits[iframe] = 0;
      }
    }

    // crate trailer
    mUnion->CrateTrailer = {0x0};
    mUnion->CrateTrailer.MustBeOne = 1;
#ifdef VERBOSE
    if (mVerbose) {
      std::cout << boost::format("%08x") % mUnion->Data
		<< " "
		<< "Crate trailer"
		<< std::endl;
    }
#endif
    next32();

    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;

    mIntegratedBytes += mByteCounter;
    mIntegratedTime += elapsed.count();
    
#ifdef VERBOSE
    if (mVerbose)
      std::cout << "-------- END ENCODE EVENT ------------------------------------------"
		<< " | " << mByteCounter << " bytes"
		<< " | " << 1.e3  * elapsed.count() << " ms"
		<< " | " << 1.e-6 * mIntegratedBytes / mIntegratedTime << " MB/s (average)"
		<< std::endl;
#endif

    return false;

  }
}}}
