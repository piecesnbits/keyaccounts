const { JsonRpc, Api } = require('eosjs')
const { JsSignatureProvider } = require('eosjs/dist/eosjs-jssig')
const { TextDecoder, TextEncoder } = require('util')
const tapos = {
  blocksBehind: 20,
  expireSeconds: 30
}
function init(keys, apiurl) {
  if (!keys) keys = []
  const signatureProvider = new JsSignatureProvider(keys)
  const fetch = require('node-fetch')
  if (!apiurl) apiurl = 'http://localhost:3051'
  var rpc = new JsonRpc(apiurl, { fetch })
  const api = new Api({ rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder() })

  return { api, rpc ,tapos}
}

module.exports = init