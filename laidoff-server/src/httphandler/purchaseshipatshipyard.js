const db = require('../db')
const dbUser = require('../dbuser')
const uuidv1 = require('uuid/v1')
const url = require('url')
const raname = require('random-name')

module.exports = app => {
  app.get('/purchaseShipAtShipyard', (req, res) => {
    const u = dbUser.findOrCreateUser(req.get('X-U') || req.query.u || uuidv1())
    const shipyard = db.findShipyard(req.query.shipyardId)
    let resultMsg, errMsg
    if (shipyard) {
      const dockedShips = db.listShipDockedAtShipyardToArray(
        shipyard.shipyard_id
      )
      if (dockedShips.length < 4) {
        const shipName = `${raname.middle()} ${raname.middle()}`
        const shipId = db.createShip(u.user_id, shipName, 0)
        db.setShipDockedShipyardId(shipId, req.query.shipyardId)
        resultMsg = '새 선박 구입 성공'
      } else {
        errMsg = '정박중 선박 초과'
      }
    } else {
      errMsg = '새 선박 구입 실패'
    }
    res.redirect(
      url.format({
        pathname: '/openShipyard',
        query: {
          shipyardId: shipyard.shipyard_id,
          resultMsg: resultMsg,
          errMsg: errMsg
        }
      })
    )
  })
}
