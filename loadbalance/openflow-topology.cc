/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Network topology
//
//                clients
//                   |
//       ------------------------
//       |        Switch        |
//       ------------------------
//        |      |      |      |
//        s0     s1     s2     s3
//
//
#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/openflow-module.h"
#include "ns3/log.h"
#include "ns3/netanim-module.h"
#include "ns3/vector.h"
#include "ns3/constant-position-mobility-model.h"
#include "openflow-loadbalancer.h"
#include "openflow-controller.h"
using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE("OpenFlowLoadBalancerSimulation");

bool verbose = false;
int client_number = 1;
int server_number = OF_DEFAULT_SERVER_NUMBER;
oflb_type lb_type = OFLB_RANDOM;
std::string out_prefix = "openflow-loadbalancer";

bool SetVerbose(std::string value) {
	verbose = true;
	return true;
}

bool SetServerNumber(std::string value) {
	try {
		server_number = atoi(value.c_str());
		return true;
	} catch (...) {
		return false;
	}
	return false;
}

bool SetType(std::string value) {
	try {
		if (value == "random") {
			lb_type = OFLB_RANDOM;
			return true;
		}
		if (value == "round-robin") {
			lb_type = OFLB_ROUND_ROBIN;
			return true;
		}
		if (value == "ip-hashing") {
			lb_type = OFLB_IP_HASHING;
			return true;
		}
	} catch (...) {
		return false;
	}
	return false;
}

bool SetOutput(std::string value) {
	try {
		out_prefix = value;
		return true;
	} catch (...) {
		return false;
	}
	return false;
}

