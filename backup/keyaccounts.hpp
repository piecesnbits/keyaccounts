#pragma once
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>



namespace keyaccounts
{

   inline constexpr eosio::symbol core_symbol{"EOS", 4};
   inline constexpr eosio::extended_symbol extended_core_symbol{core_symbol, eosio::name("eosio.token") };

   std::optional<uint32_t> rambytes_consumption;

   inline void add_rambytes_consumption(const int& rambytes){
      if(rambytes_consumption){
         rambytes_consumption = rambytes_consumption.value() + rambytes;
      }
      else{
         rambytes_consumption = rambytes;
      }
   }

   inline eosio::checksum256 pubkey_ckecksum(const eosio::public_key &pubkey){
      return eosio::sha256((char *)&pubkey, 33);
   }

   struct balances
   {
      uint64_t id;
      eosio::extended_asset balance;
      uint64_t primary_key() const { return id; }
      uint128_t by_contr_sym() const{return (uint128_t{balance.contract.value} << 64) | balance.quantity.symbol.raw();}
   };
   EOSIO_REFLECT(balances, id, balance)
   // Table definition
    typedef eosio::multi_index<"balances"_n, balances,
      eosio::indexed_by<"bycontrsym"_n, eosio::const_mem_fun<balances, uint128_t, &balances::by_contr_sym>>
    > balances_table;

   struct pubkeys{
      uint64_t id;
      eosio::public_key pubkey;
      uint64_t nonce = 0;

      uint64_t primary_key() const { return id; }
      eosio::checksum256 by_key() const { return pubkey_ckecksum(pubkey); }
   };
   EOSIO_REFLECT(pubkeys, id, pubkey, nonce)
   // EOSLIB_SERIALIZE(pubkeys, (id)(pubkey));
   typedef eosio::multi_index<"pubkeys"_n, pubkeys,
      eosio::indexed_by<"bykey"_n, eosio::const_mem_fun<pubkeys, eosio::checksum256, &pubkeys::by_key>>
   >pubkeys_table;

  
  struct config {
    double fee_pct;

  };
  EOSIO_REFLECT(config, fee_pct)
  typedef eosio::singleton<"config"_n, config
  > config_table;
//   using cfg_type = std::variant<config, bool >;




   struct keyaccounts_c : public eosio::contract
   {
      using eosio::contract::contract;


      void process(ktransaction& trx, std::vector<eosio::signature>& signatures);

      //debug/dev
      void clrpubkey(const uint64_t& pubkey_id);//warning first empty balances
      void clrbalances(const uint64_t& pubkey_id);
      void notify_transfer(eosio::name from, eosio::name to, const eosio::asset& quantity, std::string memo);
      void test(uint32_t bytes);

      
      void setconfig(std::optional<config> cfg);
      
      private:
      //internal accounting
      uint64_t reg_public_key(const eosio::public_key& pubkey, pubkeys_table& idx);
      void key_to_key_transfer(eosio::public_key& from_key, eosio::public_key& to_key, eosio::extended_asset& quantity, const std::string& memo, uint64_t& nonce);
      void key_to_account_transfer(eosio::public_key& from_key, eosio::name& to_account, eosio::extended_asset& quantity, const std::string& memo, uint64_t& nonce);
      void add_balance(const uint64_t& pubkey_id, eosio::extended_asset& value);
      void sub_balance(const uint64_t& pubkey_id, const eosio::extended_asset& value);


      template <typename T>
      void validate_signatures(const T& data, const std::vector<eosio::signature>& signatures, const std::vector<eosio::public_key>& signing_keys) {
         eosio::checksum256 digest;
         auto packed_data =eosio::pack(data);
         std::string sdata = std::string(packed_data.begin(), packed_data.end());
         digest = eosio::sha256(sdata.c_str(), sdata.length() );

         int signing_keys_size = signing_keys.size();//signing_keys must be populated by the contract so there can't be duplicates
         int signatures_size = signatures.size();//warning! user input. there can be duplicates!

         std::set<eosio::public_key> count_required_signatures; 

         for(eosio::signature sig : signatures){
            eosio::public_key res = eosio::recover_key(digest, sig);
            if (std::find(signing_keys.begin(), signing_keys.end(), res) != signing_keys.end()) {
               //signature matches one of the signing keys. in case of transfer this must be from_key
               eosio::check(count_required_signatures.insert(res).second, "Duplicate signature for "+ public_key_to_string(res) );
            }
            else {
               std::string msg = "Invalid signatures. Need signature(s) from: ";
               for(eosio::public_key pubkey : signing_keys){
                  msg += public_key_to_string(pubkey)+" ";
               }
               eosio::check(false, msg );
            }
         }
         eosio::check(
            count_required_signatures.size() == signing_keys_size, 
            "Transaction requires "+std::to_string(signing_keys_size)+" signatures, provided "+std::to_string(count_required_signatures.size()) );
      }

   };

   EOSIO_ACTIONS(keyaccounts_c,  //
                 "keyaccounts"_n,       //
                 action(clrbalances, pubkey_id),
                 action(clrpubkey, pubkey_id),
                 action(test, bytes),
                 action(setconfig, cfg),
                 action(process, trx, signatures),
                 notify(eosio::any_contract, transfer))

}  // namespace keyaccounts