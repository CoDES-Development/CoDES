
#include "coroutine-datagram-socket.h"

namespace ns3 {

    CoroutineDatagramSocket::CoroutineDatagramSocket(const NS3Socket &s) noexcept: socket(s) {
        registerCallbacks();
    }

    void CoroutineDatagramSocket::registerCallbacks() noexcept {
        if (!socket) {
            return;
        }
        socket->SetRecvCallback(MakeCallback(&CoroutineDatagramSocket::handleRecv, this));
    }

    void CoroutineDatagramSocket::clearCallbacks() noexcept {
        if (!socket) {
            return;
        }
        socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }

    void CoroutineDatagramSocket::handleRecv(Ptr<Socket>) {
        onReceive();
    }

    void CoroutineDatagramSocket::onReceive() {
        // Resume the head pending receive. When it completes it pops itself (in receiveFrom) and
        // the receiving coroutine may enqueue the next one, so always re-check the front.
        while (!pendingReceive.empty()) {
            auto operation = pendingReceive.front();
            if (!operation.resume()) {
                break;
            }
        }
    }

    CoroutineDatagramSocket::ReceiveFromOperation CoroutineDatagramSocket::receiveFrom() noexcept {
        if (closed || !socket) {
            co_return std::make_tuple(NS3Packet{}, Address{}, NS3Error::ERROR_BADF);
        }
        auto operation = makeCoroutineOperation(
                [this]() -> bool {
                    return closed || (socket && socket->GetRxAvailable() > 0);
                },
                [this]() {
                    Address from;
                    NS3Packet packet;
                    if (socket && !closed) {
                        packet = socket->RecvFrom(from);
                    }
                    return std::make_tuple(packet, from, socket ? socket->GetErrno() : NS3Error::ERROR_BADF);
                }
        );
        if (operation.done()) {
            co_return std::move(co_await std::move(operation));
        }
        pendingReceive.push_back(operation);
        auto result = std::move(co_await operation);
        pendingReceive.pop_front();
        co_return result;
    }

    CoroutineDatagramSocket::NS3Error CoroutineDatagramSocket::bind(const Address &address) noexcept {
        if (!socket) {
            return NS3Error::ERROR_BADF;
        }
        if (socket->Bind(address) != 0) {
            return socket->GetErrno();
        }
        return NS3Error::ERROR_NOTERROR;
    }

    void CoroutineDatagramSocket::close() noexcept {
        closed = true;
        // With closed==true the pending receive's condition is satisfied, so resuming it drives
        // the coroutine to co_return (with a null packet), releasing frames cleanly.
        while (!pendingReceive.empty()) {
            auto operation = pendingReceive.front();
            if (!operation.resume()) {
                break;
            }
        }
        pendingReceive.clear();
    }

    CoroutineDatagramSocket::~CoroutineDatagramSocket() noexcept {
        clearCallbacks();
        close();
    }
}
