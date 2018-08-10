const db = require('../db')

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
    const result = db.createAccount(accountId, s, v)
    res.setHeader('Content-Type', 'application/json')
    res.send(JSON.stringify({ id: accountId, result: result ? 'ok' : 'fail' }))
  })
}