int main(int argc, char *argv[]) {
	//
	// Allow the user to override any of the defaults and the above Bind() at
	// run-time, via command-line arguments
	//
	LogComponentEnable("OpenFlowLoadBalancerSimulation", LOG_LEVEL_INFO);
	LogComponentEnable("randomcontroller", LOG_LEVEL_INFO);
	GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
	CommandLine cmd;
	cmd.AddValue("v", "Verbose (turns on logging).", MakeCallback(&SetVerbose));
	cmd.AddValue("verbose", "Verbose (turns on logging).",
			MakeCallback(&SetVerbose));
	cmd.AddValue("n", "Number of Server behind the Load-Balancer.",
			MakeCallback(&SetServerNumber));
	cmd.AddValue("number", "Number of Server behind the Load-Balancer.",
			MakeCallback(&SetServerNumber));
	cmd.AddValue("t", "Load Balancer Type.", MakeCallback(&SetType));
	cmd.AddValue("type", "Load Balancer Type.", MakeCallback(&SetType));
	cmd.AddValue("o", "Output Prefix.", MakeCallback(&SetOutput));
	cmd.AddValue("output", "Output Prefix.", MakeCallback(&SetOutput));

	cmd.Parse(argc, argv);

	if (verbose) {
		LogComponentEnable("OpenFlowLoadBalancerSimulation", LOG_LEVEL_INFO);
		LogComponentEnable("OpenFlowInterface", LOG_LEVEL_INFO);
		LogComponentEnable("OpenFlowSwitchNetDevice", LOG_LEVEL_INFO);
		//LogComponentEnable ("RandomController", LOG_LEVEL_INFO);
		LogComponentEnable("RoundRobinController", LOG_LEVEL_INFO);
	}

	//
	// Explicitly create the nodes required by the topology (shown above).
	//
	NS_LOG_INFO("Create clients.");
	NodeContainer clients;
	clients.Create(client_number);

	NS_LOG_INFO("Create servers.");
	NodeContainer servers;
	servers.Create(server_number);

	NodeContainer csmaSwitch;
	csmaSwitch.Create(1);

	NodeContainer allnodes = NodeContainer(clients, servers);

	NS_LOG_INFO("Build Topology");

	CsmaHelper csma;
	csma.SetChannelAttribute("DataRate", DataRateValue(5000000));
	csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));

	// Create the csma links, from each terminal to the switch
	NetDeviceContainer clientDevices;
	NetDeviceContainer serverDevices;
	NetDeviceContainer switchDevices;
	for (int i = 0; i < server_number; i++) {
		NetDeviceContainer link = csma.Install(
				NodeContainer(servers.Get(i), csmaSwitch));
		serverDevices.Add(link.Get(0));
		switchDevices.Add(link.Get(1));
	}
	for (int i = 0; i < client_number; i++) {
		NetDeviceContainer link = csma.Install(
				NodeContainer(clients.Get(i), csmaSwitch));
		clientDevices.Add(link.Get(0));
		switchDevices.Add(link.Get(1));
	}

	// Create the switch netdevice, which will do the packet switching
	Ptr<Node> switchNode = csmaSwitch.Get(0);
	OpenFlowSwitchHelper swtch;

	Ptr<ns3::ofi::Controller> controller = NULL;

	switch (lb_type) {
	case OFLB_RANDOM: {
		NS_LOG_INFO("Using Random Load Balancer.");
		controller = CreateObject<ns3::ofi::RandomizeController>();
		swtch.Install(switchNode, switchDevices, controller);
		break;
	}
	case OFLB_ROUND_ROBIN:
		NS_LOG_INFO("Using Round-Robin Load Balancer.");
		controller = CreateObject<ns3::ofi::RoundRobinController>();
		swtch.Install(switchNode, switchDevices, controller);
		break;
	case OFLB_IP_HASHING:
		NS_LOG_INFO("Using IP-Hashing Load Balancer.");
		controller = CreateObject<ns3::ofi::IPHashingController>();
		swtch.Install(switchNode, switchDevices, controller);
		break;
	default:
		break;
	}

	// Add internet stack to the terminals
	InternetStackHelper internet;
	internet.Install(servers);
	internet.Install(clients);

	// Assign IP addresses for servers.
	NS_LOG_INFO("Assign IP Addresses for servers");
	for (int i = 0; i < server_number; i++) {
		Ptr<NetDevice> device = serverDevices.Get(i);
		Ptr<Node> node = device->GetNode();
		Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();

		int32_t interface = ipv4->GetInterfaceForDevice(device);
		if (interface == -1) {
			interface = ipv4->AddInterface(device);
		}

		Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress("10.1.1.254",
				"255.255.255.0");
		ipv4->AddAddress(interface, ipv4Addr);
		ipv4->SetMetric(interface, 1);
		ipv4->SetUp(interface);
	}

	// Assign IP addresses for clients.
	NS_LOG_INFO("Assign IP Addresses for clients.");
	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.1.1.0", "255.255.255.0");
	ipv4.Assign(clientDevices);

	// Create applications for clients.
	NS_LOG_INFO("Create Applications for clients.");
	uint16_t port = 111;	// Discard port (RFC 683)

	OnOffHelper onoff("ns3::TcpSocketFactory",
			Address(InetSocketAddress(Ipv4Address("10.1.1.254"), port)));
	onoff.SetConstantRate(DataRate("500kb/s"));

	ApplicationContainer app;

	for (int i = 0; i < client_number; i++) {
		app = onoff.Install(clients.Get(i));
		app.Start(Seconds(1.0));
		app.Stop(Seconds(10.0));
	}

	// Create Sinker for servers.
	NS_LOG_INFO("Create Sinker for servers.");

	PacketSinkHelper sink("ns3::TcpSocketFactory",
			Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
	for (int i = 0; i < server_number; i++) {
		app = sink.Install(servers.Get(i));
		app.Start(Seconds(0.0));
	}

	NS_LOG_INFO("Creat static ARP for all nodes");
	vector<Address> Macs(allnodes.GetN());
	vector<Ipv4Address> ips(allnodes.GetN());
	for (uint32_t i = 0; i < allnodes.GetN(); ++i) {
		Macs[i] = allnodes.Get(i)->GetDevice(0)->GetAddress();
		ips[i] =
				allnodes.Get(i)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
	}
	NS_LOG_INFO("get all IPs");
	for (uint32_t i = 0; i < allnodes.GetN(); ++i) {
		Ptr<ArpCache> arpCache =
				allnodes.Get(i)->GetObject<Ipv4L3Protocol>()->GetInterface(
						1)->GetArpCache();
		if (arpCache == NULL)
			arpCache = CreateObject<ArpCache>();
		for (uint32_t j = 0; j < allnodes.GetN(); ++j) {
			if (i != j) {
				if(arpCache->Lookup(ips[j])!=0)
					continue;
				ArpCache::Entry *entry = arpCache->Add(ips[j]);
				entry->MarkWaitReply(0);
				entry->MarkAlive(Macs[j]);
			}
		}
	}

	NS_LOG_INFO("Creat anim Topology");
	double xx = 1, yy = 1;

	for (uint32_t i = 0; i < clients.GetN(); i++) {
		Ptr<Node> lr = clients.Get(i);
		Ptr<ConstantPositionMobilityModel> loc = lr->GetObject<
				ConstantPositionMobilityModel>();
		if (loc == 0) {
			loc = CreateObject<ConstantPositionMobilityModel>();
			lr->AggregateObject(loc);
		}
		Vector lrl(xx, yy, 0);
		yy += 2;
		loc->SetPosition(lrl);
	}
	xx = 10;
	yy = 5;
	for (uint32_t i = 0; i < servers.GetN(); i++) {
		Ptr<Node> lr = servers.Get(i);
		Ptr<ConstantPositionMobilityModel> loc = lr->GetObject<
				ConstantPositionMobilityModel>();
		if (loc == 0) {
			loc = CreateObject<ConstantPositionMobilityModel>();
			lr->AggregateObject(loc);
		}
		Vector lrl(xx, yy, 0);
		yy += 2;
		loc->SetPosition(lrl);
	}
	xx = 5;
	yy = 5;
	for (uint32_t i = 0; i < csmaSwitch.GetN(); i++) {
		Ptr<Node> lr = csmaSwitch.Get(i);
		Ptr<ConstantPositionMobilityModel> loc = lr->GetObject<
				ConstantPositionMobilityModel>();
		if (loc == 0) {
			loc = CreateObject<ConstantPositionMobilityModel>();
			lr->AggregateObject(loc);
		}
		Vector lrl(xx, yy, 0);
		yy += 2;
		loc->SetPosition(lrl);
	}
	AnimationInterface anim("simpleOpenflow.xml");
	anim.EnablePacketMetadata(); // Optional
	anim.EnableIpv4L3ProtocolCounters(Seconds(0), Seconds(10));
	anim.EnableIpv4L3ProtocolCounters(Seconds(0), Seconds(10));
	anim.EnableIpv4RouteTracking("simpleOpenflwoRoute.xml", Seconds(0),
			Seconds(10), allnodes, Seconds(1));
	NS_LOG_INFO("Configure Tracing.");

	//
	// Configure tracing of all enqueue, dequeue, and NetDevice receive events.
	// Trace output will be sent to the file "openflow-loadbalancer.tr"
	//
	AsciiTraceHelper ascii;
	csma.EnableAsciiAll(ascii.CreateFileStream(out_prefix + ".tr"));

	//
	// Also configure some tcpdump traces; each interface will be traced.
	// The output files will be named:
	//     openflow-loadbalancer-<nodeId>-<interfaceId>.pcap
	// and can be read by the "tcpdump -r" command (use "-tt" option to
	// display timestamps correctly)
	//
	csma.EnablePcapAll(out_prefix, false);

	//
	// Now, do the actual simulation.
	//
	NS_LOG_INFO("Run Simulation.");
	Simulator::Run();
	Simulator::Destroy();
	NS_LOG_INFO("Done.");
}
