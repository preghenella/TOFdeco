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
    
  Decoder() : mVerbose(false), mSkip(0) {};
    ~Decoder() {};
    
    bool open(std::string name);
    bool load(std::string name);
    bool next();
    bool decode();
    bool close();

    void setVerbose(bool val) {mVerbose = val;};
    void setSkip(int val) {mSkip = val;};
    const Summary_t &getSummary() const {return mSummary;};

    // benchmarks
    double mIntegratedBytes = 0.;
    double mIntegratedTime = 0.;
    
  protected:

    void clear();
    void spider();
    void print(std::string what);

    std::ifstream mFile;
    char *mBuffer;
    long mSize;

    bool mVerbose;
    uint32_t mSkip;
    uint32_t mSlotID;
    uint32_t mWordType;
    Union_t *mUnion;
    Summary_t mSummary;    
    
  };
  
}}}

#endif /** _TOF_RAW_DATA_DECODER_H **/
