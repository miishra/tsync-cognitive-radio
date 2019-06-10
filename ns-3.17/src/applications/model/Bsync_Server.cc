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

#include "Bsync_Server.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Bsync_ServerApplication");
NS_OBJECT_ENSURE_REGISTERED (Bsync_Server);

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
  stop_time=20.0;
  last_internal_timer_update=0;
  m_sent=0;
  m_received=0;
  ref_flag=0;
  //cout << internal_timer << endl;
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
Bsync_Server::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

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
  NS_LOG_UNCOND("The value of initial Timestamp of Ordinary Node with ID: " << this->GetNode()->GetId() << " is " << internal_timer);
  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");
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
  if (ref_node_id>=0)
      transmitasONF(socket);
  if (Simulator::Now ().GetSeconds () < stop_time)
	  Simulator::Schedule (Seconds(period), &Bsync_Server::reachedT, this, socket);//period
}

void Bsync_Server::transmitasONF(Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this);
  //internal_timer =0;
  BsyncData Bsync_data;
  m_state = SYNCING;
  Bsync_data.sender=this->GetNode()->GetId();
  Bsync_data.type = SYNC_PULSE_PACKET;
  Bsync_data.s_sent_ts = timestamp;
  uint8_t *buffer = new uint8_t[sizeof(BsyncData)];
  memcpy((char*) buffer, &Bsync_data, sizeof(BsyncData));
  Ptr<Packet> data = Create<Packet> (buffer, sizeof(BsyncData));
  m_size = sizeof(BsyncData);

  /*Time next_schedule=Seconds(period);//0.000001
  for(int i=0;i<m_period_count;i++)
  	  next_schedule+=next_schedule;
  m_period_count+=1;
  NS_LOG_INFO(m_period_count);*/

  if (Simulator::Now ().GetSeconds () < stop_time)
  {
	  socket->SendTo (data, 0, Ipv4Address ("255.255.255.255"));
	  ++m_sent;
	  NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s Server sent " << m_size << " bytes to " <<
			  Ipv4Address ("255.255.255.255") << " port " << m_port << " with content " << ((BsyncData*) buffer)->type << " with timestamp: " << (double)((BsyncData*) buffer)->s_sent_ts);
      //Simulator::Schedule (Seconds(period+0.0000001), &Bsync_Server::transmitasONF, this, socket);//period+0.0000001
  }
}

void
Bsync_Server::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO("Server with node ID: " << this->GetNode()->GetId() << "Sent: " << m_sent << " packets and received: " << m_received << " packets");
  NS_LOG_INFO("Server with node ID: " << this->GetNode()->GetId() << "had final Timestamp: " << timestamp);

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
  NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");
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
      uint8_t *buffer = new uint8_t[packet->GetSize ()];
      packet->CopyData(buffer, packet->GetSize ());
      //std::string s = std::string((char*)buffer);
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
    	  ref_node_id=((BsyncData*) buffer)->sender;
      	  NS_LOG_INFO("At time " << Simulator::Now ().GetSeconds () << " server with node ID: " << this->GetNode()->GetId() << " chose " << ref_node_id << " as their reference node");
          NS_LOG_UNCOND("At time " << Simulator::Now ().GetSeconds () << " Ordinary Node with node ID: " << this->GetNode()->GetId() << " chose " << ref_node_id << " as their reference node and became ONREF");
          NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");
      }

      if ((ref_node_id==-1 || ref_node_id==((BsyncData*) buffer)->sender) && ((BsyncData*) buffer)->type==2)
      {
	  internal_timer = min(increment_decrement(internal_timer, 0), 1.0);
	  NS_LOG_INFO("Current value of internal timer is: " << internal_timer);
          timestamp = ((BsyncData*) buffer)->s_sent_ts;
          NS_LOG_UNCOND("Round: " << m_period_count << " of ON with Node ID: " << this->GetNode()->GetId() << " Current TimeStamp Value: " << timestamp);
          //NS_LOG_UNCOND("\n-----------------------------------------------------------------------------------------------\n");
          m_period_count+=1;

          NS_LOG_LOGIC ("Sending the reply packet");
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
      if (ref_node_id>=0 && ref_flag==0)
      {
	  ref_flag=1;
    	  Simulator::Schedule (Seconds (0.0), &Bsync_Server::reachedT, this, socket);//period*1.0
      }
      //if (ref_flag==0)
          //Simulator::Schedule (Seconds (0.0), &Bsync_Server::reachedT, this);
    }
}

} // Namespace ns3
