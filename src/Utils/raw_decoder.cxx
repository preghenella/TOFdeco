#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <cstdint>
#include "Raw/Decoder.h"

#define DECODE_VERBOSE 1
#define CHECK_VERBOSE  1

#define CHECK_DATA         1
#define CHECK_EVENTWORDS   1
#define CHECK_EVENTCOUNTER 1
#define CHECK_BUNCHCOUNTER 0

#define ERROR_RECOVER_DRM

using namespace tof::data::raw;

bool denxt(std::ifstream &is);
void declr();
bool dechk();
//bool dedmp();

bool detof(std::ifstream &is);
bool dedrm(std::ifstream &is);
bool detrm(std::ifstream &is);
bool decnx(std::ifstream &is, int htype);
//bool dehit(std::ifstream &is);
bool deerr();

struct data_t {
  uint32_t word[3];
} data;
int wordn = 2;

uint32_t current = 0;

int main(int argc, char **argv)
{

  bool verbose = false;
  std::string inFileName;
  
  /** define arguments **/
  namespace po = boost::program_options;
  po::options_description desc("Options");
  desc.add_options()
    ("help", "Print help messages")
    ("verbose,v", po::bool_switch(&verbose), "Decode verbose")
    ("input,i", po::value<std::string>(&inFileName), "Input data file")
    ("word,w",  po::value<int>(&wordn)->default_value(2), "Word where to find the data")
    ;
  /** positional arguments **/
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  
  /** process arguments **/
  try {
    /** help **/
    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 1;
    }
    po::notify(vm);
  }
  catch(std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    std::cout << desc << std::endl;
    return 1;
  }
  
  std::ifstream is;
  is.open(inFileName.c_str(), std::fstream::binary);
  if (!is.is_open()) {
    std::cerr << "cannot open " << inFileName << std::endl;
    return 1;
  }

  Decoder decoder;
  decoder.setVerbose(verbose);
  decoder.load(inFileName);
  while (!decoder.next()) decoder.decode();
  decoder.close();

  std::cout << " benchmark: decoded " << 1.e-6 * decoder.mIntegratedBytes << " MB in " << decoder.mIntegratedTime << " s"
	    << " | " << 1.e-6 * decoder.mIntegratedBytes / decoder.mIntegratedTime << " MB/s"
	    << std::endl;


}

#if 0

bool
denxt(std::ifstream &is)
{
  bool eof = is.eof();
  if (!eof) {
    is.read(reinterpret_cast<char *>(&data), sizeof(data));
    Data = &data.word[wordn];
    current++;
  }
  return eof;
}

void
declr()
{
  DRMSummaryData.Status = 0x0;
  DRMSummaryData.Status = 0x0;
  for (int itrm = 0; itrm < 10; itrm++) {
    TRMSummaryData[itrm].Status = 0x0;
    TRMSummaryData[itrm].Status = 0x0;
    for (int icnx = 0; icnx < 2; icnx++) {
      TRMChainSummaryData[itrm][icnx].Status = 0x0;
      TRMChainSummaryData[itrm][icnx].Status = 0x0;
    }
  }
}

