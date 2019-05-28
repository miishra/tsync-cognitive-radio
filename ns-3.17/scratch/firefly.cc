#include "firefly.h"
#include <unistd.h>
#include <sys/time.h>

NS_LOG_COMPONENT_DEFINE ("Firefly");

using namespace ns3;
using namespace std;


double rand_drift_multipier()
{
	double mean = 1;
	double bound = 0.1;
	Ptr<ExponentialRandomVariable> x = CreateObject<ExponentialRandomVariable> ();
	x->SetAttribute ("Mean", DoubleValue (mean));
	x->SetAttribute ("Bound", DoubleValue (bound));
	return x->GetValue ();
}

FireflyUdpAgent::FireflyUdpAgent():m_state(INITIALISING), m_clock_offset(0),
        m_clock_drift_multiplier(rand_drift_multipier()), m_interval(0),
	m_keep_clock_difference_around(0),  m_last_sync_time(localClockCurrent()),
	m_timeout_count(0),m_init_time(0), m_inited_time(0), m_status(false)
{
	bind("interval_", &m_interval);
	bind("keep_clock_difference_around_", &m_keep_clock_difference_around);

	for (int i = 0; i < MESSAGE_TYPE_SIZE; ++i)
	{
		m_received_message_count[i] = 0;
		m_sent_message_count[i] = 0;

	}
	m_firefly_sync_timer.setAgent(this);
}

FireflySyncTimer::FireflySyncTimer()
{
	m_agent = 0;
}

FireflySyncTimer::~FireflySyncTimer()
{
}

void FireflySyncTimer::expire(Ptr<EventImpl> e, Ptr<Socket> m_socket)
{
	m_agent->timeout(0, m_socket);
}
void FireflySyncTimer::setAgent(FireflyUdpAgent *agent)
{
		m_agent = agent;
}

void FireflySyncTimer::resched(double *interval)
{
	*interval=0;
}

void FireflyUdpAgent::timeout(int x, Ptr<Socket> m_socket)
{
    //printf("%02d: funcion Timeout\n", (int)addr());
	if (m_status)
	{
		bool process = true;


	if(process)
	{
		FireflyData Firefly_data;
		Ptr<Packet> data;
		++m_timeout_count;
		ostream *os;
		bool val;//1 for broadcast

		switch (m_state)
		{
			case SYNCING:
                            process = true;

			case INITIALISING:
				//m_state = SYNC_PULSE_PACKET;
				m_state = READY;

			case REQUEST:
                            //pedido de sincronizacion
                                m_init_time = 0 - localClockCurrent();
				//Firefly_data.type = LEVEL_REQUEST_PACKET;
				data = Create<Packet> (sizeof(FireflyData));
				data->CopyData (os, sizeof(FireflyData));
				memcpy((char*) os, &Firefly_data, sizeof(FireflyData));
				val=true;
				sendmsg(sizeof(FireflyData), data, m_socket, val);//IP_BROADCAST
                            	break;

			case READY:
			     //quando esta pronto para 	sincronizar
                        	Firefly_data.sender = 1; //(int) addr()
				Firefly_data.s_sent_ts = localClockCurrent();
				m_state = SYNCING;
				Firefly_data.type = SYNC_PULSE_PACKET;
                                //nuevo paquete de datos
				data = Create<Packet> (sizeof(FireflyData));
				data->CopyData (os, sizeof(FireflyData));
				memcpy((char*) os, &Firefly_data, sizeof(FireflyData));
				val=false;
				sendmsg(sizeof(FireflyData), data, m_socket, val);//m_reference

				break;

                        case NEGOCI:
                                break;

			default:
				break;
			}
		}
	}
	//m_firefly_sync_timer.resched(m_interval);
}

//funcoes para calcular o aumento o disminucao do relogio
double FireflyUdpAgent::f_simple( double x)
{
    return log(x);
}

double FireflyUdpAgent::f_inver( double x)
{
    return exp(x);
}

double FireflyUdpAgent::increment_decrement(double x, double y)
{
    double e = 0.5;
    double funcion, var1, var2;

    var1 = (f_simple(x) + e);
    var2 = f_inver(var1);//doubt

    return var2;

}

