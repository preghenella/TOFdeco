#ifndef _TOF_DATA_COMPRESSED_H_
#define _TOF_DATA_COMPRESSED_H_

#include <stdint.h>

namespace tof {
namespace data {
namespace compressed {

  /** generic word **/
  
  struct Word_t
  {
    uint32_t UNDEFINED : 31;
    uint32_t WordType  : 1;
  };

  /** data format **/
  
  struct CrateHeader_t
  {
    uint32_t BunchID      : 12;
    uint32_t EventCounter : 12;
    uint32_t DRMID        :  7;
    uint32_t MustBeOne    :  1;
  };

  struct FrameHeader_t
  {
    uint32_t NumberOfHits : 16;
    uint32_t FrameID      :  8;
    uint32_t TRMID        :  4;
    uint32_t DeltaBC      :  3;
    uint32_t MustBeZero   :  1;
  };
  
  struct PackedHit_t
  {
    uint32_t TOT          : 11;
    uint32_t Time         : 13;
    uint32_t Channel      :  3;
    uint32_t TDCID        :  4;
    uint32_t Chain        :  1;
  };

  struct CrateTrailer_t
  {
    uint32_t TRMFault03   :  3;
    uint32_t TRMFault04   :  3;
    uint32_t TRMFault05   :  3;
    uint32_t TRMFault06   :  3;
    uint32_t TRMFault07   :  3;
    uint32_t TRMFault08   :  3;
    uint32_t TRMFault09   :  3;
    uint32_t TRMFault10   :  3;
    uint32_t TRMFault11   :  3;
    uint32_t TRMFault12   :  3;
    uint32_t CrateFault   :  1;
    uint32_t MustBeOne    :  1;
  };

  /** union **/

  union Union_t
  {
    uint32_t       Data;
    Word_t         Word;
    CrateHeader_t  CrateHeader;
    FrameHeader_t  FrameHeader;
    PackedHit_t    PackedHit;
    CrateTrailer_t CrateTrailer;
  };
  
  
}}}

#endif /** _TOF_DATA_COMPRESSED_H_ **/