bool
dechk()
{

  char name[2] = {'a', 'b'};
  
  auto enabled = DRMSummaryData.DRMStatusHeader2.SlotEnableMask;
  auto participating = DRMSummaryData.DRMStatusHeader1.ParticipatingSlotID;

  if (participating != enabled) {
#if CHECK_VERBOSE
    printf("drmxx: participating/enable masks differ: %02x | %02x \n", enabled, participating);
#endif
}

#if CHECK_EVENTWORDS
  /** check DRM event words **/
  int nDetectedWords = DRMSummaryData.LastWord - DRMSummaryData.FirstWord + 1;
  int nExpectedWords = DRMSummaryData.DRMGlobalHeader.EventWords;
  if (nDetectedWords != nExpectedWords) {
    printf("drmxx: event words mismatch (detected=%d, expected=%d) \n", nDetectedWords, nExpectedWords);
  }
#endif
    
  /** loop over TRM summary data **/
  for (int itrm = 0; itrm < 10; itrm++) {

    /** check participating TRM **/
    if (!(participating & 1 << (itrm + 1))) {
      if (TRMSummaryData[itrm].Status & 0x1) {
#if CHECK_VERBOSE
	printf("trm%02d: non-participating header found \n", itrm + 3);	
#endif
      }
      continue;
    }
    
    /** check TRM header **/
    if (!(TRMSummaryData[itrm].Status & 0x1)) {
#if CHECK_VERBOSE
      printf("trm%02d: missing header \n", itrm + 3);
#endif
      continue;
    }
    
    /** check TRM trailer **/
    if (!(TRMSummaryData[itrm].Status & 0x2)) {
#if CHECK_VERBOSE
      printf("trm%02d: missing trailer \n", itrm + 3);
#endif
      continue;
    }

#if CHECK_EVENTWORDS
    /** check TRM event words **/
    int nDetectedWords = TRMSummaryData[itrm].LastWord - TRMSummaryData[itrm].FirstWord + 1;
    int nExpectedWords = TRMSummaryData[itrm].TRMGlobalHeader.EventWords;
    if (nDetectedWords != nExpectedWords) {
      printf("trm%02d: event words mismatch (detected=%d, expected=%d) \n", itrm + 3, nDetectedWords, nExpectedWords);
    }
#endif
    
#if CHECK_EVENTCOUNTER
    /** check TRM event counter **/
    if (TRMSummaryData[itrm].TRMGlobalTrailer.EventCounter !=
	DRMSummaryData.DRMGlobalTrailer.LocalEventCounter) {
#if CHECK_VERBOSE
      printf("trm%02d: event counter mismatch (TRM=%d, DRM=%d) \n", itrm + 3, TRMSummaryData[itrm].TRMGlobalTrailer.EventCounter, DRMSummaryData.DRMGlobalTrailer.LocalEventCounter);
#endif
    }
#endif
    
    /** loop over TRM chain summary data **/
    for (int ichain = 0; ichain < 2; ichain++) {

      /** check TRM chain header **/
      if (!(TRMChainSummaryData[itrm][ichain].Status & 0x1)) {
#if CHECK_VERBOSE
	printf("cn%c%02d: missing header \n", name[ichain], itrm + 3);
#endif
	continue;
      }
      
      /** check TRM chain trailer **/
      if (!(TRMChainSummaryData[itrm][ichain].Status & 0x2)) {
#if CHECK_VERBOSE
	printf("cn%c%02d: missing header \n", name[ichain], itrm + 3);
#endif
	continue;
      }
      
#if CHECK_EVENTCOUNTER
      /** check TRM chain event counter **/
      if (TRMChainSummaryData[itrm][ichain].TRMChainTrailer.EventCounter !=
	  DRMSummaryData.DRMGlobalTrailer.LocalEventCounter) {
#if CHECK_VERBOSE
	printf("cn%c%02d: event counter mismatch (chain=%d, DRM=%d) \n", name[ichain], itrm + 3, TRMChainSummaryData[itrm][ichain].TRMChainTrailer.EventCounter, DRMSummaryData.DRMGlobalTrailer.LocalEventCounter);
#endif
      }
#endif

#if CHECK_BUNCHCOUNTER
      /** check TRM chain bunch counter **/
      if (TRMChainSummaryData[itrm][ichain].TRMChainHeader.BunchID !=
	  DRMSummaryData.DRMStatusHeader3.L0BCID) {
#if CHECK_VERBOSE
	printf("cn%c%02d: bunch counter mismatch (chain=%d, DRM=%d) \n", name[ichain], itrm + 3, TRMChainSummaryData[itrm][ichain].TRMChainHeader.BunchID, DRMSummaryData.DRMStatusHeader3.L0BCID);
#endif
      }
#endif
      
    }
  }
  return false;
}

bool
detof(std::ifstream &is)
{
  if (denxt(is)) return true;
  if (Word->WordType == 4 && Word->SlotID == 1) {
    declr();
#if DECODE_VERBOSE
    printf("--- DECODE EVENT ----------------------------------------\n");
#endif
    dedrm(is);
#if CHECK_VERBOSE
    printf("--- CHECK EVENT ----------------------------------------\n");
#endif
    //    dedmp();
    dechk();
  }
  else {
    //    printf("tof: %08x -> Unknown \n", *Data);
    return false;
  }
  return true;
}