void FireflyUdpAgent::recv(Ptr<Packet> pckt, Ptr<Socket> m_socket)
{
	Ptr<Packet> data;
	uint32_t availableData;
  	//availableData = m_socket->GetRxAvailable ();
  	Ptr<Packet> m_receivedPacket = m_socket->Recv (std::numeric_limits<uint32_t>::max (), 0);
  	//NS_ASSERT (availableData == m_receivedPacket->GetSize ());
	if (m_status)
	{
		//printf("enterd recv function\n");
		FireflyData Firefly_data;
		uint8_t *buffer = new uint8_t[pckt->GetSize ()];
		pckt->CopyData (buffer, sizeof(FireflyData));
		memcpy(&Firefly_data, (char*) buffer, sizeof(FireflyData));
		++m_received_message_count[Firefly_data.type];
		bool val=false;

		printf("Receiver received ts: %lf\n", Firefly_data.r_received_ts);
		printf("Receiver sent ts: %lf\n", Firefly_data.r_sent_ts);


            switch (Firefly_data.type)
		{
			case SYNC_PULSE_PACKET:
				if (INITIALISING != m_state)
				{
					Firefly_data.r_received_ts = localClockCurrent();
					Firefly_data.type = SYNC_ACK_PACKET;
					Firefly_data.r_sent_ts = localClock(Simulator::Now().GetDouble());//packet->txtime()
					uint8_t *buffer = new uint8_t[sizeof(FireflyData)];
					memcpy((char*) buffer, &Firefly_data, sizeof(FireflyData));
					data = Create<Packet> (buffer, sizeof(FireflyData));
					sendmsg(sizeof(FireflyData), data, m_socket, val);
				}
				break;
\

			 case REFERENCE_PACKET:

                         if(m_state==INITIALISING)
                         {
                             //reference node

                                //int var = Firefly_data->level + 1;
                                //if (0 < level)
				//{
                                        m_state = READY;
                                        m_init_time += localClockCurrent();
                                        m_inited_time = localClockCurrent();
					val=true;
                                        //incremento_disminucao(m_init_time,m_inited_time);
                                        usleep(10 * (unsigned int) m_clock_drift_multiplier); //vecinos
                                        Firefly_data.sender = 1; //(int) addr()
                                        Firefly_data.type = (messageType) READY;
                                        uint8_t *buffer = new uint8_t[sizeof(FireflyData)];
					memcpy((char*) buffer, &Firefly_data, sizeof(FireflyData));
					data = Create<Packet> (buffer, sizeof(FireflyData));
					sendmsg(sizeof(FireflyData), data, m_socket, val);

                     //           }
                         }
                         else
                         {

                              FireflyData Firefly_data;
                              Firefly_data.sender = 1;//(int) addr()
                               m_state = READY;
                               Firefly_data.type = REFERENCE_PACKET;
                               Firefly_data.s_sent_ts = localClockCurrent();
                               uint8_t *buffer = new uint8_t[sizeof(FireflyData)];
			       memcpy((char*) buffer, &Firefly_data, sizeof(FireflyData));
			       data = Create<Packet> (buffer, sizeof(FireflyData));
			       sendmsg(sizeof(FireflyData), data, m_socket, val);
                           }
                       break;

		case NEGOCI:
			if (REFERENCE_PACKET == m_state)
			{
				m_state = READY;
				m_init_time += localClockCurrent();
				m_inited_time = localClockCurrent();
			}
		break;

	case SYNC_ACK_PACKET:

		if (m_state == SYNCING)
		{

			double clock = localClockCurrent();
		//	double offset = estimatedSourceTimeOffset(Firefly_data->r_received_ts, Firefly_data->s_sent_ts, clock,Firefly_data->r_sent_ts);
		//	double estimated_time = (clock + offset);
		//	setOffset(offset);


			m_state = READY;
		}
		break;

	case SYNC_ACK_DISCONNECT_PACKET:
		if (m_state == SYNCING)
		{
			double clock = localClockCurrent();
			double offset = estimatedSourceTimeOffset(Firefly_data.r_received_ts, Firefly_data.s_sent_ts, clock,Firefly_data.r_sent_ts);
			setOffset(offset);

			//m_state = REQUESTING_LEVEL;
			m_init_time = 0 - localClockCurrent();
			val=true;
			uint8_t *buffer = new uint8_t[sizeof(FireflyData)];
			memcpy((char*) buffer, &Firefly_data, sizeof(FireflyData));
			data = Create<Packet> (buffer, sizeof(FireflyData));
			sendmsg(sizeof(FireflyData), data, m_socket, val);
			//m_Firefly_sync_timer.resched(m_interval);
		}
		break;

	default:
		break;
	}


	//hdr_cmn* packet = hdr_cmn::access(pckt);

	}
	//Packet::free(pckt);
}


