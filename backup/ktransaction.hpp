namespace keyaccounts
{

   
   using to_type = std::variant<eosio::name, eosio::public_key>;
   struct transfer_data
   {
      eosio::public_key from_key;
      to_type to;
      eosio::extended_asset quantity;
      std::string memo;
   };
   EOSIO_REFLECT(transfer_data, from_key, to, quantity, memo)

   struct ktransaction_header
   {
      eosio::time_point_sec expiration;  // timestamp when user signed the transaction
      uint64_t nonce;                    //key account nonce
   };
   EOSIO_REFLECT(ktransaction_header, expiration, nonce)

   using payload_type = std::variant<transfer_data, uint64_t>;  //currently only one payload type uint64_t is for testing
   struct ktransaction
   {
      ktransaction_header header;
      payload_type payload;
      //might want to add signatures here
   };
   EOSIO_REFLECT(ktransaction, header, payload)

}  // namespace keyaccounts