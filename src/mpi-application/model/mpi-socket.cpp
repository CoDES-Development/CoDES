

#include <exception>

#include <ns3/core-module.h>
#include <ns3/network-module.h>

#include "mpi-socket.h"

ns3::MPISocket::MPISocket(size_t cacheLimit) noexcept: MPISocket(nullptr, cacheLimit) {}

ns3::MPISocket::MPISocket(const NS3Node &node, TypeId typeId, size_t cacheLimit) noexcept: MPISocket(ns3::Socket::CreateSocket(node, typeId), cacheLimit) {}

ns3::MPISocket::MPISocket(const NS3Socket &s, size_t cacheLimit) noexcept: socket(s), cache(Create<Packet>()), cacheLimit(cacheLimit) {
    registerCallbacks();
}

ns3::MPISocket::MPISocket(MPISocket &&s) :
        socket(std::exchange(s.socket, nullptr)),
        connected(s.connected),
        listening(s.listening),
        closed(s.closed),
        pendingAccept(std::exchange(s.pendingAccept, {})),
        pendingConnect(std::exchange(s.pendingConnect, {})),
        pendingSend(std::exchange(s.pendingSend, {})),
        pendingReceive(std::exchange(s.pendingReceive, {})),
        cache(std::exchange(s.cache, nullptr)),
        cacheLimit(s.cacheLimit) {
    registerCallbacks();
};

ns3::MPISocket &ns3::MPISocket::operator=(ns3::MPISocket &&s) {
    if (this == &s) {
        return *this;
    }
    clearCallbacks();
    socket = std::exchange(s.socket, nullptr);
    connected = s.connected;
    listening = s.listening;
    closed = s.closed;
    pendingAccept = std::exchange(s.pendingAccept, {});
    pendingConnect = std::exchange(s.pendingConnect, {});
    pendingSend = std::exchange(s.pendingSend, {});
    pendingReceive = std::exchange(s.pendingReceive, {});
    cache = std::exchange(s.cache, nullptr);
    cacheLimit = s.cacheLimit;
    registerCallbacks();
    return *this;
}

void ns3::MPISocket::onAccept(NS3Socket s, const Address &remoteAddress) {
    if (pendingAccept.empty()) {
        throw std::runtime_error("In-coming connection received but no pending accept");
    }
    auto operation = pendingAccept.front();
    operation.terminate(std::in_place, MPISocket{s, cacheLimit}, remoteAddress, socket->GetErrno());
}

void ns3::MPISocket::onConnect(NS3Error error) {
    if (pendingConnect.empty()) {
        throw std::runtime_error("Out-coming connection established but no pending connect");
    }
    auto operation = pendingConnect.front();
    operation.terminate(error);
}

void ns3::MPISocket::onSend() {
    for (auto operation: pendingSend) { // NOLINT copy operation to avoid iterator invalidation
        if (!operation.resume()) {
            break;
        }
    }
}

void ns3::MPISocket::onReceive() {
    for (auto operation: pendingReceive) { // NOLINT copy operation to avoid iterator invalidation
        if (!operation.resume()) {
            break;
        }
    }
}

void ns3::MPISocket::onClose(NS3Error error) {
    closed = true;
    if (error == Socket::ERROR_NOTERROR) {
        error = Socket::ERROR_SHUTDOWN;
    }
    for (auto operation: pendingAccept) { // NOLINT copy operation to avoid iterator invalidation
        operation.terminate(std::in_place, NS3Socket{}, ns3::Address{}, error);
    }
    for (auto operation: pendingConnect) { // NOLINT copy operation to avoid iterator invalidation
        operation.terminate(error);
    }
    for (auto operation: pendingSend) { // NOLINT copy operation to avoid iterator invalidation
        operation.terminate(std::in_place, 0, error);
    }
    for (auto operation: pendingReceive) { // NOLINT copy operation to avoid iterator invalidation
        operation.terminate(std::in_place, ns3::Ptr<ns3::Packet>{}, error);
    }
}

