const message = require('../message')
const replyWaiter = require('../replywaiter')

const send = async (
  seaUdpClient,
  expectedDbId,
  x,
  y,
  port1Id,
  port2Id,
  expectLand
) => {
  const buf = message.SpawnShipStruct.buffer()
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0
  }
  const replyId = replyWaiter.issueNewReplyId()
  message.SpawnShipStruct.fields.type = 4
  message.SpawnShipStruct.fields.expectedDbId = expectedDbId
  message.SpawnShipStruct.fields.x = x
  message.SpawnShipStruct.fields.y = y
  message.SpawnShipStruct.fields.port1Id = port1Id
  message.SpawnShipStruct.fields.port2Id = port2Id
  message.SpawnShipStruct.fields.replyId = replyId
  message.SpawnShipStruct.fields.expectLand = expectLand
  return replyWaiter.sendAndReplyFromSea(seaUdpClient, buf, replyId, err => {
    if (err) {
      console.error('sea udp SpawnShipStruct client error:', err)
    }
  })
}

module.exports = {
  send
}
