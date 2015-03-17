#ifndef OPENFLOW_CONTROLLER_H
#define OPENFLOW_CONTROLLER_H

#include "ns3/openflow-interface.h"
#include "ns3/openflow-switch-net-device.h"

namespace ns3 {

namespace ofi {

class RandomizeController : public Controller {
 public:
  void ReceiveFromSwitch (Ptr<OpenFlowSwitchNetDevice> swtch, ofpbuf* buffer);
  struct PortRecord
  {
    uint32_t port;                      ///< client to server in port.
  };
  typedef std::map<Ipv4Address, PortRecord> PortRecord_t;
  PortRecord_t m_portrecord;
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
