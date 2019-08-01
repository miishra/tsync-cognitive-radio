#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include <string>       // std::string
#include <iostream>     // std::cout
#include <sstream>      // std::ostringstream

#include <iomanip>
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/string.h"
#include "ns3/yans-wifi-helper.h"	`
#include "ns3/packet-sink.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/waveform-generator.h"
#include "ns3/waveform-generator-helper.h"
#include "ns3/non-communicating-net-device.h"
#include "ns3/wifi-net-device.h"

#include "ns3/aodv-module.h"

#include "Bsync_Server.h"

#include <fstream>
#include <tuple>


using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Bsync_ServerApplication");
NS_OBJECT_ENSURE_REGISTERED (Bsync_Server);

std::map<Ipv4Address, int> ip_nodeid_hash_client;

void Bsync_Server::calc_node_t()
{
  NS_LOG_FUNCTION (this);
  double* array_node_wt = new double [no_su];
  for (int i=0;i<no_su;i++)
  {
	  std::vector<int> free_channels = m_spectrumManager->GetListofFreeChannels();
	  int tot_aval_channels = free_channels.size();
	  /*for(int n : free_channels)
		  std::cout << n << '\n';*/
	  //int tot_aval_channels = m_specManager->GetTotalFreeChannelsNow();
	  array_node_wt[i]= (double) 1/tot_aval_channels;
	  NS_LOG_INFO(array_node_wt[i]);
  }
  //m_specManager->IsChannelAvailable();
  //NS_LOG_INFO(m_specManager->m_repository->m_count);
}

void Bsync_Server::link_co(int node_id, double snrval)
{
  NS_LOG_FUNCTION (this);
  array_link_co[node_id]=1/snrval;

  /*for (int j=0;j<no_su;j++)
    	  cout << array_link_co[j] << "\t";
  cout << endl;*/
}

void Bsync_Server::conflict(Ptr<Node> current_node)
{
  NS_LOG_FUNCTION (this);

  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");
  std::cout << "Making of conflict Graph started at Server: " << current_node->GetId() << std::endl;
  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");

  Ipv4GlobalRoutingHelper g;
  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (to_string(current_node->GetId()) + "dynamic-global-routing.txt", std::ios::out);
  g.PrintRoutingTableAt (Seconds (0.0), current_node, routingStream);

  for (int i=0;i<tot_su;i++)
	  ConnectedNodeStatus[i]=false;

  for (int i=0;i<Routing_Nodes.size();i++)
  {
	  if (Routing_Nodes[i]!=Ipv4Address("10.1.1.255") && Routing_Nodes[i]!=Ipv4Address("127.0.0.1"))
		  ConnectedNodeStatus[ip_nodeid_hash_client[Routing_Nodes[i]]]=true;
  }

  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");
  std::cout << "In the Conflict Graph, Following nodes are connected at Server: " << current_node->GetId() << std::endl;
  for (int j=0;j<no_su;j++)
  {
	  if (ConnectedNodeStatus[j]==true && j!= current_node->GetId())
  		  cout << "Node: " << j << "\t";
  }
  cout << endl;
  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");

  if (ref_node_id>-1)
	  color_conflict();
}

bool sortbysec(const tuple<int, double> &a,
               const tuple<int, double> &b)
{
    return (get<1>(a) > get<1>(b));
}

