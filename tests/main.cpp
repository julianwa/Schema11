#include <string>

#define CATCH_CONFIG_MAIN
#include "../third_party/catch/single_include/catch.hpp"
#include "../schema11.hpp"
#include "../third_party/json11/json11.hpp"

using namespace json11;
using namespace std;

TEST_CASE( "json11 can parse JSON strings") {

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