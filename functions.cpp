namespace keyaccounts
{

   void keyaccounts_c::key_to_key_transfer(
      eosio::public_key& from_key,
      eosio::public_key& to_key,
      eosio::extended_asset& quantity,
      const std::string& memo)
   {
      // eosio::require_auth();
      eosio::check(quantity.quantity.amount > 0, "Transfer amount must be greater then zero");
      eosio::check(from_key != to_key, "Can't transfer to the same key");

      pubkeys_table _pubkeys(get_self(), get_self().value);
      auto keyidx = _pubkeys.get_index<eosio::name("bykey")>();
      auto from_itr = keyidx.find(pubkey_ckecksum(from_key));
      auto to_itr = keyidx.find(pubkey_ckecksum(to_key));

      uint64_t from_id;  //balances scope
      uint64_t to_id;    //

      if (from_itr == keyidx.end())
      {
         eosio::check(false, eosio::public_key_to_string(from_key) + " doesn't exist in table");
      }
      else
      {
         //from key exist
         from_id = from_itr->id;
         // uint64_t expected_nonce = from_itr->nonce + 1;
         // eosio::check(nonce == expected_nonce, 
         // "Transaction out of order. Expected nonce " + std::to_string(expected_nonce) + ". Got nonce " + std::to_string(nonce));
      }

      if (to_itr == keyidx.end())
      {
         //receiving key not yet in table
         to_id = reg_public_key(to_key, _pubkeys);
      }
      else
      {
         to_id = to_itr->id;
      }

      add_balance(to_id, quantity);//
      sub_balance(from_id, quantity);//sub_balance can never consume RAM.

      // keyidx.modify(from_itr, eosio::same_payer, [&](auto& n) { 
      //    n.nonce += 1; 
      // });
   }

   void keyaccounts_c::key_to_account_transfer(
      eosio::public_key& from_key,
      eosio::name& to_account,
      eosio::extended_asset& quantity,
      const std::string& memo)
   {
      // eosio::require_auth();
      eosio::check(quantity.quantity.amount > 0, "Transfer amount must be greater then zero");
      eosio::check(eosio::is_account(to_account), "Receiving account doesn't exists");

      pubkeys_table _pubkeys(get_self(), get_self().value);
      auto keyidx = _pubkeys.get_index<eosio::name("bykey")>();
      auto from_itr = keyidx.find(pubkey_ckecksum(from_key));

      uint64_t from_id;  //balances scope

      if (from_itr == keyidx.end())
      {
         eosio::check(false, eosio::public_key_to_string(from_key) + " doesn't exist in table");
      }
      else
      {
         //from key exist
         from_id = from_itr->id;
         // uint64_t expected_nonce = from_itr->nonce + 1;
         // eosio::check(nonce == expected_nonce, "Transaction out of order. Expected nonce " +
         //                                           std::to_string(expected_nonce) + ". Got nonce " +
         //                                           std::to_string(nonce));
      }

      sub_balance(from_id, quantity);
      // keyidx.modify(from_itr, eosio::same_payer, [&](auto& n) { n.nonce += 1; });

      //inline action to to_account
      token::actions::transfer{quantity.contract, get_self()}.send(get_self(), to_account,
                                                                   quantity.quantity, memo);
   }


   std::vector<balance> keyaccounts_c::get_balances(
      const uint64_t& pubkey_id,
      const std::vector<eosio::extended_symbol>& symbols)
   {

      balances_table _balances(get_self(), pubkey_id);
      auto cs_idx = _balances.get_index<"bycontrsym"_n>();

      std::vector<balance> res;

      for (auto sym : symbols){
         uint128_t composite_id = (uint128_t{sym.get_contract().value} << 64) | sym.get_symbol().raw();
         const auto& itr = cs_idx.find(composite_id);
         if(itr != cs_idx.end() ){
            res.push_back(balance{.amount = itr->balance, .in_ram = true} );
         }
         else{

            res.push_back(balance{.amount = eosio::extended_asset{0, sym}, .in_ram = false} );
         }
      }
      return res;
   }

   void keyaccounts_c::sub_balance(
      const uint64_t& pubkey_id,
      const eosio::extended_asset& value)
   {
      balances_table _balances(get_self(), pubkey_id);
      auto by_contr_sym = _balances.get_index<"bycontrsym"_n>();
      uint128_t composite_id = (uint128_t{value.contract.value} << 64) | value.quantity.symbol.raw();
      const auto& itr = by_contr_sym.get(composite_id, "No balance with this symbol and contract." );
      eosio::check(itr.balance >= value, "Overdrawn balance.");

      _balances.modify(itr, eosio::same_payer, [&](auto& n) { n.balance -= value; });
   }

   void keyaccounts_c::add_balance(
      const uint64_t& pubkey_id,
      eosio::extended_asset& value)
   {
      balances_table _balances(get_self(), pubkey_id);
      auto by_contr_sym = _balances.get_index<"bycontrsym"_n>();
      uint128_t composite_id =(uint128_t{value.contract.value} << 64) | value.quantity.symbol.raw();
      auto itr = by_contr_sym.find(composite_id);

      if (itr == by_contr_sym.end()){
         //new token
         //RAM consumption
         _balances.emplace(get_self(),[&](auto& n){
            n.id = _balances.available_primary_key();
            n.balance = value;
         });
         add_rambytes_consumption(20);
      }
      else{
         by_contr_sym.modify(itr, eosio::same_payer, [&](auto& n) { n.balance += value; });
      }
   }

   uint64_t keyaccounts_c::reg_public_key(
      const eosio::public_key& pubkey,
      pubkeys_table& idx)
   {
      uint64_t new_id = idx.available_primary_key();
      //RAM consumption
      idx.emplace(get_self(),[&](auto& n){
         n.id = new_id;
         n.pubkey = pubkey;
      });
      add_rambytes_consumption(20);
      return new_id;
   }


   std::vector<eosio::public_key> keyaccounts_c::resolve_required_signing_keys(const eosio::public_key& pubkey){
      //todo... problem... circular msig keys and how many levels allowed?
      //check if pubkey has msig setup and get the associated public_keys
      std::set<eosio::public_key> temp;
      temp.insert(pubkey);
      std::vector<eosio::public_key> sorted_vector;
      sorted_vector.assign(temp.begin(), temp.end());
      //fetch keys todo
      return sorted_vector;
   }

   template <typename T>
   void keyaccounts_c::validate_signatures(const T& data, const std::vector<eosio::signature>& signatures, const std::vector<eosio::public_key>& signing_keys) {

         int signing_keys_size = signing_keys.size();//signing_keys must be populated by the contract so there can't be duplicates
         int signatures_size = signatures.size();//warning! user input. there can be duplicates! these will get detected.

         eosio::check(signing_keys_size == signatures_size, "Number of signatures must be equal to required signing keys." );

         eosio::checksum256 digest;
         auto packed_data =eosio::pack(data);
         std::string sdata = std::string(packed_data.begin(), packed_data.end());
         digest = eosio::sha256(sdata.c_str(), sdata.length() );

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

}  // namespace keyaccounts