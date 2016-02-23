#include <iostream>
#include <fstream>
#include <string>
#include "../third_party/catch/single_include/catch.hpp"
#include "../third_party/json11/json11.hpp"
#include "../schema11.hpp"

using namespace json11;
using namespace schema11;
using namespace std;

//
// Pretty, consumable types
//

class Action
{
protected:
    string _ModelId;
public:
    virtual ~Action() {}
    string ModelId() { return _ModelId; }
};

class BoardMoveCardAction : public Action
{
    friend Schema BindBoardMoveCardActionSchema(BoardMoveCardAction &action);
    int _FromIndex;
    int _ToIndex;
public:
    static constexpr auto kTypeName = "boardMoveCard";
    BoardMoveCardAction() : _FromIndex(0), _ToIndex(0) {}
    virtual ~BoardMoveCardAction() {}
    int FromIndex() { return _FromIndex; }
    int ToIndex() { return _ToIndex; }
};

class BoardSetTitleAction : public Action
{    
    friend Schema BindBoardSetTitleActionSchema(BoardSetTitleAction &action);
    string _Title;
public:
    static constexpr auto kTypeName = "boardSetTitle";
    virtual ~BoardSetTitleAction() {}
    string Title() { return _Title; }
};

//
// Schemas
//

Schema BindBoardMoveCardActionSchema(BoardMoveCardAction &action)
{
    return Schema::object {
        { "modelId", action._ModelId },
        { "fromIndex", action._FromIndex },
        { "toIndex", action._ToIndex }
    };
}

Schema BindBoardSetTitleActionSchema(BoardSetTitleAction &action)
{
    return Schema::object {
        { "modelId", action._ModelId },
        { "title", action._Title }
    };
}

//
// Action registry
//

class ActionRegistry
{
    using ActionWithBoundSchema = std::pair<std::shared_ptr<Action>, Schema>;
    map<string, std::function<ActionWithBoundSchema()>> _Entries;
    
public:

    template<class T>
    void Add(std::function<Schema(T &action)> bindSchema)
    {
        _Entries[T::kTypeName] = [bindSchema] {
            auto action = make_shared<T>();
            auto schema = bindSchema(*action);
            return make_pair(action, schema);
        };
    }
    
    ActionWithBoundSchema Get(const string &typeName)
    {
        return _Entries[typeName]();
    }
};

//
// Test Cases
//

TEST_CASE("can parse an array of distinct actions")
{
    // Create an Action Registry -- how and when this is done in a production
    // app is TBD. Perhaps there's a way to automate this? Perhaps it just
    // doesn't matter?
    ActionRegistry registry;
    registry.Add<BoardMoveCardAction>(&BindBoardMoveCardActionSchema);
    registry.Add<BoardSetTitleAction>(&BindBoardSetTitleActionSchema);
    
    // Construct a sample message queue
    auto messageQueue = Json::array {
        Json::object {
            { "type", "action" },
            { "action", Json::object {
                { "type", "boardMoveCard" },
                { "modelId", "1987123987" },
                { "fromIndex", 5 },
                { "toIndex", 7 }
            }}
        },
        Json::object {
            { "type", "action" },
            { "action", Json::object {
                { "type", "boardSetTitle" },
                { "modelId", "7798712398" },
                { "title", "Crazy title!" }
            }}
        }
    };
 
    // Process the message queue!
    vector<shared_ptr<Action>> actions;
    for (auto message : messageQueue) {
        if (message["type"] == "action") {
            auto action = message["action"];
            auto actionWithBoundSchema = registry.Get(action["type"].string_value());
            actionWithBoundSchema.second.from_json(action);
            actions.push_back(actionWithBoundSchema.first);
        }
    }
    
    
    REQUIRE(actions.size() == 2);
    
    auto action0 = dynamic_pointer_cast<BoardMoveCardAction>(actions[0]);
    REQUIRE(action0 != nullptr);
    REQUIRE(action0->ModelId() == "1987123987");
    REQUIRE(action0->FromIndex() == 5);
    REQUIRE(action0->ToIndex() == 7);
    
    auto action1 = dynamic_pointer_cast<BoardSetTitleAction>(actions[1]);
    REQUIRE(action1 != nullptr);
    REQUIRE(action1->ModelId() == "7798712398");
    REQUIRE(action1->Title() == "Crazy title!");
}