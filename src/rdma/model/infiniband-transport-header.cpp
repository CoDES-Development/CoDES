

#include "infiniband-transport-header.h"

namespace ns3 {
    InfinibandBaseTransportHeader::InfinibandBaseTransportHeader(InfinibandTransportOpcode opcode, bool solicited_event, bool migration_request, uint8_t pad_count, uint8_t transport_header_version, uint16_t partition_key, uint8_t ecn, uint32_t destination_qpn, uint8_t acknowledgment_request, uint32_t packet_sequence_number)
            : m_data({.opcode = opcode, .solicited_event = solicited_event, .migration_request = migration_request, .pad_count = pad_count, .transport_header_version = transport_header_version, .partition_key = partition_key, .ecn = ecn, .reserved = 0, .destination_qpn = destination_qpn, .acknowledgment_request = acknowledgment_request, .reserved2 = 0, .packet_sequence_number = packet_sequence_number}) {}

    TypeId InfinibandBaseTransportHeader::GetTypeId() {
        static TypeId tid = TypeId("ns3::InfinibandBaseTransportHeader")
                .SetParent<Header>()
                .AddConstructor<InfinibandBaseTransportHeader>()
                .AddAttribute("Opcode", "The opcode of the packet", UintegerValue(0), MakeUintegerAccessor(&InfinibandBaseTransportHeader::GetOpcode, &InfinibandBaseTransportHeader::SetOpcode), MakeUintegerChecker<uint8_t>())
                .AddAttribute("SolicitedEvent", "The solicited event of the packet", BooleanValue(false), MakeBooleanAccessor(&InfinibandBaseTransportHeader::GetSolicitedEvent, &InfinibandBaseTransportHeader::SetSolicitedEvent), MakeBooleanChecker())
                .AddAttribute("MigrationRequest", "The migration request of the packet", BooleanValue(false), MakeBooleanAccessor(&InfinibandBaseTransportHeader::GetMigrationRequest, &InfinibandBaseTransportHeader::SetMigrationRequest), MakeBooleanChecker())
                .AddAttribute("PadCount", "The pad count of the packet", UintegerValue(0), MakeUintegerAccessor(&InfinibandBaseTransportHeader::GetPadCount, &InfinibandBaseTransportHeader::SetPadCount), MakeUintegerChecker<uint8_t>())
                .AddAttribute("TransportHeaderVersion", "The transport header version of the packet", UintegerValue(0), MakeUintegerAccessor(&InfinibandBaseTransportHeader::GetTransportHeaderVersion, &InfinibandBaseTransportHeader::SetTransportHeaderVersion), MakeUintegerChecker<uint8_t>())
                .AddAttribute("PartitionKey", "The partition key of the packet", UintegerValue(0), MakeUintegerAccessor(&InfinibandBaseTransportHeader::GetPartitionKey, &InfinibandBaseTransportHeader::SetPartitionKey), MakeUintegerChecker<uint16_t>())
                .AddAttribute("ECN", "The ecn of the packet", UintegerValue(0), MakeUintegerAccessor(&InfinibandBaseTransportHeader::GetECN, &InfinibandBaseTransportHeader::SetECN), MakeUintegerChecker<uint8_t>())
                .AddAttribute("DestinationQPN", "The destination qpn of the packet", UintegerValue(0), MakeUintegerAccessor(&InfinibandBaseTransportHeader::GetDestinationQPN, &InfinibandBaseTransportHeader::SetDestinationQPN), MakeUintegerChecker<uint32_t>())
                .AddAttribute("AcknowledgmentRequest", "The acknowledgment request of the packet", UintegerValue(0), MakeUintegerAccessor(&InfinibandBaseTransportHeader::GetAcknowledgmentRequest, &InfinibandBaseTransportHeader::SetAcknowledgmentRequest), MakeUintegerChecker<uint8_t>())
                .AddAttribute("PacketSequenceNumber", "The packet sequence number of the packet", UintegerValue(0), MakeUintegerAccessor(&InfinibandBaseTransportHeader::GetPacketSequenceNumber, &InfinibandBaseTransportHeader::SetPacketSequenceNumber), MakeUintegerChecker<uint32_t>());
        return tid;
    }

