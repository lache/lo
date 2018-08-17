import { Socket } from 'dgram';

type Replier = (packet: any, err?: Error) => void;

export interface ReplyPacket {
  fields: { replyId: number };
}

let replyId = 0;
const replyWaiters: { [replyId: number]: Replier } = {};

export const issueNewReplyId = () => {
  return ++replyId;
};

export const sendAndReplyFromSea = async <T extends ReplyPacket>(
  seaUdpClient: Socket,
  buf: any,
  currentReplyId: number,
  onSendErr: (error: Error) => void,
) => {
  return new Promise<T>((resolve, reject) => {
    if (replyWaiters[currentReplyId]) {
      console.error('Previous reply waiter exist?!');
    }
    replyWaiters[currentReplyId] = (packet: T, err: Error) => {
      if (err) {
        reject(err);
      } else {
        resolve(packet);
      }
    };
    seaUdpClient.send(Buffer.from(buf), 4000, 'localhost', onSendErr);
  });
};

export const notifyWaiter = (packetStruct: ReplyPacket) => {
  const waiter = replyWaiters[packetStruct.fields.replyId];
  if (waiter) {
    waiter(packetStruct.fields, undefined);
    delete replyWaiters[packetStruct.fields.replyId];
  }
};