void Bsync_Server::color_conflict()
{
  NS_LOG_FUNCTION (this);
  int available_colors[no_su][11];

  for(int i=0;i<no_su;i++)
    {
  	  for(int j=0;j<11;j++)
  		  available_colors[i][j]=0;
    }

  std::vector<tuple <int , double>> nodeid_status;
  for(int i=0;i<no_su;i++)
  	  server_CAT[i]=250;

  for(int i=0; i<no_su;i++)
	  nodeid_status.push_back(make_tuple(i, array_link_co[i]));
  sort(nodeid_status.begin(), nodeid_status.end(), sortbysec);

  //for(int i = 0; i < no_su; i++)
      //std::cout << neighbour_status_array[i] << "\n";

  for (int i=0;i<no_su;i++)
  {
	  if (i!=m_self_node_id)//this->GetNode()->GetId()
	  {
		  for (int j=0;j<11;j++)
		  	  {
		  		  if (ConnectedNodeStatus[i]==true)
		  		  {
		  			  if (received_neighbour_channel_availability[i][j]==sent_neighbour_channel_availability[j])
		  				available_colors[i][j]=1;
		  		  }
		  	  }
	  }
  }

  for (int i=0;i<no_su;i++)
  {
	  if (get<0>(nodeid_status[i])!=m_self_node_id)
	  {
		  if (ConnectedNodeStatus[get<0>(nodeid_status[i])]==true ) //(neighbour_status_array[get<0>(nodeid_status[i])]==-1)
		  {
				  vector<int> avail_colors_this_node;
				  for(int j=0;j<11;j++)
				  {
					  if (available_colors[get<0>(nodeid_status[i])][j])
						  avail_colors_this_node.push_back(j);
				  }

				  if (avail_colors_this_node.size()>0)
				  {
					  vector<int>::iterator randIt = avail_colors_this_node.begin();
					  std::advance(randIt, std::rand() %avail_colors_this_node.size());

					  if (*randIt)
						  server_CAT[get<0>(nodeid_status[i])]= (uint8_t) *randIt;//available_colors.back()
					  else
						  server_CAT[get<0>(nodeid_status[i])]= (uint8_t)std::rand()%10+1;

					  /*if (get<0>(nodeid_status[i])==3)
						  server_CAT[get<0>(nodeid_status[i])]= (uint8_t)10;*/

					  //transmitasONF(m_socket, get<0>(nodeid_status[i]));
					  //cout << *randIt << std::endl;

					  for(int i=0;i<no_su;i++)
						  available_colors[i][*randIt]=0;

					  server_Vector.push_back(get<0>(nodeid_status[i]));
					  //avail_colors_this_node.erase(randIt);
				  }
				  else
				  {
					  server_CAT[get<0>(nodeid_status[i])]= (uint8_t)std::rand()%10+1;
					  server_Vector.push_back(get<0>(nodeid_status[i]));
				  }
		  }
	  }
  }

  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");
  std::cout << "In Coloring of Conflict Graph, Following Colors are allocated to neighbors at Server: " << m_self_node_id << std::endl;
  for (int j=0;j<no_su;j++)
  {
	  if (server_CAT[j]!=250 && j!= m_self_node_id)
		  cout << "Node: " << j << " with Color: " << (int) server_CAT[j] << "\t";
  }
  cout << endl;
  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");

  //m_SetAllottedColorsCallback_Server(server_CAT, tot_su);

  transmitasONF(m_socket);

}

TypeId
Bsync_Server::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Bsync_Server")
    .SetParent<Application> ()
    .AddConstructor<Bsync_Server> ()
    .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                   UintegerValue (9),
                   MakeUintegerAccessor (&Bsync_Server::m_port),
                   MakeUintegerChecker<uint16_t> ())
   .AddAttribute ("SimulationDuration", "Total duration of the Simulation",
					  DoubleValue (20),
					  MakeDoubleAccessor (&Bsync_Server::stop_time),
					  MakeDoubleChecker<double> ())
   .AddAttribute ("TotalSU", "Total Number of Secondary Users.",
					 IntegerValue (10),
					 MakeIntegerAccessor (&Bsync_Server::tot_su),
					 MakeIntegerChecker<int> ())
   .AddTraceSource ("SetSpecAODVCallback"," pass parameters to application ",
				   MakeTraceSourceAccessor (&Bsync_Server::m_SetSpecAODVCallback))
   .AddTraceSource ("SetAllottedColorsCallbackServer"," pass parameters to AODV ",
				   MakeTraceSourceAccessor (&Bsync_Server::m_SetAllottedColorsCallback_Server))
   /*.AddAttribute ("Node ID", "ID of node on which this sever application is installed.",
					  UintegerValue (1000),
					  MakeUintegerAccessor (&Bsync_Server::m_node_id),
					  MakeUintegerChecker<uint32_t> ())*/
  ;
  return tid;
}

