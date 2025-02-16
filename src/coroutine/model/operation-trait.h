

#ifndef NS3_COROUTINE_OPERATION_TRAIT_H
#define NS3_COROUTINE_OPERATION_TRAIT_H

#include <type_traits>

#include "operation-type.h"

namespace ns3 {
    template<typename T>
    struct CoroutineOperationMatch : std::false_type {
    };

    template<typename R>
    struct CoroutineOperationMatch<CoroutineOperation<R>> : std::true_type {
        using ResultType = R;
    };

    template<typename T>
    concept CoroutineOperationConcept=CoroutineOperationMatch<std::decay_t<T>>::value;

    template<CoroutineOperationConcept T>
    using CoroutineOperationResultType = CoroutineOperationMatch<std::decay_t<T>>::ResultType;

    template<typename T>
    concept ImmediateAwaitableConcept =requires(T t){
        { t.await_ready() } -> std::convertible_to<bool>;
        { t.await_suspend(std::declval<std::coroutine_handle<>>()) } -> std::same_as<void>;
        { t.await_resume() };
    };

    template<typename T>
    concept ConditionalAwaitableConcept =requires(T t){
        { t.await_ready() } -> std::convertible_to<bool>;
        { t.await_suspend(std::declval<std::coroutine_handle<>>()) } -> std::convertible_to<bool>;
        { t.await_resume() };
    };

    template<typename T>
    concept YieldAwaitableConcept =requires(T t){
        { t.await_ready() } -> std::convertible_to<bool>;
        { t.await_suspend(std::declval<std::coroutine_handle<>>()) } -> std::convertible_to<std::coroutine_handle<>>;
        { t.await_resume() };
    };

    template<typename T>
    concept AwaitableConcept = ImmediateAwaitableConcept<T> or ConditionalAwaitableConcept<T> or YieldAwaitableConcept<T>;

    template<AwaitableConcept T>
    using AwaitableResultType = std::decay_t<decltype(std::declval<T>().await_resume())>;
}
#endif //NS3_COROUTINE_OPERATION_TRAIT_H
