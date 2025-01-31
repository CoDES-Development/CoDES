

#ifndef NS3_PFC_QUEUE_ITEM_H
#define NS3_PFC_QUEUE_ITEM_H

#include <ns3/network-module.h>

#include "pfc-header.h"

namespace ns3 {
    template<typename T>
    class Ptr;

    class PFCQueueItem : public QueueDiscItem {
    public:
        PFCQueueItem(Ptr<Packet> p, const Address &addr, uint16_t protocol, PFCHeader header);

        ~PFCQueueItem() override = default;

        void AddHeader() override;

        const PFCHeader &GetHeader() const;

        PFCHeader &GetHeader();

        bool Mark() override;

        void Print(std::ostream &os) const override;

        uint32_t Hash(uint32_t perturbation = 0) const override;

    private:
        PFCHeader m_header;
    };
}

#endif //NS3_PFC_QUEUE_ITEM_H
