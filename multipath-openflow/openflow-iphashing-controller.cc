#include "openflow-controller.h"
#include "ns3/address.h"
//#include "ipv4-address.h"
#include "ns3/log.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/hash-function.h"
//#include "ns3/hash-function-impl.h"

#include <iostream>
#include <string>

namespace ns3 {

    namespace ofi {

        NS_LOG_COMPONENT_DEFINE("IPHashingController");
        
        void IPHashingController::ReceiveFromSwitch (Ptr<OpenFlowSwitchNetDevice> swtch,
                                                     ofpbuf* buffer) {
            
            return;
        }

    }

}





















