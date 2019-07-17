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

Conflict_G_Loc_Client ConflictGC(10,2);
int m_self_node_id_client=0;
bool *ConnectedNodeStatus_Client;
uint8_t* client_CAT;

int current_client_receive_color;

std::vector<int> childvector;

Conflict_G_Loc_Client::Conflict_G_Loc_Client (int num_su, int num_pu)
{
  NS_LOG_FUNCTION (this);
  no_su=num_su;
  no_pu=num_pu;
  ConnectedNodeStatus_Client = new bool[num_su+num_pu]();
  current_depth=0;
  client_CAT = new uint8_t[num_su]();
  array_link_co= new double[num_su]();
  array_link_adj= new double[num_su]();
  array_node_wt=0;
  array_net_T=0;
  opt_net_T=0;
  array_net_Intf=0;
  opt_net_Intf=0;
  current_client_receive_color=-1;
  m_specManager=Bsync_Client::m_spectrumManager;
}

/*TypeId
Conflict_G_Loc_Client::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Conflict_G_Loc_Client")
    .SetParent<Application> ()
    //.AddConstructor<Conflict_G_Loc_Client(int num_su, int num_pu)> ()
	.AddTraceSource ("SetAllottedColorsCallbackClient"," pass parameters to AODV ",
				   MakeTraceSourceAccessor (&Conflict_G_Loc_Client::m_SetAllottedColorsCallback_Client))
  ;
  return tid;
}*/

Conflict_G_Loc_Client::~Conflict_G_Loc_Client ()
{
  NS_LOG_FUNCTION (this);
}

