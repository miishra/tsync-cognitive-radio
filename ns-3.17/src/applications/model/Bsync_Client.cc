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

#include "Bsync_Client.h"

#include <fstream>
#include <tuple>

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Bsync_ClientApplication");
NS_OBJECT_ENSURE_REGISTERED (Bsync_Client);

void Bsync_Client::calc_node_t()
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

void Bsync_Client::link_co(int node_id, double snrval)
{
  NS_LOG_FUNCTION (this);
  array_link_co[node_id]=1/snrval;

  /*for (int j=0;j<no_su;j++)
    	  cout << array_link_co[j] << "\t";
  cout << endl;*/
}

/*void Bsync_Client::AddCAT()
{
  NS_LOG_FUNCTION (this);
  int client_CAT[no_su];
  for(int i=0;i<no_su;i++)
	  client_CAT[i]=-1;
}

void Bsync_Client::ReadCAT()
{
  NS_LOG_FUNCTION (this);
}*/

void Bsync_Client::conflict(Ptr<Node> current_node)
{
  NS_LOG_FUNCTION (this);

  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");
  std::cout << "Making of conflict Graph started at Client: " << current_node->GetId() << std::endl;
  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");

  Ipv4GlobalRoutingHelper g;
  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (to_string(current_node->GetId()) + "dynamic-global-routing-client.routes", std::ios::out);
  g.PrintRoutingTableAt (Seconds (0.0), current_node, routingStream);

  for (int i=0;i<tot_su;i++)
	  ConnectedNodeStatus_Client[i]=false;

  for (int i=0;i<Routing_Nodes_Client.size();i++)
  {
	  if (Routing_Nodes_Client[i]!=Ipv4Address("10.1.1.255") && Routing_Nodes_Client[i]!=Ipv4Address("127.0.0.1"))
		  ConnectedNodeStatus_Client[ip_nodeid_hash[Routing_Nodes_Client[i]]]=true;
  }

  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");
  std::cout << "In the Conflict Graph, Following nodes are connected at Client: " << current_node->GetId() << std::endl;
  for (int j=0;j<no_su;j++)
  {
	  if (ConnectedNodeStatus_Client[j]==true && j!= current_node->GetId())
		  cout << "Node: " << j << "\t";
  }
  cout << endl;
  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");

  color_conflict();
}

void Bsync_Client::GetRoutingTableClient(std::vector<Ipv4Address> received_Routing_Nodes, int node_id)
{
	NS_LOG_FUNCTION (this);
	if (node_id==this->GetNode()->GetId() && Routing_Nodes_Client.size()==0)
	{
		//std::cout << "At: " << Simulator::Now ().GetSeconds () << " Received Routing Nodes at Node: " << node_id << std::endl;
		Routing_Nodes_Client=received_Routing_Nodes;
		/*for (int i=0;i<Routing_Nodes_Client.size();i++)
			std::cout << "Connected to Node: " << Routing_Nodes_Client[i] << "\t";
		std::cout << std::endl;*/

	}
}

std::vector<Ipv4Address> Bsync_Client::fetchReoutingNodesClient()
{
  NS_LOG_FUNCTION (this);
  return Routing_Nodes_Client;
}

bool sortbysecond(const tuple<int, double> &a,
               const tuple<int, double> &b)
{
    return (get<1>(a) > get<1>(b));
}

