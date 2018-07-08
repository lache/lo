const db = require('../db')
const dbUser = require('../dbuser')
const uuidv1 = require('uuid/v1')
const deletePort = require('../toseaserver/deleteport')
const url = require('url')

module.exports = app => {
  app.get('/sell_port', (req, res) => {
    const u = dbUser.findOrCreateUser(req.query.u || uuidv1())
    if (req.query.r) {
      const port = db.findPort(req.query.r)
      if (port) {
        db.deletePort(req.query.r)
        deletePort.send(req.app.get('seaUdpClient'), req.query.r)
      }
    }
    res.redirect(
      url.format({
        pathname: '/port',
        query: {
          u: u.guid,
          currentFirstKey: req.query.currentFirstKey
        }
      })
    )
  })
}
