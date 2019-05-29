#include "Bsync_App-helper.h"
#include "ns3/Bsync_Server.h"
#include "ns3/Bsync_Client.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

Bsync_ServerHelper::Bsync_ServerHelper (uint16_t port)
{
  m_factory.SetTypeId (Bsync_Server::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void
Bsync_ServerHelper::SetAttribute (
  std::string name,
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
Bsync_ServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
Bsync_ServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
Bsync_ServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
Bsync_ServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<Bsync_Server> ();
  node->AddApplication (app);

  return app;
}

Bsync_ClientHelper::Bsync_ClientHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (Bsync_Client::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
}

Bsync_ClientHelper::Bsync_ClientHelper (Ipv4Address address, uint16_t port)
{
  m_factory.SetTypeId (Bsync_Client::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port));
}

Bsync_ClientHelper::Bsync_ClientHelper (Ipv6Address address, uint16_t port)
{
  m_factory.SetTypeId (Bsync_Client::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port));
}

void
Bsync_ClientHelper::SetAttribute (
  std::string name,
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
Bsync_ClientHelper::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<Bsync_Client>()->SetFill (fill);
}

void
Bsync_ClientHelper::SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength)
{
  app->GetObject<Bsync_Client>()->SetFill (fill, dataLength);
}

void
Bsync_ClientHelper::SetFill (Ptr<Application> app, uint8_t *fill, uint32_t fillLength, uint32_t dataLength)
{
  app->GetObject<Bsync_Client>()->SetFill (fill, fillLength, dataLength);
}

ApplicationContainer
Bsync_ClientHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
Bsync_ClientHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
Bsync_ClientHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
Bsync_ClientHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<Bsync_Client> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
