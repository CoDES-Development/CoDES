

#include "pfc-tag.h"

ns3::TypeId ns3::PFCTag::GetTypeId() {
    static TypeId tid = TypeId("ns3::PFCTag")
            .SetParent<Tag>()
            .SetGroupName("RDMA")
            .AddConstructor<PFCTag>()
            .AddAttribute("incoming_device",
                          "the incoming device index of the packet",
                          UintegerValue(0),
                          MakeUintegerAccessor(&PFCTag::GetIncomingDevice, &PFCTag::SetIncomingDevice),
                          MakeUintegerChecker<uint32_t>());
    return tid;
}

ns3::PFCTag::PFCTag(uint32_t incomingDevice) : m_incoming_device(incomingDevice) {}

ns3::TypeId ns3::PFCTag::GetInstanceTypeId() const {
    return GetTypeId();
}

uint32_t ns3::PFCTag::GetSerializedSize() const {
    return sizeof(m_incoming_device) ;
}

void ns3::PFCTag::Serialize(TagBuffer i) const {
    i.WriteU32(m_incoming_device);
}

void ns3::PFCTag::Deserialize(TagBuffer i) {
    m_incoming_device = i.ReadU32();
}

void ns3::PFCTag::Print(std::ostream &os) const {
    os << "Incoming device: " << m_incoming_device;
}

uint32_t ns3::PFCTag::GetIncomingDevice() const {
    return m_incoming_device;
}

void ns3::PFCTag::SetIncomingDevice(uint32_t incoming_device) {
    m_incoming_device = incoming_device;
}
