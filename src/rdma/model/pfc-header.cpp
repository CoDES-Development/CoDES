

#include "pfc-header.h"

ns3::PFCHeader::PFCHeader(uint16_t mask, const rdma::PFCPauseTimeVector &pause_time) : m_op_code(rdma::PFC_OP_CODE), m_mask(mask), m_pause_time(pause_time) {}

ns3::TypeId ns3::PFCHeader::GetTypeId() {
    static TypeId tid = TypeId("ns3::PFCHeader")
            .SetParent<Header>()
            .SetGroupName("RDMA")
            .AddConstructor<PFCHeader>();
    return tid;
}

ns3::TypeId ns3::PFCHeader::GetInstanceTypeId() const {
    return GetTypeId();
}

uint32_t ns3::PFCHeader::GetSerializedSize() const {
    return SIZE;
}

void ns3::PFCHeader::Serialize(Buffer::Iterator start) const {
    start.WriteU16(m_op_code);
    start.WriteU16(m_mask);
    for (size_t i = 0; i < rdma::PFC_PAUSE_TIME_VECTOR_LENGTH; ++i) {
        start.WriteU16(m_pause_time[i]);
    }
}

uint32_t ns3::PFCHeader::Deserialize(Buffer::Iterator start) {
    m_op_code = start.ReadU16();
    m_mask = start.ReadU16();
    for (size_t i = 0; i < rdma::PFC_PAUSE_TIME_VECTOR_LENGTH; ++i) {
        m_pause_time[i] = start.ReadU16();
    }
    return SIZE;
}

void ns3::PFCHeader::Print(std::ostream &os) const {
    os << "PFCHeader{op_code=" << m_op_code << ", mask=" << m_mask << ", pause_time=[";
    for (size_t i = 0; i < rdma::PFC_PAUSE_TIME_VECTOR_LENGTH; ++i) {
        os << m_pause_time[i] << (i == rdma::PFC_PAUSE_TIME_VECTOR_LENGTH - 1 ? "" : ", ");
    }
    os << "]}";
}

uint16_t ns3::PFCHeader::GetOpCode() const {
    return m_op_code;
}

uint16_t ns3::PFCHeader::GetMask() const {
    return m_mask;
}

const ns3::rdma::PFCPauseTimeVector &ns3::PFCHeader::GetPauseTime() const {
    return m_pause_time;
}
