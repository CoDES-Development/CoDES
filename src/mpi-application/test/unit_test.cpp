#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include <ns3/mpi-application-module.h>
#include <ns3/mpi-application.h>

#include <cassert>
#include <coroutine>
#include <exception>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ThreeNodeTcpTopology");

// test functions: send，recv，scatter，gather，broadcast/bcast，reduce，allreduce，alltoall

ns3::MPIOperation<void>
TestSendRecv(MPICommunicator& communicator0, MPICommunicator& communicator1)
{
    // fake packet cannot be tested
    std::cout << "TestSendRecv begin" << std::endl;

    // test send & recv
    auto packet1 = Create<Packet>(1024);
    auto packet1_size = packet1->GetSize();
    co_await communicator0.Send(RawPacket, 1, packet1);
    auto packet2 = co_await communicator1.Recv(RawPacket, 0, 1024);
    NS_ASSERT_MSG(packet1_size == packet2->GetSize(),
                  "packet1->GetSize() = " << packet1_size << " != packet2->GetSize()"
                                          << packet2->GetSize());
    std::cout << "TestSendRecv pass: packet1 " << packet1->ToString() << " == packet2"
              << packet2->ToString() << std::endl;

    // test send & recv only with data
    co_await communicator0.Send(1, 12345);
    auto packet3 = co_await communicator1.Recv<int>(0);
    NS_ASSERT_MSG(packet3 == 12345, "int packet3 != 1");
    std::cout << "TestSendRecv pass: packet3 == 12345" << std::endl;

    std::cout << "TestSendRecv end" << std::endl;
}

ns3::MPIOperation<void>
TestScatter(MPICommunicator& communicator0,
            MPICommunicator& communicator1,
            MPICommunicator& communicator2)
{
    std::cout << "TestScatter begin" << std::endl;

    auto data = std::unordered_map<MPIRankIDType, int>{
        {0, 4},
        {1, 5},
        {2, 6},
    };
    auto data1 = std::unordered_map<MPIRankIDType, int>{
        {0, 1},
        {1, 1},
        {2, 1},
    };

    // test scatter
    auto op0 = communicator0.Scatter(0, data);
    auto op1 = communicator1.Scatter(0, data1);
    auto op2 = communicator2.Scatter(0, data1);
    auto packet0 = co_await op0;
    auto packet1 = co_await op1;
    auto packet2 = co_await op2;

    NS_ASSERT_MSG(packet0 == 4, "packet0 != 4");
    NS_ASSERT_MSG(packet1 == 5, "packet1 != 5");
    NS_ASSERT_MSG(packet2 == 6, "packet2 != 6");

    std::cout << "TestScatter pass: packet0 == 4, packet1 == 5, packet2 == 6" << std::endl;

    // fake packet cannot be tested

    std::cout << "TestScatter end" << std::endl;
}

ns3::MPIOperation<void>
TestGather(MPICommunicator& communicator0,
           MPICommunicator& communicator1,
           MPICommunicator& communicator2)
{
    std::cout << "TestGather begin" << std::endl;

    auto data0 = std::vector<int>{100};
    auto data1 = std::vector<int>{200};
    auto data2 = std::vector<int>{300};

    // test gather
    auto op0 = communicator0.Gather(0, data0);
    auto op1 = communicator1.Gather(0, data1);
    auto op2 = communicator2.Gather(0, data2);
    auto packet0 = co_await op0;
    auto packet1 = co_await op1;
    auto packet2 = co_await op2;

    for (auto& [rank, value] : packet0)
    {
        NS_ASSERT_MSG((int)(value[0]) == (int)(rank * 100 + 100), "gather value is incorrect");
    }

    std::cout << "TestGather pass" << std::endl;

    std::cout << "TestGather end" << std::endl;
}

ns3::MPIOperation<void>
TestBroadcast(MPICommunicator& communicator0,
              MPICommunicator& communicator1,
              MPICommunicator& communicator2)
{
    std::cout << "TestBroadcast begin" << std::endl;

    auto data0 = std::optional<int>{1};
    auto data1 = std::optional<int>{10};
    auto data2 = std::optional<int>{10};

    // test broadcast
    auto op0 = communicator0.Broadcast(0, data0);
    auto op1 = communicator1.Broadcast(0, data1);
    auto op2 = communicator2.Broadcast(0, data2);
    auto packet0 = co_await op0;
    auto packet1 = co_await op1;
    auto packet2 = co_await op2;

    NS_ASSERT_MSG(packet0 == 1, "packet0 != 1");
    NS_ASSERT_MSG(packet1 == 1, "packet1 != 1");
    NS_ASSERT_MSG(packet2 == 1, "packet2 != 1");

    std::cout << "TestBroadcast pass: packet0 == 1, packet1 == 1, packet2 == 1" << std::endl;

    std::cout << "TestBroadcast end" << std::endl;
}

