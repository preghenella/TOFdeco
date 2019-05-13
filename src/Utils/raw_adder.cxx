#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <cstdint>

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
    ("output,o", po::value<std::string>(&outFileName), "Output data file")
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

  std::ofstream os;
  os.open(outFileName.c_str(), std::fstream::binary);
  if (!is.is_open()) {
    std::cerr << "cannot open output: " << outFileName << std::endl;
    return 1;
  }

  char buffer[16];
  for (int i = 0; i < 16; ++i) buffer[i] = 0;
  while (!is.eof()) {
    is.read(buffer, 8);
    os.write(buffer, 16);
  }

  is.close();
  os.close();
  
}

