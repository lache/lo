const message = require('../message')
const db = require('../db')
const replyWaiter = require('../replywaiter')

const receive = (buf, remote) => {
  console.log(
    `SpawnShipReply from ${remote.address}:${remote.port} (len=${buf.length})`
  )
  message.SpawnShipReplyStruct._setBuff(buf)
  replyWaiter.notifyWaiter(message.SpawnShipReplyStruct)
  // console.log('UDP type: ' + message.SpawnShipReplyStruct.fields.type)
  // console.log('UDP ship_id: ' + message.SpawnShipReplyStruct.fields.shipId)
  // console.log('UDP port1_id: ' + message.SpawnShipReplyStruct.fields.port1Id)
  // console.log('UDP port2_id: ' + message.SpawnShipReplyStruct.fields.port2Id)
  const shiprouteId = db.createShiproute(
    message.SpawnShipReplyStruct.fields.port1Id,
    message.SpawnShipReplyStruct.fields.port2Id
  )
  db.setShipShiproute(message.SpawnShipReplyStruct.fields.dbId, shiprouteId)
}

module.exports = {
  receive
}
