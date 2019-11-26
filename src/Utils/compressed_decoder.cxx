#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <cstdint>
#include "Compressed/Decoder.h"

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
    //    ("word,w",  po::value<int>(&wordn)->default_value(2), "Word where to find the data")
    ;

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
  
  if (inFileName.empty()) {
      std::cout << desc << std::endl;
      return 1;
  }
  
  tof::data::compressed::Decoder decoder;
  decoder.setVerbose(verbose);
  decoder.load(inFileName);

  while (!decoder.next())
    decoder.decode();
  
  decoder.close();
  
  std::cout << " benchmark: decoded " << decoder.mIntegratedBytes << " bytes in " << decoder.mIntegratedTime << " s"
	    << " | " << 1.e-6 * decoder.mIntegratedBytes / decoder.mIntegratedTime << " MB/s"
	    << std::endl;
  

  
  return 0;
}

