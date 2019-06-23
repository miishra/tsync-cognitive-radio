#include "ns3/watchdog.h"
#include "ns3/core-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("NS Watchdog timer");

class WatchdogTestCase
{
public:
  WatchdogTestCase ();
  void DoRun ();
  void Expire (Time expected);
  bool m_expired;
  Time m_expiredTime;
  Time m_expiredArgument;
};

WatchdogTestCase::WatchdogTestCase()
{
  NS_LOG_INFO("Check that we can keepalive a watchdog");
}

void
WatchdogTestCase::Expire (Time expected)
{
  m_expired = true;
  m_expiredTime = Simulator::Now ();
  NS_LOG_INFO(m_expiredTime);
  m_expiredArgument = expected;
}

void
WatchdogTestCase::DoRun ()
{
  m_expired = false;
  NS_LOG_INFO("DoRun function");
  m_expiredArgument = Seconds (0);
  m_expiredTime = Seconds (0);
  Watchdog watchdog;
  watchdog.SetFunction (&WatchdogTestCase::Expire, this);
  watchdog.SetArguments (MicroSeconds (40));
  watchdog.Ping (MicroSeconds (10));
  Simulator::Schedule (MicroSeconds (5), &Watchdog::Ping, &watchdog, MicroSeconds (100));
  Simulator::Schedule (MicroSeconds (20), &Watchdog::Ping, &watchdog, MicroSeconds (100));
  Simulator::Schedule (MicroSeconds (23), &Watchdog::Ping, &watchdog, MicroSeconds (100));
  Simulator::Run ();
  Simulator::Destroy ();
  //NS_TEST_ASSERT_MSG_EQ (m_expired, true, "The timer did not expire ??");
  //NS_TEST_ASSERT_MSG_EQ (m_expiredTime, MicroSeconds (40), "The timer did not expire at the expected time ?");
  //NS_TEST_ASSERT_MSG_EQ (m_expiredArgument, MicroSeconds (40), "We did not get the right argument");
}

int main (int argc, char *argv[])
{
  WatchdogTestCase watchdog_timer = WatchdogTestCase ();
  watchdog_timer.DoRun();
  //Simulator::Schedule (Seconds (0.0), &WatchdogTestCase::DoRun, &watchdog_timer);
}
