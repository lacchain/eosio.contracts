#pragma once

#include <eosio/action.hpp>
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
#include <eosio/fixed_bytes.hpp>
#include <eosio/privileged.hpp>
#include <eosio/producer_schedule.hpp>

/**
 * LACCHAIN EOSIO System Contract
 *
 */

namespace lacchainsystem {

   using eosio::action_wrapper;
   using eosio::check;
   using eosio::checksum256;
   using eosio::ignore;
   using eosio::name;
   using eosio::permission_level;
   using eosio::public_key;

   struct permission_level_weight {
      permission_level  permission;
      uint16_t          weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( permission_level_weight, (permission)(weight) )
   };

   struct key_weight {
      eosio::public_key  key;
      uint16_t           weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( key_weight, (key)(weight) )
   };

   struct wait_weight {
      uint32_t           wait_sec;
      uint16_t           weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( wait_weight, (wait_sec)(weight) )
   };

   struct authority {
      uint32_t                              threshold = 0;
      std::vector<key_weight>               keys;
      std::vector<permission_level_weight>  accounts;
      std::vector<wait_weight>              waits;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( authority, (threshold)(keys)(accounts)(waits) )
   };

   struct block_header {
      uint32_t                                  timestamp;
      name                                      producer;
      uint16_t                                  confirmed = 0;
      checksum256                               previous;
      checksum256                               transaction_mroot;
      checksum256                               action_mroot;
      uint32_t                                  schedule_version = 0;
      std::optional<eosio::producer_schedule>   new_producers;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE(block_header, (timestamp)(producer)(confirmed)(previous)(transaction_mroot)(action_mroot)
                                     (schedule_version)(new_producers))
   };

   class [[eosio::contract("lacchain.system")]] lacchain : public eosio::contract {
      public:
         using contract::contract;
         /**
          * New account action, called after a new account is created. This code enforces rules
          * for new accounts.
          *
          * 1. new account entities can only be created by the permissioning committee.
          *
          * 2. new regular accounts can only be created by writers entities and the writer account
          *    must be present in the active and owner authority of the account and must be
          *    required to satisfy them.
          */
         [[eosio::action]]
         void newaccount( name creator, name name, const authority& owner, const authority& active);
         /**
          * Update authorization action updates pemission for an account.
          *
          * @param account - the account for which the permission is updated,
          * @param pemission - the permission name which is updated,
          * @param parem - the parent of the permission which is updated,
          * @param aut - the json describing the permission authorization.
          */
         [[eosio::action]]
         void updateauth(  name  account,
                           name  permission,
                           name  parent,
                           authority auth ) {}

         /**
          * Delete authorization action deletes the authorization for an account's permission.
          *
          * @param account - the account for which the permission authorization is deleted,
          * @param permission - the permission name been deleted.
          */
         [[eosio::action]]
         void deleteauth( name  account,
                          name  permission ) {}

         /**
          * Link authorization action assigns a specific action from a contract to a permission you have created. Five system
          * actions can not be linked `updateauth`, `deleteauth`, `linkauth`, `unlinkauth`, and `canceldelay`.
          * This is useful because when doing authorization checks, the EOSIO based blockchain starts with the
          * action needed to be authorized (and the contract belonging to), and looks up which permission
          * is needed to pass authorization validation. If a link is set, that permission is used for authoraization
          * validation otherwise then active is the default, with the exception of `eosio.any`.
          * `eosio.any` is an implicit permission which exists on every account; you can link actions to `eosio.any`
          * and that will make it so linked actions are accessible to any permissions defined for the account.
          *
          * @param account - the permission's owner to be linked and the payer of the RAM needed to store this link,
          * @param code - the owner of the action to be linked,
          * @param type - the action to be linked,
          * @param requirement - the permission to be linked.
          */
         [[eosio::action]]
         void linkauth(  name    account,
                         name    code,
                         name    type,
                         name    requirement  ) {}

         /**
          * Unlink authorization action it's doing the reverse of linkauth action, by unlinking the given action.
          *
          * @param account - the owner of the permission to be unlinked and the receiver of the freed RAM,
          * @param code - the owner of the action to be unlinked,
          * @param type - the action to be unlinked.
          */
         [[eosio::action]]
         void unlinkauth( name  account,
                          name  code,
                          name  type ) {}

         /**
          * Cancel delay action cancels a deferred transaction.
          *
          * @param canceling_auth - the permission that authorizes this action,
          * @param trx_id - the deferred transaction id to be cancelled.
          */
         [[eosio::action]]
         void canceldelay( permission_level canceling_auth, checksum256 trx_id ) {}