Bsync_Server::Bsync_Server ()
{
  NS_LOG_FUNCTION (this);
  period=1;
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  x->SetAttribute ("Min", DoubleValue (0.0));
  x->SetAttribute ("Max", DoubleValue (1.0));;
  internal_timer = x->GetValue () + Simulator::Now ().GetSeconds ();
  timestamp= x->GetValue();
  m_period_count=1;
  ref_node_id=-1;
  //stop_time=20.0;
  last_internal_timer_val=0;
  last_internal_timer_update=0;
  m_sent=0;
  m_received=0;
  ref_flag=0;
  isSMupdated = false;
  tot_packet_sniffed_rx=0;
  time_to_synchronize=10000;
  synchronized_flag=false;

  overhead_sync=0;
  tot_hello_sent=0;
  tot_sync_sent=0;

  //tot_su=10;

  //cout << internal_timer << endl;
}

void Bsync_Server::MonitorSniffRxCall (Ptr<const Packet> packet, uint16_t channelFreqMhz, uint16_t channelNumber, uint32_t rate, bool isShortPreamble, double signalDbm, double noiseDbm)
{
	//NS_LOG_FUNCTION (this);
	if (packet)
	{
		tot_packet_sniffed_rx++;
		Ptr<Packet> copy = packet->Copy ();
		//RrepHeader rrepHeader;
		//WifiMacHeader mh;//Ipv4Header
		//copy->RemoveHeader (mh);
		PacketTypePacketTag ptpt;
		PacketChannelPacketTag pcpt;
		bool foundpt = packet->PeekPacketTag(ptpt);
		bool foundpc = packet->PeekPacketTag(pcpt);
		//ptpt.Print(std::cout);
		//Ipv4Header iph;
		//copy->RemoveHeader (iph);
		double snrval = 10*log10(pow(10,(signalDbm-30)/10)/pow(10,(noiseDbm-30)/10));
		uint8_t *buffer = new uint8_t[11];
		packet->CopyData(buffer, 11);
		if (ptpt.sending_node_id!=-1)
		{
			//memcpy(&received_neighbour_channel_availability[ptpt.sending_node_id], buffer, 11);
			/*for(int j=0;j<11;j++)
				received_neighbour_channel_availability[ptpt.sending_node_id][j] = (bool)buffer[j];*/
			/*std::cout << "Sent by node: " << ptpt.sending_node_id << " to Node: " << this->GetNode()->GetId() << std::endl;
			for(int j=0;j<11;j++)
				std::cout << received_neighbour_channel_availability[ptpt.sending_node_id][j] << std::endl;*/
			//for(int j=0;j<11;j++)
				//NS_LOG_UNCOND(received_neighbour_channel_availability[ptpt.sending_node_id][j]);
			//NS_LOG_UNCOND("Got Hello Packet with SNR: " << snrval << " Db for a packet of type: " << ptpt.Get() << " from node: " << ptpt.sending_node_id << " with Status: " << ptpt.received_color);
			if (neighbour_status_array[ptpt.sending_node_id]<0)
				neighbour_status_array[ptpt.sending_node_id]=ptpt.received_color;
			ConnectedNodeStatus[ptpt.sending_node_id]=true;
			link_co(ptpt.sending_node_id, snrval);
		}
		//TypeHeader tHeader (AODVTYPE_RREQ);
		//packet->RemoveHeader(tHeader);
	}
}

Bsync_Server::~Bsync_Server()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_socket6 = 0;
}

void
Bsync_Server::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
Bsync_Server::MyFunction(SpectrumManager * sm)
{
  NS_LOG_FUNCTION (this);
  //std::cout << ref_node_id << std::endl;
  m_spectrumManager=sm;
  m_SetSpecAODVCallback(m_spectrumManager, sent_neighbour_channel_availability, ref_node_id);

  /*if (!isSMupdated)
  {
	  m_SetSpecAODVCallback(m_spectrumManager, sent_neighbour_channel_availability, ref_node_id);
  }*/

}