void Bsync_Client::color_conflict()
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
	  client_CAT[i]=250;

  for(int i=0; i<no_su;i++)
	  nodeid_status.push_back(make_tuple(i, array_link_co[i]));
  sort(nodeid_status.begin(), nodeid_status.end(), sortbysecond);

  /*for(int i=0;i<no_su;i++)
    {
	  std::cout << "Received neighbour channel availability for Node: " << i << " is:\n";
  	  for(int j=0;j<11;j++)
  		std::cout << received_neighbour_channel_availability[i][j] << "\t";
  	  std::cout << "\n";
    }*/

  //for(int i = 0; i < nodeid_status.size(); i++)
      //std::cout << get<0>(nodeid_status[i]) << " " << get<1>(nodeid_status[i]) << "\n";

  /*std::cout << "Sent channel availability is:\n";
  for(int i=0;i<no_su;i++)
	  std::cout << sent_neighbour_channel_availability[i] << "\t";
  std::cout << "\n";*/

  for (int i=0;i<no_su;i++)
  {
	  if (i!=m_self_node_id_client)//this->GetNode()->GetId()
	  {
		  for (int j=0;j<11;j++)
		  	  {
		  		  if (ConnectedNodeStatus_Client[i]==true)
		  		  {
		  			  if (received_neighbour_channel_availability[i][j]==sent_neighbour_channel_availability[j])
		  				available_colors[i][j]=1;
		  		  }
		  	  }
	  }
  }

  /*for(int i=0;i<no_su;i++)
        {
    	  std::cout << "Color availability for Node: " << i << " is:\n";
      	  for(int j=0;j<11;j++)
      		std::cout << available_colors[i][j] << "\t";
      	  std::cout << "\n";
        }*/

  for (int i=0;i<no_su;i++)
  {
	  if (get<0>(nodeid_status[i])!=m_self_node_id_client)
	  {
		  if (ConnectedNodeStatus_Client[get<0>(nodeid_status[i])]==true)//|| (neighbour_status_array_client[get<0>(nodeid_status[i])]==this->GetNode()) (neighbour_status_array_client[get<0>(nodeid_status[i])]==-1)
		  {
			  vector<int> avail_colors_this_node;
			  for(int j=0;j<11;j++)
			  {
				  if (available_colors[get<0>(nodeid_status[i])][j])
					  avail_colors_this_node.push_back(j);
			  }

			  //std::cout << get<0>(nodeid_status[i]) << '\t' << avail_colors_this_node.size() << std::endl;

			  if (avail_colors_this_node.size()>0)
			  {
				  vector<int>::iterator randIt = avail_colors_this_node.begin();
				  std::advance(randIt, std::rand() %avail_colors_this_node.size());

				  if (*randIt)
					  client_CAT[get<0>(nodeid_status[i])]= *randIt;
				  else
					  client_CAT[get<0>(nodeid_status[i])]= (uint8_t)std::rand()%10+1;

				  //std::cout << get<0>(nodeid_status[i]) << (int) client_CAT[get<0>(nodeid_status[i])] << std::endl;
				  //avail_colors_this_node.erase(std::remove(avail_colors_this_node.begin(), avail_colors_this_node.end(), *randIt), avail_colors_this_node.end());

				  for(int i=0;i<no_su;i++)
					  available_colors[i][*randIt]=0;

				  childvector.push_back(get<0>(nodeid_status[i]));
			  }
			  else
			  {
				  client_CAT[get<0>(nodeid_status[i])]= (uint8_t)std::rand()%10+1;
				  childvector.push_back(get<0>(nodeid_status[i]));
			  }
		  }
	  }
  }

  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");
  std::cout << "In Coloring of Conflict Graph, Following Colors are allocated to neighbors at Client: " << m_self_node_id_client << std::endl;
  for (int j=0;j<no_su;j++)
  {
	  if (client_CAT[j]!=250 && j!= m_self_node_id_client)
  		  cout << "Node: " << j << " with Color: " << (int) client_CAT[j] << "\t";
  }
  cout << endl;
  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");

  ScheduleCommands (Seconds (0.));

}

