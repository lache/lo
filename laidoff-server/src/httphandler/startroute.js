const db = require('../db')
const url = require('url')
const spawnShip = require('../toseaserver/spawnship')

module.exports = app => {
  app.get('/startRoute', async (req, res) => {
    let resultMsg, errMsg
    const shiproute = db.findShipShiproute(req.query.shipId)
    if (shiproute) {
      if (shiproute.shiproute_id > 0) {
        if (shiproute.docked_shipyard_id > 0) {
          const affectedRows = db.setShipDockedShipyardId(
            shiproute.ship_id,
            null
          )
          if (affectedRows !== 1) {
            errMsg = '항로 데이터 경고'
          }
          resultMsg = '운항 시작 성공'
          await spawnShip.send(
            req.app.get('seaUdpClient'),
            shiproute.ship_id,
            0,
            0,
            shiproute.port1_id,
            shiproute.port2_id,
            shiproute.ship_type,
            shiproute.template_id
          )
        } else {
          errMsg = '정박중인 선박 아님'
        }
      } else {
        errMsg = '항로 미확정 선박'
      }
    } else {
      errMsg = '선박 찾기 실패'
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
