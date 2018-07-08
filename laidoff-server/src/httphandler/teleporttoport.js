const db = require('../db')
const dbUser = require('../dbuser')
const uuidv1 = require('uuid/v1')
const teleportto = require('../toseaserver/teleportto')

module.exports = app => {
  app.get('/teleporttoport', (req, res) => {
    const u = dbUser.findOrCreateUser(req.query.u || uuidv1())
    const p = db.findPort(req.query.seaport || 1)
    console.log('teleport to port', p.seaport_id, p.x, p.y)
    teleportto.send(app.get('seaUdpClient'), u.guid, p.x, p.y)
    return res.render('idle', { user: u })
  })
}
