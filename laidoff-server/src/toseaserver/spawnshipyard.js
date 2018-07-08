const replyWaiter = require('../replywaiter')
const message = require('../message')

const send = async (seaUdpClient, expectedDbId, name, x, y, ownerId) => {
  const buf = message.SpawnShipyardStruct.buffer()
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0
  }
  const replyId = replyWaiter.issueNewReplyId()
  message.SpawnShipyardStruct.fields.type = 9
  message.SpawnShipyardStruct.fields.expectedDbId = expectedDbId
  message.SpawnShipyardStruct.fields.name = name
  message.SpawnShipyardStruct.fields.x = x
  message.SpawnShipyardStruct.fields.y = y
  message.SpawnShipyardStruct.fields.ownerId = ownerId
  message.SpawnShipyardStruct.fields.replyId = replyId
  return replyWaiter.sendAndReplyFromSea(seaUdpClient, buf, replyId, err => {
    if (err) {
      console.error('sea udp SpawnShipyardStruct client error:', err)
    }
  })
}

module.exports = {
  send
}
