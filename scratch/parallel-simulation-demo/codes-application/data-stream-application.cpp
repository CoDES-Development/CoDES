#include "data-stream-application.h"

#include <ns3/internet-module.h>

ns3::DataStreamApplication::DataStreamApplication(bool is_src,
                                                  Address m_addresses,
                                                  Address peer_address,
                                                  size_t maxBytes) noexcept
    : is_src(is_src),
      m_address(m_addresses),
      peer_address(peer_address),
      m_maxBytes(maxBytes)
{
}


void
ns3::DataStreamApplication::SetMaxBytes(size_t maxBytes) noexcept
{
    m_maxBytes = maxBytes;
}

void
ns3::DataStreamApplication::SetLocalAddress(Address address) noexcept
{
    m_address = address;
}

void
ns3::DataStreamApplication::SetPeerAddress(Address address) noexcept
{
    peer_address = address;
}

void
ns3::DataStreamApplication::SetIsSrc(bool is_src) noexcept
{
    this->is_src = is_src;
}

void
ns3::DataStreamApplication::StartApplication()
{
    Initialize();
}

void
ns3::DataStreamApplication::StopApplication()
{
    Finalize();
}

ns3::CoroutineOperation<void>
ns3::DataStreamApplication::run()
{
    NS_ASSERT_MSG(m_socket, "socket in DataStreamApplication is null");
    NS_ASSERT_MSG(m_socket->isConnected(), "socket in DataStreamApplication is not connected");
    NS_ASSERT_MSG(!m_socket->isClosed(), "socket in DataStreamApplication is closed");
    running = true;
    if (!m_socket->isBlocked())
    {
        if (m_maxBytes > 0)
        {
            if (is_src)
            {
                auto m_data = Create<Packet>(m_maxBytes);
                co_await m_socket->send(m_data);
            }
            else
            {
                co_await m_socket->receive(m_maxBytes);
            }
        }
        else
        {
            std::cout << "socket in DataStreamApplication is blocked" << std::endl;
        }
    }
}

ns3::CoroutineOperation<void>
ns3::DataStreamApplication::connect(std::size_t cache_limit)
{
    const auto& self = m_address;
    auto node = Application::GetNode();

    CoroutineSocket listener{node, TcpSocketFactory::GetTypeId(), cache_limit};
    listener.bind(self);

    if (!is_src)
    { // dst
        auto [s, a, e] = std::move(co_await listener.accept());
        if (e != NS3Error::ERROR_NOTERROR)
        {
            throw std::runtime_error("error when connecting to src");
            NS_ASSERT_MSG(e == NS3Error::ERROR_NOTERROR, "error when connecting to src");
        }
        NS_ASSERT_MSG(s.isConnected(), "socket in DataStreamApplication is not connected");
        auto ip = retrieveIPAddress(a);
        auto peer_ip = retrieveIPAddress(peer_address);
        NS_ASSERT_MSG(ip == peer_ip, "peer ip is not equal");
        m_socket = std::make_unique<CoroutineSocket>(std::move(s));
        NS_ASSERT_MSG(m_socket->isConnected(), "socket in DataStreamApplication is not connected");
    }
    else
    { // src
        m_socket =
            std::make_unique<CoroutineSocket>(node, TcpSocketFactory::GetTypeId(), cache_limit);
        co_await m_socket->connect(peer_address).then([](auto e) {
            if (e != NS3Error::ERROR_NOTERROR)
            {
                throw std::runtime_error("error when connecting to dst");
                NS_ASSERT_MSG(e == NS3Error::ERROR_NOTERROR, "error when connecting to dst");
            }
        });
    }
    listener.close();
}

ns3::CoroutineOperation<void>
ns3::DataStreamApplication::Initialize(size_t mtu_size)
{
    if (status != Status::INITIAL)
    {
        throw std::runtime_error("DataStreamApplication::Init() should only be called once");
    }
    auto cache_limit = mtu_size * 100;
    co_await connect(cache_limit);
    status = Status::WORKING;
    co_await run();
    std::cout << std::fixed << std::setprecision(9);
    std::cout << "Communication of DataStreamApplication is finished at time: " << Simulator::Now().GetSeconds() << " s"<< std::endl;
    co_return;
}

ns3::CoroutineOperation<void>
ns3::DataStreamApplication::Finalize()
{
    if (status != Status::WORKING)
    {
        throw std::runtime_error("DataStreamApplication::Finalize() should only be called after "
                                 "DataStreamApplication::Init()");
    }

    NS_ASSERT_MSG(m_socket, "socket in DataStreamApplication is null");
    NS_ASSERT_MSG(m_socket->isConnected(), "socket in DataStreamApplication is not connected");
    if (m_socket->isClosed())
    {
        throw std::runtime_error("socket in DataStreamApplication is closed");
    }

    if (is_src)
    {
        auto e = m_socket->closeSend();
        if (e != NS3Error::ERROR_NOTERROR)
        {
            throw std::runtime_error("error when closing send socket");
        }
    }
    else
    {
        auto e = m_socket->closeReceive();
        if (e != NS3Error::ERROR_NOTERROR)
        {
            throw std::runtime_error("error when closing receive socket");
        }
    }

    status = Status::FINALIZED;
    NS_ASSERT_MSG(m_socket->isClosed(), "socket in DataStreamApplication is not closed");
    running = false;
    co_return;
}

bool
ns3::DataStreamApplication::Initialized() const noexcept
{
    return status == Status::WORKING;
}

bool
ns3::DataStreamApplication::Finalized() const noexcept
{
    return status == Status::FINALIZED;
}
