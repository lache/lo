import { Socket } from 'dgram';
import { DeleteShipStruct } from '../message';

export const send = (seaUdpClient: Socket, shipId: number) => {
  const buf = DeleteShipStruct.buffer();
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0;
  }
  DeleteShipStruct.fields.type = 5; // Delete Ship
  DeleteShipStruct.fields.shipId = shipId;
  seaUdpClient.send(Buffer.from(buf), 4000, 'localhost', err => {
    if (err) {
      console.error('sea udp client error:', err);
    }
  });
};