TypeId
Bsync_Client::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Bsync_Client")
    .SetParent<Application> ()
    .AddConstructor<Bsync_Client> ()
    .AddAttribute ("MaxPackets",
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&Bsync_Client::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("SimulationDurationClient",
				   "Total duration of the Simulation",
				   DoubleValue (20),
				   MakeDoubleAccessor (&Bsync_Client::stop_time),
				   MakeDoubleChecker<double> ())
    .AddAttribute ("TotalSUClient",
				   "Total Number of Secondary Users.",
				   IntegerValue (10),
				   MakeIntegerAccessor (&Bsync_Client::tot_su),
				   MakeIntegerChecker<int> ())
    .AddAttribute ("Interval",
                   "The time to wait between packets",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&Bsync_Client::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&Bsync_Client::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort",
                   "The destination port of the outbound packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&Bsync_Client::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize", "Size of echo data in outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&Bsync_Client::SetDataSize,
                                         &Bsync_Client::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                   MakeTraceSourceAccessor (&Bsync_Client::m_txTrace))
	.AddTraceSource ("SetSpecAODVCallbackClient"," pass parameters to application ",
				   MakeTraceSourceAccessor (&Bsync_Client::m_SetSpecAODVCallback_Client))
	.AddTraceSource ("SetAllottedColorsCallbackClient"," pass parameters to AODV ",
				   MakeTraceSourceAccessor (&Bsync_Client::m_SetAllottedColorsCallback_Client))
  ;
  return tid;
}

Bsync_Client::Bsync_Client ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_received=0;int current_receive_color;
  int current_send_color;
  m_socket_client = 0;
  m_sendEvent = EventId ();
  m_data = 0;
  m_dataSize = 0;
  m_status = false;
  period=1;
  m_period_count=1;
  //stop_time=20.0;

  ref_node_id=-1;
  last_internal_timer_val=0;
  last_internal_timer_update=0;
  ref_flag=0;
  isSMupdated = false;
  tot_packet_sniffed_rx=0;

  //tot_su=10;

}

void Bsync_Client::MonitorSniffRxCall (Ptr<const Packet> packet, uint16_t channelFreqMhz, uint16_t channelNumber, uint32_t rate, bool isShortPreamble, double signalDbm, double noiseDbm)
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
			//NS_LOG_UNCOND("Got Hello Packet with SNR: " << snrval << " Db for a packet of type: " << ptpt.Get() << " from node: " << ptpt.sending_node_id);
			neighbour_status_array_client[ptpt.sending_node_id]=ptpt.received_color;
			//ConnectedNodeStatus_Client[ptpt.sending_node_id]=true;
			link_co(ptpt.sending_node_id, snrval);
		}
		//TypeHeader tHeader (AODVTYPE_RREQ);
		//packet->RemoveHeader(tHeader);
	}
}

Bsync_Client::~Bsync_Client()
{
  NS_LOG_FUNCTION (this);
  m_socket_client = 0;

  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
}

void
Bsync_Client::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
Bsync_Client::SetRemote (Ipv4Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = Address (ip);
  m_peerPort = port;
}

void
Bsync_Client::SetRemote (Ipv6Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = Address (ip);
  m_peerPort = port;
}

void
Bsync_Client::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
Bsync_Client::MyFunction(SpectrumManager * sm)
{
  NS_LOG_FUNCTION (this);
  m_spectrumManager=sm;

  if (!isSMupdated)
  {
	  Simulator::Schedule (Seconds (0.5), &Bsync_Client::startCG, this);
	  int client_ref_id=0;
	  /*m_free_channels_list = m_spectrumManager->GetListofFreeChannels();
	  int tot_free_channels = m_free_channels_list.size();
	  for(int i = 0; i < tot_free_channels; i++)
	  {
	  	 sent_neighbour_channel_availability[m_free_channels_list[i]]=true;
	  }*/
	  m_SetSpecAODVCallback_Client(m_spectrumManager, sent_neighbour_channel_availability, client_ref_id);
  }

}