void FireflyUdpAgent::sendmsg(int nbytes, Ptr<Packet> data, Ptr<Socket> m_socket, bool val, const char *flags)
{
	if (m_status)
	{
		FireflyData Firefly_data;
		uint8_t *buffer = new uint8_t[data->GetSize ()];
		data->CopyData (buffer, sizeof(FireflyData));
		memcpy(&Firefly_data, (char*) buffer, sizeof(FireflyData));
		++m_sent_message_count[Firefly_data.type];
		uint16_t m_port;

		//printf("Node:%2d-Firefly node sent message\n",(int)addr());

		Ptr<Packet> p;
		int n;
		int size=40;
		assert(size> 0);

		n = nbytes / size;
		//printf("the value of nbytes=%d",nbytes);
		if (nbytes == -1)
		{
			printf("Error:sendmsg() for UDP should not be -1\n");
			return;
		}

		// If they are sending data, then it must fit within a single packet.
		if (data && nbytes > size)
		{
			printf("Error: data greater than maximum UDP packet size\n");
			return;
		}
		double local_time = Simulator::Now().GetDouble();
		while (n-- > 0)
		{
			//printf("entered sendmsg----------\n");
			p = Create<Packet> (buffer, sizeof(FireflyData));
			m_socket->Send (p);
			recv(p, m_socket);
		}
		n = nbytes % size;
		if (n > 0)
		{	//printf("entered sendmsg-@@@@@@@@@@@@@@---------\n");
			p = Create<Packet> (buffer, sizeof(FireflyData));
			m_socket->Send (p);
			recv(p, m_socket);
		}
	}
	//idle();
	//printf("exiting from sendmsg");
 }

void FireflyUdpAgent::command(const char* cmd, Ptr<Socket> m_socket)
{
	Ptr<Packet> data;
	if (strcmp(cmd, "sync") == 0)
	{
	    //NS_LOG_UNCOND ("\nentered command\n");	
            m_status=true;
            FireflyData Firefly_data;
            m_state = SYNCING;
            Firefly_data.type = SYNC_PULSE_PACKET;
            Firefly_data.s_sent_ts = localClockCurrent();
	    //printf("%lf", Firefly_data.s_sent_ts);
	    bool val=true;
	    uint8_t *buffer = new uint8_t[sizeof(FireflyData)];
	    memcpy((char*) buffer, &Firefly_data, sizeof(FireflyData));
	    data = Create<Packet> (buffer, sizeof(FireflyData));
	    sendmsg(sizeof(FireflyData), data, m_socket, val);
            m_firefly_sync_timer.resched(&m_interval);//particular node
	}

        if (strcmp(cmd, "setasGPS") == 0)
	{
		FireflyData Firefly_data;
		m_state = READY;
		//m_reference=1; //(int)addr()

		//Firefly_data.type = LEVEL_DISCOVERY_PACKET;

		Firefly_data.sender = 1; //(int)addr()
		Firefly_data.s_sent_ts = localClockCurrent();
		//printf("sender address=%d,  sent time=%lf\t",Firefly_data.sender,Firefly_data.s_sent_ts);

		//printf("Using m_max_depth: %d\n", m_max_depth);
		//printf("%02d: Init: Setting level to: %d\n", (int)addr(), m_level);
		//printf("\n\n %x\n\n",mac11->pumodel_);
               // printf("the main channel of pu=%d number of data=%d x_loc=%lf\n\n",mac11->pumodel_->pu_data[0].main_channel,mac11->pumodel_->pu_data[0].number_data,mac11->pumodel_->pu_data[0].x_loc);
                //printf("the main channel of pu=%d number of data=%d \n\n",mac11->pumodel_->pu_data[1].main_channel,mac11->pumodel_->pu_data[1].number_data);
		//printf("number of primary users=%d\n",mac11->pumodel_->number_pu_);
		bool val=true;
	        uint8_t *buffer = new uint8_t[sizeof(FireflyData)];
	        memcpy((char*) buffer, &Firefly_data, sizeof(FireflyData));
	        data = Create<Packet> (buffer, sizeof(FireflyData));
	        sendmsg(sizeof(FireflyData), data, m_socket, val);
		printf("exiting from setasroot\n\n");
	}
	if (strcmp(cmd, "initialise") == 0)
	{
		//printf("\nentered initialise command\n");
		//printf("Interval before: %lf\n", m_interval);
		m_interval /= m_clock_drift_multiplier; // Because we use global scheduler
		//printf("Interval after: %lf\n", m_interval);
		//m_Firefly_sync_timer.resched(m_interval);
	}
	if (strcmp(cmd, "setasalive") == 0)
	{
		//printf("init@@@@@@@@@@@@@@@@@@\n");
		m_status = true;
	}

	if (strcmp(cmd, "setaszombie") == 0)
	{
		//printf("init***********\n");
		m_status = false;
	}

	if (strcmp(cmd, "local_clock") == 0)
	{
		int total_sent_message = 0;
		int total_received_message = 0;
		for (int i = 0; i < MESSAGE_TYPE_SIZE; ++i)
		{
			total_received_message += m_received_message_count[i];
			total_sent_message += m_sent_message_count[i];
			//fprintf(fp,"message type=%d,recv msg=%d sent msg=%d\t", i, m_received_message_count[i], m_sent_message_count[i]);
		}


                int TOTAL_MSG_SENT; //int
		TOTAL_MSG_SENT+=total_sent_message;
		int TOTAL_MSG_RECV;
		TOTAL_MSG_RECV+=total_received_message;
                int TOTAL_TIMEOUTS;
		TOTAL_TIMEOUTS+=m_timeout_count;
	}
	if (strcmp(cmd, "time_complexity") == 0)
	{
		printf("%lf %lf %lf\n",m_init_time, m_inited_time,  m_interval * m_clock_drift_multiplier);//(int) addr(),(int) m_reference
	}
}

