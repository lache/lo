const db = require('../db')
const url = require('url')

module.exports = app => {
  app.get('/deleteRoute', (req, res) => {
    let resultMsg, errMsg
    const ship = db.findShip(req.query.shipId)
    if (ship && ship.shiproute_id) {
      db.setShipShiproute(ship.ship_id, null)
      resultMsg = '항로 초기화 성공'
      const affectedRows = db.deleteShiproute(ship.shiproute_id)
      if (affectedRows !== 1) {
        errMsg = '항로 데이터 경고'
      }
    } else {
      errMsg = '항로 초기화 실패'
    }
    const qs = url.format({
      pathname: '',
      query: {
        resultMsg: resultMsg,
        errMsg: errMsg
      }
    })
    res.redirect(`script:ttl_refresh('${qs}')`)
  })
}
