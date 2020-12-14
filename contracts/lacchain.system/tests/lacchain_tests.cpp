#include <eosio/chain/abi_serializer.hpp>
#include <eosio/testing/tester.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include <contracts.hpp>
#include <cmath>

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;
using namespace boost::multiprecision;

using int128 = boost::multiprecision::int128_t;
using int256 = boost::multiprecision::int256_t;
using mvo = fc::mutable_variant_object;

class lacchain_tester : public tester {
public:

    lacchain_tester() {
        produce_blocks( 2 );

        create_accounts( { N(alice), N(eosio.token) } );

//        set_code( N(eosio), contracts::token_wasm() );
        set_code( N(eosio), contracts::lacchain_system_wasm() );
        set_abi( N(eosio), contracts::lacchain_system_abi().data() );

        produce_blocks( 2 );
    }
};

BOOST_AUTO_TEST_SUITE(lacchain_tests)

BOOST_FIXTURE_TEST_CASE( basic_tests, lacchain_tester ) try {
  cout << endl;
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END() 