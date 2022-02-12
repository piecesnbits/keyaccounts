const conf = require('./eosioConfig')
const env = require('./.env.js')
const { api, tapos } = require('./lib/eosjs')(env.keys[env.defaultChain], conf.endpoints[env.defaultChain][0])
const contractAccount = conf.accountName[env.defaultChain]
const contractActions = require('./do.js')

function chainName() {
  if (env.defaultChain == 'jungle') return 'jungle3'
  else return env.defaultChain
}

async function doAction(name, data, account, auth,permission) {
  try {
    if (!data) data = {}
    if (!account) account = contractAccount
    if (!auth) auth = account
    if(!permission) permission = 'active'
    console.log("Do Action:", name, data)
    const authorization = [{ actor: auth, permission }]
    const result = await api.transact({
      // "delay_sec": 0,
      actions: [{ account, name, data, authorization }]
    }, tapos)
    const txid = result.transaction_id
    console.log(`https://${chainName()}.bloks.io/transaction/` + txid)
    // console.log(txid)
    return result
  } catch (error) {
    console.error(error.toString())
    if (error.json) console.error("Logs:", error.json?.error?.details[1]?.message)
  }
}

const methods = {
  async setConfig(){
    const config = require('./lib/sample_config')
    const data = {cfg:config}
    await doAction('setconfig',data)
  },

  async deposit(quantity,from,){
    const data = {
      from,
      to: contractAccount,
      quantity,
      memo:"deposit"
    }
    let contract = 'eosio.token'
    // if (quantity.split(' ')[1] == 'BOID') contract = 'token.boid'
    await doAction('transfer', data, contract, from)
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