import { Socket } from 'dgram';
import { QueryNearestShipyardForShipStruct } from '../message';
import {
  issueNewReplyId,
  ReplyPacket,
  sendAndReplyFromSea,
} from '../replywaiter';

interface QueryNearestShipyardForShipReply extends ReplyPacket {
  shipyardId: number;
}

export const send = async (seaUdpClient: Socket, shipId: number) => {
  const buf = QueryNearestShipyardForShipStruct.buffer();
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0;
  }
  const replyId = issueNewReplyId();
  QueryNearestShipyardForShipStruct.fields.type = 11;
  QueryNearestShipyardForShipStruct.fields.shipId = shipId;
  QueryNearestShipyardForShipStruct.fields.replyId = replyId;
  return sendAndReplyFromSea<QueryNearestShipyardForShipReply>(
    seaUdpClient,
    buf,
    replyId,
    err => {
      if (err) {
        console.error('sea udp SpawnShipStruct client error:', err);
      }
    },
  );
};
