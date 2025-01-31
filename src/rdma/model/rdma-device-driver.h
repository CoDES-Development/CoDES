
#ifndef NS3_RDMA_DEVICE_DRIVER_H
#define NS3_RDMA_DEVICE_DRIVER_H

#include <unordered_map>

#include <ns3/core-module.h>
#include <ns3/coroutine-module.h>
#include <ns3/network-module.h>

#include "pfc-tag.h"

namespace ns3 {
    template<typename T>
    class Ptr;

    class RDMADeviceDriver : public Object {
    private:
        constexpr static const uint16_t MAC_PROTOCOL = 0x8808;
        const static Mac48Address PFC_MULTICAST_ADDRESS;

        using QueueCallbackType = Callback<void, Ptr<QueueDiscItem>>;

        Ptr<NetDevice> m_device;
        double m_pfc_pause_threshold;
        double m_pfc_resume_threshold;

        void pfc_handler(Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, const Address &from, const Address &to, NetDevice::PacketType packetType);

        void send_pause(Ptr<const Packet> trigger, uint8_t priority);

        void send_resume(Ptr<const Packet> trigger, uint8_t priority);

    public:
        static TypeId GetTypeId();

        TypeId GetInstanceTypeId() const override;

        RDMADeviceDriver() = default;

        RDMADeviceDriver(double pfc_pause_threshold, double pfc_resume_threshold);

        void Install(Ptr<NetDevice> device);
    };
}

#endif //NS3_RDMA_DEVICE_DRIVER_H
