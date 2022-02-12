const conf = require('./eosioConfig')
const env = require('./.env.js')
const { api, tapos } = require('./lib/eosjs')(env.keys[env.defaultChain], conf.endpoints[env.defaultChain][0])
const contractAccount = conf.accountName[env.defaultChain]
var watchAccountSample = require('./lib/sample_watchaccount')
function chainName() {
  if (env.defaultChain == 'jungle') return 'jungle3'
  else return env.defaultChain
}

async function doAction(name, data, account, auth) {
  try {
    if (!data) data = {}
    if (!account) account = contractAccount
    if (!auth) auth = account
    console.log("Do Action:", name, data)
    const authorization = [{ actor: auth, permission: 'active' }]
    const result = await api.transact({
      // "delay_sec": 0,
      actions: [{ account, name, data, authorization }]
    }, tapos)
    const txid = result.transaction_id
    console.log(result)
    // console.log(`https://${chainName()}.bloks.io/transaction/` + txid)
    console.log(`https://jungle.eosq.eosnation.io/tx/${txid}`);
    // console.log(txid)
    return result
  } catch (error) {
    console.error(error.toString())
    if (error.json) console.error("Logs:", error.json?.error?.details[1]?.message)
  }
}

const methods = {

  //leaderboard
  async simdonation(donator, donation) {
    donator="test";
    donation="1.0000 EOS"
    await doAction('simdonation', { donator, donation }, contractAccount)
  },
  async clrleaderb(scope) {
    scope=contractAccount;
    await doAction('clrleaderb', { scope }, contractAccount)
  },
  async setconfig(cfg) {
    cfg ={
      round_length_sec: 14*24*60*60,
      minimum_donation:"1.0000 EOS",
      enabled:1
    }
    await doAction('setconfig', {cfg}, contractAccount)
  },
  async clrconfig() {
    await doAction('clrconfig', {}, contractAccount)
  },
}


if (require.main == module) {
  if (Object.keys(methods).find(el => el === process.argv[2])) {
    console.log("Starting:", process.argv[2])
    methods[process.argv[2]](...process.argv.slice(3)).catch((error) => console.error(error))
      .then((result) => console.log('Finished'))
  } else {
    console.log("Available Commands:")
    console.log(JSON.stringify(Object.keys(methods), null, 2))
  }
}
module.exports = methods
