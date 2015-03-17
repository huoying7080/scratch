/*
 * MyTcpApp.h
 *
 *  Created on: Mar 15, 2015
 *      Author: ws
 */

#ifndef MYTCPAPP_H_
#define MYTCPAPP_H_
#include <stdint.h>
#include <string>
#include "ns3/object-factory.h"
#include "ns3/address.h"
#include "ns3/attribute.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
namespace ns3 {

class Address;
class Socket;


class MyTcpApp : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  MyTcpApp ();

  virtual ~MyTcpApp ();

  /**
   * \brief Set the upper bound for the total number of bytes to send.
   *
   * Once this bound is reached, no more application bytes are sent. If the
   * application is stopped during the simulation and restarted, the
   * total number of bytes sent is not reset; however, the maxBytes
   * bound is still effective and the application will continue sending
   * up to maxBytes. The value zero for maxBytes means that
   * there is no upper bound; i.e. data is sent until the application
   * or simulation is stopped.
   *
   * \param maxBytes the upper bound of bytes to send
   */
  void SetMaxBytes (uint32_t maxBytes);
  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);
  void SetConnectCallback (Callback<void, Ptr<Socket> > connectionSucceeded,
                               Callback<void,  Ptr<Socket> > connectionFailed);
      /**
       * \brief Detect socket recv() events such as graceful shutdown or error.
       *
       * For connection-oriented sockets, the first callback is used to signal
       * that the remote side has gracefully shut down the connection, and the
       * second callback denotes an error corresponding to cases in which
       * a traditional recv() socket call might return -1 (error), such
       * as a connection reset.  For datagram sockets, these callbacks may
       * never be invoked.
       *
       * \param normalClose this callback is invoked when the
       *        peer closes the connection gracefully
       * \param errorClose this callback is invoked when the
       *        connection closes abnormally
       */
 void SetCloseCallbacks (Callback<void, Ptr<Socket> > normalClose,Callback<void, Ptr<Socket> > errorClose);
 uint32_t GetTransmitBytes(void);
  /**
   * \brief Get the socket this application is attached to.
   * \return pointer to associated socket
   */
  Ptr<Socket> GetSocket (void) const;

protected:
  virtual void DoDispose (void);
private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  /**
   * \brief Send data until the L4 transmission buffer is full.
   */
  void SendData ();

  Ptr<Socket>     m_socket;       //!< Associated socket
  Address         m_peer;         //!< Peer address
  bool            m_connected;    //!< True if connected
  uint32_t        m_sendSize;     //!< Size of data to send each time
  uint32_t        m_maxBytes;     //!< Limit total number of bytes sent
  uint32_t        m_totBytes;     //!< Total bytes sent so far
  TypeId          m_tid;          //!< The type of protocol to use.
  Callback<void, Ptr<Socket> >                   m_connectionSucceeded;  //!< connection succeeded callback
  Callback<void, Ptr<Socket> >                   m_connectionFailed;  //!< connection succeeded callback
  Callback<void, Ptr<Socket> >                   m_normalClose;          //!< connection closed callback
  Callback<void, Ptr<Socket> >                   m_errorClose;           //!< connection closed due to errors callback
  /// Traced Callback: sent packets
  TracedCallback<Ptr<const Packet> > m_txTrace;
  TracedCallback<uint32_t>m_transBytes;

private:
  /**
   * \brief Connection Succeeded (called by Socket through a callback)
   * \param socket the connected socket
   */
  void ConnectionSucceeded (Ptr<Socket> socket);
  /**
   * \brief Connection Failed (called by Socket through a callback)
   * \param socket the connected socket
   */
  void ConnectionFailed (Ptr<Socket> socket);
  void NormalClose (Ptr<Socket> socket);
  /**
   * \brief Connection Failed (called by Socket through a callback)
   * \param socket the connected socket
   */
  void ErrorClose (Ptr<Socket> socket);
  /**
   * \brief Send more data as soon as some has been transmitted.
   */

  void DataSend (Ptr<Socket>, uint32_t); // for socket's SetSendCallback
};

} // namespace ns3

#endif /* MYTCPAPP_H_ */
