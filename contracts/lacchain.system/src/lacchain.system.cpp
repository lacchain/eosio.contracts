#include <lacchain.system/lacchain.system.hpp>
#include <lacchain.system/safe.hpp>

namespace lacchainsystem {

void lacchain::setabi( name account, const std::vector<char>& abi ) {
   abi_hash_table table(get_self(), get_self().value);
   auto itr = table.find( account.value );
   if( itr == table.end() ) {
      table.emplace( account, [&]( auto& row ) {
         row.owner = account;
         row.hash  = eosio::sha256(const_cast<char*>(abi.data()), abi.size());
      });
   } else {
      table.modify( itr, eosio::same_payer, [&]( auto& row ) {
         row.hash = eosio::sha256(const_cast<char*>(abi.data()), abi.size());
      });
   }
}

void lacchain::onerror( uint128_t sender_id, const std::vector<char>& sent_trx ) {
   eosio::check( false, "the onerror action cannot be called directly" );
}

void lacchain::setpriv( name account, uint8_t is_priv ) {
   require_auth( get_self() );
   set_privileged( account, is_priv );
}

void lacchain::setalimits( name account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight ) {
   require_auth( get_self() );
   set_resource_limits( account, ram_bytes, net_weight, cpu_weight );
}

void lacchain::setparams( const eosio::blockchain_parameters& params ) {
   require_auth( get_self() );
   set_blockchain_parameters( params );
}

void lacchain::reqauth( name from ) {
   require_auth( from );
}

void lacchain::activate( const eosio::checksum256& feature_digest ) {
   require_auth( get_self() );
   preactivate_feature( feature_digest );
}

void lacchain::reqactivated( const eosio::checksum256& feature_digest ) {
   eosio::check( is_feature_activated( feature_digest ), "protocol feature is not activated" );
}

void lacchain::newaccount( name creator, name name, const authority& owner, const authority& active ) {

   entity_table entities(get_self(), get_self().value);
   auto itr = entities.find( creator.value );

   if( itr == entities.end() ) {
      itr = entities.find( name.value );
      eosio::check(itr != entities.end(), "Entity not found");
      eosio::check(itr->type == entity_type::VALIDATOR || itr->type == entity_type::WRITER, "Only validators and writers can have an account");
      eosio::check(creator == get_self(), "Only the permissioning committee can create an entity account");
   } else {
      eosio::check(itr->type == entity_type::WRITER, "Only writers entities can create new accounts");
      eosio::check(validate_authority(itr->name, active), "invalid active authority");
      eosio::check(validate_authority(itr->name, owner), "invalid active authority");
   }
}

void lacchain::addvalidator( const name& validator,
                             const authority& owner,
                             const authority& active,
                             const eosio::block_signing_authority& validator_authority,
                             const std::string& url ) {
   require_auth( get_self() );
   
   add_entity(validator, [&]( auto& e ) {
      e.type = entity_type::VALIDATOR;
      e.url  = url;
      e.bsa  = validator_authority;
   });

   newaccount_action(get_self()).send( get_self(), validator, owner, active );
}

void lacchain::addwriter( const name& writer, const authority& owner,
                          const authority& active, const std::string& url ) {
   require_auth( get_self() );
   
   add_entity(writer, [&]( auto& e ) {
      e.type = entity_type::WRITER;
      e.url  = url;
   });

   newaccount_action(get_self()).send( get_self(), writer, owner, active );
}

void lacchain::addboot( const name& boot, const std::string& url ) {
   require_auth( get_self() );

   add_entity(boot, [&]( auto& e ) {
      e.type = entity_type::BOOT;
      e.url  = url;
   });
}

void lacchain::addobserver( const name& observer, const std::string& url ) {
   require_auth( get_self() );

   add_entity(observer, [&]( auto& e ) {
      e.type = entity_type::OBSERVER;
      e.url = url;
   });
}

void lacchain::addnetlink( const name& entityA, const name& entityB, int direction ) {
   require_auth( get_self() );

   netlink_table netlinks( get_self(), get_self().value );
   auto index = netlinks.get_index<"pair"_n>();
   auto itr = index.find( netlink::make_key(entityA.value, entityB.value) );

   auto update_link_record = [&]( auto& l ) {
      l.entityA   = entityA;
      l.entityB   = entityB;
      l.direction = direction;
   };

   if( itr == index.end() ) {
      entity_table entities(get_self(), get_self().value);

      auto itr = entities.find( entityA.value );
      eosio::check(itr != entities.end(), "entity A not found");
      itr = entities.find( entityB.value );
      eosio::check(itr != entities.end(), "entity B not found");

      netlinks.emplace( get_self(), update_link_record);
   } else {
      netlinks.modify( *itr, eosio::same_payer, update_link_record);
   }
}

void lacchain::rmnetlink( const name& entityA, const name& entityB ) {
   require_auth( get_self() );

   netlink_table netlinks( get_self(), get_self().value );
   auto index = netlinks.get_index<"pair"_n>();
   auto itr = index.find( netlink::make_key(entityA.value, entityB.value) );
   eosio::check(itr != entities.end(), "netlink not found");
   index.erase( itr );
}

void lacchain::setschedule( const std::vector<name>& validators ) {
   require_auth( get_self() );

   std::vector<eosio::producer_authority> schedule;
   schedule.resize(validators.size());

   entity_table entities(get_self(), get_self().value);
   for(const auto& v : validators) {
      auto itr = entities.find( v.value );
      eosio::check(itr != entities.end(), "Validator not found");
      eosio::check(!!itr->bsa, "Invalid block signing authority");
      schedule.emplace_back(eosio::producer_authority{itr->name, *itr->bsa});
   }

   eosio::check(schedule.size(), "Schedule cant be empty");
   set_proposed_producers( schedule );
}

bool lacchain::validate_authority(const name& writer, const authority& auth) {

   safe<uint16_t> weight_sum_without_entity = 0;
   safe<uint16_t> entity_weight = 0;
   bool entity_in_authority  = false;

   for( const auto& k : auth.keys) {
      weight_sum_without_entity += safe<uint16_t>(k.weight);
   }

   for( const auto& plw : auth.accounts) {
      if(plw.permission.actor == writer) {
         entity_weight = safe<uint16_t>(plw.weight);
         entity_in_authority = true;
         continue;
      }
      weight_sum_without_entity += safe<uint16_t>(plw.weight);
   }

   return entity_in_authority && 
            weight_sum_without_entity < auth.threshold &&
            weight_sum_without_entity + entity_weight >= auth.threshold;
}

} //lacchainsystem