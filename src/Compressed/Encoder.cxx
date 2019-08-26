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
    if (mVerbose) {
      std::cout << "-------- FLUSH ENCODER BUFFER --------------------------------------"
		<< " | " << mByteCounter << " bytes"
		<< std::endl;
    }
#endif
    mFile.write(mBuffer, mOutputByteCounter);
    mPointer = (uint32_t *)mBuffer;
    mOutputByteCounter = 0;
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
    if (mVerbose) {
      std::cout << "-------- INITIALISE ENCODER BUFFER ---------------------------------"
		<< " | " << mSize << " bytes"
		<< std::endl;
    }
#endif
    if (mBuffer) {
      std::cout << "Warning: a buffer was already allocated, cleaning" << std::endl;
      delete [] mBuffer;
    }
    mBuffer = new char[mSize];
    mPointer = (uint32_t *)mBuffer;
    mOutputByteCounter = 0;
    return false;
  }
  
  void
  Encoder::next32()
  {
    mPointer++;
    mByteCounter += 4;
  }

  bool
  Encoder::encode(const tof::data::raw::Summary_t &summary)
  {

#ifdef VERBOSE
    if (mVerbose) {
      std::cout << "-------- START ENCODE EVENT ----------------------------------------" << std::endl;
    }
#endif
    auto start = std::chrono::high_resolution_clock::now();	

    mByteCounter = 0;

    // crate header
    *mPointer  = 0x80000000;
    *mPointer |= GET_DRM_DRMID(summary.DRMGlobalHeader) << 24;
    *mPointer |= GET_DRM_LOCALEVENTCOUNTER(summary.DRMGlobalTrailer) << 12;
    *mPointer |= GET_DRM_L0BCID(summary.DRMStatusHeader3);
#ifdef VERBOSE
    if (mVerbose) {
      auto CrateHeader = reinterpret_cast<CrateHeader_t *>(mPointer);
      auto BunchID = CrateHeader->BunchID;
      auto EventCounter = CrateHeader->EventCounter;
      auto DRMID = CrateHeader->DRMID;
      std::cout << boost::format("%08x") % *mPointer << " "
		<< boost::format("Crate header          (DRMID=%d, EventCounter=%d, BunchID=%d)") % DRMID % EventCounter % BunchID
		<< std::endl;
    }
#endif
    next32();
    
    // crate orbit
    *mPointer = summary.DRMOrbitHeader;
#ifdef VERBOSE
    if (mVerbose) {
      auto CrateOrbit = reinterpret_cast<CrateOrbit_t *>(mPointer);
      auto OrbitID = CrateOrbit->OrbitID;
      std::cout << boost::format("%08x") % *mPointer << " "
		<< boost::format("Crate orbit           (OrbitID=%d)") % OrbitID
		<< std::endl;
    }
#endif
    next32();
    
    /** loop over TRMs **/

    uint8_t  nPackedHits[256] = {0};
    uint32_t PackedHit[256][256];
    for (int itrm = 0; itrm < 10; itrm++) {

      /** check if TRM is empty **/
      if (summary.TRMempty[itrm]) continue;

      uint8_t firstFilledFrame = 255;
      uint8_t lastFilledFrame = 0;

      /** SPIDER **/
      
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
            if (GET_TDCHIT_PSBITS(lhit) != 0x1)
              continue; // must be a leading hit

	    auto Chan = GET_TDCHIT_CHAN(lhit);
	    auto HitTime = GET_TDCHIT_HITTIME(lhit);
	    auto EBit = GET_TDCHIT_EBIT(lhit);
            uint32_t TOTWidth = 0;
	    
            // check next hits for packing
            for (int jhit = ihit + 1; jhit < nhits; ++jhit) {
              auto thit = summary.TDCUnpackedHit[itrm][ichain][itdc][jhit];
              if (GET_TDCHIT_PSBITS(thit) == 0x2 && GET_TDCHIT_CHAN(thit) == Chan) { // must be a trailing hit from same channel
                TOTWidth = GET_TDCHIT_HITTIME(thit) - HitTime; // compute TOT
                lhit = 0x0; // mark as used
                break;
              }
            }

	    auto iframe = HitTime >> 13;
	    auto phit = nPackedHits[iframe];

	    PackedHit[iframe][phit]  = 0x00000000;
	    PackedHit[iframe][phit] |= TOTWidth;
	    PackedHit[iframe][phit] |= HitTime << 11;
	    PackedHit[iframe][phit] |= Chan << 24;
	    PackedHit[iframe][phit] |= itdc << 27;
	    PackedHit[iframe][phit] |= ichain << 31;
	    nPackedHits[iframe]++;
	    
	    if (iframe < firstFilledFrame)
	      firstFilledFrame = iframe;
	    if (iframe > lastFilledFrame)
	      lastFilledFrame = iframe;
	    
	  }
	}
      }
            
      /** loop over frames **/
      for (int iframe = firstFilledFrame; iframe < lastFilledFrame + 1; iframe++) {

        /** check if frame is empty **/
        if (nPackedHits[iframe] == 0)
          continue;

        // frame header
	*mPointer  = 0x00000000;
	*mPointer |= (itrm + 3) << 24;
	*mPointer |= iframe << 16;
        *mPointer |= nPackedHits[iframe];
#ifdef VERBOSE
	if (mVerbose) {
	  auto FrameHeader = reinterpret_cast<FrameHeader_t *>(mPointer);
	  auto NumberOfHits = FrameHeader->NumberOfHits;
	  auto FrameID = FrameHeader->FrameID;
	  auto TRMID = FrameHeader->TRMID;
	  std::cout << boost::format("%08x") % *mPointer << " "
		    << boost::format("Frame header          (TRMID=%d, FrameID=%d, NumberOfHits=%d)") % TRMID % FrameID % NumberOfHits
		    << std::endl;
	}
#endif
	next32();
	
	// packed hits
        for (int ihit = 0; ihit < nPackedHits[iframe]; ++ihit) {
	  *mPointer = PackedHit[iframe][ihit];
#ifdef VERBOSE
	  if (mVerbose) {
	    auto PackedHit = reinterpret_cast<PackedHit_t *>(mPointer);
            auto Chain = PackedHit->Chain;
            auto TDCID = PackedHit->TDCID;
            auto Channel = PackedHit->Channel;
            auto Time = PackedHit->Time;
            auto TOT = PackedHit->TOT;
            std::cout << boost::format("%08x") % *mPointer << " "
                      << boost::format("Packed hit            (Chain=%d, TDCID=%d, Channel=%d, Time=%d, TOT=%d)") % Chain % TDCID % Channel % Time % TOT
                      << std::endl;
          }
#endif
	  next32();
        }

        nPackedHits[iframe] = 0;
      }
    }

    // crate trailer
    *mPointer = 0x80000000 | summary.faultFlags;
