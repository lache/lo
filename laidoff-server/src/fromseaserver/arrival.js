const message = require('../message')
const db = require('../db')
const dbUser = require('../dbuser')

const receive = (buf, remote) => {
  // Arrival
  message.ArrivalStruct._setBuff(buf)
  // console.log(
  //   `Arrival ship id ${message.ArrivalStruct.fields.shipId} from ${
  //     remote.address
  //   }:${remote.port} (len=${buf.length})`
  // )
  const ship = db.findShip(message.ArrivalStruct.fields.shipId)
  if (ship) {
    db.earnGoldUser(ship.user_id, 1)
    const u = db.findUserGuid(ship.user_id)
    dbUser.invalidateUserCache(u)
  } else {
    console.error(
      `Could not find ship with id ${message.ArrivalStruct.fields.shipId}!`
    )
  }
}

module.exports = {
  receive
}
