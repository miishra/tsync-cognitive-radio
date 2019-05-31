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
  int nNodes = 3;

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

  // Position our nodes with 110 m in between
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  double start = 110.0;
  for (int i=0; i<nNodes; i++) {
    positionAlloc->Add (Vector (start, 0.0, 0.0));
    start = start+10;//20
  }
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  // Read PU file
  Ptr<PUModel> puModel = CreateObject<PUModel>();
  std::string map_file = "map_PUs_multiple.txt";
  puModel->SetPuMapFile((char*)map_file.c_str());
  //Create repository
  Ptr<Repository> repo = CreateObject<Repository>();
  // Install the CR features into the nodes and return the list of devices
  NetDeviceContainer devices = wifi.InstallCR (repo, puModel, mobility, wifiPhy, wifiMac, c);

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

  uint16_t port = 9;  // well-known echo port number
  Bsync_ServerHelper server (port);
  ApplicationContainer apps = server.Install (c.Get (c.GetN()-1));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (20.0));

  apps = server.Install (c.Get (c.GetN()-2));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (20.0));

  uint32_t packetSize = 1024;
  uint32_t maxPacketCount = 2;
  Time interPacketInterval = Seconds (1.);
  Bsync_ClientHelper client (Ipv4Address ("255.255.255.255"), port);//Address(i.GetAddress (c.GetN()-1))
  client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client.SetAttribute ("PacketSize", UintegerValue (packetSize));
  apps = client.Install (c.Get (0));
  client.SetFill (apps.Get (0), "Hello World");
  apps.Start (Seconds (2.0));
  apps.Stop (Seconds (20.0));

  // Output what we are doing
  NS_LOG_UNCOND ("Starting CR test");

  Simulator::Stop (Seconds (21.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
