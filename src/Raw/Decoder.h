#ifndef _TOF_RAW_DATA_DECODER_H
#define _TOF_RAW_DATA_DECODER_H

#include <fstream>
#include <string>
#include <cstdint>
#include "Raw/dataFormat.h"

namespace tof {
namespace data {
namespace raw {
  
  class Decoder {

  public:
    
    Decoder() {};
    ~Decoder() {};

    bool init();
    bool open(std::string name);
    bool load(std::string name);
    bool read();
    bool decodeRDH();
    bool decode();
    void rewind() {mPointer = (uint32_t *)mBuffer;};
    bool close();

    void setVerbose(bool val) {mVerbose = val;};
    void setSkip(int val) {mSkip = val;};
    void setSize(long val) {mSize = val;};
    Summary_t &getSummary() {return mSummary;};

    // benchmarks
    double mIntegratedBytes = 0.;
    double mIntegratedTime = 0.;
    
  protected:

    void clear();
    void print(std::string what);
    void printRDH(std::string what);

    inline void next128();
    inline void next32();
    
    std::ifstream mFile;
    char *mBuffer = nullptr;
    long mSize = 8192;
    uint32_t *mPointer = nullptr;
    char *mRewind = nullptr;

    bool mVerbose = false;
    uint32_t mSkip = 1;
    uint32_t mSlotID;
    uint32_t mWordType;
    RDH_t *mRDH;
    Summary_t mSummary;    

    uint32_t mPageCounter = 0;
    uint32_t mByteCounter = 0;
    
  };
  
}}}

#endif /** _TOF_RAW_DATA_DECODER_H **/