         /**
          * Set code action sets the contract code for an account.
          *
          * @param account - the account for which to set the contract code.
          * @param vmtype - reserved, set it to zero.
          * @param vmversion - reserved, set it to zero.
          * @param code - the code content to be set, in the form of a blob binary..
          */
         [[eosio::action]]
         void setcode( name account, uint8_t vmtype, uint8_t vmversion, const std::vector<char>& code ) {}

         /** @}*/

         /**
          * Set abi action sets the abi for contract identified by `account` name. Creates an entry in the abi_hash_table
          * index, with `account` name as key, if it is not already present and sets its value with the abi hash.
          * Otherwise it is updating the current abi hash value for the existing `account` key.
          *
          * @param account - the name of the account to set the abi for
          * @param abi     - the abi hash represented as a vector of characters
          */
         [[eosio::action]]
         void setabi( name account, const std::vector<char>& abi );

         /**
          * On error action, notification of this action is delivered to the sender of a deferred transaction
          * when an objective error occurs while executing the deferred transaction.
          * This action is not meant to be called directly.
          *
          * @param sender_id - the id for the deferred transaction chosen by the sender,
          * @param sent_trx - the deferred transaction that failed.
          */
         [[eosio::action]]
         void onerror( uint128_t sender_id, const std::vector<char>& sent_trx );

         /**
          * Set privilege action allows to set privilege status for an account (turn it on/off).
          * @param account - the account to set the privileged status for.
          * @param is_priv - 0 for false, > 0 for true.
          */
         [[eosio::action]]
         void setpriv( name account, uint8_t is_priv );

         /**
          * Sets the resource limits of an account
          *
          * @param account - name of the account whose resource limit to be set
          * @param ram_bytes - ram limit in absolute bytes
          * @param net_weight - fractionally proportionate net limit of available resources based on (weight / total_weight_of_all_accounts)
          * @param cpu_weight - fractionally proportionate cpu limit of available resources based on (weight / total_weight_of_all_accounts)
          */
         [[eosio::action]]
         void setalimits( name account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight );

         /**
          * Set params action, sets the blockchain parameters. By tuning these parameters, various degrees of customization can be achieved.
          *
          * @param params - New blockchain parameters to set
          */
         [[eosio::action]]
         void setparams( const eosio::blockchain_parameters& params );

         /**
          * Require authorization action, checks if the account name `from` passed in as param has authorization to access
          * current action, that is, if it is listed in the action’s allowed permissions vector.
          *
          * @param from - the account name to authorize
          */
         [[eosio::action]]
         void reqauth( name from );

         /**
          * Activate action, activates a protocol feature
          *
          * @param feature_digest - hash of the protocol feature to activate.
          */
         [[eosio::action]]
         void activate( const eosio::checksum256& feature_digest );

         /**
          * Require activated action, asserts that a protocol feature has been activated
          *
          * @param feature_digest - hash of the protocol feature to check for activation.
          */
         [[eosio::action]]
         void reqactivated( const eosio::checksum256& feature_digest );


