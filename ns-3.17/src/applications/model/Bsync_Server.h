#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/Bsync_Common_headers.h"
#include "ns3/spectrum-manager.h"
#include "ns3/aodv-rtable.h"
#include <map>
#include <iostream>
#include <cassert>

namespace ns3 {

class Socket;
class Packet;

class Bsync_Server : public Application
{
public:
  static TypeId GetTypeId (void);
  Bsync_Server ();
  virtual ~Bsync_Server ();
  SpectrumManager * m_spectrumManager;
  std::map<Ipv4Address, int> ip_nodeid_hash;
  std::vector<int> m_free_channels_list;
  bool** received_neighbour_channel_availability;
  bool* sent_neighbour_channel_availability;
  int* neighbour_status_array;
  int ref_node_id;
  void startCG();
  void receivedCAT(uint8_t* received_CAT_server);

  TracedCallback<uint8_t*, int> m_SetAllottedColorsCallback_Server;

  Ptr<Socket> m_socket;
  Ptr<Socket> m_socket6;

  void reachedT(Ptr<Socket> socket);
  void transmitasONF(Ptr<Socket> socket, int sending_node_id);

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void HandleRead (Ptr<Socket> socket);
  double f_simple( double x);
  double f_inver( double x);
  double increment_decrement(double x, double y);
  void MyFunction(SpectrumManager * sm);
  void MonitorSniffRxCall (Ptr<const Packet> packet, uint16_t channelFreqMhz, uint16_t channelNumber, uint32_t rate, bool isShortPreamble, double signalDbm, double noiseDbm);
  TracedCallback<SpectrumManager *, bool *, int> m_SetSpecAODVCallback;
  TracedCallback<Ipv4Address, int> m_MyHelloReceiveCallback;
  void ReceivedNeighbourSNR(Ipv4Address source, int node_id, bool ** received_status_array);
  void GetRoutingTable();

  uint16_t m_port;
  int tot_su;
  bool isSMupdated;
  uint32_t m_sent;
  uint32_t m_received;
  int m_period_count;
  Address m_local;
  nState m_state;
  bool m_status;
  uint32_t period;
  uint32_t m_size;
  uint32_t m_node_id;
  int tot_packet_sniffed_rx;
  double stop_time;
  double internal_timer;
  double last_internal_timer_update;
  double last_internal_timer_val;
  double timestamp;
  int ref_flag;
  EventId m_event;
  //RoutingTable m_routingTableApp;
};

class Conflict_G_Loc : public Bsync_Server, public Application
{
public:
	Conflict_G_Loc(int num_su, int num_pu);
	~Conflict_G_Loc();
	void get_current_round();
	void get_no_su();
	void set_no_su();
	void get_no_pu();
	void set_no_pu();
	int no_su;
	int no_pu;
	int current_depth;
	double* array_link_co;
	double* array_link_adj;
	double array_node_wt;
	int* allotted_colors;
	double array_net_T;
	double opt_net_T;
	double array_net_Intf;
	double opt_net_Intf;
	SpectrumManager *m_specManager;
	void calc_node_t();
	void Obj();
    void link_co(int node_id, double snrval);
    void link_adj();
    double calc_backoff_cond();
    void exec_backoff_app();
    void conflict(Ptr<Node> current_node);
    void color_conflict();
private:
    void get_next_heuristic();
    void stop_current_round();
};

} // namespace ns3

