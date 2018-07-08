const queryNearestShipyardForShip = require('../toseaserver/querynearestshipyardforship')
const deleteShip = require('../toseaserver/deleteship')
const db = require('../db')
const url = require('url')

module.exports = app => {
  app.get('/moveToNearestShipyard', async (req, res) => {
    let resultMsg, errMsg
    const shipId = req.query.shipId
    const reply = await queryNearestShipyardForShip.send(
      req.app.get('seaUdpClient'),
      shipId
    )
    const nearestShipyardId = reply.shipyardId
    if (nearestShipyardId >= 0) {
      const shipyard = db.findShipyard(nearestShipyardId)
      if (shipyard) {
        const dockedShips = db.listShipDockedAtShipyardToArray(
          nearestShipyardId
        )
        if (dockedShips.length < 4) {
          deleteShip.send(req.app.get('seaUdpClient'), shipId)
          db.setShipDockedShipyardId(shipId, nearestShipyardId)
          resultMsg = '정박 성공'
        } else {
          errMsg = '가장 가까운 조선소 정박 초과'
        }
      } else {
        errMsg = '조선소 조회 오류'
      }
    } else {
      errMsg = '가장 가까운 조선소 조회 오류'
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
