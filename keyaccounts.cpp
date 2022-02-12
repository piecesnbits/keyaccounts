#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
//https://github.com/EOSIO/eosio.cdt/blob/master/libraries/eosiolib/core/eosio/crypto.hpp
#include <eosio/crypto.hpp>
#include <token/token.hpp>
#include <eosio/bytes.hpp>
#include "common.hpp"
#include "ktransaction.hpp"
#include "keyaccounts.hpp"
#include "functions.cpp"
#include "external_structs.hpp"

namespace keyaccounts
{

      void keyaccounts_c::setconfig(std::optional<config> cfg ){
         require_auth(get_self());
         config_table _config(get_self(), get_self().value);
         cfg ? _config.set(cfg.value(), get_self()) : _config.remove();
      }

      
      void keyaccounts_c::ktransfer( std::vector<ktransfer_data>& data,  uint64_t& account_nonce,  std::optional<std::vector<eosio::signature>>& signatures ){
         eosio::check(data.size(), "No transfer_data received.");

         container t;
         t.data = data;
         t.account_nonce = account_nonce;
         t.signatures = {};

         //this is the account from which the nonce should be updated
         eosio::public_key key_account = data[0].from;

         //in case of msiged key there can be many
         std::vector<eosio::public_key> required = resolve_required_signing_keys(key_account);//sorted

         //asserts when invalid signatures are received
         validate_signatures(t, signatures.value(), required);

         for( ktransfer_data d : data){
            eosio::check(d.from == key_account, "Transfer from not in scope with current keyacount");

            if (const auto key_ptr (std::get_if<eosio::public_key>(&d.to)); key_ptr){
               key_to_key_transfer(
                  d.from,
                  *key_ptr,
                  d.quantity,
                  d.memo);
            }  
            else if (const auto name_ptr (std::get_if<eosio::name>(&d.to)); name_ptr) {
               key_to_account_transfer(
                  d.from,
                  *name_ptr,
                  d.quantity,
                  d.memo);
            }


         }

      }

      void keyaccounts_c::test(uint32_t bytes){
         eosio::asset t = ram_bytes_to_gas(bytes);
         add_rambytes_consumption(20);
         add_rambytes_consumption(20);
         eosio::check(false, std::to_string(rambytes_consumption.value()));
      }

      void keyaccounts_c::notify_transfer(eosio::name from, eosio::name to, const eosio::asset& quantity, std::string memo){

         //dispatcher accepts tokens from all contracts!
         if (from == get_self() || to != get_self() ) return;
         if( from == eosio::name("eosio") || from == eosio::name("eosio.rex") || from == eosio::name("eosio.stake")) return;

         eosio::check(memo.size(), "Memo must contain a valid public key");
         eosio::check(quantity.amount > 0, "Transfer amount must be positive");//this check is not realy needed

         eosio::public_key p = eosio::public_key_from_string(memo);
         pubkeys_table _pubkeys(get_self(), get_self().value);

         eosio::extended_asset extended_quantity(quantity, get_first_receiver());

         auto keyidx = _pubkeys.get_index<eosio::name("bykey")>();
         auto key_itr = keyidx.find(pubkey_ckecksum(p));
         if(key_itr == keyidx.end() ){
            //new receiving pubkey
            //RAM consumption handled in add_balance() and reg_public_key!!
            eosio::print("new key");
            uint64_t new_id = reg_public_key(p, _pubkeys);
            add_balance( new_id, extended_quantity);
         }
         else{
            //key already in table
            //could be RAM consumption!! 
            add_balance( key_itr->id, extended_quantity);
         }
         
      }   

/*

      void keyaccounts_c::process( ktransaction& trx){
         eosio::time_point_sec now = eosio::time_point_sec(eosio::current_time_point());
         eosio::check(now < trx.header.expiration, "Transaction expired");

         std::set<eosio::public_key> temp_keys;
         std::optional<eosio::public_key> key_account;//a trx object  is scoped to a single keyaccount. This due to the nonce 
         for(const kaction& ka : trx.kactions){
            if(ka.account == get_self() ){
               if(ka.name== eosio::name("ktransfer")){
                  auto decoded = eosio::unpack<ktransfer_data>(ka.kdata.data);
                  //in case of msiged key there can be many required_signing_keys
                  std::vector<eosio::public_key> required = resolve_required_signing_keys(decoded.from);//sorted
                  eosio::check(required.size() == ka.kauthorization.size(), "Number of provided keys doesn't match with number of required keys to fullfil authorization.");
                  eosio::check(required == ka.kauthorization, "Wrong or unordered keys supplied in kauthorization");
                  temp_keys.insert(required.begin(), required.end() );  
               }
               else{
                  eosio::check(false, "unknown kaction");
               }
            }
            else{
               eosio::check(false, "not yet allowed");
            }
         }

         std::vector<eosio::public_key> required_signing_keys;
         required_signing_keys.assign(temp_keys.begin(), temp_keys.end());

         std::vector<eosio::signature> signatures = trx.signatures;
         trx.signatures.clear();//we don't need these in trx object
         validate_signatures<ktransaction>(trx, signatures, required_signing_keys);


         //    if (const auto key_ptr (std::get_if<eosio::public_key>(&td.to)); key_ptr){
         //       key_to_key_transfer(
         //          td.from_key,
         //          *key_ptr,
         //          td.quantity,
         //          td.memo,
         //          trx.header.nonce);
         //    }  
         //    else if (const auto name_ptr (std::get_if<eosio::name>(&td.to)); name_ptr) {
         //       key_to_account_transfer(
         //          td.from_key,
         //          *name_ptr,
         //          td.quantity,
         //          td.memo,
         //          trx.header.nonce);
         //    }
         //    if(rambytes_consumption){
         //       //pay gas in core token here publickey must have enough EOS or assert will happen
         //       eosio::print("Used RAM dummy val "+std::to_string(rambytes_consumption.value()));
         //    }
         


      }

*/
      //////////////////////////////////////
      void keyaccounts_c::clrpubkey(const uint64_t& pubkey_id){
         balances_table _balances( get_self(), pubkey_id);
         eosio::check(_balances.begin() == _balances.end(), "Clear balances for scope "+ std::to_string(pubkey_id)+" first");

         pubkeys_table _pubkeys(get_self(), get_self().value);
         auto itr = _pubkeys.require_find(pubkey_id, "pubkey_id does not exists");
         _pubkeys.erase(itr);
      }

      void keyaccounts_c::clrbalances(const uint64_t& pubkey_id){
         // require_auth(get_self());
         balances_table _balances( get_self(), pubkey_id);
         auto itr = _balances.begin();
         eosio::check(_balances.begin() != _balances.end(), "Balances for scope "+std::to_string(pubkey_id)+" empty");
         while(itr != _balances.end() ) {
               itr = _balances.erase(itr);
         }
      }

}  // namespace keyaccounts

EOSIO_ACTION_DISPATCHER(keyaccounts::actions)

EOSIO_ABIGEN(
    // Include the contract actions in the ABI
    variant("to_type", keyaccounts::to_type),
   //  variant("kdata_type", keyaccounts::kdata_type),
    
    actions(keyaccounts::actions),
   //  table("config"_n, keyaccounts::config),
    table("balances"_n, keyaccounts::balances),
    table("pubkeys"_n, keyaccounts::pubkeys))