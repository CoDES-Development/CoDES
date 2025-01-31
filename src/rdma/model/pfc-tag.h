

#ifndef NS3_PFC_TAG_H
#define NS3_PFC_TAG_H

#include <cstdint>

#include <ns3/network-module.h>

namespace ns3 {

    class PFCTag : public Tag {
    private:
        uint32_t m_incoming_device;
    public:
        static TypeId GetTypeId();

        PFCTag(uint32_t incomingDevice = 0);

        ~PFCTag() override = default;

        TypeId GetInstanceTypeId() const override;

        uint32_t GetSerializedSize() const override;

        void Serialize(TagBuffer i) const override;

        void Deserialize(TagBuffer i) override;

        void Print(std::ostream &os) const override;

        uint32_t GetIncomingDevice() const;

        void SetIncomingDevice(uint32_t incoming_device);
    };
}

#endif //NS3_PFC_TAG_H
