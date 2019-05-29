#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"

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

  uint16_t m_port;
  Ptr<Socket> m_socket;
  Ptr<Socket> m_socket6;
  Address m_local;
};

} // namespace ns3