std::vector<Ipv4Address>
Bsync_Server::fetchReoutingNodes()
{
  NS_LOG_FUNCTION (this);
  return Routing_Nodes;
}

void Bsync_Server::ReceivedNeighbourSNR(Ipv4Address source, int node_id, bool ** received_status_array, int sent_hello_messages_til_now)
{
	NS_LOG_FUNCTION (this);

	tot_hello_sent = sent_hello_messages_til_now;

	//std::cout << this->GetNode()->GetId() <<  endl;
	ip_nodeid_hash_client[source] = node_id;


	/*ConflictG.conflict(this->GetNode());

	m_SetAllottedColorsCallback_Server(server_CAT, tot_su);*/
	received_neighbour_channel_availability = received_status_array;
	/*for(int j=0;j<11;j++)
	{
		if (received_neighbour_channel_availability)
			NS_LOG_UNCOND(received_neighbour_channel_availability[1][j]);
	}*/
	//NS_LOG_INFO (source << "\t" << node_id);
}

void
Bsync_Server::StartApplication (void)
{

  NS_LOG_FUNCTION (this);

  overhead_per_hello = 19 + tot_su;
  overhead_sync=40;

  std::cout << "Server App started at Node: " << this->GetNode()->GetId() << std::endl;

  received_neighbour_channel_availability = new bool*[tot_su]();
  neighbour_status_array = new int[tot_su]();
  for(int i = 0; i < tot_su; i++)
	  neighbour_status_array[i]=-1;
  for(int i = 0; i < tot_su; i++)
	  received_neighbour_channel_availability[i] = new bool[11]();

  sent_neighbour_channel_availability = new bool[11]();
  for(int i = 0; i < 11; i++)
	  sent_neighbour_channel_availability[i]=1;

  current_receive_color=-1;
  current_send_color=-1;

  m_self_node_id=-1;

  server_CAT = new uint8_t[tot_su]();
  server_CAT_received = new uint8_t[tot_su]();

  ConnectedNodeStatus = new bool[tot_su]();//num_su+num_pu


  no_su=tot_su;
  no_pu=2;
  current_depth=0;
  allotted_colors = new int[no_su]();
  array_link_co= new double[no_su]();
  array_link_adj= new double[no_su]();
  array_node_wt=0;
  array_net_T=0;
  opt_net_T=0;
  array_net_Intf=0;
  opt_net_Intf=0;

  output_server.open ("results.txt", std::fstream::app);

  //m_SetAllottedColorsCallback_Server(server_CAT, tot_su);

  m_self_node_id=this->GetNode()->GetId();
  Ptr<Ipv4> ipv4 = this->GetNode()->GetObject<Ipv4> ();
  Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
  ip_nodeid_hash_client[iaddr.GetLocal()] = this->GetNode()->GetId();
  //CG.conflict();
  //m_spectrumManager->IsChannelAvailable();
  std::ostringstream oss;
  oss << "/NodeList/" << this->GetNode()->GetId() << "/DeviceList/" << "*" << "/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/NewCallback";
  Config::ConnectWithoutContext (oss.str (),MakeCallback (&Bsync_Server::MyFunction,this));

  Config::ConnectWithoutContext ("/NodeList/0/DeviceList/*/Phy/MonitorSnifferRx", MakeCallback (&Bsync_Server::MonitorSniffRxCall, this));

  oss.str("");
  oss.clear();
  oss << "/NodeList/" << this->GetNode()->GetId() << "/$ns3::aodv::RoutingProtocol/HelloReceiveCallback";
  Config::ConnectWithoutContext (oss.str (),MakeCallback (&Bsync_Server::ReceivedNeighbourSNR,this));

  oss.str("");
  oss.clear();
  oss << "/NodeList/" << this->GetNode()->GetId() << "/$ns3::aodv::RoutingProtocol/ReceivedCATCallbackServer";
  Config::ConnectWithoutContext (oss.str (),MakeCallback (&Bsync_Server::receivedCAT,this));

  oss.str("");
  oss.clear();
  oss << "/NodeList/" << this->GetNode()->GetId() << "/$ns3::aodv::RoutingProtocol/RoutingNodesCallbackServer";
  Config::ConnectWithoutContext (oss.str (),MakeCallback (&Bsync_Server::GetRoutingTable,this));

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      m_socket->Bind (local);
      if (addressUtils::IsMulticast (m_local))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, m_local);
            }
          else
            {
              NS_FATAL_ERROR ("Error: Failed to join multicast group");
            }
        }
    }

  if (m_socket6 == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket6 = Socket::CreateSocket (GetNode (), tid);
      Inet6SocketAddress local6 = Inet6SocketAddress (Ipv6Address::GetAny (), m_port);
      m_socket6->Bind (local6);
      if (addressUtils::IsMulticast (local6))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket6);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, local6);
            }
          else
            {
              NS_FATAL_ERROR ("Error: Failed to join multicast group");
            }
        }
    }

  m_socket->SetRecvCallback (MakeCallback (&Bsync_Server::HandleRead, this));
  m_socket6->SetRecvCallback (MakeCallback (&Bsync_Server::HandleRead, this));
  /*NS_LOG_UNCOND("The value of initial Timestamp of Ordinary Node with ID: " << this->GetNode()->GetId() << " is " << internal_timer);
  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");*/
}