ns3::MPISocket::AcceptOperation ns3::MPISocket::accept() {
    if (connected || closed || !socket) {
        co_return std::make_tuple(MPISocket{}, Address{}, NS3Error::ERROR_BADF);
    }
    auto operation = makeMPIOperation<std::tuple<MPISocket, Address, NS3Error>>();
    pendingAccept.push_back(operation);
    if (!listening && socket->Listen() != 0) {
        pendingAccept.pop_back();
        operation.terminate(std::make_tuple(MPISocket{}, Address{}, socket->GetErrno()));
        co_return std::move(co_await operation);
    }
    listening = true;
    auto result = std::move(co_await operation);
    pendingAccept.pop_front();
    co_return result;
}

ns3::MPISocket::NS3Error ns3::MPISocket::bind(const Address &address) noexcept {
    if (!socket) {
        return NS3Error::ERROR_BADF;
    }
    if (socket->Bind(address) != 0) {
        return socket->GetErrno();
    }
    return NS3Error::ERROR_NOTERROR;
}

ns3::MPISocket::ConnectOperation ns3::MPISocket::connect(const Address &address) noexcept {
    if (listening || closed || !socket) {
        co_return NS3Error::ERROR_BADF;
    }
    auto operation = makeMPIOperation<NS3Error>();
    pendingConnect.push_back(operation);
    if (socket->Connect(address) != 0) {
        pendingConnect.pop_back();
        operation.terminate(socket->GetErrno());
        co_return std::move(co_await operation);
    }
    connected = true;
    auto result = std::move(co_await operation);
    pendingConnect.pop_front();
    co_return result;
}

ns3::MPISocket::SendOperation ns3::MPISocket::send(NS3Packet packet) noexcept {
    if (isClosed()) {
        co_return std::make_tuple(0, NS3Error::ERROR_BADF);
    }
    SendOperation operation;
    auto size = packet->GetSize();
    if (!socket) {
        // loopback
        operation = makeMPIOperation(
                [packet, this]() {
                    if (isClosed()) {
                        return true;
                    }
                    auto available = cacheLimit - cache->GetSize();
                    if (isBlocked() || available <= 0) {
                        return false;
                    }
                    auto sent = std::min(available, (std::size_t) packet->GetSize());
                    auto fragment = packet->CreateFragment(0, sent);
                    cache->AddAtEnd(fragment);
                    packet->RemoveAtStart(sent);
                    tx_size += sent;
                    Simulator::ScheduleNow(&MPISocket::onReceive, this);
                    return packet->GetSize() <= 0;
                },
                [size, packet]() {
                    return std::make_tuple((size_t) size - packet->GetSize(), NS3Error::ERROR_NOTERROR);
                }
        );
    } else {
        operation = makeMPIOperation(
                [packet, this]() {
                    do {
                        if (isClosed()) {
                            return true;
                        }
                        auto available = socket->GetTxAvailable();
                        if (isBlocked() || available <= 0) {
                            return false;
                        }
                        auto size = std::min(available, packet->GetSize());
                        auto fragment = packet->CreateFragment(0, size);
                        auto sent = socket->Send(fragment);
                        if (sent < 0) {
                            return true;
                        }
                        packet->RemoveAtStart(sent);
                        tx_size += sent;
                    } while (packet->GetSize() > 0);
                    return packet->GetSize() <= 0;
                },
                [size, packet, this]() {
                    if (isClosed()) {
                        return std::make_tuple((size_t) size - packet->GetSize(), NS3Error::ERROR_NOTERROR);
                    }
                    return std::make_tuple((size_t) size - packet->GetSize(), socket->GetErrno());
                }
        );
    }
    if (operation.done()) {
        co_return std::move(co_await std::move(operation));
    }
    pendingSend.push_back(operation);
    auto result = std::move(co_await operation);
    pendingSend.pop_front();
    co_return result;
}

