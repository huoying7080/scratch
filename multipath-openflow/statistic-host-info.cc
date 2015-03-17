/*
 * statistic-host-info.cpp
 *
 *  Created on: Mar 15, 2015
 *      Author: ws
 */

#include "statistic-host-info.h"
#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "MyTcpApp.h"
NS_LOG_COMPONENT_DEFINE("StatisticHostInfoApplication");
namespace ns3{
using namespace std;
TypeId
StatisticHostInfo::GetTypeId() {
	static TypeId tid=TypeId ("ns3::StatisticHostInfo")
	.SetParent<Application> ()
	.AddConstructor<StatisticHostInfo> ()
	.AddAttribute ("RateThreshold", "The rate of date send.",
			DoubleValue (100),
			MakeDoubleAccessor (&StatisticHostInfo::m_rateThreshold),
			MakeDoubleChecker<double> (0))
	.AddAttribute ("TimeThreshold", "The time of date send.",
			DoubleValue (5),
			MakeDoubleAccessor (&StatisticHostInfo::m_timeThreshold),
			MakeDoubleChecker<double> (0))
	.AddAttribute ("ByteThreshold", "The amount of data to send.",
			UintegerValue (1),
			MakeUintegerAccessor (&StatisticHostInfo::m_byteThreshold),
			MakeUintegerChecker<uint32_t> (1));
	return tid;
}
StatisticHostInfo::StatisticHostInfo()
	:m_rateThreshold(0),
	 m_timeThreshold(0),
	 m_byteThreshold(0),
	 m_schedulePeriod(1)
{
	NS_LOG_FUNCTION (this);
	// TODO Auto-generated constructor stub

}

StatisticHostInfo::~StatisticHostInfo() {
	NS_LOG_FUNCTION (this);
	// TODO Auto-generated destructor stub
}
void StatisticHostInfo::StartApplication(void)
{
	 NS_LOG_FUNCTION (this);
	 NS_LOG_INFO("start application");
	 if(!m_node)
	 {
//		 uint32_t applicationNumber=m_node->GetNApplications();
//		 for(uint32_t i=1;i<applicationNumber;i++)
//		 {
//			 Ptr<Application>app=m_node->GetApplication(i);
//			 m_applications.push_back(app);
//			 Ptr<BulkSendApplication>bulkapp=DynamicCast<BulkSendApplication>(app);
//			 Ptr<Socket>socket=bulkapp->GetSocket();
//			 bulkapp->m_connected;
//
//		 }
	 }
	 CancelEvents ();
	 m_lastStartTime=Simulator::Now();
	 ScheduleStartEvent ();
}
void StatisticHostInfo::StopApplication(void)
{
	NS_LOG_FUNCTION (this);
	CancelEvents ();
}
void StatisticHostInfo::ScheduleStartEvent()
{
	NS_LOG_FUNCTION (this);
	Time offInterval = Seconds (m_schedulePeriod);
	NS_LOG_LOGIC ("start at " << offInterval);
	m_startEvent = Simulator::Schedule (offInterval, &StatisticHostInfo::statistic, this);
}
void StatisticHostInfo::ScheduleStopEvent()
{
	NS_LOG_FUNCTION (this);
}
void StatisticHostInfo::CancelEvents ()
{
	NS_LOG_FUNCTION (this);
	if(m_startEvent.IsRunning())
	{
		Simulator::Cancel(m_startEvent);
	}
}
void StatisticHostInfo::statistic()
{
	NS_LOG_FUNCTION (this);
	NS_ASSERT (m_startEvent.IsExpired ());
	m_lastStartTime=Simulator::Now();
	NS_LOG_INFO("m_lastStartTime"<<m_lastStartTime);
	map<Ptr<Application>,uint32_t>::iterator iter;
	uint32_t totbyte;
	uint32_t oldtotbyte;
	for(iter=m_applicationTransByte.begin();iter!=m_applicationTransByte.end();iter++)
	{
		NS_LOG_INFO("application :");
		Ptr<Application>app=iter->first;
		Ptr<MyTcpApp>myapp=ns3::DynamicCast<MyTcpApp>(app);
		totbyte=myapp->GetTransmitBytes();
		oldtotbyte=iter->second;
		double rate=(totbyte-oldtotbyte)/m_schedulePeriod;
	    NS_LOG_INFO("new :"<<totbyte<<" old :"<<iter->second<<" rate :"<<rate);
	    iter->second=totbyte;
	    Ptr<Socket>socket=myapp->GetSocket();
	    if(rate>300000)
	    socket->SetIpTos(0x13);
	    else
	    socket->SetIpTos(0x00);
	}
	ScheduleStartEvent();
}
DataRate
StatisticHostInfo::GetRateThreshold()
{
	NS_LOG_FUNCTION (this);
	return DataRate("10Mbps");
}
double
StatisticHostInfo::GetTimeThreshold()
{
	NS_LOG_FUNCTION (this);
	return 0.1;
}
uint32_t
StatisticHostInfo::GetByteThreshold()
{
	NS_LOG_FUNCTION (this);
	return 1;
}
void
StatisticHostInfo::SetSchedulePeriod(double period)
{
	NS_LOG_FUNCTION (this);
	m_schedulePeriod=period;
}
void StatisticHostInfo::ApplicationRegister(Ptr<Application> app)
{
	NS_LOG_FUNCTION (this);
	Ptr<MyTcpApp>myapp=ns3::DynamicCast<MyTcpApp>(app);
	uint32_t totbyte=myapp->GetTransmitBytes();
	this->m_applicationTransByte.insert(pair<Ptr<Application> , uint32_t>(app,totbyte));
}
void
StatisticHostInfo::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  // chain up
  Application::DoDispose ();
}
}