#ifdef VERBOSE
    if (mVerbose) {
      auto CrateTrailer = reinterpret_cast<CrateTrailer_t *>(mPointer);
      auto CrateFault = CrateTrailer->CrateFault;
      auto TRMFault03 = CrateTrailer->TRMFault03;
      auto TRMFault04 = CrateTrailer->TRMFault04;
      auto TRMFault05 = CrateTrailer->TRMFault05;
      auto TRMFault06 = CrateTrailer->TRMFault06;
      auto TRMFault07 = CrateTrailer->TRMFault07;
      auto TRMFault08 = CrateTrailer->TRMFault08;
      auto TRMFault09 = CrateTrailer->TRMFault09;
      auto TRMFault10 = CrateTrailer->TRMFault10;
      auto TRMFault11 = CrateTrailer->TRMFault11;
      auto TRMFault12 = CrateTrailer->TRMFault12;
      std::cout << boost::format("%08x") % *mPointer << " "
		<< boost::format("Crate trailer         (CrateFault=%d TRMFault[3-12]=0x[%x,%x,%x,%x,%x,%x,%x,%x,%x,%x])") % CrateFault % TRMFault03 % TRMFault04 % TRMFault05 % TRMFault06 % TRMFault07 % TRMFault08 % TRMFault09 % TRMFault10 % TRMFault11 % TRMFault12
		<< std::endl;
    }
#endif
    next32();

    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;

    mOutputByteCounter += mByteCounter;
    mIntegratedBytes += mByteCounter;
    mIntegratedTime += elapsed.count();
    
#ifdef VERBOSE
    if (mVerbose) {
      std::cout << "-------- END ENCODE EVENT ------------------------------------------"
		<< " | " << mByteCounter << " bytes"
		<< " | " << 1.e3  * elapsed.count() << " ms"
		<< " | " << 1.e-6 * mIntegratedBytes / mIntegratedTime << " MB/s (average)"
		<< std::endl;
    }
#endif

    return false;

  }
}}}