double FireflyUdpAgent::estimatedSourceTimeOffset(double Ljt4, double Lit2,double Lit7, double Ljt6)

{
	return ((Ljt4 - Lit2) - (Lit7 - Ljt6)) / 2;
}

double FireflyUdpAgent::localClockCurrent()
{
	return (Simulator::Now().GetDouble() * m_clock_drift_multiplier) + m_clock_offset;//NOW
}

double FireflyUdpAgent::localClock(double global_time)
{
	return (global_time * m_clock_drift_multiplier) + m_clock_offset;
}

void FireflyUdpAgent::setOffset(double offset)
{
    double ratio ;
    m_clock_offset += offset;
    m_last_sync_time = localClockCurrent();

    if (m_keep_clock_difference_around > 0 && offset!=0)
    {
         ratio = m_keep_clock_difference_around / offset;
        if (ratio < 0)
        {
            ratio = 0 - ratio;
        }
        if (ratio < 1)
        {
            ratio = 0.99;
        }
        else if (ratio > 1)
        {
        ratio = 1.01;
        }
        else
        {
        ratio = 1;
        }
    }
    else
        ratio= 1;
    m_interval *= ratio;

}

int main (int argc, char *argv[])
{
    SeedManager::SetSeed(1);
  std::string phyMode ("ErpOfdmRate54Mbps");
  int nNodes = 5;

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
    start = start+20;
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
  Ipv4InterfaceContainer interfaceip = ipv4.Assign (devices_control);

  uint16_t port = 9;

  Ptr<UniformRandomVariable> v = CreateObject<UniformRandomVariable> ();
  v->SetAttribute ("Min", DoubleValue (10));
  v->SetAttribute ("Max", DoubleValue (20));

  FireflyUdpAgent fudp;

  Ptr<Socket> socket_array[nNodes];

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  for (int i=0;i<c.GetN();i++)
{
	   socket_array[i] = Socket::CreateSocket (c.Get (i), tid); //receive socket
	   InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 4477+i);
	   socket_array[i]->Bind (local);
	 
	   /*socket_array[2*i] = Socket::CreateSocket (c.Get (i), tid); //sending socket
	   InetSocketAddress remote = InetSocketAddress (interfaceip.GetAddress (i+1), 4577+i);
	   socket_array[2*i]->Connect(remote);*/
}

  for (int i=0;i<c.GetN()-1;i++)
{
	for (int j=i+1;j<c.GetN();j++)
	{
	socket_array[i]->Connect(InetSocketAddress (interfaceip.GetAddress (i+1), 4477+i+1));
	Simulator::Schedule(Seconds (0), &FireflyUdpAgent::command, &fudp, "sync", socket_array[i]);//connect and then sync	
	}
}

for (int i=0;i<c.GetN();i++)
{
	Simulator::Schedule(Seconds (v->GetValue()), &FireflyUdpAgent::command, &fudp, "initialise", socket_array[i]);
}

for (int i=0;i<c.GetN();i++)
{
	Simulator::Schedule(Seconds (0), &FireflyUdpAgent::command, &fudp, "setasalive", socket_array[i]);
}

Simulator::Schedule(Seconds (0), &FireflyUdpAgent::command, &fudp, "setasgps", socket_array[0]);

for (int i=0;i<c.GetN();i++)
{
	Simulator::Schedule(Seconds (20), &FireflyUdpAgent::command, &fudp, "local_clock", socket_array[i]);
}

for (int i=0;i<c.GetN();i++)
{
	Simulator::Schedule(Seconds (20), &FireflyUdpAgent::command, &fudp, "time_complexity", socket_array[i]);
}

  NS_LOG_UNCOND ("Starting CR test");

  Simulator::Stop (Seconds (25.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