double Bsync_Server::f_simple(double x)
{
  NS_LOG_FUNCTION (this);
  return log(x);
}

double Bsync_Server::f_inver(double x)
{
  NS_LOG_FUNCTION (this);
  return exp(x);
}

double Bsync_Server::increment_decrement(double x, double y)
{
  NS_LOG_FUNCTION (this);
  /*double e = 0.5;
  double var1, var2;

  var1 = (f_simple(x) + e);
  var2 = f_inver(var1);//doubt8*/
  double epsilon = 0.1;

  return x+epsilon;
}

void Bsync_Server::reachedT(Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this);
  //internal_timer=0;
  /*if (ref_node_id>=0)
      transmitasONF(socket, -1);*/
  internal_timer=0;
  last_internal_timer_update = Simulator::Now ().GetSeconds ();
  //last_internal_timer_val=internal_timer;
  if (Simulator::Now ().GetSeconds () < stop_time)
	  m_event=Simulator::Schedule (Seconds(period), &Bsync_Server::reachedT, this, socket);//period
}

void Bsync_Server::startCG()
{
  NS_LOG_FUNCTION (this);

  //ConflictG = ConflictG(10,2);

  m_free_channels_list = m_spectrumManager->GetListofFreeChannels();
  //std::cout << "here" << '\n';
  int tot_free_channels = m_free_channels_list.size();
  for(int i = 0; i < tot_free_channels; i++)
  {
	  sent_neighbour_channel_availability[m_free_channels_list[i]]=true;
  }
  m_SetSpecAODVCallback(m_spectrumManager, sent_neighbour_channel_availability, ref_node_id);
  //int tot_free_channels = m_spectrumManager->GetTotalFreeChannelsNow();
  if (tot_free_channels!=0)
	  array_node_wt = 1/(tot_free_channels*1.0);
  //NS_LOG_UNCOND(ConflictG.array_node_wt);

  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");
  std::cout << "Node Weights of conflict Graph updated locally at Server: " << this->GetNode()->GetId() << std::endl;
  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");

  isSMupdated = true;

  if (ref_node_id>-1)
  {
	  conflict(this->GetNode());

	  m_SetAllottedColorsCallback_Server(server_CAT, tot_su);
  }

}

