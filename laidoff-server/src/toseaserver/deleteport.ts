import { Socket } from 'dgram';
import { DeletePortStruct } from '../message';

export const send = (seaUdpClient: Socket, seaportId: number) => {
  const buf = DeletePortStruct.buffer();
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0;
  }
  DeletePortStruct.fields.type = 8; // Delete Port
  DeletePortStruct.fields.portId = seaportId;
  seaUdpClient.send(Buffer.from(buf), 4000, 'localhost', err => {
    if (err) {
      console.error('sea udp client error:', err);
    }
  });
};
