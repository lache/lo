const db = require('../db')
const dbUser = require('../dbuser')
const uuidv1 = require('uuid/v1')

module.exports = app => {
  app.get('/mission', (req, res) => {
    const u = dbUser.findOrCreateUser(req.query.u || uuidv1())
    const m = db.findMissions()
    return res.render('mission', { user: u, rows: m })
  })
}
