#include <lacchain.system/lacchain.system.hpp>
#include <lacchain.system/safe.hpp>
//#include <eosiolib/core/datastream.hpp>
namespace lacchainsystem {

   const constexpr int64_t new_entity_ram = 1024*10;
   const constexpr int64_t new_entity_cpu = 1024;
   const constexpr int64_t new_entity_net = 1024;

   const constexpr int64_t entity_with_writer_cpu = 1024*100;
   const constexpr int64_t entity_with_writer_net = 1024*100;

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
      eosio::check(creator == get_self(), "Only the permissioning committee can create an entity account");

      entity_table entities(get_self(), get_self().value);
      auto itr = entities.find( name.value );
      eosio::check( itr != entities.end(), "Entity does not exists" );

      //set resources to newly created entity account
      set_resource_limits( name, new_entity_ram, new_entity_cpu, new_entity_net);
   } else {
      //OPTIONAL?: read the full tx and verify that this action has been authorized 
      //           by one of the "writers" authorities

      eosio::check(validate_newuser_authority(active), "invalid active authority");
      eosio::check(validate_newuser_authority(owner), "invalid owner authority");

      //0 CPU/NET for user accounts
      int64_t ram, cpu, net;
      get_resource_limits( name, ram, cpu, net);
      set_resource_limits( name, ram, 0, 0);
   }
}

void lacchain::addentity(const name& entity_name,
                         const uint64_t entity_type,
                         const std::optional<eosio::public_key> pubkey) {

   require_auth( get_self() );

   // Add new entity to the entities table
   entity_table entities(get_self(), get_self().value);
   auto itr = entities.find( entity_name.value );
   eosio::check( itr == entities.end(), "An entity with that name already exists" );

   entities.emplace( get_self(), [&]( auto& e ) {
      e.name = entity_name;
      e.type = entity_type;
   });

   auto active_authority = authority{
      1, {}, 
      {{{"eosio"_n, "active"_n}, 1}},{}
   };

   auto owner_authority = authority{
      1, {},
      {{{"eosio"_n, "active"_n}, 1}},{}
   };

   // If specified, add provided key to the active and owner authority 
   if( pubkey ) {
      active_authority.keys = {{*pubkey, 1}};
      owner_authority.keys = {{*pubkey, 1}};
   }

   // Create new entity account 
   newaccount_action(get_self(), {get_self(), "active"_n}).send(
      get_self(), 
      entity_name, 
      owner_authority,
      active_authority
   );

}



void lacchain::add_new_node( const name& node_name,
                             const node_type node_type,
                             const name& entity,
                             const std::optional<eosio::block_signing_authority> bsa ) {

   entity_table entities(get_self(), get_self().value);
   auto itr = entities.find( entity.value );
   eosio::check( itr != entities.end(), "Entity not found" ); 
   
   require_auth( entity );

   if(itr->type == entity_type::NON_PARTNER) {
      eosio::check(node_type == node_type::WRITER, "Non-partner entities can only add WRITER nodes");
   }

   node_table nodes(get_self(), get_self().value);
   auto itr_node = nodes.find( entity.value );
   eosio::check(itr_node == nodes.end(), "A node with the same name already exists");

   nodes.emplace( get_self(), [&]( auto& e ) {
      e.name     = node_name;
      e.entity   = entity;
      e.type     = node_type;
      e.bsa      = bsa;
      e.info     = "";
      e.reserved = 0;
   });
}

void lacchain::addvalidator( const name& name,
                             const struct name& entity,
                             const eosio::block_signing_authority& validator_authority) {
   add_new_node(name, node_type::VALIDATOR, entity, validator_authority);
}

void lacchain::addwriter( const name& name,
                          const struct name& entity,
                          const authority& writer_authority) {
   add_new_node(name, node_type::WRITER, entity, {});

   //Add new permission to the entity account for this writer node
   updateauth_action(get_self(), {entity, "active"_n}).send( entity, name, "owner"_n, writer_authority);

   //Add the new authority to the writer::access permission
   writers_table wrt(get_self(), get_self().value);
   auto itr = wrt.find( uint64_t(1) );

   std::vector<permission_level_weight> writers;
   if( itr == wrt.end()) {
      wrt.emplace( get_self(), [&]( auto& row ) {
         row.id      = uint64_t(1);
         row.writers.push_back({{entity, name},1});
         writers = row.writers;
      });
   } else {
      wrt.modify( itr, eosio::same_payer, [&]( auto& row ) {
         row.writers.push_back({{entity, name},1});
         writers = row.writers;
      });
   }
   
   authority auth;
   auth.threshold = 1;
   auth.accounts  = writers;

   updateauth_action(get_self(), {"writer"_n, "active"_n}).send( "writer"_n, "access"_n, "owner"_n, auth);

   //1/N CPU/NET for entity account with at least one writer node
   int64_t ram, cpu, net;
   get_resource_limits( name, ram, cpu, net);
   set_resource_limits( name, ram, 0, 0);

   set_resource_limits( entity, ram, entity_with_writer_cpu, entity_with_writer_net );
}

void lacchain::addboot( const name& name,
                        const struct name& entity ) {
   add_new_node(name, node_type::BOOT, entity, {});
}

void lacchain::addobserver( const name& name,
                            const struct name& entity ) {
   add_new_node(name, node_type::OBSERVER, entity, {});
}

