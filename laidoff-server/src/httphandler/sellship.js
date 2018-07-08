const db = require('../db')
const deleteShip = require('../toseaserver/deleteship')
const url = require('url')

module.exports = app => {
  app.get('/sellShip', (req, res) => {
    // const u = dbuser.findOrCreateUser(req.query.u || uuidv1())
    let resultMsg, errMsg
    const ship = db.findShip(req.query.shipId)
    if (ship) {
      if (ship.shiproute_id) {
        db.deleteShiproute(ship.shiproute_id)
      }
      const deletedRowCount = db.deleteShip(ship.ship_id)
      if (deletedRowCount === 1) {
        deleteShip.send(app.get('seaUdpClient'), req.query.shipId)
        resultMsg = '선박 판매 성공'
      } else if (deletedRowCount === 0) {
        resultMsg = '선박 판매 성공?!'
      } else {
        errMsg = '선박 판매 실패'
      }
    }
    const qs = url.format({
      pathname: '',
      query: {
        resultMsg: resultMsg,
        errMsg: errMsg
      }
    })
    res.redirect(`script:ttl_go_back('${qs}')`)
  })
}
