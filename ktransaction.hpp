

namespace keyaccounts
{

   using to_type = std::variant<eosio::name, eosio::public_key>;
   struct ktransfer_data
   {
      eosio::public_key from;
      to_type to;
      eosio::extended_asset quantity;
      std::string memo;
   };
   EOSIO_REFLECT(ktransfer_data, from, to, quantity, memo)



}  // namespace keyaccounts