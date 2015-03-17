#ifndef OPENFLOW_CONTROLLER_H
#define OPENFLOW_CONTROLLER_H

#include "ns3/openflow-interface.h"
#include "ns3/openflow-switch-net-device.h"

namespace ns3 {

namespace ofi {

class RandomizeController : public Controller {
 public:
  void ReceiveFromSwitch (Ptr<OpenFlowSwitchNetDevice> swtch, ofpbuf* buffer);
 
 protected:
  struct LearnedState
  {
    uint32_t port;                      ///< Learned port.
  };
  Time m_expirationTime;                ///< Time it takes for learned MAC state entry/created flow to expire.
  typedef std::map<Mac48Address, LearnedState> LearnState_t;
  LearnState_t m_learnState;            ///< Learned state data.
};

class RoundRobinController : public Controller {
 public:
  void ReceiveFromSwitch (Ptr<OpenFlowSwitchNetDevice> swtch, ofpbuf* buffer);
};

class IPHashingController : public Controller {
 public:
  void ReceiveFromSwitch (Ptr<OpenFlowSwitchNetDevice> swtch, ofpbuf* buffer);
};

}

}
#endif
