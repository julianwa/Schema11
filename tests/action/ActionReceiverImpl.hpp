//
//  ActionReceiverImpl.hpp
//  Core
//
//  Copyright (c) 2016 FiftyThree, Inc. All rights reserved.
//

#pragma once

#include <assert.h>
#include <functional>
#include <memory>

#include "Action.h"

using namespace fiftythree::core;
using std::shared_ptr;

// This class is used to recursively expand the Action types list and instantiate
// any necessary template functions in order to satisfy the linker. By simply referring
// to the function pointers, the template functions will be instantiated.
template <typename ActionReceiverT, typename Actions>
struct InstantiateReceiverMethods;

template <typename ActionReceiverT>
struct InstantiateReceiverMethods<ActionReceiverT, std::tuple<>> {
};

template <typename ActionReceiverT, typename First, typename... Others>
struct InstantiateReceiverMethods<ActionReceiverT, std::tuple<First, Others...>> {
    InstantiateReceiverMethods()
    {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"
        &ActionReceiverT::template ActionReceiverT<ActionReceiverT>::template _Execute<First>;
#pragma clang diagnostic pop
    }
    InstantiateReceiverMethods<ActionReceiverT, std::tuple<Others...>> Next;
};

// This class is used to recursively expand the Action types list and attempt to dynamic_cast
// the generic Action to each type. Rather than execute the Action upon finding a match, we
// return a lambda so that the call stack of the Action handler does not contain the Action
// types list expansion.
template <typename T, typename Actions>
struct PossiblyExecute;

template <typename T>
struct PossiblyExecute<T, std::tuple<>> {
    std::function<void(void)> operator()(const T *receiver,
                                         const std::shared_ptr<Action> &action)
    {
        return [] {
            printf("Action not supported\n");
            assert(0);
        };
    }
};

template <typename T, typename First, typename... Others>
struct PossiblyExecute<T, std::tuple<First, Others...>> {
    std::function<void(void)> operator()(T *receiver,
                                         const std::shared_ptr<Action> &action)
    {
        auto castAction = std::dynamic_pointer_cast<First>(action);
        if (castAction) {
            return [receiver, castAction] {
                receiver->Execute(castAction);
            };
        } else {
            return Next(receiver, action);
        }
    }
    PossiblyExecute<T, std::tuple<Others...>> Next;
};

// This macro needs to be included for every impl. It instantiates any template methods and
// provides the glue that binds the Execute template methods with the impl's Execute methods.

// A variant of the macro above that supports our cpp "impl" pattern.
#define ACTION_RECEIVER_STATIC_IMPL(T)                                                                                 \
    InstantiateReceiverMethods<T, T::Actions> sInstantiateReceiverMethods##T;                                          \
                                                                                                                       \
    template <> template <class ActionT> void ActionReceiverT<T>::_Execute(const std::shared_ptr<ActionT> &action)     \
    {                                                                                                                  \
        static_cast<T##Impl *>(this)->Execute(action);                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    template <> template <> void ActionReceiverT<T>::_Execute(const std::shared_ptr<fiftythree::core::Action> &action) \
    {                                                                                                                  \
        PossiblyExecute<T##Impl, T::Actions>()(static_cast<T##Impl *>(this), action)();                                \
    }

#define ACTION_RECEIVER_DYNAMIC_IMPL(T)                                                                                \
    InstantiateReceiverMethods<T, T::Actions> sInstantiateReceiverMethods##T;                                          \
                                                                                                                       \
    template <> template <class ActionT> void ActionReceiverT<T>::_Execute(const std::shared_ptr<ActionT> &action)     \
    {                                                                                                                  \
        auto basePtr = static_cast<T *>(this);                                                                         \
        auto implPtr = dynamic_cast<T##Impl *>(basePtr);                                                               \
        implPtr->Execute(action);                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    template <> template <> void ActionReceiverT<T>::_Execute(const std::shared_ptr<fiftythree::core::Action> &action) \
    {                                                                                                                  \
        auto basePtr = static_cast<T *>(this);                                                                         \
        auto implPtr = dynamic_cast<T##Impl *>(basePtr);                                                               \
        PossiblyExecute<T##Impl, T::Actions>()(implPtr, action)();                                                     \
    }
