//
//  Action.h
//  Core
//
//  Copyright (c) 2016 FiftyThree, Inc. All rights reserved.
//

#pragma once

namespace fiftythree {
namespace core {

// A base class for actions.  These might be:
//
// * Operations that mutate the model.
// * Instructions to operate a UI remotely.
// * A stream of operations to be performed by the Ink Engine".
//
// The name action was chosen because "command" was already taken in our codebase.
class Action
{
protected:
    std::string _ModelId;
public:
    // Adding a virtual destructor will make the Action type polymorphic
    virtual ~Action() {}
    
    const std::string &ModelId() { return _ModelId; }
};

}
}
