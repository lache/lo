const db = require('../db')
const dbUser = require('../dbuser')
const uuidv1 = require('uuid/v1')

module.exports = app => {
  app.get('/success', (req, res) => {
    const u = dbUser.findOrCreateUser(req.query.u || uuidv1())
    const m = db.findMission(req.query.mission || 1)
    db.earnGold(u.guid, m.reward)
    dbUser.invalidateUserCache(u)
    return res.render('success', { user: u, mission: m })
  })
}
