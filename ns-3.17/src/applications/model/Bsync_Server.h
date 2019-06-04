#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/Bsync_Common_headers.h"

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup applications
 * \defgroup Bsync_ Bsync_
 */

/**
 * \ingroup Bsync_
 * \brief A Udp Echo server
 *
 * Every packet received is sent back.
 */
class Bsync_Server : public Application
{
public:
  static TypeId GetTypeId (void);
  Bsync_Server ();
  virtual ~Bsync_Server ();

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void HandleRead (Ptr<Socket> socket);
  double f_simple( double x);
  double f_inver( double x);
  double increment_decrement(double x, double y);
  void reachedT();

  uint16_t m_port;
  int m_period_count;
  int ref_node_id;
  Ptr<Socket> m_socket;
  Ptr<Socket> m_socket6;
  Address m_local;
  nState m_state;
  bool m_status;
  uint32_t period;
  uint32_t m_size;
  uint32_t m_node_id;
  double stop_time;
  double internal_timer;
};

} // namespace ns3

