const db = require('../db')
const url = require('url')

module.exports = app => {
  app.get('/startAuth1', (req, res) => {
    const accountId = req.get('X-Account-Id')
    const a = req.get('X-Account-A')
    console.log('X-Account-Id')
    console.log(accountId)
    console.log('X-Account-A')
    console.log(a)
    let resultMsg, errMsg
    const account = db.findAccount(accountId)
    if (account) {
      console.log('Account-S')
      console.log(account.s)
      console.log('Account-V')
      console.log(account.v)
      resultMsg = '계정 조회 성공'
    } else {
      errMsg = '계정 조회 실패!'
    }
    res.redirect(
      url.format({
        pathname: '/idle',
        query: {
          resultMsg: resultMsg,
          errMsg: errMsg
        }
      })
    )
  })
}
