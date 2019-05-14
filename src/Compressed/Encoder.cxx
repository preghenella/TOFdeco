#include "Encoder.h"
#include <iostream>
#include <chrono>
#include <boost/format.hpp>

#define VERBOSE

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
    mFile.write(mBuffer, mIntegratedBytes);
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
  Encoder::alloc(long size)
  {
    mSize = size;
    mBuffer = new char[mSize];
    mUnion = reinterpret_cast<Union_t *>(mBuffer);
    return false;
  }
  
  bool
  Encoder::encode(const tof::data::raw::Summary_t &summary)
  {

#ifdef VERBOSE
    if (mVerbose)
      std::cout << "-------- START ENCODE EVENT ----------------------------------------" << std::endl;
#endif
    auto start = std::chrono::high_resolution_clock::now();	
    
    unsigned int nWords = 0;

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
    mUnion++; nWords++;
    
    /** loop over TRMs **/

    unsigned char nPackedHits[256] = {0};
    PackedHit_t PackedHit[256][256];
    for (int itrm = 0; itrm < 10; itrm++) {

      /** check if TRM is empty **/
      if (summary.TRMempty[itrm])
        continue;

      unsigned char firstFilledFrame = 255;
      unsigned char lastFilledFrame = 0;

      /** loop over TRM chains **/
      for (int ichain = 0; ichain < 2; ++ichain) {
	/** loop over TDCs **/
	for (int itdc = 0; itdc < 15; ++itdc) {

          auto nhits = summary.nTDCUnpackedHits[itrm][ichain][itdc];
          if (nhits == 0)
            continue;

          /** loop over hits **/
          for (int ihit = 0; ihit < nhits; ++ihit) {

            auto lhit = summary.TDCUnpackedHit[itrm][ichain][itdc][ihit];
            if (lhit.PSBits != 0x1)
              continue; // must be a leading hit
            auto tot = 0;

            // check next hits for packing
            for (int jhit = ihit + 1; jhit < nhits; ++jhit) {
              auto thit = summary.TDCUnpackedHit[itrm][ichain][itdc][jhit];
              if (thit.PSBits == 0x2 &&
                  thit.Chan ==
                      lhit.Chan) { // must be a trailing hit from same channel
                tot = thit.HitTime - lhit.HitTime; // compute TOT
                lhit.PSBits = 0x0;                 // mark as used
                break;
              }
            }

            auto iframe = lhit.HitTime >> 13;
            auto phit = nPackedHits[iframe];
            PackedHit[iframe][phit].Chain = ichain;
            PackedHit[iframe][phit].TDCID = itdc;
            PackedHit[iframe][phit].Channel = lhit.Chan;
            PackedHit[iframe][phit].Time = lhit.HitTime;
            PackedHit[iframe][phit].TOT = tot;
            nPackedHits[iframe]++;

            if (iframe < firstFilledFrame)
              firstFilledFrame = iframe;
            if (iframe > lastFilledFrame)
              lastFilledFrame = iframe;
          }
        }
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
	mUnion++; nWords++;
	
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
	  mUnion++; nWords++;
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
    mUnion++; nWords++;

    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;

    mIntegratedBytes += nWords * 4;
    mIntegratedTime += elapsed.count();
    
#ifdef VERBOSE
    if (mVerbose)
      std::cout << "-------- END ENCODE EVENT ------------------------------------------"
		<< " " << nWords << " words"
		<< " | " << 1.e3  * elapsed.count() << " ms"
		<< " | " << 1.e-6 * mIntegratedBytes / mIntegratedTime << " MB/s (average)"
		<< std::endl;
#endif

    return false;

  }
}}}
