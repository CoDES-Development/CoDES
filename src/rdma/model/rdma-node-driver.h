

#ifndef NS3_RDMA_NODE_DRIVER_H
#define NS3_RDMA_NODE_DRIVER_H

#include <optional>
#include <ranges>
#include <unordered_map>

#include <ns3/core-module.h>
#include <ns3/coroutine-module.h>
#include <ns3/network-module.h>

#include "pfc-header.h"
#include "rdma-utility.h"

namespace ns3 {
    template<typename T>
    class Ptr;

    template<typename T>
    class CoroutineOperation;

    class RDMANodeDriver : public Object {
    private:
        constinit const static size_t QUANTA_BIT_SIZE = 512;

        struct PauseTimer {
            Time quanta;
            Time last_updated_time;
            rdma::PFCPauseTimeVector pause_time;
            std::optional<CoroutineOperation<bool>> operation;

            void Update();
        };

        constexpr static const uint16_t WILDCARD_PROTOCOL = 0;

        Ptr<Node> m_node;
        std::unordered_map<Ptr<NetDevice>, PauseTimer> timers;

        void process_timer(Ptr<NetDevice> device, PauseTimer &timer);

        CoroutineOperation<void> register_timer(Ptr<NetDevice> device, PauseTimer &timer);

    public:
        static TypeId GetTypeId();

        TypeId GetInstanceTypeId() const override;

        void Install(Ptr<Node> node);

        void TagPacket(Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, const Address &from, const Address &to, NetDevice::PacketType packetType);

        void NotifyPauseTimeUpdated(Ptr<NetDevice> device, uint16_t mask, rdma::PFCPauseTimeVector pause_time_in_quanta);
    };
}

#endif //NS3_RDMA_NODE_DRIVER_H
