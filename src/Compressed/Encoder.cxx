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
    mPointer = (uint32_t *)mBuffer;
    mOutputByteCounter = 0;
    return false;
  }
  
  void
  Encoder::next32()
  {
    mPointer++;
    mByteCounter += 4;
#ifdef VERBOSE
    mUnion = reinterpret_cast<Union_t *>(mPointer);
#endif
  }

  bool
  Encoder::encode(const tof::data::raw::Summary_t &summary)
  {

#ifdef VERBOSE
    if (mVerbose)
      std::cout << "-------- START ENCODE EVENT ----------------------------------------" << std::endl;
#endif
    auto start = std::chrono::high_resolution_clock::now();	

    mByteCounter = 0;
    mUnion = reinterpret_cast<Union_t *>(mPointer);

    // crate header
    *mPointer  = 0x80000000;
    *mPointer |= summary.DRMGlobalHeader.DRMID << 24;
    *mPointer |= summary.DRMGlobalTrailer.LocalEventCounter << 12;
    *mPointer |= summary.DRMStatusHeader3.L0BCID;
#ifdef VERBOSE
    if (mVerbose) {
      auto BunchID = mUnion->CrateHeader.BunchID;
      auto EventCounter = mUnion->CrateHeader.EventCounter;
      auto DRMID = mUnion->CrateHeader.DRMID;
      std::cout << boost::format("%08x") % mUnion->Data
      		<< " "
		<< boost::format("Crate header          (DRMID=%d, EventCounter=%d, BunchID=%d)") % DRMID % EventCounter % BunchID
		<< std::endl;
    }
#endif
    next32();
    
    // crate orbit
    *mPointer = summary.DRMOrbitHeader.Orbit;
#ifdef VERBOSE
    if (mVerbose) {
      auto OrbitID = mUnion->CrateOrbit.OrbitID;
      std::cout << boost::format("%08x") % mUnion->Data
      		<< " "
		<< boost::format("Crate orbit           (OrbitID=%d)") % OrbitID
		<< std::endl;
    }
#endif
    next32();
    
    /** loop over TRMs **/

    unsigned char nPackedHits[256] = {0};
    PackedHit_t PackedHit[256][256];
    for (int itrm = 0; itrm < 10; itrm++) {

      /** check if TRM is empty **/
      if (summary.TRMempty[itrm]) continue;

      unsigned char firstFilledFrame = 255;
      unsigned char lastFilledFrame = 0;

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
            if (lhit.PSBits != 0x1)
              continue; // must be a leading hit
            auto tot = 0;
	    
            // check next hits for packing
            for (int jhit = ihit + 1; jhit < nhits; ++jhit) {
              auto thit = summary.TDCUnpackedHit[itrm][ichain][itdc][jhit];
              if (thit.PSBits == 0x2 && thit.Chan == lhit.Chan) { // must be a trailing hit from same channel
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
	*mPointer  = 0x00000000;
	*mPointer |= (itrm + 3) << 24;
	*mPointer |= iframe << 16;
        *mPointer |= nPackedHits[iframe];
#ifdef VERBOSE
	if (mVerbose) {
	  auto NumberOfHits = mUnion->FrameHeader.NumberOfHits;
	  auto FrameID = mUnion->FrameHeader.FrameID;
	  auto TRMID = mUnion->FrameHeader.TRMID;
	  std::cout << boost::format("%08x") % mUnion->Data
		    << " "
		    << boost::format("Frame header          (TRMID=%d, FrameID=%d, NumberOfHits=%d)") % TRMID % FrameID % NumberOfHits
		    << std::endl;
	}
#endif
	next32();
	
	// packed hits
        for (int ihit = 0; ihit < nPackedHits[iframe]; ++ihit) {
	  *mPointer  = 0x00000000;
	  *mPointer |= PackedHit[iframe][ihit].TOT;
	  *mPointer |= PackedHit[iframe][ihit].Time << 11;
	  *mPointer |= PackedHit[iframe][ihit].Channel << 24;
	  *mPointer |= PackedHit[iframe][ihit].TDCID << 27;
	  *mPointer |= PackedHit[iframe][ihit].Chain << 31;
#ifdef VERBOSE
	  if (mVerbose) {
            auto Chain = mUnion->PackedHit.Chain;
            auto TDCID = mUnion->PackedHit.TDCID;
            auto Channel = mUnion->PackedHit.Channel;
            auto Time = mUnion->PackedHit.Time;
            auto TOT = mUnion->PackedHit.TOT;
            std::cout << boost::format("%08x") % mUnion->Data << " "
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
    *mPointer  = 0x80000000;
    *mPointer |= summary.DRMFaultFlag ? 1 : 0;
    *mPointer |= (summary.TRMFaultFlag[0] << 2 | summary.TRMChainFaultFlag[0][0] << 1 | summary.TRMChainFaultFlag[0][1]) << 1;
    *mPointer |= (summary.TRMFaultFlag[1] << 2 | summary.TRMChainFaultFlag[1][0] << 1 | summary.TRMChainFaultFlag[1][1]) << 4;
    *mPointer |= (summary.TRMFaultFlag[2] << 2 | summary.TRMChainFaultFlag[2][0] << 1 | summary.TRMChainFaultFlag[2][1]) << 7;
    *mPointer |= (summary.TRMFaultFlag[3] << 2 | summary.TRMChainFaultFlag[3][0] << 1 | summary.TRMChainFaultFlag[3][1]) << 10;
    *mPointer |= (summary.TRMFaultFlag[4] << 2 | summary.TRMChainFaultFlag[4][0] << 1 | summary.TRMChainFaultFlag[4][1]) << 13;
    *mPointer |= (summary.TRMFaultFlag[5] << 2 | summary.TRMChainFaultFlag[5][0] << 1 | summary.TRMChainFaultFlag[5][1]) << 16;
    *mPointer |= (summary.TRMFaultFlag[6] << 2 | summary.TRMChainFaultFlag[6][0] << 1 | summary.TRMChainFaultFlag[6][1]) << 19;
    *mPointer |= (summary.TRMFaultFlag[7] << 2 | summary.TRMChainFaultFlag[7][0] << 1 | summary.TRMChainFaultFlag[7][1]) << 22;
    *mPointer |= (summary.TRMFaultFlag[8] << 2 | summary.TRMChainFaultFlag[8][0] << 1 | summary.TRMChainFaultFlag[8][1]) << 25;
    *mPointer |= (summary.TRMFaultFlag[9] << 2 | summary.TRMChainFaultFlag[9][0] << 1 | summary.TRMChainFaultFlag[9][1]) << 28;
#ifdef VERBOSE
    if (mVerbose) {
      auto CrateFault = mUnion->CrateTrailer.CrateFault;
      auto TRMFault03 = mUnion->CrateTrailer.TRMFault03;
      auto TRMFault04 = mUnion->CrateTrailer.TRMFault04;
      auto TRMFault05 = mUnion->CrateTrailer.TRMFault05;
      auto TRMFault06 = mUnion->CrateTrailer.TRMFault06;
      auto TRMFault07 = mUnion->CrateTrailer.TRMFault07;
      auto TRMFault08 = mUnion->CrateTrailer.TRMFault08;
      auto TRMFault09 = mUnion->CrateTrailer.TRMFault09;
      auto TRMFault10 = mUnion->CrateTrailer.TRMFault10;
      auto TRMFault11 = mUnion->CrateTrailer.TRMFault11;
      auto TRMFault12 = mUnion->CrateTrailer.TRMFault12;
      std::cout << boost::format("%08x") % mUnion->Data
		<< " "
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
