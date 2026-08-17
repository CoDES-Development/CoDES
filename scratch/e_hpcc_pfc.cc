// E_HPCC_PFC: validate the CoDES PFC pause-timer coroutine used by HPCC/RDMA.
//
// The only CoDES-coroutine piece in the HPCC/RDMA artifact is
// RDMANodeDriver::register_timer(), which arms a PFC pause using
//   makeCoroutineOperationWithTimeout(false, true, next_update_time)
// and co_awaits it; on timeout it Resumes the paused priority-queue bands and
// recursively re-registers.  This test drives that path directly (no full HPCC
// topology needed) and asserts the pause/resume timing, then runs under
// ASan/LeakSanitizer to confirm the coroutine timer is memory-safe.
//
// Setup: 2 nodes + p2p link (DataRate 512Mbps => PFC quanta = 512bit / 512Mbps
// = 1us). Root PrioQueueDisc with 3 Fifo bands on dev0. Arm PFC at t=1s pausing
// bands 0,1 for 100 quanta (=100us). Expect: paused during [1s, 1s+100us),
// resumed afterwards.

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/point-to-point-module.h>
#include <ns3/traffic-control-module.h>
#include <ns3/rdma-module.h>

using namespace ns3;

static Ptr<RDMANodeDriver> g_drv;
static Ptr<NetDevice> g_dev0;
static Ptr<PrioQueueDisc> g_prio;
static int g_paused_ok = 0;
static int g_resumed_ok = 0;

static Ptr<QueueDisc> Band(uint32_t i) {
    return g_prio->GetQueueDiscClass(i)->GetQueueDisc();
}

static void ArmPfc() {
    rdma::PFCPauseTimeVector pt{};
    pt[0] = 100;
    pt[1] = 100;
    uint16_t mask = (1 << 0) | (1 << 1);
    g_drv->NotifyPauseTimeUpdated(g_dev0, mask, pt);
    std::cout << "PFC armed t=" << Simulator::Now().GetMicroSeconds()
              << "us pause bands 0,1 x100 quanta (=100us)" << std::endl;
}

static void CheckDuring() {
    bool b0 = Band(0)->IsPaused();
    bool b1 = Band(1)->IsPaused();
    bool b2 = Band(2)->IsPaused();
    std::cout << "DURING t=" << Simulator::Now().GetMicroSeconds()
              << "us band0=" << b0 << " band1=" << b1 << " band2=" << b2 << std::endl;
    if (b0 && b1 && !b2) {
        g_paused_ok = 1;
    }
}

static void CheckAfter() {
    bool b0 = Band(0)->IsPaused();
    bool b1 = Band(1)->IsPaused();
    std::cout << "AFTER  t=" << Simulator::Now().GetMicroSeconds()
              << "us band0=" << b0 << " band1=" << b1 << std::endl;
    if (!b0 && !b1) {
        g_resumed_ok = 1;
    }
}

int main() {
    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("512Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("1us"));
    NetDeviceContainer devs = p2p.Install(nodes);
    g_dev0 = devs.Get(0);

    // InternetStackHelper aggregates a TrafficControlLayer (required by both
    // TrafficControlHelper::Install and RDMANodeDriver::Install).
    InternetStackHelper internet;
    internet.Install(nodes);

    // Root PrioQueueDisc with 3 Fifo bands on dev0 (replaces the default qdisc).
    TrafficControlHelper tch;
    uint16_t handle = tch.SetRootQueueDisc("ns3::PrioQueueDisc");
    TrafficControlHelper::ClassIdList cls =
        tch.AddQueueDiscClasses(handle, 3, "ns3::QueueDiscClass");
    tch.AddChildQueueDiscs(handle, cls, "ns3::FifoQueueDisc");
    tch.Install(g_dev0);

    // RDMA node driver (requires TrafficControlLayer, installed above).
    g_drv = CreateObject<RDMANodeDriver>();
    g_drv->Install(nodes.Get(0));

    Ptr<TrafficControlLayer> tc = nodes.Get(0)->GetObject<TrafficControlLayer>();
    g_prio = DynamicCast<PrioQueueDisc>(tc->GetRootQueueDiscOnDevice(g_dev0));
    NS_ABORT_MSG_IF(g_prio == nullptr, "root queue disc is not a PrioQueueDisc");
    NS_ABORT_MSG_IF(g_prio->GetNQueueDiscClasses() < 3, "need >=3 bands");

    Simulator::Schedule(Seconds(1.0), &ArmPfc);
    Simulator::Schedule(Seconds(1.0) + MicroSeconds(10), &CheckDuring);
    Simulator::Schedule(Seconds(1.0) + MicroSeconds(200), &CheckAfter);

    Simulator::Stop(Seconds(2.0));
    Simulator::Run();
    Simulator::Destroy();

    std::cout << "PAUSED_OK=" << g_paused_ok << " RESUMED_OK=" << g_resumed_ok << std::endl;
    if (g_paused_ok && g_resumed_ok) {
        std::cout << "E_HPCC_PFC_PASS: PFC pause-timer coroutine paused bands then "
                     "resumed via co_await timeout"
                  << std::endl;
        return 0;
    }
    std::cout << "E_HPCC_PFC_FAIL" << std::endl;
    return 1;
}
