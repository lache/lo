import { AddressInfo } from 'net';
import { QueryNearestShipyardForShipReplyStruct } from '../message';
import { notifyWaiter } from '../replywaiter';

export const receive = (buf: any, remote: AddressInfo) => {
  // QueryNearestShipyardForShipReplyStruct
  console.log(
    `QueryNearestShipyardForShipReplyStruct from ${remote.address}:${
      remote.port
    } (len=${buf.length})`,
  );
  QueryNearestShipyardForShipReplyStruct._setBuff(buf);
  notifyWaiter(QueryNearestShipyardForShipReplyStruct);
};
