const captainData = require('../data/captain')
const dbUser = require('../dbuser')
const db = require('../db')
const url = require('url')
const raname = require('random-name')

module.exports = app => {
  app.get('/confirmHireCaptain', (req, res) => {
    let resultMsg = ''
    let errMsg = ''
    const u = dbUser.findOrCreateUser(req.get('X-U'))
    if (u) {
      const ship = db.findShip(req.query.shipId)
      if (ship) {
        const captainTemplate = captainData.data[req.query.captainTemplateId]
        if (captainTemplate) {
          const captainId = db.insertCaptain(
            u.user_id,
            raname.first(),
            req.query.captainTemplateId
          )
          db.setShipCaptainId(ship.ship_id, captainId)
          resultMsg = '선장 고용 성공'
        } else {
          errMsg = '선장 고용 실패 - 데이터 오류'
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
      } else {
        res.redirect(
          url.format({
            pathname: '/idle',
            query: {
              errMsg: '선박을 찾을 수 없습니다'
            }
          })
        )
      }
    }
  })
}
