#ifndef _TOF_RAW_COMPRESSED_ENCODER_H_
#define _TOF_RAW_COMPRESSED_ENCODER_H_

#include <fstream>
#include <string>
#include <cstdint>
#include "Raw/dataFormat.h"
#include "Compressed/dataFormat.h"

namespace tof {
namespace data {
namespace compressed {
      
  class Encoder {

  public:
    
    Encoder() : mVerbose(false) {};
    ~Encoder() {};
    
    bool open(std::string name);
    bool init();
    bool encode(const tof::data::raw::Summary_t &summary);
    bool flush();
    bool close();
    void setVerbose(bool val) {mVerbose = val;};
    
    // benchmarks
    double mIntegratedBytes = 0.;
    double mIntegratedTime = 0.;
    
  protected:

    inline void next32();

    std::ofstream mFile;
    bool mVerbose;

    char *mBuffer;
    long mSize = 8192;
    char *mPointer = nullptr;

    Union_t *mUnion;

    uint32_t mByteCounter = 0;
  };
  
}}}

#endif /** _TOF_RAW_COMPRESSED_ENCODER_H_ **/
