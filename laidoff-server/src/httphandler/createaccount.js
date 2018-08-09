const db = require('../db')
const url = require('url')

module.exports = app => {
  app.get('/createAccount', (req, res) => {
    const accountId = req.get('X-Account-Id')
    const s = req.get('X-Account-S')
    const v = req.get('X-Account-V')
    console.log('X-Account-Id')
    console.log(accountId)
    console.log('X-Account-S')
    console.log(s)
    console.log('X-Account-V')
    console.log(v)
    let resultMsg, errMsg
    if (db.createAccount(accountId, s, v)) {
      resultMsg = '계정 생성 성공'
    } else {
      errMsg = '새 계정 생성 실패!'
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
