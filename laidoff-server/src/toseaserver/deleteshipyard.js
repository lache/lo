const message = require('../message')

const send = (seaUdpClient, shipyardId) => {
  const buf = message.DeleteShipyardStruct.buffer()
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0
  }
  message.DeleteShipyardStruct.fields.type = 10 // Delete Shipyard
  message.DeleteShipyardStruct.fields.shipyardId = shipyardId
  seaUdpClient.send(Buffer.from(buf), 4000, 'localhost', err => {
    if (err) {
      console.error('sea udp client error:', err)
    }
  })
}

module.exports = {
  send
}