ns3::MPISocket::ReceiveOperation ns3::MPISocket::receive(std::size_t size) noexcept {
    if (isClosed()) {
        co_return std::make_tuple(NS3Packet{}, NS3Error::ERROR_BADF);
    }
    ReceiveOperation operation;
    NS3Packet data = Create<Packet>();
    if (!socket) {
        // loopback
        operation = makeMPIOperation(
                [size, data, this]() {
                    if (isClosed()) {
                        return true;
                    }
                    if (isBlocked() || cache->GetSize() <= 0) {
                        return false;
                    }
                    auto required = size <= 0 ? cache->GetSize() : size - data->GetSize();
                    auto received = std::min(required, (std::size_t) cache->GetSize());
                    auto packet = cache->CreateFragment(0, received);
                    data->AddAtEnd(packet);
                    cache->RemoveAtStart(received);
                    rx_size += received;
                    Simulator::ScheduleNow(&MPISocket::onSend, this);
                    return data->GetSize() >= size;
                },
                [data]() {
                    return std::make_tuple(data, NS3Error::ERROR_NOTERROR);
                }
        );
    } else {
        operation = makeMPIOperation(
                [size, data, this]() {
                    do {
                        if (isClosed()) {
                            return true;
                        }
                        if (isBlocked() || socket->GetRxAvailable() <= 0) {
                            return false;
                        }
                        std::size_t required = size <= 0 ? socket->GetRxAvailable() : size - data->GetSize();
                        auto packet = socket->Recv(required, 0);
                        if (packet == nullptr) {
                            return true;
                        }
                        auto received = packet->GetSize();
                        data->AddAtEnd(packet);
                        rx_size += received;
                    } while (data->GetSize() < size);
                    return data->GetSize() >= size;
                },
                [data, this]() {
                    if (isClosed()) {
                        return std::make_tuple(data, NS3Error::ERROR_NOTERROR);
                    }
                    return std::make_tuple(data, socket->GetErrno());
                }
        );
    }
    if (operation.done()) {
        co_return std::move(co_await std::move(operation));
    }
    pendingReceive.push_back(operation);
    auto result = std::move(co_await operation);
    pendingReceive.pop_front();
    co_return result;
}

ns3::MPISocket::NS3Error ns3::MPISocket::close() noexcept {
    if (!(connected || listening) || !socket || closed) {
        return NS3Error::ERROR_NOTERROR;
    }
    if (socket->Close() != 0) {
        return socket->GetErrno();
    }
    return Socket::ERROR_NOTERROR;
}

ns3::MPISocket::NS3Error ns3::MPISocket::closeSend() noexcept {
    if (!socket || closed) {
        return NS3Error::ERROR_NOTERROR;
    }
    if (socket->ShutdownSend() != 0) {
        return socket->GetErrno();
    }
    return NS3Error::ERROR_NOTERROR;
}

ns3::MPISocket::NS3Error ns3::MPISocket::closeReceive() noexcept {
    if (!socket || closed) {
        return NS3Error::ERROR_NOTERROR;
    }
    if (socket->ShutdownRecv() != 0) {
        return socket->GetErrno();
    }
    return NS3Error::ERROR_NOTERROR;
}

void ns3::MPISocket::block() noexcept {
    blocked = true;
}

void ns3::MPISocket::unblock() noexcept {
    blocked = false;
    onSend();
    onReceive();
}

size_t ns3::MPISocket::txBytes() const noexcept {
    return tx_size;
}

size_t ns3::MPISocket::rxBytes() const noexcept {
    return rx_size;
}

void ns3::MPISocket::registerCallbacks() noexcept {
    if (!socket) {
        return;
    }
    socket->SetAcceptCallback(
            [](auto, auto &) { return true; },
            [this](auto s, auto &address) { onAccept(s, address); }
    );
    socket->SetConnectCallback(
            [this](auto) {
                onConnect(Socket::ERROR_NOTERROR);
            },
            [this](auto) {
                onConnect(socket->GetErrno());
            }
    );
    socket->SetSendCallback([this](auto, auto) { onSend(); });
    socket->SetRecvCallback([this](auto) { onReceive(); });
    socket->SetCloseCallbacks(
            [this](auto) { onClose(Socket::ERROR_NOTERROR); },
            [this](auto) { onClose(socket->GetErrno()); }
    );
}

void ns3::MPISocket::clearCallbacks() noexcept {
    if (!socket) {
        return;
    }
    socket->SetAcceptCallback({}, {});
    socket->SetConnectCallback({}, {});
    socket->SetSendCallback({});
    socket->SetRecvCallback({});
    socket->SetCloseCallbacks({}, {});
}

ns3::MPISocket::~MPISocket() noexcept {
    clearCallbacks();
}

