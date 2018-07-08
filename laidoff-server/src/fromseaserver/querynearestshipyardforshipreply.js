const message = require('../message')
const replyWaiter = require('../replywaiter')

const receive = (buf, remote) => {
  // QueryNearestShipyardForShipReplyStruct
  console.log(
    `QueryNearestShipyardForShipReplyStruct from ${remote.address}:${
      remote.port
    } (len=${buf.length})`
  )
  message.QueryNearestShipyardForShipReplyStruct._setBuff(buf)
  replyWaiter.notifyWaiter(message.QueryNearestShipyardForShipReplyStruct)
}

module.exports = {
  receive
}
