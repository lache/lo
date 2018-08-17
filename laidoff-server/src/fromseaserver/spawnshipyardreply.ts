import { AddressInfo } from 'net';
import { SpawnShipyardReplyStruct } from '../message';
import { notifyWaiter } from '../replywaiter';

export const receive = (buf: any, remote: AddressInfo) => {
  // SpawnShipyardReply
  console.log(
    `SpawnShipyardReply from ${remote.address}:${remote.port} (len=${
      buf.length
    })`,
  );
  SpawnShipyardReplyStruct._setBuff(buf);
  notifyWaiter(SpawnShipyardReplyStruct);
};
