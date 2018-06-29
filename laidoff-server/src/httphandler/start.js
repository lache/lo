const db = require('../db')
const dbUser = require('../dbuser')
const uuidv1 = require('uuid/v1')

module.exports = app => {
  app.get('/start', (req, res) => {
    const u = dbUser.findOrCreateUser(req.query.u || uuidv1())
    const m = db.findMission(req.query.mission || 1)
    return res.render('start', { user: u, mission: m })
  })
}
