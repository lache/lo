const dbUser = require('../dbuser')
const uuidv1 = require('uuid/v1')

module.exports = app => {
  app.get('/loan', (req, res) => {
    const u = dbUser.findOrCreateUser(req.query.u || uuidv1())
    return res.render('loan', { user: u })
  })
}
