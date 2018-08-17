import { Socket } from 'dgram';
import { TravelToStruct } from '../message';

export const send = (
  seaUdpClient: Socket,
  id: string,
  x: number,
  y: number,
) => {
  const buf = TravelToStruct.buffer();
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0;
  }
  TravelToStruct.fields.type = 2;
  TravelToStruct.fields.id = id;
  TravelToStruct.fields.x = x;
  TravelToStruct.fields.y = y;
  seaUdpClient.send(Buffer.from(buf), 4000, 'localhost', err => {
    if (err) {
      console.error('sea udp client error:', err);
    }
  });
};
