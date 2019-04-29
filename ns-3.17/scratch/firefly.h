#ifndef NS_FIREFLY_H
#define NS_FIREFLY_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/aodv-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/packet-sink.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/net-device.h"
#include "ns3/object-vector.h"
#include "ns3/trace-source-accessor.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <functional>

#define NOW Scheduler::instance().clock()

#define neighbour_size  4

using namespace ns3;

typedef int32_t nsaddr_t;

struct ns_addr_t {
   int32_t addr_;
   int32_t port_;
};

enum messageType
{
	INITIALIZATION_PACKET,
	REFERENCE_PACKET,
	SYNC_PULSE_PACKET,
	SYNC_ACK_PACKET,
	SYNC_REQUEST_PACKET,
	SYNC_ACK_DISCONNECT_PACKET,
	MESSAGE_TYPE_SIZE, //FINAL
};

enum nodeState
{
	INITIALISING,	
	READY,
	SYNCING,
        REQUEST,
        NEGOCI,
};

struct ReferenceNode
{
	int ID_node;
	bool status;
	int descendant_count;
        float value;
	//ReferenceNode();
        ReferenceNode(): ID_node(0), status(true)
	{
	}

};

class FireflyUdpAgent;

class FireflySyncTimer
{
    public:
            FireflySyncTimer();
            ~FireflySyncTimer();
            void setAgent(FireflyUdpAgent *agent);
    protected:
            void expire(Ptr<EventImpl> event, Ptr<Node> node);
            FireflyUdpAgent *m_agent;
    private:
};

class FireflyUdpAgent
{
public:
	FireflyUdpAgent();
	void sendmsg(int nbytes, Ptr<Packet> data, Ptr<Node> node, bool val, const char *flags = 0);
	void recv(Ptr<Packet> pckt, Ptr<Node> node);
	void command(const char* cmd, Ptr<Node> node);
	void timeout(int, Ptr<Node> node);
protected:
	static double estimatedSourceTimeOffset(double Ljt4, double Lit2,double Lit7, double Ljt6);
	double localClockCurrent();
	double localClock(double global_clock);
	void setOffset(double offset);	
	bool setAsReferenceNode(int ref_node_id, int descendant_count = 0);
	//bool updateMyChild(int child_node_id, int descendant_count = 0);
	bool removeFromReferenceNode(int child_node_id);
	float decrease();
        float increase();
	//bool disconnectNodeToFormNewCluster(int child_node_id);
	bool connectNodeToCluster(int child_node_id);
        double increment_decrement(double,double);
        double f_simple(double);
	double f_inver(double);
        
	nodeState m_state;
	ns_addr_t m_ref;
	double m_clock_offset;
	double m_clock_drift_multiplier;
	FireflySyncTimer m_firefly_sync_timer;
	double m_interval;
	double m_keep_clock_difference_around;
	bool m_ref_sync_timer;
	double m_last_sync_time;
	int m_received_message_count[MESSAGE_TYPE_SIZE];
	int m_sent_message_count[MESSAGE_TYPE_SIZE];
	int m_timeout_count;
	int m_sync_fail_count;
	double m_init_time;
	double m_inited_time;
	bool m_status;
	ReferenceNode *m_ref_pointer;
	int available_channel_list[MAX_CHANNELS] ;
};
struct FireflyData
{
	messageType type;	
	int sender;
	double s_sent_ts;
	double r_received_ts;
	double r_sent_ts;
	int descendant_count;
	
};
#endif
