const db = require('../db')
const url = require('url')
const srp = require('../../build/Debug/module')

module.exports = app => {
  app.get('/startAuth2', (req, res) => {
    const accountId = req.get('X-Account-Id')
    const m = req.get('X-Account-M')
    let resultMsg, errMsg
    const account = db.findAccount(accountId)
    if (account) {
      console.log('X-Account-Id')
      console.log(accountId)
      console.log('X-Account-M')
      console.log(m)

      const ver = req.app.get('verifierMap')[accountId]
      if (ver) {
        const mBuf = Buffer.from(m, 'hex')
        const hamkBuf = srp.VerifierVerifySession(ver, mBuf)
        if (hamkBuf.length > 0) {
          res.setHeader('Content-Type', 'application/json')
          res.send(JSON.stringify({ HAMK: hamkBuf.toString('hex') }))
        } else {
          console.error('User authentication failed!')
          errMsg = '인증 실패'
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
        console.error('No pending verifier exist')
        errMsg = '잘못된 접근!'
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