

#ifndef NS3_RDMA_UTILITY_H
#define NS3_RDMA_UTILITY_H

#include <array>

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/traffic-control-module.h>

namespace ns3 {
    template<typename T>
    class Ptr;

    namespace rdma {
        using PFCPriorityType = uint8_t;
        static constinit const PFCPriorityType HIGHEST_PRIORITY = 0x00;
        static constinit const PFCPriorityType LOWEST_PRIORITY = 0xff;

        constinit static const uint16_t PFC_OP_CODE = 0x0101;
        constinit static const size_t PFC_PAUSE_TIME_VECTOR_LENGTH = 8;
        constinit static const size_t PFC_PADDING_SIZE = 26;

        using PFCPauseTimeInQuantaType = uint16_t;
        using PFCPauseTimeVector = std::array<PFCPauseTimeInQuantaType, PFC_PAUSE_TIME_VECTOR_LENGTH>;

        Ptr<PrioQueueDisc> get_priority_queue(Ptr<NetDevice> device);

        DataRate get_data_rate(Ptr<NetDevice> device);
    }
}

#endif //NS3_RDMA_UTILITY_H
