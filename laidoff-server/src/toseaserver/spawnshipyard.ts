import { Socket } from 'dgram';
import { SpawnShipyardStruct } from '../message';
import {
  issueNewReplyId,
  ReplyPacket,
  sendAndReplyFromSea,
} from '../replywaiter';

interface SpawnShipyardReply extends ReplyPacket {
  dbId: number;
  existing: number;
  tooClose: boolean;
}

export const send = async (
  seaUdpClient: Socket,
  expectedDbId: number,
  name: string,
  x: number,
  y: number,
  ownerId: number,
) => {
  const buf = SpawnShipyardStruct.buffer();
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0;
  }
  const replyId = issueNewReplyId();
  SpawnShipyardStruct.fields.type = 9;
  SpawnShipyardStruct.fields.expectedDbId = expectedDbId;
  SpawnShipyardStruct.fields.name = name;
  SpawnShipyardStruct.fields.x = x;
  SpawnShipyardStruct.fields.y = y;
  SpawnShipyardStruct.fields.ownerId = ownerId;
  SpawnShipyardStruct.fields.replyId = replyId;
  return sendAndReplyFromSea<SpawnShipyardReply>(
    seaUdpClient,
    buf,
    replyId,
    err => {
      if (err) {
        console.error('sea udp SpawnShipyardStruct client error:', err);
      }
    },
  );
};
