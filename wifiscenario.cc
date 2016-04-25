/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor-helper.h"


//  Network Topology
//   Wifi 10.1.4.0
//                 AP
//  *    *    *    *
//  |    |    |    |    
// n5   n6   n7   n0 
//                   


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");

int 
main (int argc, char *argv[])
{
  std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i;
  bool verbose = true;
  bool info=true;
  uint32_t nWifi = 3;
  float APposx =2.25;
  float APposy =-2;
  uint32_t flownum = 0;
  float thrpt =0;
  int64_t Delay = 0;
  //float av_thrpt =0;
  //float summed_thrpt =0;
  bool tracing = false;
  //std::ofstream outputfile;
  std::ostringstream s;
  time_t timex;
  time(&timex);
  RngSeedManager::SetSeed(timex);
  RngSeedManager::SetRun(1);
  std::ofstream log("log_result_file.txt", std::ios_base::app | std::ios_base::out);
 
  CommandLine cmd;
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("APposx", "X-position of AP.", APposx);
  cmd.AddValue ("APposy", "Y-position of AP.", APposy);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);
  cmd.AddValue ("info", "Output measurement results", info);

  cmd.Parse (argc,argv);
  s << "Result_users_" << nWifi<<".txt";
  std::string query(s.str());
  //outputfile.open (query.c_str());
  
  // Check for valid number of csma or wifi nodes
  // 250 should be enough, otherwise IP addresses 
  // soon become an issue
  if (nWifi > 250)
    {
      std::cout << "Too many wifi or csma nodes, no more than 250 each." << std::endl;
      return 1;
    }

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }


  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi-1);
  NodeContainer wifiApNode;
  wifiApNode.Create (1);
  std::cout << "AP Creation succesfull." << std::endl;

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());
  phy.Set ("ShortGuardEnabled", BooleanValue (1));

  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetStandard (WIFI_PHY_STANDARD_80211n_2_4GHZ);
  Config::SetDefault ("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue (40.046));
  HtWifiMacHelper mac = HtWifiMacHelper::Default ();
  StringValue DataRate = HtWifiMacHelper::DataRateForMcs (7);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", DataRate,"ControlMode", DataRate);

  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);
  std::cout << "STA install succesfull." << std::endl;
  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);
  std::cout << "AP install succesfull." << std::endl;
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (40));
  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (2),
                                 "DeltaY", DoubleValue (2),
                                 "GridWidth", UintegerValue (4),
                                 "LayoutType", StringValue ("RowFirst"));
  //mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
  //                        "Bounds", RectangleValue (Rectangle (0.0, 5.0, -0.001, 5.0)));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiStaNodes);
  Ptr<ListPositionAllocator> positionAlloc =CreateObject<ListPositionAllocator>();
  positionAlloc->Add (Vector (APposx, APposy, 0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);

  InternetStackHelper stack;
  stack.Install (wifiStaNodes);
  stack.Install (wifiApNode);
  Ipv4AddressHelper address;

 
  Ipv4InterfaceContainer staInterfaces;
 
  Ipv4InterfaceContainer apInterfaces;
  address.SetBase ("10.1.4.0", "255.255.255.0");
  apInterfaces = address.Assign (apDevices);
  staInterfaces = address.Assign (staDevices);


  UdpEchoServerHelper echoServer (99);

  ApplicationContainer serverApps = echoServer.Install (wifiApNode.Get(0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (11.0));

  UdpEchoClientHelper echoClient (apInterfaces.GetAddress (0), 99);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1000));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds(0.00001)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1472));

  ApplicationContainer clientApps = 
  echoClient.Install (wifiStaNodes);
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (5.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (10));

  if (tracing == true)
    {

      phy.EnablePcap ("scena", apDevices.Get (0));

    }
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();


  AnimationInterface anim ("animation.xml");
  anim.SetMaxPktsPerTraceFile(100000000);
  Simulator::Run ();

  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  for (i = stats.begin (); i != stats.end (); ++i)
    {
          flownum++;
          Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);

          thrpt = i->second.rxBytes*8/((i->second.timeLastRxPacket.GetSeconds()-i->second.timeFirstTxPacket.GetSeconds())*1024);
          if((i->second.rxPackets !=0)){
                Delay = (i->second.delaySum.GetMilliSeconds() / i->second.rxPackets);
          }
          else{
                Delay = 99999;
                std::cout<< "Delay Error"<<"\n";
          };

          if(info){

                std::cout << "Flow " << i->first<< " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
                std::cout << "Throughput: " <<  thrpt << " Kb/s"<<"\n";
          
                std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
                std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";

                std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
                std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
                std::cout << "  Delay:      " << Delay << "\n";                                 
          };
          log << Delay<<"\n";
    }
  if ((i->first) != 40){
        std::cout << "!!! Number of flows<40 !!!" << "\n";
        for(uint32_t idxx = 0; idxx<(40-(i->first)); idxx++){
             log <<0<<"\n";
        };   
  };
  //av_thrpt =  summed_thrpt /flownum;
  //outputfile << av_thrpt <<"\n";
  //log << nWifi <<","<<av_thrpt<<"\n";
  //outputfile.close();
  //flowmon.SerializeToXmlFile ("wifiscenout.xml", false, false);

  Simulator::Destroy ();
  return 0;

}
