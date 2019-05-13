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

    unsigned int nPackedHits[10][256];
    PackedHit_t PackedHit[10][256][64];
    for (int itrm = 0; itrm < 10; itrm++) {

      /** reset TRM frames **/
      for (int iframe = 0; iframe < 256; ++iframe)
	nPackedHits[itrm][iframe] = 0;

      /** loop over TRM chains **/
      for (int ichain = 0; ichain < 2; ++ichain) {
	/** loop over TDCs **/
	for (int itdc = 0; itdc < 15; ++itdc) {
	  /** loop over hits **/
	  for (int iihit = 0; iihit < summary.nTDCUnpackedHits[itrm][ichain][itdc]; ++iihit) {
	  auto hit    = summary.TDCUnpackedHit[itrm][ichain][itdc][iihit];
	  if (hit.PSBits != 0x1) continue; // TEMP

	  auto iframe = hit.HitTime >> 13;
	  auto ihit   = nPackedHits[itrm][iframe];
	  PackedHit[itrm][iframe][ihit].Chain   = ichain;
	  PackedHit[itrm][iframe][ihit].TDCID   = itdc;
	  PackedHit[itrm][iframe][ihit].Channel = hit.Chan;
	  PackedHit[itrm][iframe][ihit].Time    = hit.HitTime;
	  PackedHit[itrm][iframe][ihit].TOT     = 0x7ff;
	  nPackedHits[itrm][iframe]++;

	  }}}

      /** loop over frames **/
      for (int iframe = 0; iframe < 256; iframe++) {

	if (nPackedHits[itrm][iframe] == 0) continue;

	// frame header
	mUnion->FrameHeader = {0x0};
	mUnion->FrameHeader.MustBeZero = 0;
	mUnion->FrameHeader.TRMID = itrm + 3;
	mUnion->FrameHeader.FrameID = iframe;
	mUnion->FrameHeader.NumberOfHits = nPackedHits[itrm][iframe];
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
	for (int ihit = 0; ihit < nPackedHits[itrm][iframe]; ++ihit) {
	  mUnion->PackedHit = PackedHit[itrm][iframe][ihit];
#ifdef VERBOSE
	  if (mVerbose) {
	    std::cout << boost::format("%08x") % mUnion->Data
		      << " "
		      << "Packed hit"
		      << std::endl;
	  }
#endif
	  mUnion++; nWords++;
	}	
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
