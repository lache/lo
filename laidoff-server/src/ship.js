const db = require('./db')
const raname = require('random-name')
const spawnShip = require('./toseaserver/spawnship')

const execCreateShipWithRoute = async (
  seaUdpClient,
  userId,
  xc0,
  yc0,
  expectLand,
  r0,
  r1
) => {
  const shipName = `${raname.middle()} ${raname.middle()}`
  const dbId = db.createShip(userId, shipName, expectLand)
  const p0 = db.findPort(r0)
  const p1 = db.findPort(r1)
  if (p0 && p1) {
    const reply = await spawnShip.send(
      seaUdpClient,
      dbId,
      xc0,
      yc0,
      p0.seaport_id,
      p1.seaport_id,
      expectLand
    )
    if (reply.dbId === dbId) {
      return dbId
    } else {
      console.error(`Spawn ship request id and result id mismatch`)
      db.deleteShip(dbId)
    }
  } else {
    console.error(`Ports cannot be found - ${p0}, ${p1}`)
    db.deleteShip(dbId)
  }
  return 0
}

module.exports = {
  execCreateShipWithRoute
}
