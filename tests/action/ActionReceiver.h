//
//  ActionReceiver.h
//  Core
//
//  Copyright (c) 2016 FiftyThree, Inc. All rights reserved.
//

#pragma once

#include <functional>
#include "Action.h"


///////////////////////////////////

template <typename Type, typename Collection>
struct CanExecuteAction;

template <typename Type>
struct CanExecuteAction<Type, std::tuple<>> {
    // If we reach this point, then we didn't match any Action types in the list
    typedef std::false_type result;
};

template <typename Type, typename... Others>
struct CanExecuteAction<Type, std::tuple<Type, Others...>> {
    // If we reach this point, then we have a direct match to a Action type in the list
    typedef std::true_type result;
};

template <typename First, typename... Others>
struct CanExecuteAction<fiftythree::core::Action, std::tuple<First, Others...>> {
    // We always accept the Action type. Whether the Action is supported is determined
    // at runtime.
    typedef std::true_type result;
};

template <typename Type, typename First, typename... Others>
struct CanExecuteAction<Type, std::tuple<First, Others...>> {
    // Recurse to see if we find a match for Type in the list of Action types
    typedef typename CanExecuteAction<Type, std::tuple<Others...>>::result result;
};

///////////////////////////////////

struct ActionReceiver {
    virtual void Execute(const std::shared_ptr<fiftythree::core::Action> &action) = 0;
};

template <typename DerivedT>
struct ActionReceiverT {
    template <typename ActionT>
    void Execute(const std::shared_ptr<ActionT> &Action)
    {
        static_assert(CanExecuteAction<ActionT, typename DerivedT::Actions>::result::value,
                      "Receiver is not spec'd to receive Action");
        _Execute(Action);
    }

private:
    template <typename ActionT>
    void _Execute(const std::shared_ptr<ActionT> &);
};

// This class can be used to abstract away a given Action receiver so that
// the party executing the Action doesn't need to know the receiver's type.
class OpaqueActionReceiver
{
public:
    OpaqueActionReceiver()
    {
        _Execute = nullptr;
    }

    template <class ReceiverT>
    OpaqueActionReceiver(const std::shared_ptr<ReceiverT> &receiver)
    {
        _Execute = [receiver](const std::shared_ptr<fiftythree::core::Action> &action) {
            receiver->Execute(action);
        };
    }

    void Execute(const std::shared_ptr<fiftythree::core::Action> &action)
    {
        _Execute(action);
    }

private:
    std::function<void(const std::shared_ptr<fiftythree::core::Action> &action)> _Execute;
};