bool
dedrm(std::ifstream &is)
{
  /** DRM global header **/
  DRMSummaryData.DRMGlobalHeader = *DRMGlobalHeader;
  DRMSummaryData.Status |= 0x1;
  DRMSummaryData.FirstWord = current - 1;
  uint32_t drmID = DRMGlobalHeader->DRMID;
#if DECODE_VERBOSE
  printf("drm%02d: \t %08x \t -> DRM Global Header \n", drmID, *Data);
#endif
  /** DRM status headers **/
  if (denxt(is)) return true;
  DRMSummaryData.DRMStatusHeader1 = *DRMStatusHeader1;
#if DECODE_VERBOSE
  printf("drm%02d: \t %08x \t --> DRM Status Header 1 \n", drmID, *Data);
#endif
  if (denxt(is)) return true;
  DRMSummaryData.DRMStatusHeader2 = *DRMStatusHeader2;
#if DECODE_VERBOSE
  printf("drm%02d: \t %08x \t --> DRM Status Header 2 \n", drmID, *Data);
#endif
  if (denxt(is)) return true;
  DRMSummaryData.DRMStatusHeader3 = *DRMStatusHeader3;
#if DECODE_VERBOSE
  printf("drm%02d: \t %08x \t --> DRM Status Header 3 \n", drmID, *Data);
#endif
  if (denxt(is)) return true;
  DRMSummaryData.DRMStatusHeader4 = *DRMStatusHeader4;
#if DECODE_VERBOSE
  printf("drm%02d: \t %08x \t --> DRM Status Header 4 \n", drmID, *Data);
#endif
  if (denxt(is)) return true;
  //  DRMSummaryData.DRMStatusHeader5 = *DRMStatusHeader5;
#if DECODE_VERBOSE
  printf("drm%02d: \t %08x \t --> DRM Status Header 5 \n", drmID, *Data);
#endif
  /** loop over DRM data **/
  while (!denxt(is)) {
    if (Word->WordType == 5 && Word->SlotID == 1) {
      DRMSummaryData.DRMGlobalTrailer = *DRMGlobalTrailer;
      DRMSummaryData.Status |= 0x2;
      DRMSummaryData.LastWord = current - 1;
#if DECODE_VERBOSE
      printf("drm%02d: \t %08x \t -> DRM Global Trailer \n", drmID, *Data);
#endif
      return false;
    }
    else if (Word->WordType == 4 && Word->SlotID != 1) {      
#ifndef ERROR_RECOVER_DRM
      if (detrm(is)) return true;
#else
      detrm(is);
#endif
    }
    else if (Word->WordType == 7) {
#if DECODE_VERBOSE
      printf("drm%02d: \t %08x \t -> Filler \n", drmID, *Data);
#endif
    }
    else {
#if DECODE_VERBOSE
      printf("drm%02d: \t %08x \t [ERROR] \n", drmID, *Data);
#endif
#ifndef ERROR_RECOVER_DRM
      return true;
#endif
    }
  } /** end of loop over DRM data **/
  return false;
}

bool
detrm(std::ifstream &is)
{
  uint32_t slotID = TRMGlobalHeader->SlotID;
  uint32_t itrm = slotID - 3;
  TRMSummaryData[itrm].TRMGlobalHeader = *TRMGlobalHeader;
  TRMSummaryData[itrm].Status |= 0x1;
  TRMSummaryData[itrm].FirstWord = current - 1;
#if DECODE_VERBOSE
  printf("trm%02d: \t %08x \t --> TRM Global Header \n", slotID, *Data);
#endif
  while (!denxt(is)) {
    if ((Word->WordType == 0 || Word->WordType == 2) && Word->SlotID == slotID) {
#ifndef ERROR_RECOVER_TRM
      if (decnx(is, Word->WordType)) return true;
#else
      decnx(is, Word->WordType);
#endif
    }
    else if (Word->WordType == 5 && Word->SlotID == 7) {
      TRMSummaryData[itrm].TRMGlobalTrailer = *TRMGlobalTrailer;
      TRMSummaryData[itrm].Status |= 0x2;
      TRMSummaryData[itrm].LastWord = current - 1;
#if DECODE_VERBOSE
      printf("trm%02d: \t %08x \t --> TRM Global Trailer \n", slotID, *Data);
#endif
      return false;
    }
    else {
#if DECODE_VERBOSE
      printf("trm%02d: \t %08x \t [ERROR] \n", slotID, *Data);
#endif
      return true;
    }
  } 
  return true;
}

bool
decnx(std::ifstream &is, int htype)
{
  uint32_t slotID = TRMChainHeader->SlotID;
  uint32_t itrm = slotID - 3;
  uint32_t ichain = htype / 2;
  TRMChainSummaryData[itrm][ichain].TRMChainHeader = *TRMChainHeader;
  TRMChainSummaryData[itrm][ichain].Status |= 0x1;
  char name[2] = {'a', 'b'};
  char NAME[2] = {'A', 'B'};
#if DECODE_VERBOSE
  printf("cn%c%02d: \t %08x \t ---> Chain-%c Header \n", name[ichain], slotID, *Data, NAME[ichain]);
#endif
  while (!denxt(is)) {
    if (Word->WordType == 6) deerr();
    else if (Word->WordType == htype + 1) {
      TRMChainSummaryData[itrm][ichain].TRMChainTrailer = *TRMChainTrailer;
      TRMChainSummaryData[itrm][ichain].Status |= 0x2;
#if DECODE_VERBOSE
      printf("cn%c%02d: \t %08x \t ---> Chain-%c Trailer \n", name[ichain], slotID, *Data, NAME[ichain]);
#endif
      return false;
    }
    else {
#if DECODE_VERBOSE
      printf("cn%c%02d: \t %08x \t [ERROR] \n", name[ichain], slotID, *Data);
#endif
      return true;
    }
  }
  return true;
}

bool
deerr()
{
  uint32_t tdcID = 0;
#if DECODE_VERBOSE
  printf("err%02d: \t %08x \t ----> TDC Error \n", tdcID, *Data);
#endif
  return false;
}

#endif