         /**
          * Add new lacchain entity
          *
          * @param entity_name - entity name.
          * @param entity_type - entity type (PARTNER, NON_PARTNER)
          * @param pubkey - public key to be added as authority for the entity account active permission (optional).
          */
         [[eosio::action]]
         void addentity(const name& entity_name,
                        const entity_type entity_type,
                        const std::optional<eosio::public_key> pubkey) {

         /**
          * Add new validator node
          *
          * @param name - validator node name.
          * @param entity - the parent entity of the node
          * @param validator_authority - the weighted threshold multisig block signing authority of the block producer used to sign blocks.
          */
         [[eosio::action]]
         void addvalidator(
            const name& name,
            const name& entity,
            const eosio::block_signing_authority& validator_authority
         );

         /**
          * Add new writer node
          *
          * @param name - writer node name.
          * @param entity - the parent entity of the node
          * @param writer_authority - new entity authority for this writer node
          */
         [[eosio::action]]
         void addwriter(
            const name& name,
            const name& entity,
            const authority& writer_authority
         );

         /**
          * Add new boot node
          *
          * @param name - boot node name.
          * @param entity - the parent entity of the node
          */
         [[eosio::action]]
         void addboot(
            const name& name,
            const name& entity
         );

         /**
          * Add new observer node
          *
          * @param name - observer node name.
          * @param entity - the parent entity of the node
          */
         [[eosio::action]]
         void addobserver(
            const name& observer,
            const authority& owner
         );

         /**
          * Add a new network link between two nodes
          *
          * @param nodeA - node A.
          * @param nodeB - node B.
          * @param direction - link direction
          */
         [[eosio::action]]
         void addnetlink(
            const name& nodeA,
            const name& nodeB,
            int direction
         );

         /**
          * Remove a new network link between two nodes
          *
          * @param nodeA - node A.
          * @param nodeB - node B.
          */
         [[eosio::action]]
         void rmnetlink(
            const name& nodeA,
            const name& nodeB
         );

         /**
          * Set node information
          *
          * @param node - node name
          * @param info - node information
          */
         [[eosio::action]]
         void setnodeinfo(
            const name& node,
            const std::string& info
         );

         /**
          * Set entity information
          *
          * @param entity - entity name
          * @param info - entity information
          */
         [[eosio::action]]
         void setentinfo(
            const name& entity,
            const std::string& info
         );

         /**
          * Set schedule action, sets a new list of active validators, by proposing a schedule change, once the block that
          * contains the proposal becomes irreversible, the schedule is promoted to "pending"
          * automatically. Once the block that promotes the schedule is irreversible, the schedule will
          * become "active".
          *
          * @param schedule - New list of active validators to set
          */
         [[eosio::action]]
         void setschedule( const std::vector<name>& validators );

         enum entity_type {
            PARTNER     = 1,
            NON_PARTNER = 2,
         };

         struct [[eosio::table]] entity {
            name        name;
            entity_type type;
            std::string info;
            //TODO: agregar info solo para ser modificada x la entidad

            uint64_t primary_key()const { return name.value; }
            EOSLIB_SERIALIZE( entity, (name)(type)(info))
         };

         typedef eosio::multi_index< "entity"_n, entity > entity_table;

         enum node_type {
            VALIDATOR = 1,
            WRITER    = 2,
            BOOT      = 3,
            OBSERVER  = 4
         };

         using bsa = eosio::block_signing_authority;
         
         struct [[eosio::table]] node {
            name                name;
            name                entity;
            int                 type;
            std::optional<bsa>  bsa;
            bool                enabled;
            std::string         info;
            uint64_t            reserved = 0;

            uint64_t primary_key()const { return id; }

            EOSLIB_SERIALIZE( node, (name)(entity)(type)(bsa)(enabled)(info)(reserved) )
         };

         typedef eosio::multi_index< "node"_n, node > node_table;

         enum link_direction {
            AB   = 1,
            BA   = 2,
            BOTH = 3,
         };

         struct [[eosio::table]] netlink {
            uint64_t            id;
            name                nodeA;
            name                nodeB;
            int                 direction;

            uint64_t primary_key()const { return id; }
            uint128_t secondary_key()const {  return make_key(nodeA.value, nodeB.value); }

            static uint128_t make_key(uint64_t a, uint64_t b) {
               if( b > a ) std::swap(a,b);
               return uint128_t(a) << 64 | uint128_t(b);
            }

            EOSLIB_SERIALIZE( netlink, (id)(nodeA)(nodeB)(direction) )
         };

         typedef eosio::multi_index< "netlink"_n, netlink,
            eosio::indexed_by<"pair"_n, eosio::const_mem_fun<netlink, uint128_t, &netlink::secondary_key>>
         > netlink_table;

         struct [[eosio::table]] abi_hash {
            name              owner;
            checksum256       hash;
            uint64_t primary_key()const { return owner.value; }

            EOSLIB_SERIALIZE( abi_hash, (owner)(hash) )
         };

         typedef eosio::multi_index< "abihash"_n, abi_hash > abi_hash_table;

         using newaccount_action = action_wrapper<"newaccount"_n, &lacchain::newaccount>;
         using updateauth_action = action_wrapper<"updateauth"_n, &lacchain::updateauth>;
         using deleteauth_action = action_wrapper<"deleteauth"_n, &lacchain::deleteauth>;
         using linkauth_action = action_wrapper<"linkauth"_n, &lacchain::linkauth>;
         using unlinkauth_action = action_wrapper<"unlinkauth"_n, &lacchain::unlinkauth>;
         using canceldelay_action = action_wrapper<"canceldelay"_n, &lacchain::canceldelay>;
         using setcode_action = action_wrapper<"setcode"_n, &lacchain::setcode>;
         using setabi_action = action_wrapper<"setabi"_n, &lacchain::setabi>;
         using setpriv_action = action_wrapper<"setpriv"_n, &lacchain::setpriv>;
         using setalimits_action = action_wrapper<"setalimits"_n, &lacchain::setalimits>;
         using setparams_action = action_wrapper<"setparams"_n, &lacchain::setparams>;
         using reqauth_action = action_wrapper<"reqauth"_n, &lacchain::reqauth>;
         using activate_action = action_wrapper<"activate"_n, &lacchain::activate>;
         using reqactivated_action = action_wrapper<"reqactivated"_n, &lacchain::reqactivated>;

         [[eosio::action]]
         void dummy() {
            eosio::print("I'm a dummy action!");
         }

      private:

         void add_new_node(const name& node_name,
                           const node_type node_type,
                           const name& owner_entity,
                           const authority& owner, const authority& active,
                           const std::optional<eosio::block_signing_authority> bsa);

         bool validate_newuser_authority(const authority& auth);
   };
}