void Bsync_Client::ReceivedNeighbourSNR(Ipv4Address source, int node_id, bool ** received_status_array, int sent_hello_messages_till_now)
{
	NS_LOG_FUNCTION (this);

	tot_hello_sent = sent_hello_messages_till_now;

	//std::cout << source << node_id << " " <<  endl;
	ip_nodeid_hash[source] = node_id;

	/*conflict(this->GetNode());

	m_SetAllottedColorsCallback_Client(client_CAT, tot_su);*/
	received_neighbour_channel_availability = received_status_array;

	/*for(int i=0;i<no_su;i++)
	    {
		  std::cout << "Received neighbour channel availability for Node: " << i << " is:\n";
	  	  for(int j=0;j<11;j++)
	  		std::cout << received_neighbour_channel_availability[i][j] << "\t";
	  	  std::cout << "\n";
	    }*/

	//NS_LOG_INFO (source << "\t" << node_id);
}

void
Bsync_Client::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  std::cout << "Client App started at Node: " << this->GetNode()->GetId() << std::endl;

  neighbour_status_array_client = new int[tot_su]();
  received_neighbour_channel_availability = new bool*[tot_su]();
  for(int i = 0; i < tot_su; i++)
    received_neighbour_channel_availability[i] = new bool[11]();

  sent_neighbour_channel_availability = new bool[11]();

  no_su=tot_su;
  no_pu=2;
  ConnectedNodeStatus_Client = new bool[tot_su]();
  current_depth=0;
  client_CAT = new uint8_t[tot_su]();
  array_link_co= new double[tot_su]();
  array_link_adj= new double[tot_su]();
  array_node_wt=0;
  array_net_T=0;
  opt_net_T=0;
  array_net_Intf=0;
  opt_net_Intf=0;
  current_client_receive_color=-1;

  m_self_node_id_client=this->GetNode()->GetId();
  Ptr<Ipv4> ipv4 = this->GetNode()->GetObject<Ipv4> ();
  Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
  ip_nodeid_hash[iaddr.GetLocal()] = this->GetNode()->GetId();
  //CG.conflict();
  //m_spectrumManager->IsChannelAvailable();
  std::ostringstream oss;
  oss << "/NodeList/" << this->GetNode()->GetId() << "/DeviceList/" << "*" << "/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/NewCallbackClient";
  Config::ConnectWithoutContext (oss.str (),MakeCallback (&Bsync_Client::MyFunction,this));

  Config::ConnectWithoutContext ("/NodeList/0/DeviceList/*/Phy/MonitorSnifferRx", MakeCallback (&Bsync_Client::MonitorSniffRxCall, this));

  oss.str("");
  oss.clear();
  oss << "/NodeList/" << this->GetNode()->GetId() << "/$ns3::aodv::RoutingProtocol/HelloReceiveCallback";
  Config::ConnectWithoutContext (oss.str (),MakeCallback (&Bsync_Client::ReceivedNeighbourSNR,this));

  oss.str("");
  oss.clear();
  oss << "/NodeList/" << this->GetNode()->GetId() << "/$ns3::aodv::RoutingProtocol/RoutingNodesCallbackClient";
  Config::ConnectWithoutContext (oss.str (),MakeCallback (&Bsync_Client::GetRoutingTableClient,this));

  if (m_socket_client == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket_client = Socket::CreateSocket (GetNode (), tid);
      m_socket_client->SetAllowBroadcast(true);
      /*if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
        {
          m_socket_client->Bind();
          m_socket_client->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
      else if (Ipv6Address::IsMatchingType(m_peerAddress) == true)
        {
          m_socket_client->Bind6();
          m_socket_client->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort));
        }*/
    }

  m_socket_client->SetRecvCallback (MakeCallback (&Bsync_Client::HandleRead, this));

  //ScheduleCommands (Seconds (0.));Initially Here
  //ScheduleTransmit (Seconds (0.));
}

