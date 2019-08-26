#ifndef _TOF_DATA_RAW_H_
#define _TOF_DATA_RAW_H_

#include <stdint.h>
#include <vector>

//#define GET_BITMASK_AT_POS(x, m, p)

#define IS_DRM_COMMON_HEADER(x)        ( (x & 0xF0000000) == 0x40000000 )
#define IS_DRM_GLOBAL_HEADER(x)        ( (x & 0xF000000F) == 0x40000001 )
#define IS_LTM_GLOBAL_HEADER(x)        ( (x & 0xF000000F) == 0x40000002 )
#define IS_TRM_GLOBAL_HEADER(x)        ( (x & 0xF0000000) == 0x40000000 )
#define IS_TRM_CHAINA_HEADER(x)        ( (x & 0xF0000000) == 0x00000000 )
#define IS_TRM_CHAINB_HEADER(x)        ( (x & 0xF0000000) == 0x20000000 )

#define IS_DRM_GLOBAL_TRAILER(x)       ( (x & 0xF000000F) == 0x50000001 )
#define IS_LTM_GLOBAL_TRAILER(x)       ( (x & 0xF000000F) == 0x50000002 )
#define IS_TRM_GLOBAL_TRAILER(x)       ( (x & 0xF0000003) == 0x50000003 )
#define IS_TRM_CHAINA_TRAILER(x)       ( (x & 0xF0000000) == 0x10000000 )
#define IS_TRM_CHAINB_TRAILER(x)       ( (x & 0xF0000000) == 0x30000000 )

#define IS_TDC_ERROR(x)                ( (x & 0xF0000000) == 0x60000000 )
#define IS_FILLER(x)                   ( (x & 0xFFFFFFFF) == 0x70000000 )
#define IS_TDC_HIT(x)                  ( (x & 0x80000000) == 0x80000000 )


#define GET_DRM_DRMID(x)               ( (x & 0x0FE00000) >> 21 )
#define GET_DRM_PARTICIPATINGSLOTID(x) ( (x & 0x00007FF0) >>  4 )
#define GET_DRM_SLOTENABLEMASK(x)      ( (x & 0x00007FF0) >>  4 )
#define GET_DRM_L0BCID(x)              ( (x & 0x0000FFF0) >>  4 )
#define GET_DRM_LOCALEVENTCOUNTER(x)   ( (x & 0x0000FFF0) >>  4 )

#define GET_TRM_SLOTID(x)              ( (x & 0x0000000F) )
#define GET_TRM_EVENTNUMBER(x)         ( (x & 0x07FE0000) >> 17 )
#define GET_TRM_EVENTWORDS(x)          ( (x & 0x0001FFF0) >>  4 )

#define GET_TRMCHAIN_BUNCHID(x)        ( (x & 0x0000FFF0) >>  4 )
#define GET_TRMCHAIN_EVENTCOUNTER(x)   ( (x & 0x0FFF0000) >> 16 )
#define GET_TRMCHAIN_STATUS(x)         ( (x & 0x0000000F) )

#define GET_TDCHIT_HITTIME(x)          ( (x & 0x001FFFFF) )
#define GET_TDCHIT_CHAN(x)             ( (x & 0x00E00000) >>  3 )
#define GET_TDCHIT_TDCID(x)            ( (x & 0x0F000000) >> 24 )
#define GET_TDCHIT_EBIT(x)             ( (x & 0x10000000) >> 29 )
#define GET_TDCHIT_PSBITS(x)           ( (x & 0x60000000) >> 29 )

namespace tof {
namespace data {
namespace raw {

  /** flags **/
  
  enum EFlag_t {
    Flag_Header = 0x1,
    Flag_Trailer = 0x2,
    Flag_EventCounter = 0x4,
  };
  
  /** generic word **/
  
  struct Word_t
  {
    uint32_t SlotID     :  4;
    uint32_t UNDEFINED  : 24;
    uint32_t WordType   :  4;
  };

  /** RDH **/

  struct RDHWord_t {
    uint32_t Data[4];
  };
  
  struct RDHWord0_t {
    uint32_t HeaderVersion   :  8;
    uint32_t HeaderSize      :  8;
    uint32_t BlockLength     : 16;
    uint32_t FeeID           : 16;
    uint32_t PriorityBit     :  8;
    uint32_t RESERVED        :  8;
    uint32_t OffsetNewPacket : 16;
    uint32_t MemorySize      : 16;
    uint32_t UnkID           :  8;
    uint32_t PacketCounter   :  8;
    uint32_t CruID           : 12;
    uint32_t DataPathWrapper :  4;
  };

  struct RDHWord1_t {
    uint32_t TrgOrbit        : 32;
    uint32_t HbOrbit         : 32;
    uint32_t RESERVED1       : 32;
    uint32_t RESERVED2       : 32;
  };
  
  struct RDHWord2_t {
    uint32_t TrgBC        : 12;
    uint32_t RESERVED1    :  4;
    uint32_t HbBC         : 12;
    uint32_t RESERVED2    :  4;
    uint32_t TrgType      : 32;
    uint32_t RESERVED3    : 32;
    uint32_t RESERVED4    : 32;
  };

  struct RDHWord3_t {
    uint32_t DetectorField : 16;
    uint32_t Par           : 16;
    uint32_t StopBit       :  8;
    uint32_t PagesCounter  : 16;
    uint32_t RESERVED1     : 32;
    uint32_t RESERVED2     : 32;
    uint32_t RESERVED3     :  8;
  };

  union RDH_t
  {
    uint32_t    Data[4];
    RDHWord0_t  Word0;
    RDHWord1_t  Word1;
    RDHWord2_t  Word2;
    RDHWord3_t  Word3;
  };

  /** DRM data **/
  
