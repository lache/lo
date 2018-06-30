const dbUser = require('../dbuser')
const db = require('../db')
const url = require('url')

module.exports = app => {
  app.get('/fireCaptain', (req, res) => {
    let resultMsg = ''
    let errMsg = ''
    const u = dbUser.findOrCreateUser(req.get('X-U'))
    if (u) {
      const ship = db.findShip(req.query.shipId)
      if (ship) {
        if (ship.captain_id) {
          const captain = db.findCaptain(ship.captain_id)
          if (captain) {
            db.deleteCaptain(captain.captain_id)
            db.setShipCaptainId(ship.ship_id, null)
            resultMsg = '해고 성공!!!'
          } else {
            errMsg = '선장이 존재하지 않음'
          }
        } else {
          errMsg = '선장이 없는 선박'
        }
      } else {
        errMsg = '선박 없음'
      }
      res.redirect(
        url.format({
          pathname: '/openShip',
          query: {
            shipId: ship.ship_id,
            resultMsg,
            errMsg
          }
        })
      )
      return
    } else {
      errMsg = '플레이어 없음'
    }
    res.redirect(
      url.format({
        pathname: '/idle',
        query: {
          errMsg: errMsg
        }
      })
    )
  })
}
