#include <lacchain.system/lacchain.system.hpp>
#include <lacchain.system/safe.hpp>
//#include <eosiolib/core/datastream.hpp>
namespace lacchainsystem {

const constexpr int64_t new_entity_ram = 1024*1024*10;

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

void lacchain::onblock( const block_header& bh ) {
   //TODO: track schedule here
   eosio::internal_use_do_not_use::prints("test!!");
}

void lacchain::newaccount( name creator, name name, const authority& owner, const authority& active ) {

   entity_table entities(get_self(), get_self().value);
   auto itr = entities.find( creator.value );

   //Check if the creator of the new account is a Lacchain entity
   //this could be for a new validator node or new user account.
   if( itr != entities.end() ) {
      node_table nodes(get_self(), get_self().value);
      auto itr_node = nodes.find( name.value );

      // 1) Check if validator node account is being created
      if( itr_node != nodes.end() ) {
         eosio::check(itr_node->type == node_type::VALIDATOR, "newaccount only for validators nodes");
         transfer_ram_from_entity(creator, name, 10240);

      // 2) A new user account is being created
      } else {
         //OPTIONAL?: read the full tx and verify that this action has been authorized 
         //           by one of the "writers" authorities
         eosio::check(validate_newuser_authority(active), "invalid active authority");
         eosio::check(validate_newuser_authority(owner), "invalid owner authority");

         //0 CPU/NET + 3k RAM for user accounts
         int64_t ram, cpu, net;
         get_resource_limits( name, ram, cpu, net);
         set_resource_limits( name, ram, 0, 0);
         transfer_ram_from_entity(creator, name, 3*1024);
      }
   } else {
      //Check that permissioning committee is creating a new Lacchain entity account
      eosio::check(creator == get_self(), "Only the permissioning committee can create an entity account");

      entity_table entities(get_self(), get_self().value);
      auto itr = entities.find( name.value );
      eosio::check( itr != entities.end(), "Entity does not exists" );

      //set resources to newly created entity account
      set_resource_limits( name, new_entity_ram, new_entity_cpu, new_entity_net);
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
   if (!is_account(entity_name))
      newaccount_action(get_self(), {get_self(), "active"_n}).send(
         get_self(), 
         entity_name, 
         owner_authority,
         active_authority
      );

}

void lacchain::rmentity(const name& entity_name) {

   require_auth( get_self() );

   entity_table entities(get_self(), get_self().value);
   auto entity_itr = entities.find( entity_name.value );
   eosio::check( entity_itr != entities.end(), "Entity not found" );
   // Remove entity from entity table
   entities.erase( entity_itr );

   //Remove all nodes
   node_table nodes(get_self(), get_self().value);
   auto by_entity_index = nodes.get_index<"by.entity"_n>();

   std::vector<name> validators;
   std::vector<name> writers;
   auto itr_node = by_entity_index.lower_bound( entity_name.value );
   while(itr_node != by_entity_index.end() && itr_node->entity == entity_name ) {
      if(itr_node->type == node_type::WRITER) {
         writers.push_back(itr_node->name);
      } else if (itr_node->type == node_type::VALIDATOR) {
         validators.push_back(itr_node->name);
      }
      by_entity_index.erase(itr_node);
      itr_node = by_entity_index.lower_bound( entity_name.value );
   }

   // TODO: Check that no validator is in the current schedule

   if (writers.size() > 0) {
      // deleteauth for every writer from entity account 
      for (auto &writer : writers) {
         deleteauth_action(get_self(), {entity_name, "active"_n}).send( entity_name, writer );
      }

      // delete permission writer in entity, and corresponding table
      unlinkauth_action(get_self(), {entity_name, "active"_n}).send( entity_name, "eosio"_n, "newaccount"_n );
      unlinkauth_action(get_self(), {entity_name, "active"_n}).send( entity_name, "eosio"_n, "setram"_n );
      unlinkauth_action(get_self(), {entity_name, "active"_n}).send( entity_name, "writer"_n, "run"_n );

      writers_table wrt(get_self(), entity_name.value);
      auto writers_itr = wrt.find( uint64_t(1) );
      wrt.erase( writers_itr );
      deleteauth_action(get_self(), {entity_name, "active"_n}).send( entity_name, "writer"_n );

      // remove entity from writer permission and corresponding table
      authority global_writers;
      global_writers.threshold = 1;

      std::vector<permission_level_weight> ent_writers_vector;
      writers_table global_wrt(get_self(), get_self().value );
      auto itr = global_wrt.find( uint64_t(1) );
      global_wrt.modify( itr, eosio::same_payer, [&]( auto& row ) {
         permission_level_weight entity_plw = {{entity_name, "writer"_n},1};
         auto entity_it = std::find(row.writers.cbegin(), row.writers.cend(), entity_plw);
         row.writers.erase( entity_it );
         ent_writers_vector = row.writers;
      });
      global_writers.accounts = ent_writers_vector;
      // If there was only one writer entity, the following would fail for being empty.
      // It is reasonable to keep at least one writer, thus we might add a check for this.
      updateauth_action(get_self(), {"writer"_n, "active"_n}).send( "writer"_n, "access"_n, "active"_n, global_writers);
   }

   // Removes entity's nodes from the groups where they belong
   netgroup_table netgroups(get_self(), get_self().value);
   for (auto &netgroup : netgroups) {
      netgroups.modify( netgroup, eosio::same_payer, [&]( auto& row ){
         auto new_end = remove_if( row.nodes.begin(), row.nodes.end(), 
            [&]( name& node ){
               bool is_writer = ( find(writers.begin(), writers.end(), node) != writers.end() );
               bool is_validator = ( find(validators.begin(), validators.end(), node) != validators.end() );
               return (is_writer || is_validator);
            } );
         row.nodes.resize( new_end - row.nodes.begin() );
      });
   }
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
   auto itr_node = nodes.find( node_name.value );
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

   auto active_authority = authority{
      1, {},
      {{{entity, "active"_n}, 1}},{}
   };

   auto owner_authority = authority{
      1, {},
      {{{entity, "owner"_n}, 1}},{}
   };

   // Create new validator account
   newaccount_action(get_self(), {entity, "active"_n}).send(
      entity, name, owner_authority, active_authority
   );

}


void lacchain::addwriter( const name& name,
                          const struct name& entity,
                          const authority& writer_authority) {

   check(name != "writer"_n, "Writer name can't be 'writer'");

   add_new_node(name, node_type::WRITER, entity, {});

   auto add_to_writer_table = [&](uint64_t scope, const struct name& e, const struct name& n) -> std::vector<permission_level_weight> {

      std::vector<permission_level_weight> accounts;

      auto insert_new = [](std::vector<permission_level_weight>& accts, const permission_level_weight& pl) {

         auto it = std::find(accts.cbegin(), accts.cend(), pl);
         if(it != accts.end()) return;

         accts.insert(
            std::upper_bound( accts.begin(), accts.end(), pl ),
            pl
        );
      };

      writers_table wrt(get_self(), scope);
      auto itr = wrt.find( uint64_t(1) );
      if( itr == wrt.end()) {
         wrt.emplace( get_self(), [&]( auto& row ) {
            row.id      = uint64_t(1);
            insert_new(row.writers, {{e, n},1});
            accounts = row.writers;
         });
      } else {
         auto old_writers = itr->writers;
         wrt.modify( itr, eosio::same_payer, [&]( auto& row ) {
            insert_new(row.writers, {{e, n},1});
            accounts = row.writers;
         });
         if (old_writers == accounts) return {};
      }

      return accounts;
   };

   //Add new writer to the list of global writers
   authority global_writers;
   global_writers.threshold = 1;
   global_writers.accounts = add_to_writer_table(get_self().value, entity, "writer"_n);

   //Add new writer to the list of entity writers
   authority entity_writers;
   entity_writers.threshold = 1;
   entity_writers.accounts = add_to_writer_table(entity.value, entity, name);

   //Add new permission to the entity account for this writer node
   // TODO: check if writer already exists, to handle the error.
   updateauth_action(get_self(), {entity, "active"_n}).send( entity, name, "owner"_n, writer_authority);

   //Update entity "writer" permission
   updateauth_action(get_self(), {entity, "active"_n}).send( entity, "writer"_n, "active"_n, entity_writers);

   if (!global_writers.accounts.empty()) {
      //Update writer "access" permission
      updateauth_action(get_self(), {"writer"_n, "owner"_n}).send( "writer"_n, "access"_n, "active"_n, global_writers);

      //Link general "writer" with eosio::newaccount / writer::fuel / eosio::setram
      linkauth_action(get_self(), {entity, "active"_n}).send( entity, "eosio"_n, "newaccount"_n, "writer"_n);
      linkauth_action(get_self(), {entity, "active"_n}).send( entity, "eosio"_n, "setram"_n, "writer"_n);
      linkauth_action(get_self(), {entity, "active"_n}).send( entity, "writer"_n, "run"_n, "writer"_n);
   }
   //1/N CPU/NET for entity account with at least one writer node
   int64_t ram, cpu, net;
   get_resource_limits( entity, ram, cpu, net);
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

void lacchain::rmnode( const name& node_name ){

   node_table nodes(get_self(), get_self().value);
   auto itr_node = nodes.find( node_name.value );
   check( itr_node != nodes.end(), "Node does not exist" );

   name entity = itr_node->entity;
   require_auth( entity );

   if (itr_node->type == 1){ // validator case
   // TODO: check that it is not in the schedule
   }
   if (itr_node->type == 2){ // writer case
      writers_table wrts(get_self(), entity.value);
      auto writers_itr = wrts.find( uint64_t(1) );
      wrts.modify( writers_itr, eosio::same_payer, [&](auto& row){
         permission_level_weight this_writer = {{entity, node_name},1};
         auto new_end = remove( row.writers.begin(), row.writers.end(), this_writer );
         row.writers.resize( new_end - row.writers.begin() ); // because 'remove' preserves previous size
         authority entity_writers;
         entity_writers.threshold = 1;
         entity_writers.accounts = row.writers;
         if (row.writers.size() > 0) updateauth_action(get_self(), {entity, "active"_n})
           .send( entity, "writer"_n, "active"_n, entity_writers);
      });
      deleteauth_action(get_self(), {entity, "owner"_n}).send( entity, node_name );
   }
   nodes.erase( itr_node );
   // remove node from every group to which it belongs.
   netgroup_table netgroups(get_self(), get_self().value);
   for (auto &netgroup : netgroups) {
      netgroups.modify( netgroup, eosio::same_payer, [&]( auto& row ){
         auto new_end = remove( row.nodes.begin(), row.nodes.end(), node_name );
         row.nodes.resize( new_end - row.nodes.begin() ); // idem
      });
   }
}

void lacchain::removeentwri (const name& entity) {
   writers_table wrts(get_self(), entity.value);
   auto writers_itr = wrts.find( uint64_t(1) );
   check( (writers_itr->writers).empty(), "This entity still has writer nodes");

   unlinkauth_action(get_self(), {entity, "active"_n}).send( entity, "eosio"_n, "newaccount"_n );
   unlinkauth_action(get_self(), {entity, "active"_n}).send( entity, "eosio"_n, "setram"_n );
   unlinkauth_action(get_self(), {entity, "active"_n}).send( entity, "writer"_n, "run"_n );
   deleteauth_action(get_self(), {entity, "active"_n}).send( entity, "writer"_n );

   writers_table global_wrts(get_self(), get_self().value);
   auto g_writers_itr = global_wrts.find( uint64_t(1) );
   global_wrts.modify( g_writers_itr, eosio::same_payer, [&](auto& row){
      permission_level_weight this_entity = {{entity, "writer"_n},1};
      auto new_end = remove( row.writers.begin(), row.writers.end(), this_entity );
      row.writers.resize( new_end - row.writers.begin() );
      authority global_writers;
      global_writers.threshold = 1;
      global_writers.accounts = row.writers;
      updateauth_action(get_self(), {"writer"_n, "owner"_n}).send( "writer"_n, "access"_n, 
         "active"_n, global_writers);
   });
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

   nodes.modify( itr_node, itr_node->entity, [&]( auto& row ) {
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
   eosio::check(itr_entity != entities.end(), "entity does not exist");

   require_auth( itr_entity->name );

   entities.modify( itr_entity, itr_entity->name, [&]( auto& row ) {
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

void lacchain::transfer_ram_from_entity(const name& entity, const name& account, int64_t ram_bytes) {
   int64_t e_ram, e_cpu, e_net;
   get_resource_limits( entity, e_ram, e_cpu, e_net);
   set_resource_limits( entity, e_ram-ram_bytes, e_cpu, e_net);

   int64_t a_ram, a_cpu, a_net;
   get_resource_limits( account, a_ram, a_cpu, a_net);
   set_resource_limits( account, a_ram+ram_bytes, a_cpu, a_net);
}

void lacchain::setram( const name& entity, const name& account, int64_t ram_bytes) {
   check(ram_bytes > 0, "ram_bytes must be positive");

   entity_table entities(get_self(), get_self().value);
   auto itr_entity = entities.find( entity.value );
   eosio::check(entities.find( entity.value ) != entities.end(), "entity does not exists");

   require_auth( entity );

   node_table nodes(get_self(), get_self().value);
   eosio::check( entities.find( account.value ) == entities.end()
              && nodes.find( account.value ) == nodes.end()
              && account != "writer"_n, "only user accounts can be changed");

   transfer_ram_from_entity(entity, account, ram_bytes);
}


} //lacchainsystem