#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/openflow-module.h"
#include "ns3/log.h"
#include "ns3/netanim-module.h"
#include "ns3/vector.h"
#include <iostream>
#include "openflow-loadbalancer.h"
#include "openflow-controller.h"
namespace ns3{
using namespace std;
class FatTreeHelper: public Object
{
public:
	static TypeId GetTypeId(void);
	FatTreeHelper(unsigned n);
	virtual ~FatTreeHelper();
	void Create(void);
	NodeContainer& AllNodes(void)  { return m_node; };
	NodeContainer& CoreNodes(void) { return m_core; };
	NodeContainer& AggrNodes(void) { return m_aggr; };
	NodeContainer& EdgeNodes(void) { return m_edge; };
	NodeContainer& HostNodes(void) { return m_host; };
	Ipv4InterfaceContainer& CoreInterfaces(void) { return m_coreIface; };
	Ipv4InterfaceContainer& AggrInterfaces(void) { return m_aggrIface; };
	Ipv4InterfaceContainer& EdgeInterfaces(void) { return m_edgeIface; };
	Ipv4InterfaceContainer& HostInterfaces(void) { return m_hostIface; };
	static void EnableAscii (std::ostream &os, uint32_t nodeid, uint32_t deviceid);
	NodeContainer getnode(void);
	NodeContainer getcore(void);
	NodeContainer getaggr(void);
	NodeContainer gethost(void);
	NodeContainer getedge(void);
	void 	AssignIP (Ipv4AddressHelper address);
	void    IncastAppModel(string rate,uint32_t packetSize, uint32_t maxByte);
	void assignOpenflowController(Ptr<ns3::ofi::RandomizeController> controller);

private:
	void	AssignIP (Ptr<NetDevice> c, uint32_t address, Ipv4InterfaceContainer &con);
	static void EnableAscii (Ptr<Node> node, Ptr<NetDevice> device);
	static unsigned m_size;
	DataRate m_heRate;
	DataRate	m_eaRate;
	DataRate	m_acRate;
	Time	m_heDelay;
	Time	m_eaDelay;
	Time	m_acDelay;
	NodeContainer	m_node;
	NodeContainer	m_core;
	NodeContainer	m_aggr;
	NodeContainer	m_host;
	NodeContainer	m_edge;
	vector<NetDeviceContainer> m_coreDevice;
	vector<NetDeviceContainer> m_aggrDevice;
	vector<NetDeviceContainer> m_edgeDevice;
	vector<NetDeviceContainer> m_hostDevice;
	Ipv4InterfaceContainer	m_hostIface;
	Ipv4InterfaceContainer	m_edgeIface;
	Ipv4InterfaceContainer	m_aggrIface;
	Ipv4InterfaceContainer	m_coreIface;
	ObjectFactory	m_channelFactory;
	ObjectFactory	m_cpFactory;
	ObjectFactory	m_rpFactory;
};
};
