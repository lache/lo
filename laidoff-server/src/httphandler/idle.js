const dbUser = require('../dbuser')
const uuidv1 = require('uuid/v1')

module.exports = app => {
  app.get('/idle', (req, res) => {
    const u = dbUser.findOrCreateUser(req.get('X-U') || req.query.u || uuidv1())
    return res.render('idle', {
      user: u,
      resultMsg: req.query.resultMsg,
      errMsg: req.query.errMsg,
      menuToggle: req.query.mt === 'false'
    })
  })
}
