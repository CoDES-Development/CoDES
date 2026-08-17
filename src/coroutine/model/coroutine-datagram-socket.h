

#ifndef NS3_COROUTINE_DATAGRAM_SOCKET_H
#define NS3_COROUTINE_DATAGRAM_SOCKET_H

#include <deque>
#include <tuple>

#include <ns3/core-module.h>
#include <ns3/network-module.h>

#include "operation.h"

namespace ns3 {

    /**
     * \brief A thin coroutine wrapper around an ns-3 Socket providing DATAGRAM (UDP) semantics.
     *
     * Unlike CoroutineSocket::receive() -- which is stream oriented (Socket::Recv + byte-count
     * accumulation, no peer address, TCP-sized cache) -- receiveFrom() uses Socket::RecvFrom to
     * deliver ONE datagram together with its sender Address, and preserves the per-packet tags
     * (e.g. Ipv4PacketInfoTag / SocketIpTtlTag) that datagram protocols such as RIP require.
     *
     * A co_await on receiveFrom() suspends until the underlying socket has data (driven by the
     * socket's receive callback), then resumes and returns {packet, fromAddress, errno}.
     */
    class CoroutineDatagramSocket {
    private:
        using NS3Socket = Ptr<Socket>;
        using NS3Packet = Ptr<Packet>;
        using NS3Error = Socket::SocketErrno;
        using ReceiveFromOperation = CoroutineOperation<std::tuple<NS3Packet, Address, NS3Error>>;
        using ReceiveFromOperationQueue = std::deque<ReceiveFromOperation>;

        NS3Socket socket;
        bool closed = false;
        ReceiveFromOperationQueue pendingReceive;

        void registerCallbacks() noexcept;

        void clearCallbacks() noexcept;

        void handleRecv(Ptr<Socket> socket);

        void onReceive();

    public:
        explicit CoroutineDatagramSocket(const NS3Socket &s) noexcept;

        CoroutineDatagramSocket(const CoroutineDatagramSocket &) = delete;

        CoroutineDatagramSocket &operator=(const CoroutineDatagramSocket &) = delete;

        /**
         * \brief Receive a single datagram.
         * \return a coroutine operation yielding {packet, senderAddress, errno}.
         */
        ReceiveFromOperation receiveFrom() noexcept;

        NS3Error bind(const Address &address) noexcept;

        constexpr inline bool isClosed() const noexcept {
            return closed;
        }

        /**
         * \brief Mark closed and drain any pending receive so coroutine frames complete/release.
         */
        void close() noexcept;

        ~CoroutineDatagramSocket() noexcept;
    };
}

#endif //NS3_COROUTINE_DATAGRAM_SOCKET_H
