

#include <ns3/traffic-control-module.h>

#include "pfc-tag.h"
#include "rdma-node-driver.h"

NS_LOG_COMPONENT_DEFINE("RDMANodeDriver");

void ns3::RDMANodeDriver::PauseTimer::Update() {
    if (operation != std::nullopt) {
        operation->terminate(false);
        operation = std::nullopt;
    }
    rdma::PFCPauseTimeInQuantaType delta = ((Simulator::Now() - last_updated_time) / quanta).Round();
    for (size_t i = 0; i < rdma::PFC_PAUSE_TIME_VECTOR_LENGTH; i++) {
        if (pause_time[i] > 0) {
            pause_time[i] -= delta;
        }
    }
}

void ns3::RDMANodeDriver::process_timer(Ptr<NetDevice> device, ns3::RDMANodeDriver::PauseTimer &timer) {
    auto priority_queue = rdma::get_priority_queue(device);
    for (size_t i = 0; i < std::min(priority_queue->GetNQueueDiscClasses(), rdma::PFC_PAUSE_TIME_VECTOR_LENGTH); i++) {
        if (timer.pause_time[i] > 0) {
            priority_queue->GetQueueDiscClass(i)->GetQueueDisc()->Pause();
        } else {
            priority_queue->GetQueueDiscClass(i)->GetQueueDisc()->Resume();
        }
    }
}

ns3::CoroutineOperation<void> ns3::RDMANodeDriver::register_timer(Ptr<NetDevice> device, ns3::RDMANodeDriver::PauseTimer &timer) {
    auto range = timer.pause_time | std::views::filter([](auto x) { return x > 0; });
    if (std::ranges::empty(range)) {
        co_return;
    }
    auto next_update = std::ranges::min(range);
    auto next_update_time = timer.quanta * next_update;
    auto operation = makeCoroutineOperationWithTimeout(false, true, next_update_time);
    timer.operation = operation;
    if (!co_await operation) {
        co_return;
    }
    timer.operation = std::nullopt;
    timer.Update();
    process_timer(device, timer);
    register_timer(device, timer);
}

ns3::TypeId ns3::RDMANodeDriver::GetTypeId() {
    static TypeId tid = TypeId("ns3::RDMANodeDriver")
            .SetParent<Object>()
            .SetGroupName("RDMA")
            .AddConstructor<RDMANodeDriver>();
    return tid;
}

ns3::TypeId ns3::RDMANodeDriver::GetInstanceTypeId() const {
    return GetTypeId();
}

void ns3::RDMANodeDriver::Install(Ptr<Node> node) {
    m_node = node;
    auto tc = node->GetObject<TrafficControlLayer>();
    if (tc == nullptr) {
        NS_LOG_WARN("node " << node << " does not have a traffic control layer, skip it");
        return;
    }
    tc->RegisterProtocolHandler(MakeCallback(&RDMANodeDriver::TagPacket, this), WILDCARD_PROTOCOL, nullptr);
}

void ns3::RDMANodeDriver::TagPacket(Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, const Address &from, const Address &to, NetDevice::PacketType packetType) {
    packet->AddPacketTag(PFCTag{device->GetIfIndex()});
}

void ns3::RDMANodeDriver::NotifyPauseTimeUpdated(Ptr<NetDevice> device, uint16_t mask, ns3::rdma::PFCPauseTimeVector pause_time_in_quanta) {
    if (!timers.contains(device)) {
        auto speed = rdma::get_data_rate(device);
        auto quanta = speed.CalculateBitsTxTime(QUANTA_BIT_SIZE);
        PauseTimer timer{quanta, Simulator::Now(), {}, std::nullopt};
        timers[device] = timer;
    } else {
        timers[device].Update();
    }
    for (size_t i = 0; i < rdma::PFC_PAUSE_TIME_VECTOR_LENGTH; i++) {
        if ((mask & (1 << i)) != 0) {
            timers[device].pause_time[i] = pause_time_in_quanta[i];
        }
    }
    process_timer(device, timers[device]);
    register_timer(device, timers[device]);
}