const message = require('../message')
const replyWaiter = require('../replywaiter')

const receive = (buf, remote) => {
  // SpawnPortReply
  console.log(
    `SpawnPortReply from ${remote.address}:${remote.port} (len=${buf.length})`
  )
  message.SpawnPortReplyStruct._setBuff(buf)
  replyWaiter.notifyWaiter(message.SpawnPortReplyStruct)
}

module.exports = {
  receive
}
