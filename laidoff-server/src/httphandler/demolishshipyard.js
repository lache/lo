const db = require('../db')
const dbUser = require('../dbuser')
const uuidv1 = require('uuid/v1')
const deleteShipyard = require('../toseaserver/deleteshipyard')
const url = require('url')

module.exports = app => {
  app.get('/demolishShipyard', (req, res) => {
    const u = dbUser.findOrCreateUser(req.query.u || uuidv1())
    let resultMsg = ''
    let errMsg = ''
    if (req.query.shipyardId) {
      const shipyard = db.findShipyard(req.query.shipyardId)
      if (shipyard) {
        db.deleteShipyard(req.query.shipyardId)
        deleteShipyard.send(req.app.get('seaUdpClient'), req.query.shipyardId)
        resultMsg = '조선소 철거 성공'
      } else {
        errMsg = '존재하지 않는 조선소'
      }
    }
    res.redirect(
      url.format({
        pathname: '/idle',
        query: {
          u: u.guid,
          resultMsg: resultMsg,
          errMsg: errMsg
        }
      })
    )
  })
}
