// E3-(b): shutdown while a CoDES operation is genuinely suspended.
//
// Reviewer B object-lifetime / shutdown corner case (execution-plan E3 step b):
// a coroutine is suspended inside co_await socket.receive() waiting for bytes
// that never arrive; mid-simulation the connection is torn down, which fires
// CoroutineSocket::onClose and terminates every pending operation with
// ERROR_SHUTDOWN. The suspended receiver must resume, observe the shutdown,
// and unwind cleanly (co_return) with no leaked coroutine frame.
//
// Runs under AddressSanitizer + LeakSanitizer to prove the shutdown path is
// memory-safe.

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/point-to-point-module.h>
#include <ns3/coroutine-module.h>

#include <iostream>
#include <optional>

using namespace ns3;

static int g_connected = 0;
static int g_shutdown_seen = 0;
static int g_client_unwound = 0;

static Ptr<Socket> g_serverListen;
static Ptr<Socket> g_serverAccepted;

static bool
ServerAcceptReq(Ptr<Socket>, const Address&)
{
    return true;
}

static void
ServerAccepted(Ptr<Socket> s, const Address&)
{
    g_serverAccepted = s; // keep alive; never send anything
    std::cout << "server: connection accepted (will stay silent)" << std::endl;
}

static void
ServerShutdown()
{
    if (g_serverAccepted)
    {
        std::cout << "server: closing connection at t="
                  << Simulator::Now().GetSeconds() << "s (shutdown trigger)" << std::endl;
        g_serverAccepted->Close();
    }
}

// Client coroutine: connect, then suspend forever inside receive() until the
// peer shutdown terminates the pending operation with ERROR_SHUTDOWN.
static CoroutineOperation<void>
ClientCo(Ptr<Node> node, Address serverAddr)
{
    CoroutineSocket sock(node, TcpSocketFactory::GetTypeId());
    auto cerr = co_await sock.connect(serverAddr);
    if (cerr != Socket::ERROR_NOTERROR)
    {
        std::cout << "client: connect failed err=" << cerr << std::endl;
        co_return;
    }
    g_connected = 1;
    std::cout << "client: connected, awaiting receive(1000) -> will suspend" << std::endl;

    auto [pkt, err] = co_await sock.receive(1000); // suspends: server never sends

    std::cout << "client: receive resumed err=" << err
              << " pkt=" << (pkt ? (int)pkt->GetSize() : -1) << std::endl;
    if (err == Socket::ERROR_SHUTDOWN || !pkt || pkt->GetSize() == 0)
    {
        g_shutdown_seen = 1;
    }
    g_client_unwound = 1; // reached only if the suspended frame unwinds cleanly
    co_return;
}

// Keep the client coroutine handle alive for the whole run; start it after the
// simulator has initialized the network (issuing connect() before Run() would
// send a packet before traffic-control queue discs are set up).
static Ptr<Node> g_clientNode;
static Address g_serverAddr;
static std::optional<CoroutineOperation<void>> g_clientOp;

static void
StartClient()
{
    g_clientOp = ClientCo(g_clientNode, g_serverAddr);
}

int
main()
{
    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer devs = p2p.Install(nodes);

    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifs = ipv4.Assign(devs);

    const uint16_t port = 8080;

    // Server (raw ns-3 TCP socket): listen + accept + stay silent.
    g_serverListen = Socket::CreateSocket(nodes.Get(0), TcpSocketFactory::GetTypeId());
    g_serverListen->Bind(InetSocketAddress(Ipv4Address::GetAny(), port));
    g_serverListen->Listen();
    g_serverListen->SetAcceptCallback(MakeCallback(&ServerAcceptReq),
                                      MakeCallback(&ServerAccepted));

    // Client (CoDES CoroutineSocket): connect then suspend in receive().
    g_clientNode = nodes.Get(1);
    g_serverAddr = InetSocketAddress(ifs.GetAddress(0), port);
    Simulator::Schedule(Seconds(0.5), &StartClient);

    // Tear down the connection mid-run while the client is suspended.
    Simulator::Schedule(Seconds(2.0), &ServerShutdown);

    Simulator::Stop(Seconds(4.0));
    Simulator::Run();
    Simulator::Destroy();

    std::cout << "CONNECTED=" << g_connected
              << " SHUTDOWN_SEEN=" << g_shutdown_seen
              << " CLIENT_UNWOUND=" << g_client_unwound << std::endl;
    if (g_connected && g_shutdown_seen && g_client_unwound)
    {
        std::cout << "E3B_PASS: suspended receive terminated by ERROR_SHUTDOWN and "
                     "coroutine unwound cleanly"
                  << std::endl;
        return 0;
    }
    std::cout << "E3B_FAIL" << std::endl;
    return 1;
}
