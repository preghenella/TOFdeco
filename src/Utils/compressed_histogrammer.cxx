#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <cstdint>
#include "Compressed/Decoder.h"

#include "TH1F.h"
#include "TH2F.h"
#include "TFile.h"

#define N_ORBIT_BC 3564
#define N_ORBIT_TDC_BINS N_ORBIT_BC * 1024

const double BC_FREQUENCY = 40.07897e6; // [Hz]
const double BC_WIDTH = 1.e6 / BC_FREQUENCY; // [us]
const double TDC_BIN_WIDTH = BC_WIDTH / 1024.; // [us]

int main(int argc, char **argv)
{

  bool verbose = false;
  std::string inFileName, outFileName;
  uint32_t spacingWindow, matchingWindow, latencyWindow;
  
  /** define arguments **/
  namespace po = boost::program_options;
  po::options_description desc("Options");
  desc.add_options()
    ("help"                                                                   , "Print help messages")
    ("verbose,v"  , po::bool_switch(&verbose)                                 , "Decode verbose")
    ("input,i"    , po::value<std::string>(&inFileName)->required()           , "Input data file")
    ("output,o"   , po::value<std::string>(&outFileName)->required()          , "Output data file")
    ("spacing,s"  , po::value<uint32_t>(&spacingWindow)->default_value(1188)  , "Spacing window (BC)")
    ("matching,m" , po::value<uint32_t>(&matchingWindow)->default_value(1192) , "Matching window (BC)")
    ("latency,l"  , po::value<uint32_t>(&latencyWindow)->default_value(1196)  , "Latency window (BC)")
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
  
  tof::data::compressed::Decoder decoder;
  decoder.setVerbose(verbose);
  decoder.load(inFileName);
  
  auto hBunchID = new TH1F("hBunchID", "", N_ORBIT_BC, 0., N_ORBIT_BC);
  auto hFrameID = new TH1F("hFrameID", "", 256, 0., 256.);
  auto hTime = new TH1F("hTime", "", 8192, 0., 8192.);
  auto hHitTime = new TH1F("hHitTime", "", 4096, 0., 2097152.);
  auto hOrbitTime = new TH1F("hOrbitTime", "", 8192, 0., N_ORBIT_TDC_BINS);
  auto hTRM_TDCID = new TH2F("hTRM_TDCID", "", 10, 0., 10., 30, 0., 30.);  

  auto h2 = new TH2F("h2", "", 256, 0., 256., 4096, 0., 8192.);
  
  std::map<uint32_t, TH1 *> mapBC_OrbitTime;
  
  /** loop over data **/
  while (!decoder.next()) {
    decoder.decode();

    auto summary = decoder.getSummary();
    auto BunchID = summary.CrateHeader.BunchID;

    int windowStart = (BunchID - latencyWindow) * 1024;
    
    hBunchID->Fill(BunchID);
    if (!mapBC_OrbitTime.count(BunchID))
      mapBC_OrbitTime[BunchID] = new TH1F(Form("hOrbitTime_BC%d", BunchID), "", 8192, 0., N_ORBIT_TDC_BINS);

    for (int ihit = 0; ihit < summary.nHits; ++ihit) {
      auto FrameID = summary.FrameHeader[ihit].FrameID;
      auto TRMID = summary.FrameHeader[ihit].TRMID;
      auto Time = summary.PackedHit[ihit].Time;
      auto TOT = summary.PackedHit[ihit].TOT;
      auto Channel = summary.PackedHit[ihit].Channel;
      auto TDCID = summary.PackedHit[ihit].TDCID;
      auto Chain = summary.PackedHit[ihit].Chain;
      auto index = Channel + 8 * TDCID + 120 * Chain + 240 * TRMID;

      hFrameID->Fill(FrameID);
      hTime->Fill(Time);

      uint32_t HitTime = Time + (FrameID << 13);
      hHitTime->Fill(HitTime);

      h2->Fill(FrameID, Time);
      
      int OrbitTime = windowStart + HitTime;
      if (OrbitTime < 0) OrbitTime += N_ORBIT_BC * 1024;
      hOrbitTime->Fill(OrbitTime);
      mapBC_OrbitTime[BunchID]->Fill(OrbitTime);
      
      
    }
    
  } /** end of decode loop **/

  decoder.close();
  
  auto fout = TFile::Open(outFileName.c_str(), "RECREATE");
  hBunchID->Write();
  hFrameID->Write();
  hTime->Write();
  hHitTime->Write();
  hOrbitTime->Write();
  h2->Write();
  for (auto mapBC : mapBC_OrbitTime) {
    mapBC.second->Write();
  }
  fout->Close();

  return 0;
}

