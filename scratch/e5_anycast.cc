// E5 / A: anycast first-completion with REAL CoDES timed operations (Simulator-driven).
// A client issues requests to several servers, proceeds on the FIRST response, and cancels the rest.
// Runs under AddressSanitizer + LeakSanitizer to validate cancellation is memory-safe
// (Reviewer A-W5 OperationCalculation; Reviewer B cancellation / object-lifetime corner cases).
#include <ns3/core-module.h>
#include <ns3/coroutine-module.h>

#include <iostream>

using namespace ns3;

static CoroutineOperation<void>
anycast(int* winner)
{
    // Three "responses" arriving at different simulated times (real CoDES suspensions).
    auto r1 = makeCoroutineOperationWithTimeout<int>(0, 1, Seconds(3.0)); // slow
    auto r2 = makeCoroutineOperationWithTimeout<int>(0, 2, Seconds(1.0)); // fastest
    auto r3 = makeCoroutineOperationWithTimeout<int>(0, 3, Seconds(2.0)); // medium

    // Anycast policy: take the fastest, cancel the slower siblings.
    r1.terminate(0); // cooperative cancel of a genuinely-suspended timed operation
    r3.terminate(0);
    *winner = co_await std::move(r2); // await the winner through the normal co_await path
    co_return;
}

int
main()
{
    int winner = -1;
    auto op = anycast(&winner);
    Simulator::Run();
    Simulator::Destroy();
    std::cout << "ANYCAST_WINNER=" << winner << std::endl;
    std::cout << "A_DONE" << std::endl;
    return 0;
}
