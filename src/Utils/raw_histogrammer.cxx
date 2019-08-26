 #include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <cstdint>
#include "Raw/Decoder.h"

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
  
  tof::data::raw::Decoder decoder;
  decoder.setVerbose(verbose);
  decoder.init();
  if (decoder.open(inFileName)) return 1;

  auto hRDH_MemorySize = new TH1F("hRDH_MemorySize", "", 8192, 0., 8192.); 
  auto hDRM_L0BCID = new TH1F("hDRM_L0BCID", "", N_ORBIT_BC, 0., N_ORBIT_BC);
  auto hCrate_Channel = new TH1F("hCrate_Channel", "", 2400, 0., 2400.);
  auto hTDC_HitTime = new TH1F("hTDC_HitTime", "", 4096, 0., 2097152.);
  auto hTDC_HitTime_us = new TH1F("hTDC_HitTime_us", ";hit time (us)", 8192, 0., 2097152 * TDC_BIN_WIDTH);
  auto hOrbit_Time = new TH1F("hOrbit_Time", "", 8192, 0., N_ORBIT_TDC_BINS);
  auto hOrbit_Time_us = new TH1F("hOrbit_Time_us", "", 8192, 0., N_ORBIT_BC * BC_WIDTH);
  auto hTRM_TDCID = new TH2F("hTRM_TDCID", "", 10, 0., 10., 30, 0., 30.);
  
  auto h2 = new TH2F("h2", "", N_ORBIT_BC, 0., N_ORBIT_BC, 8192, 0., N_ORBIT_TDC_BINS);
  
  std::map<uint32_t, TH1 *> mapBC_Orbit_Time;
  std::map<uint32_t, TH1 *> mapBC_Orbit_Time_us;
  
  /** loop over pages **/
  while (!decoder.read()) {
    
    /** decode RDH open **/
    decoder.decodeRDH();
    
    hRDH_MemorySize->Fill(decoder.getSummary().RDHWord0.MemorySize);
    
    /** decode loop **/
    while (!decoder.decode()) {

      auto summary = decoder.getSummary();
      auto DRM_L0BCID = GET_DRM_L0BCID(summary.DRMStatusHeader3);
      int windowStart = (DRM_L0BCID - latencyWindow) * 1024;

      hDRM_L0BCID->Fill(DRM_L0BCID);
      if (!mapBC_Orbit_Time.count(DRM_L0BCID))
	mapBC_Orbit_Time[DRM_L0BCID] = new TH1F(Form("hOrbit_Time_BC%d", DRM_L0BCID), "", 8192, 0., N_ORBIT_TDC_BINS);
      if (!mapBC_Orbit_Time_us.count(DRM_L0BCID))
	mapBC_Orbit_Time_us[DRM_L0BCID] = new TH1F(Form("hOrbit_Time_us_BC%d", DRM_L0BCID), "", 8192, 0., N_ORBIT_BC * BC_WIDTH);
      
      for (int itrm = 0; itrm < 10; ++itrm) {
	for (int ichain = 0; ichain < 2; ++ichain) {
	  for (int itdc = 0; itdc < 15; ++itdc) {
	    for (int ihit = 0; ihit < summary.nTDCUnpackedHits[itrm][ichain][itdc]; ++ihit) {
	      auto PSBits = GET_TDCHIT_PSBITS(summary.TDCUnpackedHit[itrm][ichain][itdc][ihit]);
	      if (PSBits != 0x1) continue;
	      auto TDC_HitTime = GET_TDCHIT_HITTIME(summary.TDCUnpackedHit[itrm][ichain][itdc][ihit]);
	      auto TDCID = GET_TDCHIT_TDCID(summary.TDCUnpackedHit[itrm][ichain][itdc][ihit]);
	      auto Chan = GET_TDCHIT_CHAN(summary.TDCUnpackedHit[itrm][ichain][itdc][ihit]);
	      auto index = Chan + 8 * TDCID + 120 * ichain + 240 * itrm;
	      hTRM_TDCID->Fill(itrm, itdc + 15 * ichain);
	      hCrate_Channel->Fill(index);
	      hTDC_HitTime->Fill(TDC_HitTime);
	      hTDC_HitTime_us->Fill(TDC_HitTime * TDC_BIN_WIDTH);
	      if (TDC_HitTime < 4096) continue;
	      int Orbit_Time = windowStart + TDC_HitTime;
	      if (Orbit_Time < 0) {
		Orbit_Time += N_ORBIT_BC * 1024;
	      }
	      hOrbit_Time->Fill(Orbit_Time);
	      hOrbit_Time_us->Fill(Orbit_Time * TDC_BIN_WIDTH);
	      mapBC_Orbit_Time[DRM_L0BCID]->Fill(Orbit_Time);
	      mapBC_Orbit_Time_us[DRM_L0BCID]->Fill(Orbit_Time * TDC_BIN_WIDTH);
	    }
	  }
	}
      }
      
    } /** end of decode loop **/
    
    /** read page **/
    decoder.read();
    
    /** decode RDH close **/
    decoder.decodeRDH();
    
  } /** end of loop over pages **/
  
  
  decoder.close();
  
  auto fout = TFile::Open(outFileName.c_str(), "RECREATE");
  hRDH_MemorySize->Write();
  hDRM_L0BCID->Write();
  hTDC_HitTime->Write();
  hTDC_HitTime_us->Write();
  hOrbit_Time->Write();
  hOrbit_Time_us->Write();
  hCrate_Channel->Write();
  hTRM_TDCID->Write();
  for (auto mapBC : mapBC_Orbit_Time) {
    mapBC.second->Write();
  }
  for (auto mapBC : mapBC_Orbit_Time_us) {
    mapBC.second->Write();
  }
  fout->Close();

  return 0;
}

