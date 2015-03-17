#include "openflow-controller.h"
#include "openflow-loadbalancer.h"

#include "ns3/random-variable.h"
#include "ns3/log.h"

namespace ns3 {

namespace ofi {
uint32_t ConvertIpv4AddressToUint32(Ipv4Address input);
NS_LOG_COMPONENT_DEFINE("randomcontroller");
void RandomizeController::ReceiveFromSwitch(Ptr<OpenFlowSwitchNetDevice> swtch,
		ofpbuf* buffer) {
	if (m_switches.find(swtch) == m_switches.end()) {
		//NS_LOG_ERROR ("Can't receive from this switch, not registered to the Controller.");
		return;
	}

	// We have received any packet at this point, so we pull the header to figure out what type of packet we're handling.
	uint8_t type = GetPacketType(buffer);

	if (type == OFPT_PACKET_IN) // The switch didn't understand the packet it received, so it forwarded it to the controller.
			{
		ofp_packet_in * opi = (ofp_packet_in*) ofpbuf_try_pull(buffer,
				offsetof(ofp_packet_in, data));
		int port = ntohs(opi->in_port);/* Port on which frame was received. */
		uint16_t out_port = OFPP_FLOOD; //flow out put port
		uint16_t in_port = OFPP_NONE; //flow input port

		Ipv4Address flow_dst_nw_addr; //flow destination ip address
		Ipv4Address flow_src_nw_addr; //flow source ip address
		Ipv4Address server_addr("10.1.1.254"); //known server address
		bool isToServer = false;

		Mac48Address flow_dst_dl_addr; //flow destination MAC address
		Mac48Address flow_src_dl_addr; //flow source MAC address

		//Create matching key.
		sw_flow_key key;
		key.wildcards = 0;
		flow_extract(buffer, port != -1 ? port : OFPP_NONE, &key.flow);

		NS_LOG_INFO("dl_type:"<<ntohs(key.flow.dl_type));
		FILE *pfile;
		char * outchar = NULL;
		uint64_t len = 0;
		pfile = open_memstream(&outchar, &len);
		//      memset(outchar,0,sizeof(outchar));
		flow_print(pfile, &key.flow);
		fclose(pfile);
		outchar[len - 1] = 0;
		NS_LOG_INFO(outchar);
		//get input port from flow info
		in_port = ntohs(key.flow.in_port);

		//get destination and source IP address from flow info
		flow_dst_nw_addr.Set(key.flow.nw_dst);
		flow_src_nw_addr.Set(key.flow.nw_src);
		isToServer = server_addr.IsEqual(flow_dst_nw_addr);

		//get destination and source MAC address from flow info
		flow_src_dl_addr.CopyFrom(key.flow.dl_src);
		flow_dst_dl_addr.CopyFrom(key.flow.dl_dst);

		//check if it is a broadcast msg on link layer
		if (flow_dst_dl_addr.IsBroadcast()) {
			//if it is a broadcast msg on link layer, assume it is for ARP, flood it.
			uint32_t min = 0;
			uint32_t max = OF_DEFAULT_SERVER_NUMBER;
			UniformVariable uv; // random variable generator
			out_port = uv.GetInteger(min, max); //FIXME. here assuming only clients will initiate ARP. .
		} else { //not an ARP message
				 //compare flow dest ip address with server ip address key.flow.nw_dst
			if (isToServer) //flow is to server, randomly select a port to forward.
			{
				uint32_t min = 0;
				uint32_t max = OF_DEFAULT_SERVER_NUMBER;
				UniformVariable uv;        // random variable generator
				out_port = uv.GetInteger(min, max);
			} else //otherwise, look up the <IP,port> mapping table to see if we understand this flow from previous experience
			{
				PortRecord_t::iterator st = m_portrecord.find(flow_dst_nw_addr);
				if (st != m_portrecord.end()) {
					out_port = st->second.port;
				} else {
					//we don't understand the flow yet, flood it.
					out_port = OFPP_FLOOD;
				}
			}
		}
		// Create output-to-port action
		ofp_action_output x[1];
		x[0].type = htons(OFPAT_OUTPUT);
		x[0].len = htons(sizeof(ofp_action_output));
		x[0].port = out_port;
		// Create a new flow from client to server that output port is random.
		ofp_flow_mod* ofm = BuildFlow(key, opi->buffer_id, OFPFC_ADD, x,
				sizeof(x), OFP_FLOW_PERMANENT, OFP_FLOW_PERMANENT);

		SendToSwitch(swtch, ofm, ofm->header.length);

		//record the client <IP, port> pair for future use
		if (isToServer) {
			PortRecord_t::iterator st = m_portrecord.find(flow_src_nw_addr);
			if (st == m_portrecord.end()) // We haven't learned our source IP yet.
					{
				PortRecord pr;
				pr.port = in_port;
				m_portrecord.insert(std::make_pair(flow_src_nw_addr, pr));

				//switch in port with out port.
				key.flow.in_port = out_port;
				//switch destination and source IP Addresses and MAC addresses
				key.flow.nw_dst = ConvertIpv4AddressToUint32(flow_src_nw_addr);
				key.flow.nw_src = ConvertIpv4AddressToUint32(flow_dst_nw_addr);
				flow_src_dl_addr.CopyTo(key.flow.dl_dst);
				flow_dst_dl_addr.CopyTo(key.flow.dl_src);
				// Create output-to-port action
				ofp_action_output x2[1];
				x2[0].type = htons(OFPAT_OUTPUT);
				x2[0].len = htons(sizeof(ofp_action_output));
				x2[0].port = in_port;
				// Create a new flow from src to dst that output port is random.
				ofp_flow_mod* ofm2 = BuildFlow(key, opi->buffer_id,
						OFPFC_MODIFY, x2, sizeof(x2), OFP_FLOW_PERMANENT,
						OFP_FLOW_PERMANENT);
				SendToSwitch(swtch, ofm2, ofm2->header.length);
			}
		}
	}
	return;
}
ofp_packet_out*
BuildPackoutfrompacet(uint32_t port, void *packet, size_t packet_len) {
	ofp_action_output x[1];
	x[0].type = htons(OFPAT_OUTPUT);
	x[0].len = htons(sizeof(ofp_action_output));
	x[0].port = port;
	ofp_packet_out* pko = (ofp_packet_out*) malloc(
			sizeof(ofp_packet_out) + packet_len + sizeof(x));
	pko->header.version = OFP_VERSION;
	pko->header.type = OFPT_PACKET_OUT;
	pko->header.length = htons(sizeof(ofp_packet_out) + packet_len + sizeof(x));
	pko->buffer_id = -1;
	pko->actions_len = sizeof(x);
	memcpy(pko->actions, x, sizeof(x));
	memcpy(pko->actions + sizeof(x), packet, packet_len);
	return pko;
}
uint32_t ConvertIpv4AddressToUint32(Ipv4Address input) {
	//need to convert Ipv4Address type to uint32_t since Ipv4Address type and key.flow.nw_dst/nw_src have different endian schemes.
	uint8_t buf[4];
	uint8_t tmp = 0;
	input.Serialize(buf);

	//swap buf[0] & buf[3]
	tmp = buf[0];
	buf[0] = buf[3];
	buf[3] = tmp;

	//swap buf[1] & buf[2]
	tmp = buf[1];
	buf[1] = buf[2];
	buf[2] = tmp;

	Ipv4Address new_input = Ipv4Address::Deserialize(buf);
	return new_input.Get();
}
}

}
