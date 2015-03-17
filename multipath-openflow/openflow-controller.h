#ifndef OPENFLOW_CONTROLLER_H
#define OPENFLOW_CONTROLLER_H

#include "ns3/openflow-interface.h"
#include "ns3/openflow-switch-net-device.h"
#include<boost/graph/graph_traits.hpp>
#include<boost/graph/adjacency_list.hpp>
#include<boost/graph/dijkstra_shortest_paths.hpp>
namespace ns3 {

namespace ofi {
using namespace boost;
using namespace std;
struct VertexProperty{
	int id;
};
struct EdgeProperty{
	short srcport;
	short dstport;
	int src;
	int dst;
};
typedef adjacency_list<vecS,vecS,undirectedS,int,EdgeProperty> Graph;

typedef graph_traits<Graph>::vertex_descriptor VertexDescriptor;
typedef graph_traits<Graph>::edge_descriptor EdgeDescriptor;
typedef graph_traits<Graph>::vertex_iterator VertexIterator;
typedef graph_traits<Graph>::edge_iterator  EdgeIterator;

class RandomizeController : public Controller {
 public:
  void ReceiveFromSwitch (Ptr<OpenFlowSwitchNetDevice> swtch, ofpbuf* buffer);
  void init();
  void listSwitchID();
  vector<vector<EdgeProperty> >topo;
  map<uint32_t,uint32_t>ipHostMap;
  struct PortRecord
  {
    uint32_t port;                      ///< client to server in port.
  };
  typedef std::map<Ipv4Address, PortRecord> PortRecord_t;
  PortRecord_t m_portrecord;

  Graph g;
};

class RoundRobinController : public Controller {
 public:
  void ReceiveFromSwitch (Ptr<OpenFlowSwitchNetDevice> swtch, ofpbuf* buffer);
protected:
  struct LearnedState
  {
    uint32_t port;                      ///< Learned port.
  };
  typedef std::map<Mac48Address, LearnedState> LearnState_t;
  LearnState_t m_learnState;            ///< Learned state data.

  struct RoundRobinState {
	  uint32_t port;  // last used port
  };
  typedef std::map<Ipv4Address, RoundRobinState> RoundRobinState_t;
  RoundRobinState_t m_lastState;

};

class IPHashingController : public Controller {
 public:
  void ReceiveFromSwitch (Ptr<OpenFlowSwitchNetDevice> swtch, ofpbuf* buffer);
};

}

}
#endif
