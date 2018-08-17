import { AddressInfo } from 'net';
import { SpawnPortReplyStruct } from '../message';
import { notifyWaiter } from '../replywaiter';

export const receive = (buf: any, remote: AddressInfo) => {
  // SpawnPortReply
  console.log(
    `SpawnPortReply from ${remote.address}:${remote.port} (len=${buf.length})`,
  );
  SpawnPortReplyStruct._setBuff(buf);
  notifyWaiter(SpawnPortReplyStruct);
};
