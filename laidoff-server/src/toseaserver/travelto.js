const message = require('../message')

const send = (seaUdpClient, id, x, y) => {
  const buf = message.TravelToStruct.buffer()
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0
  }
  message.TravelToStruct.fields.type = 2
  message.TravelToStruct.fields.id = id
  message.TravelToStruct.fields.x = x
  message.TravelToStruct.fields.y = y
  seaUdpClient.send(Buffer.from(buf), 4000, 'localhost', err => {
    if (err) {
      console.error('sea udp client error:', err)
    }
  })
}

module.exports = {
  send
}
