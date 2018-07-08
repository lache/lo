const db = require('../db')
const dbUser = require('../dbuser')
const uuidv1 = require('uuid/v1')
const despawnShip = require('../toseaserver/despawnship')
const url = require('url')

module.exports = app => {
  app.get('/sell_vessel', (req, res) => {
    const u = dbUser.findOrCreateUser(req.query.u || uuidv1())
    if (req.query.s) {
      db.deleteShip(req.query.s)
      despawnShip.send(app.get('seaUdpClient'), req.query.s)
    }
    res.redirect(
      url.format({
        pathname: '/vessel',
        query: {
          u: u.guid,
          currentFirstKey: req.query.currentFirstKey
        }
      })
    )
  })
}
