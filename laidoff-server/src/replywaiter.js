let replyId = 0
let replyWaiters = {}

const issueNewReplyId = () => {
  return ++replyId
}

const sendAndReplyFromSea = async (seaUdpClient, buf, replyId, onSendErr) => {
  return new Promise((resolve, reject) => {
    if (replyWaiters[replyId]) {
      console.error('Previous reply waiter exist?!')
    }
    replyWaiters[replyId] = (packet, err) => {
      if (err) {
        reject(err)
      } else {
        resolve(packet)
      }
    }
    seaUdpClient.send(Buffer.from(buf), 4000, 'localhost', onSendErr)
  })
}

const notifyWaiter = packetStruct => {
  const waiter = replyWaiters[packetStruct.fields.replyId]
  if (waiter) {
    waiter(packetStruct.fields, null)
    delete replyWaiters[packetStruct.fields.replyId]
  }
}

module.exports = {
  issueNewReplyId,
  sendAndReplyFromSea,
  notifyWaiter
}
