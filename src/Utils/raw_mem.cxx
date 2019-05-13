#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <chrono>

int main(int argc, char **argv)
{

  std::string inFileName;
  std::string outFileName;
  
  /** define arguments **/
  namespace po = boost::program_options;
  po::options_description desc("Options");
  desc.add_options()
    ("help", "Print help messages")
    ("input,i", po::value<std::string>(&inFileName), "Input data file")
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
  
  std::ifstream is;
  is.open(inFileName.c_str(), std::fstream::binary);
  if (!is.is_open()) {
    std::cerr << "cannot open input: " << inFileName << std::endl;
    return 1;
  }

  is.seekg(0, is.end);
  long bytes = is.tellg();
  bytes = (bytes / 16) * 16;
  long lines = bytes / 16;
  is.seekg(0);
  char *ibuffer = new char[bytes];
  is.read(ibuffer, bytes);

  char *obuffer = new char[bytes / 2];

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0 ; i < lines; ++i)
    memcpy(obuffer += 8 , ibuffer += 16, 8);

  auto finish = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = finish - start;
  std::cout << elapsed.count() << std::endl;
  
  //  delete [] ibuffer;
  //  delete [] obuffer;
  is.close();
  
}

