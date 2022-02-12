const { setAbiAction, setCodeAction } = require('./lib/encodeContractData')
const conf = require('./eosioConfig')
const fs = require('fs-extra')
const env = require('./.env.js')


const methods = {

  async kylin(type) {
    const { api, tapos } = require('./lib/eosjs')(env.keys.kylin, conf.endpoints.kylin[0])

    const authorization = [{ actor: conf.accountName.kylin, permission: 'active' }]
    if (!type) type = 'debug'

    console.log("Pushing ABI");
    const result = await api.transact({ actions: [setAbiAction(`../build/${type}/${conf.contractName}.abi`, authorization)] }, tapos).catch(err => console.log(err.toString()))
    if (result) console.log('https://kylin.bloks.io/transaction/' + result.transaction_id)

    console.log("Pushing WASM");
    const result2 = await api.transact({ actions: [setCodeAction(`../build/${type}/${conf.contractName}.wasm`, authorization)] }, tapos).catch(err => console.log(err.toString()))
    if (result2) console.log('https://kylin.bloks.io/transaction/' + result2.transaction_id)

  },
  async jungle(type) {
    console.log(env.keys.jungle)
    const { api, tapos } = require('./lib/eosjs')(env.keys.jungle, conf.endpoints.jungle[0])

    const authorization = [{ actor: conf.accountName.jungle, permission: 'active' }]
    if (!type) type = 'debug'

    console.log("Pushing ABI");
    const result = await api.transact({ actions: [setAbiAction(`../build/${conf.contractName}.abi`, authorization)] }, tapos).catch(err => console.log(err.toString()))
    if (result) console.log('https://jungle3.bloks.io/transaction/' + result.transaction_id)

    console.log("Pushing WASM");
    const result2 = await api.transact({ actions: [setCodeAction(`../build/${conf.contractName}.wasm`, authorization)] }, tapos).catch(err => console.log(err.toString()))
    if (result2) console.log('https://jungle3.bloks.io/transaction/' + result2.transaction_id)

  },
  async eos(type) {
    const { api, tapos } = require('./lib/eosjs')(env.keys.eos, conf.endpoints.eos[0])

    const authorization = [{ actor: conf.accountName.eos, permission: 'active' }]
    if (!type) type = 'debug'

    console.log("Pushing ABI");
    const result = await api.transact({ actions: [setAbiAction(`../build/${type}/${conf.contractName}.abi`, authorization)] }, tapos).catch(err => console.log(err.toString()))
    if (result) console.log('https://bloks.io/transaction/' + result.transaction_id)

    console.log("Pushing WASM");
    const result2 = await api.transact({ actions: [setCodeAction(`../build/${type}/${conf.contractName}.wasm`, authorization)] }, tapos).catch(err => console.log(err.toString()))
    if (result2) console.log('https://bloks.io/transaction/' + result2.transaction_id)

  }
}

if (require.main == module) {
  if (Object.keys(methods).find(el => el === process.argv[2])) {
    console.log("Starting:", process.argv[2])
    methods[process.argv[2]](...process.argv.slice(3)).catch((error) => console.error(error.toString()))
      .then((result) => console.log('Finished'))
  } else {
    console.log("Available Commands:")
    console.log(JSON.stringify(Object.keys(methods), null, 2))
  }
}