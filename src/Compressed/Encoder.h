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
    bool alloc(long size);
    bool encode(const tof::data::raw::Summary_t &summary);
    bool flush();
    bool close();
    void setVerbose(bool val) {mVerbose = val;};
    
    // benchmarks
    double mIntegratedBytes = 0.;
    double mIntegratedTime = 0.;
    
  protected:

    std::ofstream mFile;
    bool mVerbose;

    char *mBuffer;
    long mSize;
    Union_t *mUnion;
    
  };
  
}}}

#endif /** _TOF_RAW_COMPRESSED_ENCODER_H_ **/