ns3::MPIOperation<void>
TestReduce(MPICommunicator& communicator0,
           MPICommunicator& communicator1,
           MPICommunicator& communicator2)
{
    std::cout << "TestReduce begin" << std::endl;

    auto data0 = 1;
    auto data1 = 2;
    auto data2 = 3;

    // test reduce
    auto op0 = communicator0.Reduce<MPIOperator::MAX>(0, data0);
    auto op1 = communicator1.Reduce<MPIOperator::MAX>(0, data1);
    auto op2 = communicator2.Reduce<MPIOperator::MAX>(0, data2);
    auto packet0 = co_await op0;
    auto packet1 = co_await op1;
    auto packet2 = co_await op2;

    NS_ASSERT_MSG(packet0 == 3, "packet0 != 3");
    NS_ASSERT_MSG(!packet1.has_value(), "packet1.has_value()");
    NS_ASSERT_MSG(!packet2.has_value(), "packet2.has_value()");
    std::cout << "TestReduce pass: root packet max == 3" << std::endl;

    auto op4 = communicator0.Reduce<MPIOperator::SUM>(0, data0);
    auto op5 = communicator1.Reduce<MPIOperator::SUM>(0, data1);
    auto op6 = communicator2.Reduce<MPIOperator::SUM>(0, data2);
    auto packet4 = co_await op4;
    auto packet5 = co_await op5;
    auto packet6 = co_await op6;

    NS_ASSERT_MSG(packet4 == 6, "packet4 != 6");
    NS_ASSERT_MSG(!packet5.has_value(), "packet5.has_value()");
    NS_ASSERT_MSG(!packet6.has_value(), "packet6.has_value()");
    std::cout << "TestReduce pass: root packet sum == 6" << std::endl;

    std::cout << "TestReduce end" << std::endl;
}

ns3::MPIOperation<void>
TestAllReduce(MPICommunicator& communicator0,
              MPICommunicator& communicator1,
              MPICommunicator& communicator2)
{
    std::cout << "TestAllReduce begin" << std::endl;

    auto data0 = 1;
    auto data1 = 2;
    auto data2 = 3;
    
    auto op0 = communicator0.AllReduce<MPIOperator::MAX>(data0);
    auto op1 = communicator1.AllReduce<MPIOperator::MAX>(data1);
    auto op2 = communicator2.AllReduce<MPIOperator::MAX>(data2);
    auto packet0 = co_await op0;
    auto packet1 = co_await op1;
    auto packet2 = co_await op2;
    
    NS_ASSERT_MSG(packet0 == 3, "packet0 != 3");
    NS_ASSERT_MSG(packet1 == 3, "packet1 != 3");
    NS_ASSERT_MSG(packet2 == 3, "packet2 != 3");
    std::cout << "TestAllReduce pass: packet max == " << packet0 << std::endl;

    // root is 0
    auto op4 = communicator0.AllReduce<MPIOperator::SUM>(1);
    auto op5 = communicator1.AllReduce<MPIOperator::SUM>(2);
    auto op6 = communicator2.AllReduce<MPIOperator::SUM>(3);
    auto packet4 = co_await op4;
    auto packet5 = co_await op5;
    auto packet6 = co_await op6;

    NS_ASSERT_MSG(packet4 == 6, "packet4 != 6");
    NS_ASSERT_MSG(packet5 == 6, "packet5 != 6");
    NS_ASSERT_MSG(packet6 == 6, "packet6 != 6");
    std::cout << "TestAllReduce pass: packet sum == " << packet4 << std::endl;

    std::cout << "TestAllReduce end" << std::endl;
}

ns3::MPIOperation<void>
TestAllToAll(MPICommunicator& communicator0,
             MPICommunicator& communicator1,
             MPICommunicator& communicator2)
{
    std::cout << "TestAllToAll begin" << std::endl;

    auto data0 = std::unordered_map<MPIRankIDType, int>{{0, 0}, {1, 1}, {2, 2}};
    auto data1 = std::unordered_map<MPIRankIDType, int>{{0, 1}, {1, 2}, {2, 3}};
    auto data2 = std::unordered_map<MPIRankIDType, int>{{0, 2}, {1, 3}, {2, 4}};

    // test alltoall
    auto op0 = communicator0.AllToAll(data0);
    auto op1 = communicator1.AllToAll(data1);
    auto op2 = communicator2.AllToAll(data2);
    auto packet0 = co_await op0;
    auto packet1 = co_await op1;
    auto packet2 = co_await op2;

    // test packet0
    for (auto& [rank, value] : packet0)
    {
        NS_ASSERT_MSG(value == (int)rank + 0, "value != data[rank]");
    }
    // test packet1
    for (auto& [rank, value] : packet1)
    {
        NS_ASSERT_MSG(value == (int)rank + 1, "value != data[rank]");
    }
    // test packet2
    for (auto& [rank, value] : packet2)
    {
        NS_ASSERT_MSG(value == (int)rank + 2, "value != data[rank]");
    }

    std::cout << "TestAllToAll pass: value == data[rank]" << std::endl;

    std::cout << "TestAllToAll end" << std::endl;
}

