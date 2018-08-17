import { Socket } from 'dgram';
import { SpawnShipStruct } from '../message';
import {
  issueNewReplyId,
  ReplyPacket,
  sendAndReplyFromSea,
} from '../replywaiter';

interface SpawnShipReply extends ReplyPacket {
  dbId: number;
}

export const send = async (
  seaUdpClient: Socket,
  expectedDbId: number,
  x: number,
  y: number,
  port1Id: number,
  port2Id: number,
  expectLand: number,
  shipTemplateId: number,
) => {
  const buf = SpawnShipStruct.buffer();
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0;
  }
  const replyId = issueNewReplyId();
  SpawnShipStruct.fields.type = 4;
  SpawnShipStruct.fields.expectedDbId = expectedDbId;
  SpawnShipStruct.fields.x = x;
  SpawnShipStruct.fields.y = y;
  SpawnShipStruct.fields.port1Id = port1Id;
  SpawnShipStruct.fields.port2Id = port2Id;
  SpawnShipStruct.fields.replyId = replyId;
  SpawnShipStruct.fields.expectLand = expectLand;
  SpawnShipStruct.fields.shipTemplateId = shipTemplateId;
  return sendAndReplyFromSea<SpawnShipReply>(
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
