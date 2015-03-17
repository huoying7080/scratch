#include "openflow-controller.h"
#include "openflow-loadbalancer.h"

#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("RoundRobinController");

namespace ns3 {

namespace ofi {

void RoundRobinController::ReceiveFromSwitch(Ptr<OpenFlowSwitchNetDevice> swtch, ofpbuf* buffer) {
	NS_LOG_FUNCTION (this);
	// Check switch
	if (m_switches.find(swtch) == m_switches.end()) {
		NS_LOG_ERROR ("Can't receive from this switch, not registered to the Controller.");
		return;
	}

	// We have received any packet at this point, so we pull the header to figure out what type of packet we're handling.
	uint8_t type = GetPacketType(buffer);

	if (type == OFPT_PACKET_IN) // The switch didn't understand the packet it received, so it forwarded it to the controller.
	{
		NS_LOG_LOGIC ("Branch in : type == OFPT_PACKET_IN");

		ofp_packet_in * opi = (ofp_packet_in*)ofpbuf_try_pull (buffer, offsetof (ofp_packet_in, data));
		int port = ntohs (opi->in_port);

		// Create matching key.
		sw_flow_key key;
		key.wildcards = 0;
		flow_extract (buffer, port != -1 ? port : OFPP_NONE, &key.flow);

		uint16_t out_port = OFPP_FLOOD; // default flood

		uint16_t in_port = ntohs (key.flow.in_port);
		NS_LOG_LOGIC (in_port);

		Mac48Address src_macAddr, dst_macAddr;
		src_macAddr.CopyFrom(key.flow.dl_src);
		dst_macAddr.CopyFrom (key.flow.dl_dst);

		NS_LOG_LOGIC (src_macAddr);
		NS_LOG_LOGIC (dst_macAddr);

		Ipv4Address src_ipv4Addr (ntohl (key.flow.nw_src));
		Ipv4Address dst_ipv4Addr (ntohl (key.flow.nw_dst));
		Ipv4Address server_ipv4Addr ("10.1.1.254");  //TODO: should be parameter
		int server_number = OF_DEFAULT_SERVER_NUMBER; //TODO: should be parameter

		NS_LOG_LOGIC (src_ipv4Addr);
		NS_LOG_LOGIC (dst_ipv4Addr);

		// learn from each src_macAddr and in_port
		LearnState_t::iterator st = m_learnState.find (src_macAddr);
		if (st == m_learnState.end()) { // not found, learn it this time!
			LearnedState ls;
			ls.port = in_port;
			m_learnState.insert(std::make_pair(src_macAddr, ls));
			NS_LOG_INFO ("Learning... mac=" << src_macAddr << " port=" << in_port);
		}

		bool isArpProbe = false;
		if (src_ipv4Addr.IsEqual(src_ipv4Addr.GetZero())) {
			NS_LOG_LOGIC ("Branch in : Sender ipv4 address all zero - ARP probe packet");
			isArpProbe = true;
		} else {
			NS_LOG_LOGIC ("Branch in : Guess this is a normal data packet");
		}

		if (!dst_macAddr.IsBroadcast()) {
			NS_LOG_LOGIC ("Branch in : Not broadcast");

			LearnState_t::iterator st = m_learnState.find (dst_macAddr);
			if (st != m_learnState.end()) {
				out_port = st->second.port;
				NS_LOG_LOGIC ("Branch in : Found learned! mac=" << dst_macAddr << " out_port=" << out_port);
			} else {
				NS_LOG_LOGIC ("Branch in : Mac never learned before"); // never come in...
			}

		} else {
			NS_LOG_LOGIC ("Branch in : Broadcast");
			if (isArpProbe) {
				// prevent client bothering all servers, select a server to respond to it.
				RoundRobinState_t::iterator iter = m_lastState.find(server_ipv4Addr);
				if (iter != m_lastState.end()) {
					out_port = iter->second.port;
					out_port = (out_port + 1) % server_number;
					iter->second.port = out_port; // update
				} else {
					out_port = 0;
					RoundRobinState rrs;
					rrs.port = out_port;
					m_lastState.insert(std::make_pair(server_ipv4Addr, rrs)); // insert new
				}
			} else {
				out_port = OFPP_FLOOD;
			}
		}

		// Create output-to-port action
		ofp_action_output x[1];
		x[0].type = htons (OFPAT_OUTPUT);
		x[0].len = htons (sizeof(ofp_action_output));
		x[0].port = out_port;

		NS_LOG_FUNCTION (out_port);

		// Create a new flow
		ofp_flow_mod* ofm = BuildFlow(key, opi->buffer_id, OFPFC_ADD, x, sizeof(x), OFP_FLOW_PERMANENT, OFP_FLOW_PERMANENT);
		SendToSwitch(swtch, ofm, ofm->header.length);

	}
    return;
}

}

}