void
Bsync_Client::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO("Client with node ID: " << this->GetNode()->GetId() << "Sent: " << m_sent << " packets and received: " << m_received << " packets");
  NS_LOG_INFO("Client with node ID: " << this->GetNode()->GetId() << "had final Timestamp: " << Simulator::Now ().GetSeconds ()-1);

  if (m_socket_client != 0)
    {
      m_socket_client->Close ();
      m_socket_client->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket_client = 0;
    }

  Simulator::Cancel (m_sendEvent);
}

void
Bsync_Client::SetDataSize (uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << dataSize);

  //
  // If the client is setting the echo packet data size this way, we infer
  // that she doesn't care about the contents of the packet at all, so
  // neither will we.
  //
  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
  m_size = dataSize;
}

uint32_t
Bsync_Client::GetDataSize (void) const
{
  NS_LOG_FUNCTION (this);
  return m_size;
}

void
Bsync_Client::SetFill (std::string fill)
{
  NS_LOG_FUNCTION (this << fill);

  uint32_t dataSize = fill.size () + 1;

  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memcpy (m_data, fill.c_str (), dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void
Bsync_Client::SetFill (uint8_t fill, uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << fill << dataSize);
  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memset (m_data, fill, dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void
Bsync_Client::SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << fill << fillSize << dataSize);
  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  if (fillSize >= dataSize)
    {
      memcpy (m_data, fill, dataSize);
      m_size = dataSize;
      return;
    }

  //
  // Do all but the final fill.
  //
  uint32_t filled = 0;
  while (filled + fillSize < dataSize)
    {
      memcpy (&m_data[filled], fill, fillSize);
      filled += fillSize;
    }

  //
  // Last fill may be partial
  //
  memcpy (&m_data[filled], fill, dataSize - filled);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void
Bsync_Client::ScheduleTransmit (Time dt, Ptr<Packet> data, int sending_node_id)
{
  NS_LOG_FUNCTION (this << dt);
  m_sendEvent = Simulator::Schedule (dt, &Bsync_Client::Send, this, data, sending_node_id);
}

void
Bsync_Client::ScheduleCommands (Time dt)
{
  NS_LOG_FUNCTION (this << dt);
  m_ControlEvent = Simulator::Schedule (Seconds (0.), &Bsync_Client::Client_Bsync_Logic, this);
}

double Bsync_Client::f_simple(double x)
{
  NS_LOG_FUNCTION (this);
  return log(x);
}

double Bsync_Client::f_inver(double x)
{
  NS_LOG_FUNCTION (this);
  return exp(x);
}

double Bsync_Client::increment_decrement(double x, double y)
{
  NS_LOG_FUNCTION (this);
  /*double e = 0.5;
  double var1, var2;

  var1 = (f_simple(x) + e);
  var2 = f_inver(var1);//doubt8*/
  double epsilon = 0.1;

  return x+epsilon;
}

void Bsync_Client::startCG()
{
  NS_LOG_FUNCTION (this);

  //ConflictG = Conflict_G_Loc(3, 2);
  m_free_channels_list = m_spectrumManager->GetListofFreeChannels();
  int tot_free_channels = m_free_channels_list.size();
  for(int i = 0; i < tot_free_channels; i++)
  {
	  sent_neighbour_channel_availability[m_free_channels_list[i]]=true;
  }
  int client_ref_id=0;
  m_SetSpecAODVCallback_Client(m_spectrumManager, sent_neighbour_channel_availability, client_ref_id);
  //for(int n : free_channels)
	  //std::cout << n << '\n';
  //int tot_free_channels = m_spectrumManager->GetTotalFreeChannelsNow();
  if (tot_free_channels!=0)
	  array_node_wt = 1/(tot_free_channels*1.0);
  //NS_LOG_UNCOND(array_node_wt);

  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");
  std::cout << "Node Weights of conflict Graph updated locally at Client: " << this->GetNode()->GetId() << std::endl;
  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");

  conflict(this->GetNode());

  m_SetAllottedColorsCallback_Client(client_CAT, tot_su);

  isSMupdated = true;

  //std::cout << m_socket_client->GetAllowBroadcast() << std::endl;
}

void
Bsync_Client::Client_Bsync_Logic ()
{
	NS_LOG_FUNCTION (this);
	for(int i=0; i<childvector.size(); i++)
	{
		internal_timer =0;
		++m_sent;
		m_status=true;
		BsyncData Bsync_data_send;
		m_state = SYNCING;
		Bsync_data_send.sender=m_self_node_id_client;
		Bsync_data_send.type = SYNC_PULSE_PACKET;
		Bsync_data_send.s_sent_ts = Simulator::Now ().GetSeconds ();
		uint8_t *buffer = new uint8_t[sizeof(BsyncData)];
		memcpy((char*) buffer, &Bsync_data_send, sizeof(BsyncData));
		Ptr<Packet> data = Create<Packet> (buffer, sizeof(BsyncData));
		m_size = sizeof(BsyncData);
		if (Simulator::Now ().GetSeconds () < stop_time)
			ScheduleTransmit (Seconds (0.), data, childvector[i]);
	}
  Simulator::Schedule (Seconds (period*1.0), &Bsync_Client::Client_Bsync_Logic, this);
}

void
Bsync_Client::Send (Ptr<Packet> data, int sending_node_id)
{
  NS_LOG_FUNCTION (this);

  std::map<Ipv4Address, int>::iterator it;

  Ipv4Address current_Sending_IP;

  //std::cout << ip_nodeid_hash.size() << std::endl;

  for (it = ip_nodeid_hash.begin(); it != ip_nodeid_hash.end(); ++it)
  {
	  if (it->second == sending_node_id)
	  {
		  current_Sending_IP = it->first;
		  break;
	  }
  }

  SetRemote(current_Sending_IP,9);

  //m_socket_client->Cleanup();
  //m_socket_client->Bind();
  m_socket_client->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));

  m_txTrace (data);
  m_socket_client->Send (data);

  uint8_t *buffer = new uint8_t[data->GetSize ()];
  data->CopyData(buffer, data->GetSize ());

  ++m_sent;

  //NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");
  //NS_LOG_UNCOND("Round: " << m_period_count << " of Master Node with Node ID: " << m_self_node_id_client << " Current TimeStamp Value: " << (double)((BsyncData*) buffer)->s_sent_ts);
  m_period_count+=1;

  if (Ipv4Address::IsMatchingType (m_peerAddress))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent " << m_size << " bytes to " <<
                   Ipv4Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort << " with content " << ((BsyncData*) buffer)->type << " with timestamp: " << (double)((BsyncData*) buffer)->s_sent_ts);
    }
  else if (Ipv6Address::IsMatchingType (m_peerAddress))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent " << m_size << " bytes to " <<
                   Ipv6Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort << " with content " << ((BsyncData*) buffer)->type << " with timestamp: " << (double)((BsyncData*) buffer)->s_sent_ts);
    }

  //m_socket_client->Close();

  //m_socket_client->Cleanup();

  /*if (m_sent < m_count)
    {
      ScheduleTransmit (m_interval, data);
    }*/
}

void
Bsync_Client::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
	  ++m_received;
	  uint8_t *buffer = new uint8_t[packet->GetSize ()];
	  packet->CopyData(buffer, packet->GetSize ());
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client received " << packet->GetSize () << " bytes from " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                       InetSocketAddress::ConvertFrom (from).GetPort () << " with content " << ((BsyncData*) buffer)->type << " with timestamp: " << (double)((BsyncData*) buffer)->r_sent_ts);
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client received " << packet->GetSize () << " bytes from " <<
                       Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " <<
                       Inet6SocketAddress::ConvertFrom (from).GetPort () << " with content " << ((BsyncData*) buffer)->type << " with timestamp: " << (double)((BsyncData*) buffer)->r_sent_ts);
        }
    }
}

} // Namespace ns3
