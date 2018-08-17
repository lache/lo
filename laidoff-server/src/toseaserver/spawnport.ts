import { Socket } from 'dgram';
import { SpawnPortStruct } from '../message';
import {
  issueNewReplyId,
  ReplyPacket,
  sendAndReplyFromSea,
} from '../replywaiter';

interface SpawnPortReply extends ReplyPacket {
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
  expectLand: number,
) => {
  const buf = SpawnPortStruct.buffer();
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0;
  }
  const replyId = issueNewReplyId();
  SpawnPortStruct.fields.type = 6;
  SpawnPortStruct.fields.expectedDbId = expectedDbId;
  SpawnPortStruct.fields.name = name;
  SpawnPortStruct.fields.x = x;
  SpawnPortStruct.fields.y = y;
  SpawnPortStruct.fields.ownerId = ownerId;
  SpawnPortStruct.fields.replyId = replyId;
  SpawnPortStruct.fields.expectLand = expectLand;
  return sendAndReplyFromSea<SpawnPortReply>(
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
