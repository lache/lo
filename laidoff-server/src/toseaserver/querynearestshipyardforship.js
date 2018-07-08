const message = require('../message')
const replyWaiter = require('../replywaiter')

const send = async (seaUdpClient, shipId) => {
  const buf = message.QueryNearestShipyardForShipStruct.buffer()
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0
  }
  const replyId = replyWaiter.issueNewReplyId()
  message.QueryNearestShipyardForShipStruct.fields.type = 11
  message.QueryNearestShipyardForShipStruct.fields.shipId = shipId
  message.QueryNearestShipyardForShipStruct.fields.replyId = replyId
  return replyWaiter.sendAndReplyFromSea(seaUdpClient, buf, replyId, err => {
    if (err) {
      console.error('sea udp SpawnShipStruct client error:', err)
    }
  })
}

module.exports = {
  send
}
