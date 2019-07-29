#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/aodv-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/packet-sink.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

NS_LOG_COMPONENT_DEFINE ("CRTest");

using namespace ns3;

int main (int argc, char *argv[])
{

  SeedManager::SetSeed(1);
  std::string phyMode ("ErpOfdmRate54Mbps");
  int nNodes = 10;
  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");
  NS_LOG_UNCOND("Number of Nodes in the Scenario: " << nNodes << "\n\n");
  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");
  double simulation_duration=20.0;

  CommandLine cmd;

  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("numNodes", "Number of nodes", nNodes);

  cmd.Parse (argc, argv);

  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
      StringValue (phyMode));

  NodeContainer c;
  c.Create (nNodes);

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211g);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  // This is one parameter that matters when using FixedRssLossModel
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (0) );
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a non-QoS upper mac, and disable rate control
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
      "DataMode",StringValue (phyMode),
      "ControlMode",StringValue (phyMode));
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");

  MobilityHelper mobility;

  /*mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                      "X", StringValue ("0.0"),
                                      "Y", StringValue ("0.0"),
                                      "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=30]"));*/

  // Position our nodes with 110 m in between
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  double start = 0.0;
  for (int i=0; i<nNodes; i++)
  {
  	  positionAlloc->Add (Vector (start, 0.0, 0.0));
  	  start = start+5;//10
  }
  mobility.SetPositionAllocator (positionAlloc);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  /*mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
		  "GridWidth", StringValue ("50.0"),
		  "MinX", StringValue ("0.0"), "MinY", StringValue ("0.0"), "DeltaX", StringValue ("10.0"),
		  "DeltaY", StringValue ("10.0"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");*/

  // Read PU file
  Ptr<PUModel> puModel = CreateObject<PUModel>();
  std::string map_file = "map_PUs_multiple.txt";
  puModel->SetPuMapFile((char*)map_file.c_str());
  //Create repository
  Ptr<Repository> repo = CreateObject<Repository>();
  // Install the CR features into the nodes and return the list of devices
  NetDeviceContainer devices = wifi.InstallCR (repo, puModel, mobility, wifiPhy, wifiMac, c);

  NS_LOG_INFO("Allocated MAC address for Nodes are: ");
  for (uint32_t i=0; i<devices.GetN(); i++) {
	  NS_LOG_INFO(devices.Get(i)->GetAddress());
    }

  //ObjectFactory sm = wifi.m_stationManager;
  /*Ptr<ns3::RegularWifiMac> rwm;
  SpectrumManager *sm = new SpectrumManager(rwm,0);
  NS_LOG_UNCOND(sm->IsChannelAvailable());*/

  // For each CR, we have 3 interfaces. Save the first interface in each
  // node which is the CTRL_IFACE in the device_control array.
  NetDeviceContainer devices_control;
  for (uint32_t i=0; i<devices.GetN(); i=i+3) {
    devices_control.Add(devices.Get(i));
  }

  InternetStackHelper internet;
  AodvHelper aodv;
  internet.SetRoutingHelper(aodv);
  internet.InstallCR (repo, c);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  // IP addresses are only assigned for control devices
  Ipv4InterfaceContainer i = ipv4.Assign (devices_control);

  NS_LOG_UNCOND("Initially: \n");
  NS_LOG_UNCOND("Number of Master Nodes: " << nNodes-nNodes+1);
  NS_LOG_UNCOND("Number of Ordinary Nodes: " << c.GetN()-1);
  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  uint16_t port = 9;  // well-known echo port number

  //Config::Set ("/NodeList/*/ApplicationList/*/$ns3::Bsync_Server/TotalSU", IntegerValue(nNodes));
  //Config::Set ("/NodeList/*/ApplicationList/*/$ns3::Bsync_Server/TotalSU", DoubleValue(simulation_duration));

  Bsync_ServerHelper server (port, simulation_duration, nNodes);
  ApplicationContainer apps;
  for(int i=0;i<c.GetN();i++)
  {
	  if (i!=c.GetN()/2)
	  {
		  apps = server.Install (c.Get (i));
		  apps.Start (Seconds (1.0));
		  apps.Stop (Seconds (simulation_duration));
	  }
  }

  /*apps = server.Install (c.Get (c.GetN()-2));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (20.0));*/

  uint32_t packetSize = 1024;
  uint32_t maxPacketCount = 2;
  Time interPacketInterval = Seconds (1.);
  Bsync_ClientHelper client (Ipv4Address ("1.1.1.1"), port, simulation_duration, nNodes);//problem setting specific ip
  client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client.SetAttribute ("PacketSize", UintegerValue (packetSize));
  apps = client.Install (c.Get (c.GetN()/2));//0
  //client.SetFill (apps.Get (0), "Hello World");
  apps.Start (Seconds (2.0));
  apps.Stop (Seconds (simulation_duration));

  // Output what we are doing
  NS_LOG_UNCOND ("Starting CR test");

  Simulator::Stop (Seconds (simulation_duration+1.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
