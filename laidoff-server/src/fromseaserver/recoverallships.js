const db = require('../db')
const spawnPort = require('../toseaserver/spawnport')
const spawnShipyard = require('../toseaserver/spawnshipyard')
const spawnShip = require('../toseaserver/spawnship')

const receive = async (seaUdpClient, buf, remote) => {
  // RecoverAllShips
  console.log(
    `RecoverAllShips from ${remote.address}:${remote.port} (len=${buf.length})`
  )
  console.log('A new sea-server instance requested recovering.')
  console.log('Recovering in progress...')
  // recovering ports
  let portCount = 0
  const ports = await db.listPortToArray()
  for (let i = 0; i < ports.length; i++) {
    const row = ports[i]
    await spawnPort.send(
      seaUdpClient,
      row.seaport_id,
      row.name,
      row.x,
      row.y,
      row.owner_id,
      row.seaport_type
    )
    portCount++
  }
  console.log(`  ${portCount} port(s) recovered...`)
  // recovering shipyards
  let shipyardCount = 0
  const shipyards = await db.listShipyardToArray()
  for (let i = 0; i < shipyards.length; i++) {
    const row = shipyards[i]
    await spawnShipyard.send(
      seaUdpClient,
      row.shipyard_id,
      row.name,
      row.x,
      row.y,
      row.owner_id
    )
    shipyardCount++
  }
  console.log(`  ${shipyardCount} shipyard(s) recovered...`)
  // recovering ships
  let shipShiprouteCount = 0
  let shipDockedCount = 0
  const ships = db.listShipShiprouteToArray()
  for (let i = 0; i < ships.length; i++) {
    const row = ships[i]
    if (!row.docked_shipyard_id) {
      await spawnShip.send(
        seaUdpClient,
        row.ship_id,
        0,
        0,
        row.port1_id,
        row.port2_id,
        row.ship_type,
        row.template_id
      )
      shipShiprouteCount++
    } else {
      shipDockedCount++
    }
  }
  console.log(
    `  ${shipShiprouteCount} ship(s) recovered... (${shipDockedCount} docked ships excluded)`
  )
  console.log(`Recovering Done.`)
}

module.exports = {
  receive
}