void Bsync_Server::receivedCAT(uint8_t* received_CAT_server)
{
  NS_LOG_FUNCTION (this);
  server_CAT_received = received_CAT_server;

  if (current_receive_color==-1 && isSMupdated==true)
  {
	  current_receive_color = (int) server_CAT_received[this->GetNode()->GetId()];
	  //if (current_receive_color<20)
		  //m_spectrumManager->SetBsyncColor(current_receive_color);
	  /*if (this->GetNode()->GetId()==6)
		  m_spectrumManager->SetBsyncColor(10);*/
	  std::cout << "Current Receive Color for Node: " << this->GetNode()->GetId() << " is: " << current_receive_color << std::endl;
  }
  //std::cout << "CAT received at Node: " << this->GetNode()->GetId() << " is: " << (int) server_CAT_received[this->GetNode()->GetId()]<< std::endl;
  /*for(int i=0;i<tot_su;i++)
    {
  	  std::cout << (int) server_CAT_received[i] << '\t';
    }
    std::cout << std::endl;*/
}

void Bsync_Server::GetRoutingTable(std::vector<Ipv4Address> received_Routing_Nodes, int node_id)
{
	NS_LOG_FUNCTION (this);
	if (node_id==this->GetNode()->GetId() && Routing_Nodes.size()==0)
	{
		//std::cout << "At: " << Simulator::Now ().GetSeconds () << " Received Routing Nodes at Node: " << node_id << std::endl;
		Routing_Nodes=received_Routing_Nodes;
		/*for (int i=0;i<Routing_Nodes.size();i++)
			std::cout << "Connected to Node: " << Routing_Nodes[i] << "\t";
		std::cout << std::endl;*/

	}
}

void Bsync_Server::transmitasONF(Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this);
	//m_SetAllottedColorsCallback_Server(server_CAT, tot_su);
	for(int i=0; i<server_Vector.size(); i++)
	{
		tot_sync_sent++;
		//internal_timer =0;
		BsyncData Bsync_data;
		m_state = SYNCING;
		Bsync_data.sender=this->GetNode()->GetId();
		Bsync_data.type = SYNC_PULSE_PACKET;
		Bsync_data.s_sent_ts = timestamp;
		uint8_t *buffer = new uint8_t[sizeof(BsyncData)];
		memcpy((char*) buffer, &Bsync_data, sizeof(BsyncData));

		//pcpt.SetAttribute()

		Ptr<Packet> data = Create<Packet> (buffer, sizeof(BsyncData));

		//m_spectrumManager->SetTxColor(server_CAT[server_Vector[i]]);

		//PacketChannelPacketTag pct = ns3::PacketChannelPacketTag((uint16_t) server_CAT[server_Vector[i]]);
		//std::cout << pct.GetInstanceTypeId() << std::endl;
		//data->AddPacketTag(pct);

		m_size = sizeof(BsyncData);

		/*Time next_schedule=Seconds(period);//0.000001
		for(int i=0;i<m_period_count;i++)
			next_schedule+=next_schedule;
		m_period_count+=1;
		NS_LOG_INFO(m_period_count);*/

		std::map<Ipv4Address, int>::iterator it;

		Ipv4Address current_Sending_IP;
		for (it = ip_nodeid_hash_client.begin(); it != ip_nodeid_hash_client.end(); ++it)
		{
			if (it->second == server_Vector[i])
		  	{
		  		current_Sending_IP = it->first;
		  		break;
		  	}
		}

		if (Simulator::Now ().GetSeconds () < stop_time)
		{
			//socket->Cleanup();
			socket->Connect (InetSocketAddress (current_Sending_IP,9));//InetSocketAddress (current_Sending_IP,9)

			//socket->SendTo (data, 0, current_Sending_IP);//Ipv4Address ("255.255.255.255")

			socket->Send(data);

			++m_sent;
			NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s Server sent " << m_size << " bytes to " <<
			current_Sending_IP << " port " << m_port << " with content " << ((BsyncData*) buffer)->type << " with timestamp: " << (double)((BsyncData*) buffer)->s_sent_ts);
		    //Simulator::Schedule (Seconds(period+0.0000001), &Bsync_Server::transmitasONF, this, socket);//period+0.0000001
		}
	}
	timestamp = timestamp + period;
	Simulator::Schedule (Seconds (period*1.0), &Bsync_Server::transmitasONF, this, socket);
}

