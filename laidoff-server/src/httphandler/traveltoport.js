const db = require('../db')
const dbUser = require('../dbuser')
const uuidv1 = require('uuid/v1')
const travelto = require('../toseaserver/travelto')

module.exports = app => {
  app.get('/traveltoport', (req, res) => {
    const u = dbUser.findOrCreateUser(req.query.u || uuidv1())
    const p = db.findPort(req.query.seaport || 1)
    console.log('travel to port', p.seaport_id, p.x, p.y)
    travelto.send(req.app.get('seaUdpClient'), u.guid, p.x, p.y)
    return res.render('idle', { user: u })
  })
}
