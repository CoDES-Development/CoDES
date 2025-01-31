

#include "rdma-utility.h"

namespace ns3::rdma {
    NS_LOG_COMPONENT_DEFINE("RdmaUtility");

    Ptr<PrioQueueDisc> get_priority_queue(Ptr<NetDevice> device) {
        auto tc = device->GetNode()->GetObject<TrafficControlLayer>();
        if (tc == nullptr) {
            return nullptr;
        }
        auto queue = tc->GetRootQueueDiscOnDevice(device);
        return DynamicCast<PrioQueueDisc>(queue);
    }

    DataRate get_data_rate(Ptr<NetDevice> device) {
        DataRateValue value;
        if (!device->GetAttributeFailSafe("DataRate", value)) {
            NS_ASSERT(device->GetChannel()->GetAttributeFailSafe("DataRate", value));
        }
        return value.Get();
    }
}
