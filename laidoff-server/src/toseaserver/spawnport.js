const message = require('../message')
const replyWaiter = require('../replywaiter')

const send = async (
  seaUdpClient,
  expectedDbId,
  name,
  x,
  y,
  ownerId,
  expectLand
) => {
  const buf = message.SpawnPortStruct.buffer()
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0
  }
  const replyId = replyWaiter.issueNewReplyId()
  message.SpawnPortStruct.fields.type = 6
  message.SpawnPortStruct.fields.expectedDbId = expectedDbId
  message.SpawnPortStruct.fields.name = name
  message.SpawnPortStruct.fields.x = x
  message.SpawnPortStruct.fields.y = y
  message.SpawnPortStruct.fields.ownerId = ownerId
  message.SpawnPortStruct.fields.replyId = replyId
  message.SpawnPortStruct.fields.expectLand = expectLand
  return replyWaiter.sendAndReplyFromSea(seaUdpClient, buf, replyId, err => {
    if (err) {
      console.error('sea udp SpawnShipStruct client error:', err)
    }
  })
}

module.exports = {
  send
}
