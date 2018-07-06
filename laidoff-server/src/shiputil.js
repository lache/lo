const db = require('./db')
const dbUser = require('./dbuser')
const uuidv1 = require('uuid/v1')
const url = require('url')
const shipData = require('./data/ship')
const captainData = require('./data/captain')
const shipStatUtil = require('./shipstatutil')

const openShipHandler = (req, res) => {
  const u = dbUser.findOrCreateUser(req.get('X-U') || req.query.u || uuidv1())
  const ship = db.findShip(req.query.shipId)
  if (ship) {
    let seaport1, seaport2
    const dockedShipyard = ship.docked_shipyard_id
      ? db.findShipyard(ship.docked_shipyard_id)
      : null
    if (ship.shiproute_id) {
      const shiproute = db.findShipShiproute(ship.ship_id)
      seaport1 = db.findPort(shiproute.port1_id)
      seaport2 = db.findPort(shiproute.port2_id)
    } else {
      seaport1 = db.findPort(req.query.seaport1Id)
      seaport2 = db.findPort(req.query.seaport2Id)
    }
    const captain = ship.captain_id ? db.findCaptain(ship.captain_id) : null
    const captainTemplate = captain
      ? captainData.data[captain.template_id]
      : null
    const shipTemplate = ship.template_id
      ? shipData.data[ship.template_id]
      : shipData.data[1]
    const shipStat = shipStatUtil.addShipStat(shipTemplate, captainTemplate)
    return res.render('openship', {
      user: u,
      ship: ship,
      dockedShipyard: dockedShipyard,
      seaport1: seaport1,
      seaport2: seaport2,
      shipTemplate: shipTemplate,
      captain: captain,
      captainTemplate: captainTemplate,
      shipStat: shipStat,
      resultMsg: req.query.resultMsg,
      errMsg: req.query.errMsg
    })
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

module.exports = {
  openShipHandler
}
