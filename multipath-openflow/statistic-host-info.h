/*
 * statistic-host-info.h
 *
 *  Created on: Mar 15, 2015
 *      Author: ws
 */

#ifndef STATISTIC_HOST_INFO_H_
#define STATISTIC_HOST_INFO_H_
#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include <vector>
#include <map>
#include "ns3/data-rate.h"
#include "ns3/socket.h"
namespace ns3{
class StatisticHostInfo: public ns3::Application {
public:
	static TypeId GetTypeId(void);
	StatisticHostInfo();
	virtual ~StatisticHostInfo();
	DataRate GetRateThreshold();
	double GetTimeThreshold();
	uint32_t GetByteThreshold();
	void SetSchedulePeriod(double period);
	void ApplicationRegister(Ptr<Application>);
protected:
  virtual void DoDispose (void);
private:
    virtual void StartApplication (void);     // Called at time specified by Start
	virtual void StopApplication (void);
	void CancelEvents ();
	std::vector<Ptr<Application> >m_applications;
	std::map<Ptr<Application>,uint32_t>m_applicationTransByte;
	std::map<Ptr<Socket>,double>m_socketRate;
	EventId m_startEvent;
	double m_rateThreshold;
	double m_timeThreshold;
	uint32_t m_byteThreshold;
	double m_schedulePeriod;
	Time m_lastStartTime;
private:
	void statistic(void);
	void ScheduleStartEvent();
	void ScheduleStopEvent();
	void ConnectionSucceeded(Ptr<Socket>socket);
	void ConnectionFailed(Ptr<Socket>socket);
	void ThresholdCalculate();
};
}
#endif /* STATISTIC_HOST_INFO_H_ */
