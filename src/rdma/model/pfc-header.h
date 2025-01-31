


#ifndef NS3_PFC_HEADER_H
#define NS3_PFC_HEADER_H

#include <array>
#include <inttypes.h>

#include <ns3/network-module.h>

#include "rdma-utility.h"

namespace ns3 {
    class PFCHeader : public Header {
    private:
        constinit static const uint32_t SIZE = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(rdma::PFCPauseTimeVector);

        uint16_t m_op_code;
        uint16_t m_mask;
        rdma::PFCPauseTimeVector m_pause_time;
    public:
        PFCHeader() = default;

        explicit PFCHeader(uint16_t mask, const rdma::PFCPauseTimeVector &pause_time);

        ~PFCHeader() override = default;

        static TypeId GetTypeId();

        TypeId GetInstanceTypeId() const override;

        uint32_t GetSerializedSize() const override;

        void Serialize(Buffer::Iterator start) const override;

        uint32_t Deserialize(Buffer::Iterator start) override;

        void Print(std::ostream &os) const override;

        uint16_t GetOpCode() const;

        uint16_t GetMask() const;

        const rdma::PFCPauseTimeVector &GetPauseTime() const;
    };
}

#endif //NS3_PFC_HEADER_H