void
Bsync_Server::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  //NS_LOG_INFO(m_spectrumManager->IsChannelAvailable());
  //Ptr<WifiNetDevice> wd = DynamicCast<WifiNetDevice> (this->GetNode()->GetDevice(0));
  //wd->GetMac();
  if (timestamp > stop_time)
	  timestamp = timestamp - period;
  NS_LOG_INFO("Server with node ID: " << this->GetNode()->GetId() << "Sent: " << m_sent << " packets and received: " << m_received << " packets");
  NS_LOG_INFO("Server with node ID: " << this->GetNode()->GetId() << "had final Timestamp: " << timestamp);
  NS_LOG_INFO("Server with node ID: " << this->GetNode()->GetId() << " took: " << time_to_synchronize << " seconds to synchronize.");
  NS_LOG_INFO("Server with node ID: " << this->GetNode()->GetId() << " Sent following number of Sync messages and Sync replies: " << tot_sync_sent);
  NS_LOG_INFO("Server with node ID: " << this->GetNode()->GetId() << " Sent following number of Hello messages: " << tot_hello_sent);
  NS_LOG_INFO("Server with node ID: " << this->GetNode()->GetId() << " has total protocol overhead: " << tot_hello_sent*overhead_per_hello + tot_sync_sent*overhead_sync);
  output_server << this->GetNode()->GetId() << "\t" << "S" << "\t" << m_sent << "\t" << m_received << "\t" << timestamp << "\t" << time_to_synchronize << "\t" << tot_sync_sent << "\t" << tot_hello_sent << "\t" << tot_hello_sent*overhead_per_hello + tot_sync_sent*overhead_sync << std::endl;
  output_server.close();

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
  if (m_socket6 != 0)
    {
      m_socket6->Close ();
      m_socket6->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
  /*NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");

  NS_LOG_UNCOND("Total number of received Packets Sniffed at node: " << this->GetNode()->GetId() << " is: " << tot_packet_sniffed_rx);
  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");*/

  /*for (int j=0;j<5;j++)
	  cout << ConnectedNodeStatus[j];
  cout << endl;*/

  /*for(auto elem : ip_nodeid_hash)
  {
     std::cout << elem.first << " " << elem.second << "\n";
  }*/

  //Ptr<ns3::Ipv4RoutingHelper> routing = node -> GetObject<RoutingProtocol>();

  //std::cout << "Current Receive Color for Node: " << this->GetNode()->GetId() << " is: " << current_receive_color << std::endl;

}

