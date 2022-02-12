#pragma once
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>




namespace keyaccounts
{

   inline constexpr eosio::symbol core_symbol{"EOS", 4};
   inline constexpr eosio::extended_symbol extended_core_symbol{core_symbol, eosio::name("eosio.token") };

   std::optional<uint32_t> rambytes_consumption;//holds consumed rambytes if any.

   inline void add_rambytes_consumption(const int& rambytes){
      if(rambytes_consumption){
         rambytes_consumption = rambytes_consumption.value() + rambytes;
      }
      else{
         rambytes_consumption = rambytes;
      }
   }

   inline eosio::checksum256 pubkey_ckecksum(const eosio::public_key &pubkey){
      auto packed_data = eosio::pack(pubkey);
      std::string sdata = std::string(packed_data.begin(), packed_data.end());
      return eosio::sha256(sdata.c_str(), sdata.length() );
       
      // return eosio::sha256((char *)&pubkey, 33);
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
   bool freeze = false;
   bool contract_pays_ram = true;
   eosio::asset fixed_fee{1, core_symbol};
  };
  EOSIO_REFLECT(config, freeze)
  typedef eosio::singleton<"config"_n, config
  > config_table;
//   using cfg_type = std::variant<config, bool >;

   struct balance{
      eosio::extended_asset amount;
      bool in_ram;
   };

   struct container{
      std::vector<ktransfer_data> data;
      uint64_t account_nonce;
      std::vector<eosio::signature> signatures;//sign action data with empty signatures
   };
   EOSIO_REFLECT(container, data, account_nonce, signatures)




   struct keyaccounts_c : public eosio::contract
   {
      using eosio::contract::contract;


      // void process(ktransaction& trx);

      //debug/dev
      void clrpubkey(const uint64_t& pubkey_id);//warning first empty balances
      void clrbalances(const uint64_t& pubkey_id);
      void notify_transfer(eosio::name from, eosio::name to, const eosio::asset& quantity, std::string memo);
      void test(uint32_t bytes);

      
      void setconfig(std::optional<config> cfg);
      void ktransfer( std::vector<ktransfer_data>& data,  uint64_t& account_nonce,  std::optional<std::vector<eosio::signature>>& signatures);
      
      private:
      //internal accounting
      std::vector<balance> get_balances(const uint64_t& pubkey_id, const std::vector<eosio::extended_symbol>& symbols);
      uint64_t reg_public_key(const eosio::public_key& pubkey, pubkeys_table& idx);
      void key_to_key_transfer(eosio::public_key& from_key, eosio::public_key& to_key, eosio::extended_asset& quantity, const std::string& memo);
      void key_to_account_transfer(eosio::public_key& from_key, eosio::name& to_account, eosio::extended_asset& quantity, const std::string& memo);
      void add_balance(const uint64_t& pubkey_id, eosio::extended_asset& value);
      void sub_balance(const uint64_t& pubkey_id, const eosio::extended_asset& value);

      std::vector<eosio::public_key> resolve_required_signing_keys(const eosio::public_key& pubkey);



      template <typename T>
      void validate_signatures(const T& data, const std::vector<eosio::signature>& signatures, const std::vector<eosio::public_key>& signing_keys);

   };

   EOSIO_ACTIONS(keyaccounts_c,  //
                 "keyaccounts"_n,       //
                 action(clrbalances, pubkey_id),
                 action(clrpubkey, pubkey_id),
                 action(ktransfer, data, account_nonce, signatures),
                 action(test, bytes),
                 action(setconfig, cfg),
                 notify(eosio::any_contract, transfer))

}  // namespace keyaccounts