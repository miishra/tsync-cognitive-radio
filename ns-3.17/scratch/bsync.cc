#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/aodv-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/packet-sink.h"
#include "ns3/gnuplot.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <numeric>

using namespace std;

NS_LOG_COMPONENT_DEFINE ("CRTest");

using namespace ns3;

int main (int argc, char *argv[])
{
  int NumSim = 1;
  int num_scenarios=7;

  std::vector<double> average_time_taken[num_scenarios];
  std::vector<double> average_overhead_taken[num_scenarios];

  std::vector<double> conf_interval_time, sdv_val_time, mean_time;
  std::vector<double> conf_interval_overhead, sdv_val_overhead, mean_overhead;

  int nNodeArray[9] = {50, 10, 20, 40, 60, 80, 100, 120, 140};

  int color_mode_main[11];
  for(int k=0;k<11;k++)
	  color_mode_main[k]=0;

  for(int m=0;m<NumSim;m++)
  {
	  double time_taken_array[num_scenarios];
	  double overhead_taken_array[num_scenarios];
	  for(int k=0;k<num_scenarios;k++)
	  {
		  time_taken_array[k]=0;
		  overhead_taken_array[k]=0;
	  }
	  std::fstream main_aodv_reader;

	  for(int k=0;k<num_scenarios;k++)
	  {
			//std::remove("./src/aodv/model/AODVparameters.h");
			main_aodv_reader.open("./src/aodv/model/AODVparameters.h", ios::out | ios::trunc);

			int nNodes = nNodeArray[k];


			main_aodv_reader << "int aodv_no_su = " << nNodes << ";" << std::endl;
			main_aodv_reader.close();


			time_taken_array[k]=10000;


			SeedManager::SetSeed(rand()%1000+10);

			std::string phyMode ("ErpOfdmRate54Mbps");
			NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");
			NS_LOG_UNCOND("Number of Nodes in the Scenario: " << nNodes << "\n\n");
			NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");
			double simulation_duration=100.0;

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

			mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
												"X", StringValue ("0.0"),
												"Y", StringValue ("0.0"),
												"Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=20]"));

			// Position our nodes with 110 m in between
			/*Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
			double start = 0.0;
			for (int i=0; i<nNodes; i++)
			{
				  positionAlloc->Add (Vector (start, 0.0, 0.0));
				  start = start+10;//10
			}
			mobility.SetPositionAllocator (positionAlloc);

			mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");*/

			/*mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
				  "GridWidth", StringValue ("20.0"),
				  "MinX", StringValue ("0.0"), "MinY", StringValue ("0.0"), "DeltaX", StringValue ("10.0"),
				  "DeltaY", StringValue ("10.0"));*/
			mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

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
			NS_LOG_UNCOND("Number of Master Nodes: " << nNodes-nNodes+2);
			NS_LOG_UNCOND("Number of Ordinary Nodes: " << c.GetN()-2);
			NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");

			Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

			uint16_t port = 9;  // well-known echo port number

			//Config::Set ("/NodeList/*/ApplicationList/*/$ns3::Bsync_Server/TotalSU", IntegerValue(nNodes));
			//Config::Set ("/NodeList/*/ApplicationList/*/$ns3::Bsync_Server/TotalSU", DoubleValue(simulation_duration));

			std::fstream main_reader;
			main_reader.open("results.txt", std::fstream::app);
			main_reader << std::endl;
			main_reader.close();

			Bsync_ServerHelper server (port, simulation_duration, nNodes);
			ApplicationContainer apps;
			for(int i=1;i<c.GetN()-1;i++)
			{
			  apps = server.Install (c.Get (i));
			  apps.Start (Seconds (1.0));
			  apps.Stop (Seconds (simulation_duration));
			}

			/*apps = server.Install (c.Get (c.GetN()-2));
			apps.Start (Seconds (1.0));
			apps.Stop (Seconds (20.0));*/

			int tot_master_nodes=2;

			for(int j=0;j<tot_master_nodes;j++)
			{
			  uint32_t packetSize = 1024;
			  uint32_t maxPacketCount = 2;
			  Time interPacketInterval = Seconds (1.);
			  Bsync_ClientHelper client (Ipv4Address ("1.1.1.1"), port, simulation_duration, nNodes);//problem setting specific ip
			  client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
			  client.SetAttribute ("Interval", TimeValue (interPacketInterval));
			  client.SetAttribute ("PacketSize", UintegerValue (packetSize));
			  if (j==0)
				  apps = client.Install (c.Get (0));//0
			  else
				  apps = client.Install (c.Get (c.GetN()-1));
			  //client.SetFill (apps.Get (0), "Hello World");
			  apps.Start (Seconds (2.0));
			  apps.Stop (Seconds (simulation_duration));
			}

			// Output what we are doing
			NS_LOG_UNCOND ("Starting CR test");

			Simulator::Stop (Seconds (simulation_duration+1.0));
			Simulator::Run ();
			Simulator::Destroy ();

			main_reader.open("results.txt", std::fstream::in);//| std::fstream::trunc
			int SentBytes[nNodes], ReceivedBytes[nNodes], TotalSyncSent[nNodes], TotalHelloSent[nNodes], TotalOverHeadBytes[nNodes];
			double FinalTimestamp[nNodes], TimeToSyncronize[nNodes];

			for(int l=0;l<nNodes;l++)
			{
				SentBytes[l]=0, ReceivedBytes[l]=0, TotalSyncSent[l]=0, TotalHelloSent[l]=0, TotalOverHeadBytes[l]=0;
				FinalTimestamp[l]=0.0, TimeToSyncronize[l]=0.0;
			}

			std::string line;
			while (std::getline(main_reader, line))
			{
			  int node_id;
			  std::string mode;
			  std::istringstream iss(line);
			  int SentBytes_temp, ReceivedBytes_temp, TotalSyncSent_temp, TotalHelloSent_temp, TotalOverHeadBytes_temp;
			  double FinalTimestamp_temp, TimeToSyncronize_temp;
			  if (iss >> node_id >> mode >> SentBytes_temp >> ReceivedBytes_temp >> FinalTimestamp_temp >> TimeToSyncronize_temp >> TotalSyncSent_temp >> TotalHelloSent_temp >> TotalOverHeadBytes_temp)
			  {
				  SentBytes[node_id] = SentBytes_temp, ReceivedBytes[node_id] = ReceivedBytes_temp, FinalTimestamp[node_id] = FinalTimestamp_temp, TimeToSyncronize[node_id] = TimeToSyncronize_temp, TotalSyncSent[node_id] = TotalSyncSent_temp;
				  TotalHelloSent[node_id] = TotalHelloSent_temp, TotalOverHeadBytes[node_id] = TotalOverHeadBytes_temp;
			  }
			  else if (!(iss >> node_id >> mode >> SentBytes_temp >> ReceivedBytes_temp >> FinalTimestamp_temp >> TimeToSyncronize_temp >> TotalSyncSent_temp >> TotalHelloSent_temp >> TotalOverHeadBytes_temp))
			  {
				  continue;
			  }
			}

			main_reader.close();

			std::fstream main_color_reader;
			main_color_reader.open("colors_allotted.txt", std::fstream::in);
			line.clear();
			while (std::getline(main_color_reader, line))
			{
			  int node_id;
			  std::istringstream iss(line);
			  int temp;
			  iss >> node_id;
			  for(int k=0;k<11;k++)
			  {
				  iss >> temp;
				  color_mode_main[k]+=temp;
			  }
			}
			main_color_reader.close();

			int max_time=0;
			int max_overhead=0;
			for(int j=0;j<nNodes;j++)
			{
				if (TimeToSyncronize[j]!=10000)
				{
					if (TimeToSyncronize[j]>max_time)
						max_time = TimeToSyncronize[j];
				}

				if (TotalOverHeadBytes[j]>max_overhead)
						max_overhead = TotalOverHeadBytes[j];
			}

			time_taken_array[k]=max_time;
			overhead_taken_array[k]=max_overhead;

			main_reader.open("results.txt", std::fstream::out | std::fstream::trunc);
			main_reader.close();

			main_color_reader.open("colors_allotted.txt", std::fstream::out | std::fstream::trunc);
			main_color_reader.close();
	  }

		  for (int i=0; i<num_scenarios; i++)
		  {
			  //cout << time_taken_array[i] << "\t" << overhead_taken_array[i] << std::endl;
			  average_time_taken[i].push_back(time_taken_array[i]);
			  average_overhead_taken[i].push_back(overhead_taken_array[i]);
		  }

  }

  	 double sum, mean, sq_sum, stdev;
  	 for (int i=0; i<num_scenarios; i++)
  	 {
  		sum = std::accumulate(average_time_taken[i].begin(), average_time_taken[i].end(), 0.0);
  		mean = sum / average_time_taken[i].size();

  		std::vector<double> diff(average_time_taken[i].size());
  		std::transform(average_time_taken[i].begin(), average_time_taken[i].end(), diff.begin(),
  		               std::bind2nd(std::minus<double>(), mean));
  		sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
  		stdev = std::sqrt(sq_sum / average_time_taken[i].size());

  		mean_time.push_back(mean);
  		sdv_val_time.push_back(stdev);

  		sum = std::accumulate(average_overhead_taken[i].begin(), average_overhead_taken[i].end(), 0.0);
		mean = sum / average_overhead_taken[i].size();

		std::vector<double> diff2(average_overhead_taken[i].size());
		std::transform(average_overhead_taken[i].begin(), average_overhead_taken[i].end(), diff2.begin(),
					   std::bind2nd(std::minus<double>(), mean));
		sq_sum = std::inner_product(diff2.begin(), diff2.end(), diff2.begin(), 0.0);
		stdev = std::sqrt(sq_sum / average_overhead_taken[i].size());

		mean_overhead.push_back(mean);
		sdv_val_overhead.push_back(stdev);
  	 }

  	for (int i=0; i<num_scenarios; i++)
	  {
			cout << mean_time[i] << "\t" << mean_overhead[i] << std::endl;
			cout << sdv_val_time[i] << "\t" << sdv_val_overhead[i] << std::endl;
	  }

  	for (int i=0; i<num_scenarios; i++)
  	{
  		conf_interval_time.push_back(4.3026*(sdv_val_time[i]/std::sqrt(NumSim)));
  		conf_interval_overhead.push_back(4.3026*(sdv_val_overhead[i]/std::sqrt(NumSim)));
  	}

  	for (int i=0; i<num_scenarios; i++)
  	{
  		std::cout << "Confidence Interval for time taken in case of: " << nNodeArray[i] << " nodes is: " << conf_interval_time[i] << std::endl;
  		std::cout << "Confidence Interval for Overhead occurred in case of: " << nNodeArray[i] << " nodes is: " << conf_interval_overhead[i] << std::endl;
  	}

  	 //Create 2-D data and plot
  	 std::string fileNameWithNoExtension = "TimeToSynchronize";
     std::string graphicsFileName        = "TimeToSynchronize.png";
     std::string plotFileName            = "TimeToSynchronize.plt";
     std::string plotTitle               = "TimeToSynchronize Plot";
     std::string dataTitle               = "TimeToSynchronize Data";

     // Instantiate the plot and set its title.
     Gnuplot plot (graphicsFileName);
     plot.SetTitle (plotTitle);

     // Make the graphics file, which the plot file will create when it
     // is used with Gnuplot, be a PNG file.
     plot.SetTerminal ("png");

     // Set the labels for each axis.
     plot.SetLegend ("Node IDs","Time Taken to Synchronize");

     // Set the range for the x axis.
     //plot.AppendExtra ("set xrange [0:nNodes]");

     // Instantiate the dataset, set its title, and make the points be
     // plotted along with connecting lines.
     Gnuplot2dDataset dataset;
     dataset.SetTitle (dataTitle);
     dataset.SetStyle (Gnuplot2dDataset::LINES_POINTS);

     //double x;
     //double y;

     // Create the 2-D dataset.
     for (int i=0; i<num_scenarios; i++)
       {
    	 dataset.Add ((double) nNodeArray[i]*1.0, mean_time[i]);
       }

     // Add the dataset to the plot.
     plot.AddDataset (dataset);

     // Open the plot file.
     std::ofstream plotFile (plotFileName.c_str());

     // Write the plot file.
     plot.GenerateOutput (plotFile);

     // Close the plot file.
     plotFile.close ();



     //Create 2-D data and plot
	  std::string fileNameNoExtension = "TotalOverHeadBytes";
	  std::string FileName        = "TotalOverHeadBytes.png";
	  std::string plotName            = "TotalOverHeadBytes.plt";
	  std::string Title               = "TotalOverHeadBytes Plot";
	  std::string dataplotTitle               = "TotalOverHeadBytes Data";

	  // Instantiate the plot and set its title.
	  Gnuplot plot2 (FileName);
	  plot2.SetTitle (Title);

	  // Make the graphics file, which the plot file will create when it
	  // is used with Gnuplot, be a PNG file.
	  plot2.SetTerminal ("png");

	  // Set the labels for each axis.
	  plot2.SetLegend ("Node IDs","Time Overhead to Synchronize");

	  // Set the range for the x axis.
	  //plot.AppendExtra ("set xrange [0:nNodes]");

	  // Instantiate the dataset, set its title, and make the points be
	  // plotted along with connecting lines.
	  Gnuplot2dDataset dataset2;
	  dataset2.SetTitle (dataplotTitle);
	  dataset2.SetStyle (Gnuplot2dDataset::LINES_POINTS);

	  //double x;
	  //double y;

	  // Create the 2-D dataset.
	  for (int i=0; i<num_scenarios; i++)
		{
		 dataset2.Add ((double) nNodeArray[i]*1.0, mean_overhead[i]);
		}

	  // Add the dataset to the plot.
	  plot2.AddDataset (dataset2);

	  // Open the plot file.
	  std::ofstream plotFile2 (plotName.c_str());

	  // Write the plot file.
	  plot2.GenerateOutput (plotFile2);

	  // Close the plot file.
	  plotFile2.close ();

	  //Create 2-D data and plot
	  std::string fileNameNoExtensionColor = "ColorDistribution";
	  std::string FileNameColor        = "ColorDistribution.png";
	  std::string plotNameColor            = "ColorDistribution.plt";
	  std::string TitleColor               = "ColorDistribution Plot";
	  std::string dataplotTitleColor               = "ColorDistribution Data";

	  // Instantiate the plot and set its title.
	  Gnuplot plot3 (FileNameColor);
	  plot3.SetTitle (TitleColor);

	  // Make the graphics file, which the plot file will create when it
	  // is used with Gnuplot, be a PNG file.
	  plot3.SetTerminal ("png");

	  // Set the labels for each axis.
	  plot3.SetLegend ("Colors","Number of times it was Allocated");

	  // Set the range for the x axis.
	  //plot.AppendExtra ("set xrange [0:nNodes]");

	  // Instantiate the dataset, set its title, and make the points be
	  // plotted along with connecting lines.
	  Gnuplot2dDataset dataset3;
	  dataset3.SetTitle (dataplotTitleColor);
	  dataset3.SetStyle (Gnuplot2dDataset::LINES_POINTS);

	  //double x;
	  //double y;

	  // Create the 2-D dataset.
	  for (int i=0; i<11; i++)
		{
		 dataset3.Add ((double) i*1.0, color_mode_main[i]);
		}

	  // Add the dataset to the plot.
	  plot3.AddDataset (dataset3);

	  // Open the plot file.
	  std::ofstream plotFile3 (plotNameColor.c_str());

	  // Write the plot file.
	  plot3.GenerateOutput (plotFile3);

	  // Close the plot file.
	  plotFile3.close ();

  return 0;
}
