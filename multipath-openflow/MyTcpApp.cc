/*
 * MyTcpApp.cpp
 *
 *  Created on: Mar 15, 2015
 *      Author: ws
 */

#include "MyTcpApp.h"

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tcp-socket-factory.h"

NS_LOG_COMPONENT_DEFINE ("MyTcpApp");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (MyTcpApp);

TypeId
MyTcpApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MyTcpApp")
    .SetParent<Application> ()
    .AddConstructor<MyTcpApp> ()
    .AddAttribute ("SendSize", "The amount of data to send each time.",
                   UintegerValue (512),
                   MakeUintegerAccessor (&MyTcpApp::m_sendSize),
                   MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("Remote", "The address of the destination",
                   AddressValue (),
                   MakeAddressAccessor (&MyTcpApp::m_peer),
                   MakeAddressChecker ())
    .AddAttribute ("MaxBytes",
                   "The total number of bytes to send. "
                   "Once these bytes are sent, "
                   "no data  is sent again. The value zero means "
                   "that there is no limit.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&MyTcpApp::m_maxBytes),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Protocol", "The type of protocol to use.",
                   TypeIdValue (TcpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&MyTcpApp::m_tid),
                   MakeTypeIdChecker ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&MyTcpApp::m_txTrace))
    .AddTraceSource("TransByte","The total byte transmitted",
    		         MakeTraceSourceAccessor(&MyTcpApp::m_transBytes))
     ;
  return tid;
}


MyTcpApp::MyTcpApp ()
  : m_socket (0),
    m_connected (false)
{
  NS_LOG_FUNCTION (this);
}

MyTcpApp::~MyTcpApp ()
{
  NS_LOG_FUNCTION (this);
  m_socket=0;
}
void
MyTcpApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_sendSize = packetSize;
  m_maxBytes = 0;
//  m_dataRate = dataRate;
}
void
MyTcpApp::SetMaxBytes (uint32_t maxBytes)
{
  NS_LOG_FUNCTION (this << maxBytes);
  m_maxBytes = maxBytes;
}

Ptr<Socket>
MyTcpApp::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

void
MyTcpApp::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket = 0;
  // chain up
  Application::DoDispose ();
}

// Application Methods
void MyTcpApp::StartApplication (void) // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO("start TCP application");
  // Create the socket if not already
  if (m_socket)
    {


      if (Inet6SocketAddress::IsMatchingType (m_peer))
        {
          m_socket->Bind6 ();
        }
      else if (InetSocketAddress::IsMatchingType (m_peer))
        {
          m_socket->Bind ();
        }

      m_socket->Connect (m_peer);
      m_socket->ShutdownRecv ();
      m_socket->SetConnectCallback (
        MakeCallback (&MyTcpApp::ConnectionSucceeded, this),
        MakeCallback (&MyTcpApp::ConnectionFailed, this));
      m_socket->SetSendCallback (
        MakeCallback (&MyTcpApp::DataSend, this));
      m_socket->SetCloseCallbacks(
    	MakeCallback (&MyTcpApp::NormalClose, this),
    	MakeCallback (&MyTcpApp::ErrorClose, this));
    }
  if (m_connected)
    {
      SendData ();
    }
}

void MyTcpApp::StopApplication (void) // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_connected = false;
    }
  else
    {
      NS_LOG_WARN ("MyTcpApp found null socket to close in StopApplication");
    }
}


// Private helpers

void MyTcpApp::SendData (void)
{
  NS_LOG_FUNCTION (this);

  while (m_maxBytes == 0 || m_totBytes < m_maxBytes)
    { // Time to send more
      uint32_t toSend = m_sendSize;
      // Make sure we don't send too many
      if (m_maxBytes > 0)
        {
          toSend = std::min (m_sendSize, m_maxBytes - m_totBytes);
        }
      NS_LOG_LOGIC ("sending packet at " << Simulator::Now ());
      Ptr<Packet> packet = Create<Packet> (toSend);
      m_txTrace (packet);
      int actual = m_socket->Send (packet);
      if (actual > 0)
        {
          m_totBytes += actual;
        }
      // We exit this loop when actual < toSend as the send side
      // buffer is full. The "DataSent" callback will pop when
      // some buffer space has freed ip.
      if ((unsigned)actual != toSend)
        {
          break;
        }
    }
  // Check if time to close (all sent)
  if (m_totBytes == m_maxBytes && m_connected)
    {
      m_socket->Close ();
      m_connected = false;
    }
}

void MyTcpApp::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("MyTcpApp Connection succeeded");
  m_connected = true;
  SendData ();
  if(!m_connectionSucceeded.IsNull()){
	  m_connectionSucceeded(socket);
  }
}

void MyTcpApp::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("MyTcpApp, Connection Failed");
  if(!m_connectionFailed.IsNull()){
	  m_connectionFailed(socket);
  }
}
void MyTcpApp::NormalClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("MyTcpApp Connection closed");
  m_connected = false;
  if(!m_normalClose.IsNull()){
	  m_normalClose(socket);
  }
}

void MyTcpApp::ErrorClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("MyTcpApp Connection closed");
  m_connected = false;
  if(!m_errorClose.IsNull()){
	  m_errorClose(socket);
  }
}
void MyTcpApp::DataSend (Ptr<Socket>, uint32_t)
{
  NS_LOG_FUNCTION (this);

  if (m_connected)
    { // Only send new data if the connection has completed
      Simulator::ScheduleNow (&MyTcpApp::SendData, this);
    }
}
void
MyTcpApp::SetConnectCallback (
  Callback<void, Ptr<Socket> > connectionSucceeded,
  Callback<void, Ptr<Socket> > connectionFailed)
{
  NS_LOG_FUNCTION (this << &connectionSucceeded << &connectionFailed);
  m_connectionSucceeded = connectionSucceeded;
  m_connectionFailed = connectionFailed;
}

void
MyTcpApp::SetCloseCallbacks (
  Callback<void, Ptr<Socket> > normalClose,
  Callback<void, Ptr<Socket> > errorClose)
{
  NS_LOG_FUNCTION (this << &normalClose << &errorClose);
  m_normalClose = normalClose;
  m_errorClose = errorClose;
}
uint32_t
MyTcpApp::GetTransmitBytes()
{
	return m_totBytes;
}

} // Namespace ns
