#define CATCH_CONFIG_MAIN
#include "../third_party/catch/single_include/catch.hpp"
#include "../schema11.hpp"

TEST_CASE( "dummy test case") {

    std::vector<int> v( 6 );

    REQUIRE( v.size() == Hello() );
}