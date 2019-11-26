#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <chrono>
#include "Raw/Decoder.h"
#include "Raw/Checker.h"
#include "Compressed/Encoder.h"

int main(int argc, char **argv)
{

  bool verbose = false;
  std::string inFileName, outFileName;
  
  /** define arguments **/
  namespace po = boost::program_options;
  po::options_description desc("Options");
  desc.add_options()
    ("help", "Print help messages")
    ("verbose,v", po::bool_switch(&verbose), "Decode verbose")
    ("input,i", po::value<std::string>(&inFileName), "Input data file")
    ("output,o", po::value<std::string>(&outFileName), "Output data file")
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

  if (inFileName.empty() || outFileName.empty()) {
    std::cout << desc << std::endl;
    return 1;
  }
  
  tof::data::raw::Decoder decoder;
  decoder.setVerbose(verbose);
  decoder.init();
  if (decoder.open(inFileName)) return 1;

  tof::data::raw::Checker checker;
  checker.setVerbose(verbose);
  
  tof::data::compressed::Encoder encoder;
  encoder.setVerbose(verbose);
  encoder.init();
  if (encoder.open(outFileName)) return 1;

  /** chrono **/
  std::chrono::time_point<std::chrono::high_resolution_clock> start, finish;
  std::chrono::duration<double> elapsed;
  double integratedTime;
 
  /** loop over pages **/
  while (!decoder.read()) {

    /** get start chrono **/
    start = std::chrono::high_resolution_clock::now();	
    
    /** decode RDH open **/
    decoder.decodeRDH();
    
    /** decode loop **/
    while (!decoder.decode()) {
      
      /** check: if error rewind, print and pause **/
      if (checker.check(decoder.getSummary())) {
	decoder.rewind();
	decoder.setVerbose(true);
	checker.setVerbose(true);
	decoder.decodeRDH();
	while (!decoder.decode())
	  if (checker.check(decoder.getSummary()))
	    getchar();
	decoder.setVerbose(verbose);
	checker.setVerbose(verbose);
      }
      
      /** encode **/
      encoder.encode(decoder.getSummary());

    } /** end of decode loop **/

    /** pause chrono for IO operation **/
    finish = std::chrono::high_resolution_clock::now();
    elapsed = finish - start;
    integratedTime += elapsed.count();
    
    /** read page **/
    decoder.read();

    /** restart chrono after IO operation **/
    start = std::chrono::high_resolution_clock::now();
    
    /** decode RDH close **/
    decoder.decodeRDH();
    
    /** get finish chrono and increment **/
    finish = std::chrono::high_resolution_clock::now();
    elapsed = finish - start;
    integratedTime += elapsed.count();
    
    /** flush encoder **/
    encoder.flush();

  } /** end of loop over pages **/
  
  encoder.close();
  decoder.close();

  std::cout << " decoder benchmark: " << decoder.mIntegratedBytes << " bytes in " << decoder.mIntegratedTime << " s"
	    << " | " << 1.e-6 * decoder.mIntegratedBytes / decoder.mIntegratedTime << " MB/s"
	    << std::endl;
  
  std::cout << " checker benchmark: " << decoder.mIntegratedBytes << " bytes in " << checker.mIntegratedTime << " s"
	    << " | " << 1.e-6 * decoder.mIntegratedBytes / checker.mIntegratedTime << " MB/s"
	    << std::endl;
  
  std::cout << " encoder benchmark: " << encoder.mIntegratedBytes << " bytes in " << encoder.mIntegratedTime << " s"
	    << " | " << 1.e-6 * encoder.mIntegratedBytes / encoder.mIntegratedTime << " MB/s"
	    << std::endl;

  std::cout << " local benchmark: " << integratedTime << " s" << std::endl;
  
  return 0;
}

