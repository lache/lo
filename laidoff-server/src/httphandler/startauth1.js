const db = require('../db')
const url = require('url')
const srp = require('../../build/Release/module')

module.exports = app => {
  app.get('/startAuth1', (req, res) => {
    const accountId = req.get('X-Account-Id')
    const a = req.get('X-Account-A')
    let resultMsg, errMsg
    const account = db.findAccount(accountId)
    if (account) {
      console.log('X-Account-Id')
      console.log(accountId)
      console.log('Account-S')
      console.log(account.s)
      console.log('Account-V')
      console.log(account.v)
      console.log('X-Account-A')
      console.log(a)

      const sBuf = Buffer.from(account.s, 'hex')
      const vBuf = Buffer.from(account.v, 'hex')
      const aBuf = Buffer.from(a, 'hex')

      const ver = srp.VerifierNew(accountId, sBuf, vBuf, aBuf)
      console.log('verifier')
      console.log(ver.verifier)
      console.log('B')
      console.log(ver.B.length)
      console.log(ver.B.toString('hex'))
      if (ver.B.length > 0) {
        req.app.get('verifierMap')[accountId] = ver.verifier
        res.setHeader('Content-Type', 'application/json')
        res.send(JSON.stringify({ B: ver.B.toString('hex') }))
      } else {
        errMsg = '데이터 오류'
        res.redirect(
          url.format({
            pathname: '/idle',
            query: {
              resultMsg: resultMsg,
              errMsg: errMsg
            }
          })
        )
      }
    } else {
      errMsg = '계정 조회 실패!'
      res.redirect(
        url.format({
          pathname: '/idle',
          query: {
            resultMsg: resultMsg,
            errMsg: errMsg
          }
        })
      )
    }
  })
}
