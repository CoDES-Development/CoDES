
#include <ns3/traffic-control-module.h>

#include "pfc-queue-item.h"
#include "pfc-tag.h"
#include "rdma-device-driver.h"
#include "rdma-node-driver.h"
#include "rdma-utility.h"

namespace ns3 {
    NS_LOG_COMPONENT_DEFINE("RDMADeviceDriver");

    const Mac48Address RDMADeviceDriver::PFC_MULTICAST_ADDRESS = "01:80:C2:00:00:01";

    void RDMADeviceDriver::pfc_handler(Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, const Address &from, const Address &to, NetDevice::PacketType packetType) {
        NS_ASSERT(device != nullptr);
        NS_ASSERT(protocol == MAC_PROTOCOL);
        PFCHeader header;
        packet->PeekHeader(header);
        if (header.GetOpCode() != rdma::PFC_OP_CODE) {
            NS_LOG_WARN("packet is not a PFC packet, skip it");
            return;
        }
        auto driver = device->GetNode()->GetObject<RDMANodeDriver>();
        driver->NotifyPauseTimeUpdated(device, header.GetMask(), header.GetPauseTime());
    }

    void RDMADeviceDriver::send_pause(Ptr<const Packet> trigger, uint8_t priority) {
        PFCTag tag;
        trigger->PeekPacketTag(tag);
        auto node = m_device->GetNode();
        auto device = node->GetDevice(tag.GetIncomingDevice());
        if (device == nullptr) {
            NS_LOG_WARN("device " << tag.GetIncomingDevice() << " not found, skip it");
            return;
        }
        auto tc = node->GetObject<TrafficControlLayer>();
        if (tc == nullptr) {
            NS_LOG_WARN("node " << node << " does not have a traffic control layer, skip it");
            return;
        }
        auto packet = Create<Packet>(rdma::PFC_PADDING_SIZE);
        rdma::PFCPauseTimeVector vector;
        vector[priority] = 0xffff; // 65535
        PFCHeader header{(uint16_t) (1 << priority), vector};
        tc->Send(device, Create<PFCQueueItem>(packet, PFC_MULTICAST_ADDRESS, MAC_PROTOCOL, header));
    }

    void RDMADeviceDriver::send_resume(Ptr<const Packet> trigger, uint8_t priority) {
        PFCTag tag;
        trigger->PeekPacketTag(tag);
        auto node = m_device->GetNode();
        auto device = node->GetDevice(tag.GetIncomingDevice());
        if (device == nullptr) {
            NS_LOG_WARN("device " << tag.GetIncomingDevice() << " not found, skip it");
            return;
        }
        auto tc = node->GetObject<TrafficControlLayer>();
        if (tc == nullptr) {
            NS_LOG_WARN("node " << node << " does not have a traffic control layer, skip it");
            return;
        }
        auto packet = Create<Packet>(rdma::PFC_PADDING_SIZE);
        rdma::PFCPauseTimeVector vector;
        PFCHeader header{(uint16_t) (1 << priority), vector};
        tc->Send(device, Create<PFCQueueItem>(packet, PFC_MULTICAST_ADDRESS, MAC_PROTOCOL, header));
    }

    TypeId RDMADeviceDriver::GetTypeId() {
        static TypeId tid = TypeId("RDMADeviceDriver")
                .SetParent<Object>()
                .SetGroupName("RDMA")
                .AddConstructor<RDMADeviceDriver>()
                .AddAttribute("PfcPauseThreshold", "The threshold to pause the queue", DoubleValue(0.9), MakeDoubleAccessor(&RDMADeviceDriver::m_pfc_pause_threshold), MakeDoubleChecker<double>(0.0, 1.0))
                .AddAttribute("PfcResumeThreshold", "The threshold to resume the queue", DoubleValue(0.5), MakeDoubleAccessor(&RDMADeviceDriver::m_pfc_resume_threshold), MakeDoubleChecker<double>(0.0, 1.0));
        return tid;
    }

    TypeId RDMADeviceDriver::GetInstanceTypeId() const {
        return GetTypeId();
    }

    RDMADeviceDriver::RDMADeviceDriver(double pfc_pause_threshold, double pfc_resume_threshold) : m_pfc_pause_threshold(pfc_pause_threshold), m_pfc_resume_threshold(pfc_resume_threshold) {}

    void RDMADeviceDriver::Install(Ptr<NetDevice> device) {
        NS_ASSERT(device != nullptr);
        auto node = device->GetNode();
        auto tc = node->GetObject<TrafficControlLayer>();
        if (tc == nullptr) {
            NS_LOG_WARN("node " << node << " does not have a traffic control layer, skip it");
            return;
        }
        auto priority_queue = rdma::get_priority_queue(device);
        if (priority_queue == nullptr) {
            NS_LOG_WARN("queue discipline of device " << device << " is not a priority queue, skip it");
            return;
        }
        if (priority_queue->GetNQueueDiscClasses() > rdma::PFC_PAUSE_TIME_VECTOR_LENGTH) {
            NS_LOG_WARN("queue discipline of device " << device << " has too much priorities, skip it");
            return;
        }
        m_device = device;
        tc->RegisterProtocolHandler(MakeCallback(&RDMADeviceDriver::pfc_handler, this), MAC_PROTOCOL, device);
        for (size_t j = 0; j < priority_queue->GetNQueueDiscClasses(); ++j) {
            auto q = priority_queue->GetQueueDiscClass(j)->GetQueueDisc();
            q->TraceConnectWithoutContext("Enqueue", static_cast<QueueCallbackType>( [=, this](auto item) {
                auto max = q->GetMaxSize();
                auto current = q->GetCurrentSize();
                if (current >= QueueSize{max.GetUnit(), static_cast<uint32_t>(max.GetValue() * m_pfc_pause_threshold)}) {
                    send_pause(item->GetPacket(), j);
                }
            }));
            q->TraceConnectWithoutContext("Dequeue", static_cast<QueueCallbackType>( [=, this](auto item) {
                auto max = q->GetMaxSize();
                auto current = q->GetCurrentSize();
                if (current <= QueueSize{max.GetUnit(), static_cast<uint32_t>(max.GetValue() * m_pfc_resume_threshold)}) {
                    send_resume(item->GetPacket(), j);
                }
            }));
        }
    }
}
