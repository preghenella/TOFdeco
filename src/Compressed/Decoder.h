#ifndef _TOF_RAW_COMPRESSED_DECODER_H_
#define _TOF_RAW_COMPRESSED_DECODER_H_

#include <fstream>
#include <string>
#include <cstdint>
#include "Compressed/dataFormat.h"

namespace tof {
namespace data {
namespace compressed {
      
  class Decoder {

  public:
    
  Decoder() : mVerbose(false) {};
    ~Decoder() {};
    
    bool open(std::string name);
    bool load(std::string name);
    bool next();
    bool decode();
    bool close();
    void setVerbose(bool val) {mVerbose = val;};

    // benchmarks
    double mIntegratedBytes = 0.;
    double mIntegratedTime = 0.;
    
  protected:

    void print(std::string what);

    std::ifstream mFile;
    char *mBuffer;
    long mSize;

    bool mVerbose;
    Union_t *mUnion;

    
  };
  
}}}

#endif /** _TOF_RAW_COMPRESSED_DECODER_H_ **/
