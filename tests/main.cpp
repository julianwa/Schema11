#include <string>

#define CATCH_CONFIG_MAIN
#include "../third_party/catch/single_include/catch.hpp"
#include "../schema11.hpp"
#include "../third_party/json11/json11.hpp"

using namespace json11;
using namespace schema11;
using namespace std;

TEST_CASE("json11 can parse JSON strings") 
{
	const string jsonStr = R"({
		"number": 5,
		"string": "string"
	})";

	string err;
	const auto json = Json::parse(jsonStr, err);

	REQUIRE(err.empty());
	REQUIRE(json["number"].is_number());
	REQUIRE(json["string"].is_string());
}

TEST_CASE("can create an empty schema") 
{
    Schema schema;
}

TEST_CASE("can create a simple schema")
{
    Schema schema = Schema::object {
        { "nullProp", Schema(nullptr) },
        { "intProp", Schema(Schema::Type::NUMBER) },
        { "boolProp", Schema(Schema::Type::BOOL) },
        { "stringProp", Schema(Schema::Type::STRING) }
    };
    REQUIRE(schema.is_object());
    REQUIRE(schema["noSuchProp"].is_null());
    REQUIRE(schema["nullProp"].is_null());
    REQUIRE(schema["intProp"].is_number());
    REQUIRE(schema["boolProp"].is_bool());
    REQUIRE(schema["stringProp"].is_string());
}