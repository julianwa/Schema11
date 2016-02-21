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

TEST_CASE("can create a flat schema")
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

TEST_CASE("can create a nested schema")
{
    Schema subSchema = Schema::object {
        { "nestedIntProp", Schema(Schema::Type::NUMBER) },
        { "nestedBoolProp", Schema(Schema::Type::BOOL) }
    };
    
    Schema schema = Schema::object {
        { "intProp", Schema(Schema::Type::NUMBER) },
        { "nestedProps", subSchema }
    };
    REQUIRE(schema.is_object());
    REQUIRE(schema["intProp"].is_number());
    REQUIRE(schema["nestedProps"].is_object());
    REQUIRE(schema["nestedProps"]["nestedIntProp"].is_number());
    REQUIRE(schema["nestedProps"]["nestedBoolProp"].is_bool());
}

TEST_CASE("testing value converter")
{
	{
		int intValue;
		ValueConverter converter = PrimitiveConverter(intValue);
		converter.FromJson(Json(5));
		REQUIRE(intValue == 5);
	}
	{
		int intValue = 6;
		Json json;
		PrimitiveConverter(intValue).ToJson(json);
		REQUIRE(json.int_value() == 6);
	}
	{
		bool boolValue = false;
		ValueConverter converter = PrimitiveConverter(boolValue);
		converter.FromJson(Json(true));
		REQUIRE(boolValue == true);
	}
	{
		string stringValue;
		ValueConverter converter = PrimitiveConverter(stringValue);
		converter.FromJson(Json("string"));
		REQUIRE(stringValue == "string");
	}
}

TEST_CASE("can process simple schema")
{
	struct Nested
	{
		int stringProp;
	};
	struct Simple
	{
		int intProp;
		bool boolProp;
		Nested nestedProp;
	};
	
	Simple simple;
	
    Schema subSchema = Schema::object {
        { "nestedBoolProp", Schema(Schema::Type::STRING) /*, PrimitiveConverter(&simple.nestedProp.stringProp)*/ }
    };
	
	Schema schema = Schema::object {
		{ "intProp", Schema(Schema::Type::NUMBER)/*, PrimitiveConverter(&simple.intProp) */ },
		{ "boolProp", Schema(Schema::Type::BOOL)/*, PrimitiveConverter(&simple.boolProp) */ },
		{ "nestedProp", subSchema}
	};
	
	string err;
	auto json = Json::parse("{ \"intProp\": 5 }", err);
	REQUIRE(err.empty());
	REQUIRE(json["intProp"] == 5);
}