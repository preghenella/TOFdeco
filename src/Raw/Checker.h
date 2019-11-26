#ifndef _TOF_RAW_DATA_CHECKER_H
#define _TOF_RAW_DATA_CHECKER_H

#include <fstream>
#include <string>
#include <cstdint>
#include "Raw/dataFormat.h"

namespace tof {
namespace data {
namespace raw {
  
  class Checker {

  public:
    
    Checker() {};
    ~Checker() {};

    bool check(tof::data::raw::Summary_t &summary);
    void setVerbose(bool val) {mVerbose = val;};

    // benchmarks
    double mIntegratedTime = 0.;
    
  protected:

    bool mVerbose = false;
    
  };
  
}}}

#endif /** _TOF_RAW_DATA_CHECKER_H **/