    TypeId InfinibandBaseTransportHeader::GetInstanceTypeId() const {
        return GetTypeId();
    }

    uint32_t InfinibandBaseTransportHeader::GetSerializedSize() const {
        return SIZE;
    }

    void InfinibandBaseTransportHeader::Serialize(Buffer::Iterator start) const {
        start.Write(m_data.raw, SIZE);
    }

    uint32_t InfinibandBaseTransportHeader::Deserialize(Buffer::Iterator start) {
        start.Read(m_data.raw, SIZE);
        return SIZE;
    }

    void InfinibandBaseTransportHeader::Print(std::ostream &os) const {
        os << "InfinibandBaseTransportHeader{"
           << "opcode=" << m_data.opcode
           << ", solicited_event=" << m_data.solicited_event
           << ", migration_request=" << m_data.migration_request
           << ", pad_count=" << m_data.pad_count
           << ", transport_header_version=" << m_data.transport_header_version
           << ", partition_key=" << m_data.partition_key
           << ", ecn=" << m_data.ecn
           << ", destination_qpn=" << m_data.destination_qpn
           << ", acknowledgment_request=" << m_data.acknowledgment_request
           << ", packet_sequence_number=" << m_data.packet_sequence_number
           << "}";
    }

    InfinibandTransportOpcode InfinibandBaseTransportHeader::GetOpcode() const {
        return m_data.opcode;
    }

    void InfinibandBaseTransportHeader::SetOpcode(InfinibandTransportOpcode opcode) {
        m_data.opcode = opcode;
    }

    bool InfinibandBaseTransportHeader::GetSolicitedEvent() const {
        return m_data.solicited_event;
    }

    void InfinibandBaseTransportHeader::SetSolicitedEvent(bool solicited_event) {
        m_data.solicited_event = solicited_event;
    }

    bool InfinibandBaseTransportHeader::GetMigrationRequest() const {
        return m_data.migration_request;
    }

    void InfinibandBaseTransportHeader::SetMigrationRequest(bool migration_request) {
        m_data.migration_request = migration_request;
    }

    uint8_t InfinibandBaseTransportHeader::GetPadCount() const {
        return m_data.pad_count;
    }

    void InfinibandBaseTransportHeader::SetPadCount(uint8_t pad_count) {
        m_data.pad_count = pad_count;
    }

    uint8_t InfinibandBaseTransportHeader::GetTransportHeaderVersion() const {
        return m_data.transport_header_version;
    }

    void InfinibandBaseTransportHeader::SetTransportHeaderVersion(uint8_t transport_header_version) {
        m_data.transport_header_version = transport_header_version;
    }

    uint16_t InfinibandBaseTransportHeader::GetPartitionKey() const {
        return m_data.partition_key;
    }

    void InfinibandBaseTransportHeader::SetPartitionKey(uint16_t partition_key) {
        m_data.partition_key = partition_key;
    }

    uint8_t InfinibandBaseTransportHeader::GetECN() const {
        return m_data.ecn;
    }

    void InfinibandBaseTransportHeader::SetECN(uint8_t ecn) {
        m_data.ecn = ecn;
    }

    uint32_t InfinibandBaseTransportHeader::GetDestinationQPN() const {
        return m_data.destination_qpn;
    }

    void InfinibandBaseTransportHeader::SetDestinationQPN(uint32_t destination_qpn) {
        m_data.destination_qpn = destination_qpn;
    }

    bool InfinibandBaseTransportHeader::GetAcknowledgmentRequest() const {
        return m_data.acknowledgment_request;
    }

    void InfinibandBaseTransportHeader::SetAcknowledgmentRequest(bool acknowledgment_request) {
        m_data.acknowledgment_request = acknowledgment_request;
    }

    uint32_t InfinibandBaseTransportHeader::GetPacketSequenceNumber() const {
        return m_data.packet_sequence_number;
    }

    void InfinibandBaseTransportHeader::SetPacketSequenceNumber(uint32_t packet_sequence_number) {
        m_data.packet_sequence_number = packet_sequence_number;
    }
}

