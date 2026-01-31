#ifndef DATA_STREAM_APPLICATION_APPLICATION_H
#define DATA_STREAM_APPLICATION_APPLICATION_H

#include "ns3/mpi-communicator.h"
#include <ns3/core-module.h>
#include <ns3/coroutine-module.h>
#include <ns3/network-module.h>

namespace ns3
{
// application only contains two nodes : src & dst
class DataStreamApplication : public Application
{
  public:
    enum class Status
    {
        INITIAL,
        WORKING,
        FINALIZED,
    };

  private:
    using NS3Node = Ptr<Node>;

    bool running = false;
    Status status = Status::INITIAL;

    std::unique_ptr<CoroutineSocket> m_socket;
    bool is_src = true; // src or dst
    Address m_address;
    Address peer_address;
    size_t m_maxBytes = 0;

    CoroutineOperation<void> connect(size_t cache_limit);

  public:
    DataStreamApplication(bool is_src,
                          Address m_addresses,
                          Address peer_address,
                          size_t maxBytes) noexcept;

    void SetMaxBytes(size_t maxBytes) noexcept;

    void SetLocalAddress(Address address) noexcept;
    void SetPeerAddress(Address address) noexcept;
    void SetIsSrc(bool is_src) noexcept;

    void StartApplication() override;

    void StopApplication() override;

    CoroutineOperation<void> run();

    CoroutineOperation<void> Initialize(size_t mtu_size = 1492);

    ns3::CoroutineOperation<void> Finalize();

    bool Initialized() const noexcept;

    bool Finalized() const noexcept;

    ~DataStreamApplication() override = default;
};
} // namespace ns3
#endif // DATA_STREAM_APPLICATION_H
