const message = require('../message')
const replyWaiter = require('../replywaiter')

const receive = (buf, remote) => {
  // SpawnShipyardReply
  console.log(
    `SpawnShipyardReply from ${remote.address}:${remote.port} (len=${
      buf.length
    })`
  )
  message.SpawnShipyardReplyStruct._setBuff(buf)
  replyWaiter.notifyWaiter(message.SpawnShipyardReplyStruct)
}

module.exports = {
  receive
}
