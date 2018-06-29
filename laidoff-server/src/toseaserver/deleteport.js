const message = require('../message')

const send = (seaUdpClient, seaportId) => {
  const buf = message.DeletePortStruct.buffer()
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0
  }
  message.DeletePortStruct.fields.type = 8 // Delete Port
  message.DeletePortStruct.fields.portId = seaportId
  seaUdpClient.send(Buffer.from(buf), 4000, 'localhost', err => {
    if (err) {
      console.error('sea udp client error:', err)
    }
  })
}

module.exports = {
  send
}