void lacchain::netaddgroup(const name& name, const std::vector<struct name>& nodes) {
   require_auth( get_self() );

   netgroup_table netgroups(get_self(), get_self().value);
   auto itr_group = netgroups.find( name.value );
   eosio::check(itr_group == netgroups.end(), "A group with the same name already exists");

   netgroups.emplace( get_self(), [&]( auto& row ) {
      row.name = name;
      row.nodes = nodes;
   });
}

void lacchain::netrmgroup(const name& name) {
   require_auth( get_self() );

   netgroup_table netgroups(get_self(), get_self().value);
   auto itr_group = netgroups.find( name.value );
   eosio::check(itr_group != netgroups.end(), "The group does not exists");

   netgroups.erase( itr_group );
}

void lacchain::netsetgroup(const name& node, const std::vector<struct name>& groups) {
   require_auth( get_self() );

   node_table nodes(get_self(), get_self().value);
   auto itr_node = nodes.find( node.value );
   eosio::check(itr_node != nodes.end(), "node does not exists");

   nodes.modify( itr_node, eosio::same_payer, [&]( auto& row ) {
      row.net_groups = groups;
   });
}

void lacchain::setschedule( const std::vector<name>& validators ) {
   require_auth( get_self() );

   //TODO: change to vector<eosio::producer_authority>
   std::vector<eosio::producer_key> schedule;
   schedule.resize(validators.size());

   //TODO: use standard serialization
   char* buffer = (char*)malloc(1024);
   eosio::datastream<char*> ds(buffer, 1024);
   
   ds << eosio::unsigned_int(validators.size());

   node_table nodes(get_self(), get_self().value);
   for(const auto& v : validators) {
      auto itr = nodes.find( v.value );
      eosio::check(itr != nodes.end() && itr->type == node_type::VALIDATOR, "Validator not found");
      eosio::check(!!itr->bsa, "Invalid block signing authority");
      schedule.push_back(eosio::producer_key{
         itr->name, std::get<eosio::block_signing_authority_v0>(*itr->bsa).keys[0].key
      });

      eosio::print("name =>", "[", itr->name ,"][", v.value, "][", itr->name.value, "]");
      ds << v.value;
      ds << std::get<eosio::block_signing_authority_v0>(*itr->bsa).keys[0].key;
   }
   
   //TODO: check serialization   
   eosio::check(schedule.size(), "Schedule cant be empty");
   
   eosio::print(" => [");
   eosio::printhex((char*)buffer, ds.tellp());
   eosio::print(" ]");
   //auto packed_prods = eosio::pack( schedule );
   eosio::internal_use_do_not_use::set_proposed_producers((char*)buffer, ds.tellp());
   //set_proposed_producers( schedule );
}

bool lacchain::validate_newuser_authority(const authority& auth) {

   safe<uint16_t> weight_sum_without_entity = 0;
   safe<uint16_t> entity_weight = 0;
   bool writer_in_authority  = false;

   for( const auto& k : auth.keys) {
      weight_sum_without_entity += safe<uint16_t>(k.weight);
   }

   for( const auto& plw : auth.accounts) {
      if(plw.permission.actor == "writer"_n) {
         eosio::check(plw.permission.permission == "access"_n, "only `access` permission is allowed");
         entity_weight = safe<uint16_t>(plw.weight);
         writer_in_authority = true;
      } else { 
         weight_sum_without_entity += safe<uint16_t>(plw.weight);
      }
   }

   return writer_in_authority && 
          weight_sum_without_entity < auth.threshold &&
          weight_sum_without_entity + entity_weight == auth.threshold;
}

void lacchain::setnodeinfo(const name& node, const std::string& info) {

   node_table nodes(get_self(), get_self().value);
   auto itr_node = nodes.find( node.value );
   eosio::check(itr_node != nodes.end(), "node does not exists");

   require_auth( itr_node->entity );

   nodes.modify( itr_node, eosio::same_payer, [&]( auto& row ) {
      row.info = info;
   });
}

void lacchain::setnodexinfo(const name& node, const std::string& ext_info) {

   node_table nodes(get_self(), get_self().value);
   auto itr_node = nodes.find( node.value );
   eosio::check(itr_node != nodes.end(), "node does not exists");

   require_auth( get_self() );

   nodes.modify( itr_node, eosio::same_payer, [&]( auto& row ) {
      row.ext_info = ext_info;
   });
}

void lacchain::setentinfo(const name& entity, const std::string& info) {
   entity_table entities(get_self(), get_self().value);
   auto itr_entity = entities.find( entity.value );
   eosio::check(itr_entity != entities.end(), "entity does not exists");

   require_auth( itr_entity->name );

   entities.modify( itr_entity, eosio::same_payer, [&]( auto& row ) {
      row.info = info;
   });
}

void lacchain::setentxinfo(const name& entity, const std::string& ext_info) {
   entity_table entities(get_self(), get_self().value);
   auto itr_entity = entities.find( entity.value );
   eosio::check(itr_entity != entities.end(), "entity does not exists");

   require_auth( get_self() );

   entities.modify( itr_entity, eosio::same_payer, [&]( auto& row ) {
      row.ext_info = ext_info;
   });
}


} //lacchainsystem