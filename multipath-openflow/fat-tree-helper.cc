#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/openflow-module.h"
#include "ns3/log.h"
#include "ns3/netanim-module.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/vector.h"
#include<sstream>
#include<stdint.h>
#include<stdlib.h>
#include<iostream>
#include<boost/graph/graph_traits.hpp>
#include<boost/graph/adjacency_list.hpp>
#include<boost/graph/dijkstra_shortest_paths.hpp>
#include"fat-tree-helper.h"
#define CpNetDevice CsmaNetDevice
#define RpNetDevice CsmaNetDevice
namespace ns3{
using namespace std;
using namespace ns3::ofi;
using namespace boost;
NS_LOG_COMPONENT_DEFINE("FatTreeHelper");
NS_OBJECT_ENSURE_REGISTERED(FatTreeHelper);
unsigned FatTreeHelper::m_size=0;

TypeId
FatTreeHelper::GetTypeId() {
	static TypeId tid =
			TypeId("ns3::FatTreeHelper").SetParent<Object>().AddAttribute(
					"HeDataRate",
					"The default data rate for point to point links",
					DataRateValue(DataRate("100Mbps")),
					MakeDataRateAccessor(&FatTreeHelper::m_heRate),
					MakeDataRateChecker()).AddAttribute("EaDataRate",
					"The default data rate for point to point links",
					DataRateValue(DataRate("100Mbps")),
					MakeDataRateAccessor(&FatTreeHelper::m_eaRate),
					MakeDataRateChecker()).AddAttribute("AcDataRate",
					"The default data rate for point to point links",
					DataRateValue(DataRate("100Mbps")),
					MakeDataRateAccessor(&FatTreeHelper::m_acRate),
					MakeDataRateChecker()).AddAttribute("HeDelay",
					"Transmission delay through the channel",
					TimeValue(NanoSeconds(0)),
					MakeTimeAccessor(&FatTreeHelper::m_heDelay),
					MakeTimeChecker()).AddAttribute("EaDelay",
					"Transmission delay through the channel",
					TimeValue(NanoSeconds(0)),
					MakeTimeAccessor(&FatTreeHelper::m_eaDelay),
					MakeTimeChecker()).AddAttribute("AcDelay",
					"Transmission delay through the channel",
					TimeValue(NanoSeconds(0)),
					MakeTimeAccessor(&FatTreeHelper::m_acDelay),
					MakeTimeChecker());
	return tid;
}
FatTreeHelper::FatTreeHelper(unsigned N)
{
	m_size=N;
	m_channelFactory.SetTypeId("ns3::CsmaChannel");
	m_rpFactory.SetTypeId("ns3::CsmaNetDevice");
	m_cpFactory.SetTypeId("ns3::CsmaNetDevice");
}
FatTreeHelper::~FatTreeHelper()
{
}
void
FatTreeHelper::Create()
{
	const unsigned N = m_size;
	const unsigned numST = 2*N;
	const unsigned numCore = N*N;
	const unsigned numAggr = numST * N;
	const unsigned numEdge = numST * N;
	const unsigned numHost = numEdge * N;
	const unsigned numTotal= numCore + numAggr + numEdge + numHost;
	m_coreDevice=vector<NetDeviceContainer>(numCore);
	m_aggrDevice=vector<NetDeviceContainer>(numAggr);
	m_edgeDevice=vector<NetDeviceContainer>(numEdge);
	m_hostDevice=vector<NetDeviceContainer>(numHost);
	NS_LOG_INFO("creating fat-tree nodes.");
	m_node.Create(numTotal);

	for(unsigned j=0;j<2*N;j++) { // For every subtree
		for(unsigned i=j*2*N; i<=j*2*N+N-1; i++) { // First N nodes
			m_edge.Add(m_node.Get(i));
		}
		for(unsigned i=j*2*N+N; i<=j*2*N+2*N-1; i++) { // Last N nodes
			m_aggr.Add(m_node.Get(i));
		}
	};
	for(unsigned i=4*N*N; i<5*N*N; i++) {
		m_core.Add(m_node.Get(i));
	};
	for(unsigned i=5*N*N; i<numTotal; i++) {
		m_host.Add(m_node.Get(i));
	};
	// How to set a hash function to the hash routing????
	NS_LOG_INFO("creating fat-tree links.");
	CsmaHelper csmachainal;
	csmachainal.SetChannelAttribute("DataRate", DataRateValue(m_heRate));
	csmachainal.SetChannelAttribute("Delay", TimeValue(m_heDelay));
	NS_LOG_INFO("creating fat-tree edge host links.");
	for (unsigned j=0; j<numST; j++) { // For each subtree
		for(unsigned i=0; i<N; i++) { // For each edge
			for(unsigned m=0; m<N; m++) { // For each port of edge
				// Connect edge to host
				Ptr<Node> eNode = m_edge.Get(j*N+i);
				Ptr<Node> hNode = m_host.Get(j*N*N+i*N+m);
				NodeContainer link=NodeContainer(eNode,hNode);
				NetDeviceContainer netdev;
				netdev=csmachainal.Install(link);
				m_edgeDevice[j*N+i].Add(netdev.Get(0));
				m_hostDevice[j*N*N+i*N+m].Add(netdev.Get(1));
				// Set routing for end host: Default route only
				// Set IP address for end host
			};
		};
	};
	NS_LOG_INFO("creating fat-tree edge aggregation links.");
	csmachainal.SetChannelAttribute("DataRate", DataRateValue(m_eaRate));
	csmachainal.SetChannelAttribute("Delay", TimeValue(m_eaDelay));
	for (unsigned j=0; j<numST; j++) { // For each subtree
		for(unsigned i=0; i<N; i++) { // For each edge
			for(unsigned m=0; m<N; m++) { // For each aggregation
				// Connect edge to aggregation
				Ptr<Node> aNode = m_aggr.Get(j*N+m);
				Ptr<Node> eNode = m_edge.Get(j*N+i);
				NodeContainer link=NodeContainer(aNode,eNode);
				NetDeviceContainer netdev;
				netdev=csmachainal.Install(link);
				m_aggrDevice[j*N+m].Add(netdev.Get(0));
				m_edgeDevice[j*N+i].Add(netdev.Get(1));
			} ;
		};
	};
	NS_LOG_INFO("creating fat-tree aggregation edge links.");
	csmachainal.SetChannelAttribute("DataRate", DataRateValue(m_acRate));
	csmachainal.SetChannelAttribute("Delay", TimeValue(m_acDelay));
	for(unsigned j=0; j<numST; j++) { // For each subtree
		for(unsigned i=0; i<N; i++) { // For each aggr
			for(unsigned m=0; m<N; m++) { // For each port of aggr
				// Connect aggregation to core
				Ptr<Node> cNode = m_core.Get(i*N+m);
				Ptr<Node> aNode = m_aggr.Get(j*N+i);
				NodeContainer link=NodeContainer(cNode,aNode);
				NetDeviceContainer netdev;
				netdev=csmachainal.Install(link);
				m_coreDevice[i*N+m].Add(netdev.Get(0));
				m_aggrDevice[j*N+i].Add(netdev.Get(1));
			};
		};
	};
	NS_LOG_INFO ("Creat anim Topology");
	NS_LOG_INFO ("host "<<m_host.GetN()<<"edge "<<m_edge.GetN()<<"aggre "<<m_aggr.GetN()<<"core "<<m_core.GetN());
	double xx=0,yy=1;
	double step=2;
	for (uint32_t i = 0; i < m_host.GetN(); i++) {
		Ptr<Node> lr = m_host.Get(i);
		Ptr<ConstantPositionMobilityModel> loc = lr->GetObject<ConstantPositionMobilityModel> ();
		if (loc == 0) {
			loc = CreateObject<ConstantPositionMobilityModel>();
			lr->AggregateObject(loc);
		}
		Vector lrl(xx, yy, 0);
		xx += step;
		loc->SetPosition(lrl);
	}
	double step2=m_size*step;
	xx=step/2;
	yy=step*4;
	for (uint32_t i = 0; i < m_edge.GetN(); i++) {
		Ptr<Node> lr = m_edge.Get(i);
		Ptr<ConstantPositionMobilityModel> loc = lr->GetObject<
				ConstantPositionMobilityModel>();
		if (loc == 0) {
			loc = CreateObject<ConstantPositionMobilityModel>();
			lr->AggregateObject(loc);
		}
		Vector lrl(xx, yy, 0);
		xx += step2;
		loc->SetPosition(lrl);
	}
	xx=step/2;
	yy=step*2*4;
	for (uint32_t i = 0; i < m_aggr.GetN(); i++) {
		Ptr<Node> lr = m_aggr.Get(i);
		Ptr<ConstantPositionMobilityModel> loc = lr->GetObject<
				ConstantPositionMobilityModel>();
		if (loc == 0) {
			loc = CreateObject<ConstantPositionMobilityModel>();
			lr->AggregateObject(loc);
		}
		Vector lrl(xx, yy, 0);
		xx += step2;
		loc->SetPosition(lrl);
	}
	double step3=2*m_size*step;
	xx=step;
	yy=step*3*4;
	for (uint32_t i = 0; i < m_core.GetN(); i++) {
		Ptr<Node> lr = m_core.Get(i);
		Ptr<ConstantPositionMobilityModel> loc = lr->GetObject<
				ConstantPositionMobilityModel>();
		if (loc == 0) {
			loc = CreateObject<ConstantPositionMobilityModel>();
			lr->AggregateObject(loc);
		}
		Vector lrl(xx, yy, 0);
		xx += step3;
		loc->SetPosition(lrl);
	}
	NS_LOG_INFO("end fat-tree topo create");
}
NodeContainer
FatTreeHelper::gethost()
{
	return m_host;
}
NodeContainer
FatTreeHelper::getedge()
{
	return m_edge;
}
NodeContainer
FatTreeHelper::getcore()
{
	return m_core;
}
NodeContainer
FatTreeHelper::getnode()
{
	return m_node;
}
NodeContainer
FatTreeHelper::getaggr()
{
	return m_aggr;
}
void
FatTreeHelper::AssignIP (Ptr<NetDevice> c, uint32_t address, Ipv4InterfaceContainer &con)
{
	NS_LOG_FUNCTION_NOARGS ();

	Ptr<Node> node = c->GetNode ();
	NS_ASSERT_MSG (node, "FatTreeHelper::AssignIP(): Bad node");

	Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
	NS_ASSERT_MSG (ipv4, "FatTreeHelper::AssignIP(): Bad ipv4");

	int32_t ifIndex = ipv4->GetInterfaceForDevice (c);
	if (ifIndex == -1) {
		ifIndex = ipv4->AddInterface (c);
	};
	NS_ASSERT_MSG (ifIndex >= 0, "FatTreeHelper::AssignIP(): Interface index not found");

	Ipv4Address addr(address);
	Ipv4InterfaceAddress ifaddr(addr, 0xFFFFFFFF);
	ipv4->AddAddress (ifIndex, ifaddr);
	ipv4->SetMetric (ifIndex, 1);
	ipv4->SetUp (ifIndex);
	con.Add (ipv4, ifIndex);
	Ipv4AddressGenerator::AddAllocated (addr);
}
void
FatTreeHelper::AssignIP (Ipv4AddressHelper address)
{
	for(unsigned i=0;i<2*m_size*m_size*m_size;++i)
	{
		address.Assign(m_hostDevice[i]);
	}
	NS_LOG_INFO("Creat static ARP for all nodes");
		vector<Address> Macs(m_host.GetN());
		vector<Ipv4Address> ips(m_host.GetN());
		for (uint32_t i = 0; i < m_host.GetN(); ++i) {
			Macs[i] = m_host.Get(i)->GetDevice(0)->GetAddress();
			ips[i] =
					m_host.Get(i)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
		}
		NS_LOG_INFO("get all IPs");
		for (uint32_t i = 0; i < m_host.GetN(); ++i) {
			Ptr<ArpCache> arpCache =
					m_host.Get(i)->GetObject<Ipv4L3Protocol>()->GetInterface(1)->GetArpCache();
			if (arpCache == NULL)
				arpCache = CreateObject<ArpCache>();
			for (uint32_t j = 0; j < m_host.GetN(); ++j) {
				if (i != j) {
					if (arpCache->Lookup(ips[j]) != 0)
						continue;
					ArpCache::Entry *entry = arpCache->Add(ips[j]);
					entry->MarkWaitReply(0);
					entry->MarkAlive(Macs[j]);
				}
			}
		}
}

void
FatTreeHelper::EnableAscii (std::ostream &os, uint32_t nodeid, uint32_t deviceid)
{

}
void
FatTreeHelper::IncastAppModel(string rate,uint32_t packetSize, uint32_t maxBytes)
{
	OnOffHelper clientHelper("ns3::TcpSocketFactory", Address());
	clientHelper.SetConstantRate(DataRate(rate), packetSize);
	clientHelper.SetAttribute("OnTime",
			StringValue("ns3::ConstantRandomVariable[Constant=1]"));
	clientHelper.SetAttribute("OffTime",
			StringValue("ns3::ConstantRandomVariable[Constant=0]"));
//	BulkSendHelper clientHelper ("ns3::TcpSocketFactory", Address ());
	// Set the amount of data to send in bytes.  Zero is unlimited.
	clientHelper.SetAttribute("MaxBytes", UintegerValue(maxBytes));
	ApplicationContainer sinkApps;
	ApplicationContainer clientApps;
	uint16_t portStart = 5000;
	vector<uint16_t> port;
	vector<PacketSinkHelper> sinkHelper;
	Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable>();
	double rn;
	NS_LOG_INFO("Assign server");
	for (uint32_t i = 0; i < 1; ++i) {
		port.push_back(portStart);
		Address sinkLocalAddress(
				InetSocketAddress(Ipv4Address::GetAny(), portStart));
		sinkHelper.push_back(
				PacketSinkHelper("ns3::TcpSocketFactory", sinkLocalAddress));
		portStart++;
	}

	NS_LOG_INFO("Assign client");
	m_host.Get(0)->GetDevice(0);
	sinkApps.Add(sinkHelper[0].Install(m_host.Get(0)));
	AddressValue remoteAddress(
					InetSocketAddress(m_host.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(),
							port[0]));
	for (uint32_t i = 0; i < 1; ++i) {
		for (uint32_t j = 1; j < m_host.GetN(); ++j) {
			rn = x->GetValue(0, 1);
			clientHelper.SetAttribute("Remote", remoteAddress);
			ApplicationContainer app = clientHelper.Install(m_host.Get(j));
			app.Start(Seconds(0.1 + rn));
			app.Stop(Seconds(0.1 +10 + rn));
			clientApps.Add(app);
		}
	}

	sinkApps.Start(Seconds(0.1));
	sinkApps.Stop(Seconds(15));
}
void
FatTreeHelper::assignOpenflowController(Ptr<ns3::ofi::RandomizeController> controller)
{
	NS_LOG_INFO("begin create openflow switch");
	Graph g= controller->g;
	vector<vector<EdgeProperty> >topo=controller->topo;
	const unsigned N = m_size;
	const unsigned numST = 2*N;
	const unsigned numCore = N*N;
	const unsigned numAggr = numST * N;
	const unsigned numEdge = numST * N;
	const unsigned numHost = numEdge * N;
	const unsigned numTotal= numCore + numAggr + numEdge + numHost;
	vector<int>corePortNum=vector<int>(numCore,0);
	vector<int>aggrPortNum=vector<int>(numAggr,0);
	vector<int>edgePortNum=vector<int>(numEdge,0);
	vector<int>hostPortNum=vector<int>(numHost,0);
	NS_LOG_LOGIC("creating fat-tree nodes.");
	int switchid=0;
	int edgenum=0;
	int aggrnum=0;
	int corenum=0;
	OpenFlowSwitchHelper swtch;
	NS_LOG_INFO("begin get edge and aggregation switch interface");
	for(unsigned j=0;j<2*N;j++) { // For every subtree
		for(unsigned i=j*2*N; i<=j*2*N+N-1; i++) { // First N nodes
			add_vertex(switchid,g);
			vector<EdgeProperty>edges;
			topo.push_back(edges);
			swtch.SetDeviceAttribute("ID",UintegerValue(switchid));
			swtch.Install(m_node.Get(i),m_edgeDevice[edgenum],controller);
			switchid++;
			edgenum++;
		}
		for(unsigned i=j*2*N+N; i<=j*2*N+2*N-1; i++) { // Last N nodes
			add_vertex(switchid,g);
			vector<EdgeProperty>edges;
			topo.push_back(edges);
			swtch.SetDeviceAttribute("ID",UintegerValue(switchid));
			swtch.Install(m_node.Get(i),m_aggrDevice[aggrnum],controller);
			switchid++;
			aggrnum++;
		}
	};
	NS_LOG_INFO("begin get core switch interface");
	for(unsigned i=4*N*N; i<5*N*N; i++) {
		NS_LOG_INFO("begin core :"<<i);
		add_vertex(switchid,g);
		vector<EdgeProperty>edges;
		topo.push_back(edges);
		swtch.SetDeviceAttribute("ID",UintegerValue(switchid));
		swtch.Install(m_node.Get(i),m_coreDevice[corenum],controller);
		switchid++;
		corenum++;
	};
	for(unsigned i=5*N*N; i<numTotal; i++) {
		add_vertex(switchid,g);
		vector<EdgeProperty>edges;
		topo.push_back(edges);
		switchid++;
	};
	NS_LOG_INFO("begin create edge ");
	// How to set a hash function to the hash routing????
	for (unsigned j=0; j<numST; j++) { // For each subtree
		for(unsigned i=0; i<N; i++) { // For each edge
			for(unsigned m=0; m<N; m++) { // For each port of edge
				// Connect edge to host

				Ptr<Node> eNode = m_edge.Get(j*N+i);
				Ptr<Node> hNode = m_host.Get(j*N*N+i*N+m);
				EdgeProperty edge;
				edge.src=eNode->GetId();
				edge.dst=hNode->GetId();
				edge.srcport=edgePortNum[j*N+i];
				edgePortNum[j*N+i]++;
				edge.dstport=0;
				topo[edge.src].push_back(edge);
				EdgeProperty edge2;
				edge2.dst=edge.src;
				edge2.src=edge.dst;
				edge2.srcport=edge.dstport;
				edge2.dstport=edge.srcport;
				topo[edge2.src].push_back(edge2);
				add_edge(edge.src,edge.dst,edge,g);
				// Set routing for end host: Default route only
				// Set IP address for end host
			};
		};
	};
	for (unsigned j=0; j<numST; j++) { // For each subtree
		for(unsigned i=0; i<N; i++) { // For each edge
			for(unsigned m=0; m<N; m++) { // For each aggregation
				// Connect edge to aggregation
				Ptr<Node> aNode = m_aggr.Get(j*N+m);
				Ptr<Node> eNode = m_edge.Get(j*N+i);
				EdgeProperty edge;
				edge.src=aNode->GetId();
				edge.dst=eNode->GetId();
				edge.srcport=edgePortNum[j*N+m];
				edge.srcport=edgePortNum[j*N+i];
				edgePortNum[j*N+m]++;
				edgePortNum[j*N+i]++;
				topo[edge.src].push_back(edge);
				EdgeProperty edge2;
				edge2.dst=edge.src;
				edge2.src=edge.dst;
				edge2.srcport=edge.dstport;
				edge2.dstport=edge.srcport;
				topo[edge2.src].push_back(edge2);
				add_edge(edge.src,edge.dst,edge,g);
			} ;
		};
	};

	for(unsigned j=0; j<numST; j++) { // For each subtree
		for(unsigned i=0; i<N; i++) { // For each aggr
			for(unsigned m=0; m<N; m++) { // For each port of aggr
				// Connect aggregation to core
				Ptr<Node> cNode = m_core.Get(i*N+m);
				Ptr<Node> aNode = m_aggr.Get(j*N+i);
				EdgeProperty edge;
				edge.src=cNode->GetId();
				edge.dst=aNode->GetId();
				edge.srcport=edgePortNum[i*N+m];
				edge.srcport=edgePortNum[j*N+i];
				edgePortNum[i*N+m]++;
				edgePortNum[j*N+i]++;
				topo[edge.src].push_back(edge);
				EdgeProperty edge2;
				edge2.dst=edge.src;
				edge2.src=edge.dst;
				edge2.srcport=edge.dstport;
				edge2.dstport=edge.srcport;
				topo[edge2.src].push_back(edge2);
				add_edge(edge.src,edge.dst,edge,g);
			};
		};
	};
    map<uint32_t,uint32_t>iphostmap=controller->ipHostMap;
    for(uint32_t i; i<m_host.GetN();++i)
    {
    	Ipv4Address ip=m_host.Get(i)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
    	iphostmap.insert(pair<uint32_t,uint32_t>(ip.Get(),m_host.Get(i)->GetId()));
    }
}

};
