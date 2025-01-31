

#ifndef NS3_INFINIBAND_TRANSPORT_HEADER_H
#define NS3_INFINIBAND_TRANSPORT_HEADER_H

#include <utility>

#include <ns3/core-module.h>
#include <ns3/network-module.h>

namespace ns3 {
    enum InfinibandTransportOpcode : uint8_t {
        RC_SEND_FIRST = 0b00000000,
        RC_SEND_MIDDLE = 0b00000001,
        RC_SEND_LAST = 0b00000010,
        RC_SEND_LAST_WITH_IMMEDIATE = 0b00000011,
        RC_SEND_ONLY = 0b00000100,
        RC_SEND_ONLY_WITH_IMMEDIATE = 0b00000101,
        RC_RDMA_WRITE_FIRST = 0b00000110,
        RC_RDMA_WRITE_MIDDLE = 0b00000111,
        RC_RDMA_WRITE_LAST = 0b00001000,
        RC_RDMA_WRITE_LAST_WITH_IMMEDIATE = 0b00001001,
        RC_RDMA_WRITE_ONLY = 0b00001010,
        RC_RDMA_WRITE_ONLY_WITH_IMMEDIATE = 0b00001011,
        RC_RDMA_READ_REQUEST = 0b00001100,
        RC_RDMA_READ_RESPONSE_FIRST = 0b00001101,
        RC_RDMA_READ_RESPONSE_MIDDLE = 0b00001110,
        RC_RDMA_READ_RESPONSE_LAST = 0b00001111,
        RC_RDMA_READ_RESPONSE_ONLY = 0b00010000,
        RC_ACKNOWLEDGE = 0b00010001,
        RC_ATOMIC_ACKNOWLEDGE = 0b00010010,
        RC_COMPARE_SWAP = 0b00010011,
        RC_FETCH_ADD = 0b00010100,
        RC_RESERVED = 0b00010101,
        RC_SEND_LAST_WITH_INVALIDATE = 0b00010110,
        RC_SEND_ONLY_WITH_INVALIDATE = 0b00010111,

        // only rc opcodes are listed so far
    };

    static_assert(sizeof(InfinibandTransportOpcode) == sizeof(uint8_t));

    class InfinibandBaseTransportHeader : public Header {
    private:
        constinit static const size_t SIZE = 12;
        union Data {
            uint8_t raw[SIZE];
            struct {
                InfinibandTransportOpcode opcode;
                uint8_t solicited_event: 1;
                uint8_t migration_request: 1;
                uint8_t pad_count: 2;
                uint8_t transport_header_version: 4;
                uint16_t partition_key;
                uint8_t ecn: 2;
                uint8_t reserved: 6;
                uint32_t destination_qpn: 24;
                uint8_t acknowledgment_request: 1;
                uint8_t reserved2: 7;
                uint32_t packet_sequence_number: 24;
            };
        };
        static_assert(sizeof(Data) == SIZE);

        Data m_data;
    public:
        InfinibandBaseTransportHeader() = default;

        InfinibandBaseTransportHeader(InfinibandTransportOpcode opcode, bool solicited_event, bool migration_request, uint8_t pad_count, uint8_t transport_header_version, uint16_t partition_key, uint8_t ecn, uint32_t destination_qpn, uint8_t acknowledgment_request, uint32_t packet_sequence_number);

        ~InfinibandBaseTransportHeader() override = default;

        static TypeId GetTypeId();

        TypeId GetInstanceTypeId() const override;

        uint32_t GetSerializedSize() const override;

        void Serialize(Buffer::Iterator start) const override;

        uint32_t Deserialize(Buffer::Iterator start) override;

        void Print(std::ostream &os) const override;

        InfinibandTransportOpcode GetOpcode() const;

        bool GetSolicitedEvent() const;

        bool GetMigrationRequest() const;

        uint8_t GetPadCount() const;

        uint8_t GetTransportHeaderVersion() const;

        uint16_t GetPartitionKey() const;

        uint8_t GetECN() const;

        uint32_t GetDestinationQPN() const;

        bool GetAcknowledgmentRequest() const;

        uint32_t GetPacketSequenceNumber() const;

        void SetOpcode(InfinibandTransportOpcode opcode);

        void SetSolicitedEvent(bool solicited_event);

        void SetMigrationRequest(bool migration_request);

        void SetPadCount(uint8_t pad_count);

        void SetTransportHeaderVersion(uint8_t transport_header_version);

        void SetPartitionKey(uint16_t partition_key);

        void SetECN(uint8_t ecn);

        void SetDestinationQPN(uint32_t destination_qpn);

        void SetAcknowledgmentRequest(bool acknowledgment_request);

        void SetPacketSequenceNumber(uint32_t packet_sequence_number);
    };

    class InfinibandExtendedTransportHeader : public Header {
    private:
        struct Data {
            uint64_t virtual_address;
            uint32_t remote_key;
            uint32_t dma_length;
        };
        constinit static const uint8_t SIZE = sizeof(Data);
        static_assert(SIZE == 16);
    public:
    };
}

#endif //NS3_INFINIBAND_TRANSPORT_HEADER_H
