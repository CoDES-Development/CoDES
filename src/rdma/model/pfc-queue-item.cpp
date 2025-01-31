

#include "pfc-queue-item.h"

ns3::PFCQueueItem::PFCQueueItem(Ptr<Packet> p, const Address &addr, uint16_t protocol, ns3::PFCHeader header) : QueueDiscItem(p, addr, protocol), m_header(header) {}

void ns3::PFCQueueItem::AddHeader() {
    GetPacket()->AddHeader(m_header);
}

const ns3::PFCHeader &ns3::PFCQueueItem::GetHeader() const {
    return m_header;
}

ns3::PFCHeader &ns3::PFCQueueItem::GetHeader() {
    return m_header;
}

bool ns3::PFCQueueItem::Mark() {
    return false;
}

void ns3::PFCQueueItem::Print(std::ostream &os) const {
    os << m_header;
    QueueDiscItem::Print(os);
}

uint32_t ns3::PFCQueueItem::Hash(uint32_t perturbation) const {
    return Hash32((char *) &perturbation, sizeof(perturbation));
}