void
Bsync_Server::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet;
  Address from;
  BsyncData Bsync_data;
  Bsync_data.r_received_ts = Simulator::Now ().GetSeconds ();
  while ((packet = socket->RecvFrom (from)))
    {
	  ++m_received;

	  //std::cout << "Server with Node ID: " << this->GetNode()->GetId() << " Received from: " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " On Color: " << current_receive_color << std::endl;

      uint8_t *buffer = new uint8_t[packet->GetSize ()];
      /*PacketTypePacketTag ptpt;
      PacketChannelPacketTag pcpt;
      bool foundpt = packet->PeekPacketTag(ptpt);
      bool foundpc = packet->PeekPacketTag(pcpt);*/
      packet->CopyData(buffer, packet->GetSize ());
      //std::string s = std::string((char*)buffer);

      if (Simulator::Now().GetSeconds()-((BsyncData*) buffer)->s_sent_ts < 0.5 && synchronized_flag==false)
      {
    	  time_to_synchronize=Simulator::Now().GetSeconds()-1;
    	  synchronized_flag=true;
      }

      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server received " << packet->GetSize () << " bytes from " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                       InetSocketAddress::ConvertFrom (from).GetPort () << " with content " << ((BsyncData*) buffer)->type << " with timestamp: " << (double)((BsyncData*) buffer)->s_sent_ts);
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server received " << packet->GetSize () << " bytes from " <<
                       Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " <<
                       Inet6SocketAddress::ConvertFrom (from).GetPort () << " with content " << ((BsyncData*) buffer)->type << " with timestamp: " << (double)((BsyncData*) buffer)->s_sent_ts);
        }

      packet->RemoveAllPacketTags ();
      packet->RemoveAllByteTags ();

      if (ref_node_id==-1 && ((BsyncData*) buffer)->type==2)
      {
    	  ref_node_id=(int)((BsyncData*) buffer)->sender;
      	  NS_LOG_INFO("At time " << Simulator::Now ().GetSeconds () << " server with node ID: " << this->GetNode()->GetId() << " chose " << ref_node_id << " as their reference node");
          NS_LOG_UNCOND("At time " << Simulator::Now ().GetSeconds () << " Ordinary Node with node ID: " << this->GetNode()->GetId() << " chose " << ref_node_id << " as their reference node and became ONREF");
          NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");
          Simulator::Schedule (Seconds (1.0), &Bsync_Server::startCG, this);
      }

      if ((ref_node_id==-1 || ref_node_id==(int)((BsyncData*) buffer)->sender) && (int)((BsyncData*) buffer)->type==2)
      {
    	  last_internal_timer_val=internal_timer;
		  internal_timer = min(increment_decrement(internal_timer, 0), 1.0);
		  if (last_internal_timer_update==0)
		  {
			  m_event=Simulator::Schedule (Seconds (period-internal_timer), &Bsync_Server::reachedT, this, socket);//period*0.0
			  last_internal_timer_update = Simulator::Now ().GetSeconds ();
		  }
		  else if (Simulator::Now ().GetSeconds () - last_internal_timer_update > period-last_internal_timer_val)
		  {
			  Simulator::Cancel(m_event);
			  m_event=Simulator::Schedule (Seconds (period-internal_timer), &Bsync_Server::reachedT, this, socket);//period*0.0
			  last_internal_timer_update = Simulator::Now ().GetSeconds ();
		  }
		  NS_LOG_INFO("Current value of internal timer is: " << internal_timer);

          if (((BsyncData*) buffer)->s_sent_ts>timestamp)
        	  timestamp = ((BsyncData*) buffer)->s_sent_ts;

          //NS_LOG_UNCOND("Round: " << m_period_count << " of ON with Node ID: " << this->GetNode()->GetId() << " Current TimeStamp Value: " << timestamp);
          //NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");
          m_period_count+=1;

          NS_LOG_LOGIC ("Sending the reply packet");

          tot_sync_sent++;

		  socket->SetAllowBroadcast(true);
		  m_status=true;
		  m_state = READY;
		  Bsync_data.sender=this->GetNode()->GetId();
		  Bsync_data.type = SYNC_ACK_PACKET;
		  Bsync_data.r_sent_ts = Simulator::Now ().GetSeconds ();
		  uint8_t *repbuffer = new uint8_t[sizeof(BsyncData)];
		  memcpy((char*) repbuffer, &Bsync_data, sizeof(BsyncData));
		  Ptr<Packet> data = Create<Packet> (repbuffer, sizeof(BsyncData));
		  m_size = sizeof(BsyncData);

		  //socket->Connect (from);

		  socket->SendTo (data, 0, from);//packet
		  ++m_sent;

		  if (InetSocketAddress::IsMatchingType (from))
		    {
			  NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server sent " << data->GetSize () << " bytes to " <<
						 InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
						 InetSocketAddress::ConvertFrom (from).GetPort ());
		    }
		  else if (Inet6SocketAddress::IsMatchingType (from))
		    {
			  NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server sent " << data->GetSize () << " bytes to " <<
						 Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " <<
						 Inet6SocketAddress::ConvertFrom (from).GetPort ());
		    }
	        //Simulator::Schedule (Seconds (period*1.0), &Bsync_Server::reachedT, this);
      }
      /*if (ref_node_id>=0 && ref_flag==0)
      {
	  ref_flag=1;
    	  Simulator::Schedule (Seconds (0.0), &Bsync_Server::reachedT, this, socket);//period*1.0
      }*/
      //if (ref_flag==0)
          //Simulator::Schedule (Seconds (0.0), &Bsync_Server::reachedT, this);
    }
}

} // Namespace ns3
