// E6: exception propagation through co_await, validated under AddressSanitizer + LeakSanitizer.
// An exception thrown inside a co_await'ed operation must be stored into the Promise
// (std::exception_ptr), re-thrown at the awaiting site (result()), and be catchable by an
// ordinary try/catch in the awaiting coroutine -- with clean coroutine-frame release on the
// exceptional path (Reviewer B: exception-propagation corner case).
#include <ns3/core-module.h>
#include <ns3/coroutine-module.h>

#include <iostream>
#include <stdexcept>

using namespace ns3;

// Case 1: exception thrown synchronously inside the operation body (eager start).
static CoroutineOperation<int>
throwImmediately()
{
    throw std::runtime_error("boom-sync");
    co_return 0; // unreachable; makes this a coroutine
}

// Case 2: exception thrown AFTER a real suspension (async failure), driven by resume().
static CoroutineOperation<int>
throwAfterSuspend()
{
    co_await std::suspend_always{};
    throw std::runtime_error("boom-async");
    co_return 0; // unreachable
}

// Awaiting coroutine: co_await the operation and catch any propagated exception.
static CoroutineOperation<void>
client(CoroutineOperation<int> op, const char* tag, int* caught)
{
    try
    {
        int v = co_await op;
        std::cout << tag << " UNEXPECTED_VALUE=" << v << std::endl;
    }
    catch (const std::exception& e)
    {
        *caught = 1;
        std::cout << tag << " CAUGHT: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << tag << " CAUGHT_UNKNOWN" << std::endl;
    }
    co_return;
}

int
main()
{
    int c1 = 0;
    int c2 = 0;

    // Case 1 -- synchronous throw inside the operation.
    {
        auto op = throwImmediately();
        auto cl = client(op, "E6_SYNC", &c1);
    }

    // Case 2 -- throw after a genuine suspension, resumed by the Simulator.
    auto op2 = throwAfterSuspend();
    auto cl2 = client(op2, "E6_ASYNC", &c2);
    Simulator::Schedule(Seconds(1.0), [op2]() { op2.resume(); });
    Simulator::Run();
    Simulator::Destroy();

    std::cout << "E6_SYNC_CAUGHT=" << c1 << std::endl;
    std::cout << "E6_ASYNC_CAUGHT=" << c2 << std::endl;
    if (c1 && c2)
    {
        std::cout << "E6_PASS: exception propagated through co_await in both cases" << std::endl;
    }
    else
    {
        std::cout << "E6_FAIL" << std::endl;
    }
    return 0;
}
