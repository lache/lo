import { Socket } from 'dgram';
import { TeleportToStruct } from '../message';

export const send = (
  seaUdpClient: Socket,
  id: string,
  x: number,
  y: number,
) => {
  const buf = TeleportToStruct.buffer();
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0;
  }
  TeleportToStruct.fields.type = 3;
  TeleportToStruct.fields.id = id;
  TeleportToStruct.fields.x = x;
  TeleportToStruct.fields.y = y;
  seaUdpClient.send(Buffer.from(buf), 4000, 'localhost', err => {
    if (err) {
      console.error('sea udp client error:', err);
    }
  });
};