ns3::MPIOperation<void>
TestAll(std::vector<Ptr<MPIApplication>> mpi_applications)
{
    NS_ASSERT_MSG(mpi_applications.size() >= 3, "mpi_applications.size() < 3");

    NS_ASSERT_MSG(mpi_applications[0]->Initialized(),
                  "mpi_applications[0]->Initialized() == false");
    NS_ASSERT_MSG(mpi_applications[1]->Initialized(),
                  "mpi_applications[1]->Initialized() == false");
    NS_ASSERT_MSG(mpi_applications[2]->Initialized(),
                  "mpi_applications[2]->Initialized() == false");

    MPICommunicator& communicator0 = mpi_applications[0]->communicator(WORLD_COMMUNICATOR);
    MPICommunicator& communicator1 = mpi_applications[1]->communicator(WORLD_COMMUNICATOR);
    MPICommunicator& communicator2 = mpi_applications[2]->communicator(WORLD_COMMUNICATOR);

    co_await TestSendRecv(communicator0, communicator1);
    co_await TestScatter(communicator0, communicator1, communicator2);
    co_await TestGather(communicator0, communicator1, communicator2);
    co_await TestBroadcast(communicator0, communicator1, communicator2);
    co_await TestReduce(communicator0, communicator1, communicator2);
    co_await TestAllReduce(communicator0, communicator1, communicator2);
    co_await TestAllToAll(communicator0, communicator1, communicator2);
}

int
main(int argc, char* argv[])
{
    // 创建节点：3个终端节点 + 1个路由器
    NodeContainer terminalNodes;
    terminalNodes.Create(3); // 创建3个终端节点

    NodeContainer routerNode;
    routerNode.Create(1); // 创建1个路由器节点

    // 创建每条点对点链路（终端节点与路由器连接）
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2us"));

    // 存储每条链路的设备容器
    NetDeviceContainer devices[3];
    for (int i = 0; i < 3; ++i)
    {
        NodeContainer link(terminalNodes.Get(i), routerNode.Get(0));
        devices[i] = p2p.Install(link);
    }

    // 为每个节点安装 Internet 协议栈
    InternetStackHelper stack;
    stack.Install(terminalNodes);
    stack.Install(routerNode);

    // 为每条链路分配 IP 地址
    Ipv4AddressHelper address;
    Ipv4InterfaceContainer interfaces[3];
    for (int i = 0; i < 3; ++i)
    {
        std::ostringstream subnet;
        subnet << "10.1." << i + 1 << ".0";
        address.SetBase(subnet.str().c_str(), "255.255.255.0");
        interfaces[i] = address.Assign(devices[i]);
    }

    // 启动全局路由
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // 安装mpi应用
    std::map<MPIRankIDType, Address> addresses;
    std::map<Address, MPIRankIDType> ranks;
    ApplicationContainer applications;
    for (int i = 0; i < 3; ++i)
    { // 准备addresses和ranks
        MPIRankIDType rankID = i;
        addresses[rankID] = InetSocketAddress{interfaces[i].GetAddress(0), 1000};
        ranks[interfaces[i].GetAddress(0)] = rankID;
    }

    // 输出map addresses和ranks
    for (auto& [rank, address] : addresses)
    {
        std::cout << "rank: " << rank << " address: " << address << std::endl;
    }
    for (auto& [address, rank] : ranks)
    {
        std::cout << "address: " << address << " rank: " << rank << std::endl;
    }

    std::vector<Ptr<MPIApplication>> mpi_applications;
    for (int i = 0; i < 3; ++i)
    { // 安装，启动3个mpi应用
        std::queue<std::function<MPIOperation<void>(MPIApplication&)>> functions{};
        functions.emplace([=](MPIApplication& application) -> ns3::MPIOperation<void> {
            co_await application.Initialize();
        });
        auto mpi_application = Create<MPIApplication>(i, addresses, ranks, std::move(functions));
        terminalNodes.Get(i)->AddApplication(mpi_application);
        applications.Add(mpi_application);
        mpi_applications.push_back(mpi_application);
    }

    applications.Start(Seconds(0.0));
    applications.Stop(Seconds(100.0));

    // test
    // lamba表达式
    ns3::Simulator::Schedule(Seconds(5.0),
                             [mpi_applications]() -> void { TestAll(mpi_applications); });

    // 启动仿真
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
