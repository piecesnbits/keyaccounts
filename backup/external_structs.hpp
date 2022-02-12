#pragma once
#include <eosio/asset.hpp>

namespace keyaccounts
{  //does this needs to be in this namespace?

   struct exchange_state
   {
      eosio::asset supply;
      struct connector
      {
         eosio::asset balance;
         double weight = .5;

         EOSLIB_SERIALIZE(connector, (balance)(weight))
      };
      connector base;
      connector quote;
      uint64_t primary_key() const { return supply.symbol.raw(); }
      EOSLIB_SERIALIZE(exchange_state, (supply)(base)(quote))
   };
   typedef eosio::multi_index<"rammarket"_n, exchange_state> rammarket;

   eosio::asset ram_bytes_to_gas(uint32_t bytes)
   {
      rammarket market(eosio::name("eosio"), eosio::name("eosio").value);
      auto m = market.get(eosio::symbol("RAMCORE", 4).raw(), "RAMCORE not found");
      int core_precision_pow = pow(10, core_symbol.precision());
      double quote = (m.quote.balance.amount / core_precision_pow) * m.quote.weight; //will weight ever be updated? else multiplication isn't needed.
      double base = m.base.balance.amount * m.base.weight;
      double p = quote / base;  //price in EOS per byte
      eosio::asset gas{static_cast<int64_t>(bytes * p * core_precision_pow), core_symbol};
      return gas;
   }
}  // namespace keyaccounts
