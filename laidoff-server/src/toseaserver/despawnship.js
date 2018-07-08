const message = require('../message')

const send = (seaUdpClient, shipId) => {
  const buf = message.DeleteShipStruct.buffer()
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0
  }
  message.DeleteShipStruct.fields.type = 5 // Delete Ship
  message.DeleteShipStruct.fields.shipId = shipId
  seaUdpClient.send(Buffer.from(buf), 4000, 'localhost', err => {
    if (err) {
      console.error('sea udp client error:', err)
    }
  })
}

module.exports = {
  send
}