  struct DRMCommonHeader_t {
    uint32_t Payload   : 28;
    uint32_t WordType  :  4;
  };
  
  struct DRMOrbitHeader_t {
    uint32_t Orbit     : 32;
  };

  struct DRMGlobalHeader_t
  {
    uint32_t SlotID     :  4;
    uint32_t EventWords : 17;
    uint32_t DRMID      :  7;
    uint32_t WordType   :  4;
  };
  
  struct DRMStatusHeader1_t
  {
    uint32_t SlotID              :  4;
    uint32_t ParticipatingSlotID : 11;
    uint32_t CBit                :  1;
    uint32_t VersID              :  5;
    uint32_t DRMhSize            :  4;
    uint32_t UNDEFINED           :  3;
    uint32_t WordType            :  4;
  };
  
  struct DRMStatusHeader2_t
  {
    uint32_t SlotID         :  4;
    uint32_t SlotEnableMask : 11;
    uint32_t MBZ            :  1;
    uint32_t FaultID        : 11;
    uint32_t RTOBit         :  1;
    uint32_t WordType       :  4;
  };
  
  struct DRMStatusHeader3_t
  {
    uint32_t SlotID      : 4;
    uint32_t L0BCID      : 12;
    uint32_t RunTimeInfo : 12; // check
    uint32_t WordType    : 4;
  };
  
  struct DRMStatusHeader4_t
  {
    uint32_t SlotID      :  4;
    uint32_t Temperature : 10;
    uint32_t MBZ1        :  1;
    uint32_t ACKBit      :  1;
    uint32_t SensAD      :  3;
    uint32_t MBZ2        :  1;
    uint32_t UNDEFINED   :  8;
    uint32_t WordType    :  4;
  };
  
  struct DRMStatusHeader5_t
  {
    uint32_t UNKNOWN    : 32;
  };
  
  struct DRMGlobalTrailer_t
  {
    uint32_t SlotID            :  4;
    uint32_t LocalEventCounter : 12;
    uint32_t UNDEFINED         : 12;
    uint32_t WordType          :  4;
  };
  
  /** TRM data **/
  
  struct TRMGlobalHeader_t
  {
    uint32_t SlotID      :  4;
    uint32_t EventWords  : 13;
    uint32_t EventNumber : 10;
    uint32_t EBit        :  1;
    uint32_t WordType    :  4;
  };
  
  struct TRMGlobalTrailer_t
  {
    uint32_t MustBeThree  :  2;
    uint32_t EventCRC     : 12;
    uint32_t Temp         :  8;
    uint32_t SendAd       :  3;
    uint32_t Chain        :  1;
    uint32_t TSBit        :  1;
    uint32_t LBit         :  1;
    uint32_t WordType     :  4;
  };
  
  /** TRM-chain data **/

  struct TRMChainHeader_t
  {
    uint32_t SlotID   :  4;
    uint32_t BunchID  : 12;
    uint32_t PB24Temp :  8;
    uint32_t PB24ID   :  3;
    uint32_t TSBit    :  1;
    uint32_t WordType :  4;
  };
  
  struct TRMChainTrailer_t
  {
    uint32_t Status       :  4;
    uint32_t MBZ          : 12;
    uint32_t EventCounter : 12;
    uint32_t WordType     :  4;
  };
  
  /** TDC hit **/
  
  struct TDCPackedHit_t
  {
    uint32_t HitTime  : 13;
    uint32_t TOTWidth :  8;
    uint32_t Chan     :  3;
    uint32_t TDCID    :  4;
    uint32_t EBit     :  1;
    uint32_t PSBits   :  2;
    uint32_t MBO      :  1;
  };
  
  struct TDCUnpackedHit_t
  {
    uint32_t HitTime : 21; // leading or trailing edge measurement
    uint32_t Chan    :  3; // TDC channel number
    uint32_t TDCID   :  4; // TDC ID
    uint32_t EBit    :  1; // E bit
    uint32_t PSBits  :  2; // PS bits
    uint32_t MBO     :  1; // must-be-one bit
  };

  /** summary data **/

  struct Summary_t
  {
    RDHWord0_t         RDHWord0;
    RDHWord1_t         RDHWord1;
    RDHWord2_t         RDHWord2;
    RDHWord3_t         RDHWord3;
    uint32_t DRMCommonHeader;
    uint32_t DRMOrbitHeader;
    uint32_t DRMGlobalHeader;
    uint32_t DRMStatusHeader1;
    uint32_t DRMStatusHeader2;
    uint32_t DRMStatusHeader3;
    uint32_t DRMStatusHeader4;
    uint32_t DRMStatusHeader5;
    uint32_t DRMGlobalTrailer;
    uint32_t TRMGlobalHeader[10];
    uint32_t TRMGlobalTrailer[10];
    uint32_t TRMChainHeader[10][2];
    uint32_t TRMChainTrailer[10][2];
    uint32_t TDCUnpackedHit[10][2][15][256];
    uint8_t  nTDCUnpackedHits[10][2][15];
    // derived data
    bool TRMempty[10];
    // status
    bool decodeError;
    uint32_t faultFlags;
  };

      
  /** counter data **/
  
  struct TRMChainCounterData_t
  { 
    uint32_t ExpectedData;
    uint32_t DetectedData;
    uint32_t EventCounterMismatch;
    uint32_t BadStatus;
  };
  
  struct TRMCounterData_t
  {
    uint32_t ExpectedData;
    uint32_t DetectedData;
    uint32_t EventWordsMismatch;
    uint32_t EventCounterMismatch;
  };
  
  struct DRMCounterData_t
  {
    uint32_t ExpectedData;
    uint32_t DetectedData;
    uint32_t EventWordsMismatch;
  };
  
}}}

#endif /** _TOF_DATA_RAW_H_ **/