void Conflict_G_Loc_Client::calc_node_t()
{
  NS_LOG_FUNCTION (this);
  double* array_node_wt = new double [no_su];
  for (int i=0;i<no_su;i++)
  {
	  std::vector<int> free_channels = m_specManager->GetListofFreeChannels();
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

void Conflict_G_Loc_Client::link_co(int node_id, double snrval)
{
  NS_LOG_FUNCTION (this);
  array_link_co[node_id]=1/snrval;

  /*for (int j=0;j<no_su;j++)
    	  cout << array_link_co[j] << "\t";
  cout << endl;*/
}

/*void Conflict_G_Loc_Client::AddCAT()
{
  NS_LOG_FUNCTION (this);
  int client_CAT[no_su];
  for(int i=0;i<no_su;i++)
	  client_CAT[i]=-1;
}

void Conflict_G_Loc_Client::ReadCAT()
{
  NS_LOG_FUNCTION (this);
}*/

void Conflict_G_Loc_Client::conflict(Ptr<Node> current_node)
{
  NS_LOG_FUNCTION (this);

  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");
  std::cout << "Making of conflict Graph started at Client: " << current_node->GetId() << std::endl;
  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");

  Ipv4GlobalRoutingHelper g;
  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (to_string(current_node->GetId()) + "dynamic-global-routing-client.routes", std::ios::out);
  g.PrintRoutingTableAt (Seconds (0.0), current_node, routingStream);

  std::ifstream infile(to_string(current_node->GetId()) + "dynamic-global-routing-client.routes");

  std::string line;
  int current_node_id=-1, line_count=1, hop_count;
  std::string dest_ip, link_status, gen_string, delim="\t";
  while (std::getline(infile, line))
  {
  	std::istringstream iss(line);
  	/*if (!(iss >> gen_string))
  	{
  		break;
  	}*/
  	//std::cout << line << endl;
  	if (line_count>3)
  	{
  		size_t pos = 0;
  		std::string token;
  		int place=1;
  		while ((pos = line.find(delim)) != std::string::npos) {
  		    token = line.substr(0, pos);
  		    if (place==1)
  		    	dest_ip.assign(token);
  		    else if (place==4)
  		    	link_status.assign(token);
  		    else if (place==6)
  		    	hop_count=stoi(token);
  		    line.erase(0, pos + delim.length());
  		    place++;
  		}
  	}
  	line_count++;
  	if (link_status.compare("UP")==0 && hop_count==1)
  	{
  		ConnectedNodeStatus_Client[ip_nodeid_hash[Ipv4Address(dest_ip.c_str())]]=true;
  	}
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

  ConflictGC.color_conflict();
}

bool sortbysecond(const tuple<int, double> &a,
               const tuple<int, double> &b)
{
    return (get<1>(a) > get<1>(b));
}

void Conflict_G_Loc_Client::color_conflict()
{
  NS_LOG_FUNCTION (this);
  std::vector<int> available_colors;
  std::vector<tuple <int , double>> nodeid_status;
  for(int i=0;i<no_su;i++)
	  client_CAT[i]=250;

  for(int i=0; i<no_su;i++)
	  nodeid_status.push_back(make_tuple(i, array_link_co[i]));
  sort(nodeid_status.begin(), nodeid_status.end(), sortbysecond);

  //for(int i = 0; i < nodeid_status.size(); i++)
      //std::cout << get<0>(nodeid_status[i]) << " " << get<1>(nodeid_status[i]) << "\n";

  for (int i=0;i<no_su;i++)
  {
	  if (i!=m_self_node_id_client)//this->GetNode()->GetId()
	  {
		  for (int j=0;j<11;j++)
		  	  {
		  		  if (ConnectedNodeStatus_Client[i]==true)
		  		  {
		  			  if (received_neighbour_channel_availability[i][j]==sent_neighbour_channel_availability[j])
		  				  available_colors.push_back(j);
		  		  }
		  	  }
	  }
  }

  for (int i=0;i<no_su;i++)
  {
	  if (get<0>(nodeid_status[i])!=m_self_node_id_client)
	  {
		  if (ConnectedNodeStatus_Client[get<0>(nodeid_status[i])]==true)//|| (neighbour_status_array_client[get<0>(nodeid_status[i])]==this->GetNode()) (neighbour_status_array_client[get<0>(nodeid_status[i])]==-1)
		  {
			  vector<int>::iterator randIt = available_colors.begin();
			  std::advance(randIt, std::rand() %available_colors.size());
			  client_CAT[get<0>(nodeid_status[i])]= *randIt;//available_colors.back()
			  //client_CAT[get<0>(nodeid_status[i])] = *randIt;
			  //std::cout << get<0>(nodeid_status[i]) << (int) client_CAT[get<0>(nodeid_status[i])] << std::endl;
			  available_colors.erase(std::remove(available_colors.begin(), available_colors.end(), *randIt), available_colors.end());
			  childvector.push_back(get<0>(nodeid_status[i]));
			  //ScheduleCommands (Seconds (0.), get<0>(nodeid_status[i]));
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
  m_socket = 0;
  m_sendEvent = EventId ();
  m_data = 0;
  m_dataSize = 0;
  m_status = false;
  period=1;
  m_period_count=1;
  stop_time=20.0;

  ref_node_id=-1;
  last_internal_timer_val=0;
  last_internal_timer_update=0;
  ref_flag=0;
  isSMupdated = false;
  tot_packet_sniffed_rx=0;

  tot_su=10;

  neighbour_status_array_client = new int[tot_su]();
  received_neighbour_channel_availability = new bool*[tot_su]();
  for(int i = 0; i < tot_su; i++)
	  received_neighbour_channel_availability[i] = new bool[11]();

  sent_neighbour_channel_availability = new bool[11]();
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
			ConnectedNodeStatus_Client[ptpt.sending_node_id]=true;
			ConflictGC.link_co(ptpt.sending_node_id, snrval);
		}
		//TypeHeader tHeader (AODVTYPE_RREQ);
		//packet->RemoveHeader(tHeader);
	}
}

Bsync_Client::~Bsync_Client()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;

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
	  m_SetSpecAODVCallback_Client(m_spectrumManager, sent_neighbour_channel_availability, client_ref_id);
  }

}

void Bsync_Client::ReceivedNeighbourSNR(Ipv4Address source, int node_id, bool** received_status_array)
{
	NS_LOG_FUNCTION (this);
	//std::cout << this->GetNode()->GetId() <<  endl;
	ip_nodeid_hash[source] = node_id;

	/*ConflictGC.conflict(this->GetNode());

	m_SetAllottedColorsCallback_Client(client_CAT, tot_su);*/
	received_neighbour_channel_availability = received_status_array;
	/*for(int j=0;j<11;j++)
	{
		if (received_neighbour_channel_availability)
			NS_LOG_UNCOND(received_neighbour_channel_availability[1][j]);
	}*/
	//NS_LOG_INFO (source << "\t" << node_id);
}

void
Bsync_Client::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  std::cout << "Client App started at Node: " << this->GetNode()->GetId() << std::endl;

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

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      m_socket->SetAllowBroadcast(true);
      if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
        {
          m_socket->Bind();
          m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
      else if (Ipv6Address::IsMatchingType(m_peerAddress) == true)
        {
          m_socket->Bind6();
          m_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
    }

  m_socket->SetRecvCallback (MakeCallback (&Bsync_Client::HandleRead, this));

  //ScheduleCommands (Seconds (0.));Initially Here
  //ScheduleTransmit (Seconds (0.));
}

void
Bsync_Client::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO("Client with node ID: " << this->GetNode()->GetId() << "Sent: " << m_sent << " packets and received: " << m_received << " packets");
  NS_LOG_INFO("Client with node ID: " << this->GetNode()->GetId() << "had final Timestamp: " << Simulator::Now ().GetSeconds ()-1);

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
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
	  ConflictGC.array_node_wt = 1/(tot_free_channels*1.0);
  //NS_LOG_UNCOND(ConflictGC.array_node_wt);

  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");
  std::cout << "Node Weights of conflict Graph updated locally at Client: " << this->GetNode()->GetId() << std::endl;
  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");

  ConflictGC.conflict(this->GetNode());

  m_SetAllottedColorsCallback_Client(client_CAT, tot_su);

  isSMupdated = true;
}

void
Bsync_Client::Client_Bsync_Logic ()
{
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

  //NS_ASSERT (m_sendEvent.IsExpired ());

  /*Ptr<Packet> p;
  if (m_dataSize)
    {
      //
      // If m_dataSize is non-zero, we have a data buffer of the same size that we
      // are expected to copy and send.  This state of affairs is created if one of
      // the Fill functions is called.  In this case, m_size must have been set
      // to agree with m_dataSize
      //
      NS_ASSERT_MSG (m_dataSize == m_size, "Bsync_Client::Send(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "Bsync_Client::Send(): m_dataSize but no m_data");
      p = Create<Packet> (m_data, m_dataSize);
    }
  else
    {
      //
      // If m_dataSize is zero, the client has indicated that she doesn't care
      // about the data itself either by specifying the data size by setting
      // the corresponding atribute or by not calling a SetFill function.  In
      // this case, we don't worry about it either.  But we do allow m_size
      // to have a value different from the (zero) m_dataSize.
      //
      p = Create<Packet> (m_size);
    }*/
  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well

  std::map<Ipv4Address, int>::const_iterator it;

  Ipv4Address current_Sending_IP;
  for (it = ip_nodeid_hash.begin(); it != ip_nodeid_hash.end(); ++it)
  {
	  //std::cout << it->first << std::endl;
	  if (it->second == sending_node_id)
	  {
		  current_Sending_IP = it->first;
		  break;
	  }
  }
  SetRemote(current_Sending_IP,9);

  m_txTrace (data);
  //std::cout << current_Sending_IP << std::endl;
  m_socket->Send (data);

  uint8_t *buffer = new uint8_t[data->GetSize ()];
  data->CopyData(buffer, data->GetSize ());

  ++m_sent;

  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");
  NS_LOG_UNCOND("Round: " << m_period_count << " of Master Node with Node ID: " << m_self_node_id_client << " Current TimeStamp Value: " << (double)((BsyncData*) buffer)->s_sent_ts);
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
