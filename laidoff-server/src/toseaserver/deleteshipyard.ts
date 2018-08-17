import { Socket } from 'dgram';
import { DeleteShipyardStruct } from '../message';

export const send = (seaUdpClient: Socket, shipyardId: number) => {
  const buf = DeleteShipyardStruct.buffer();
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0;
  }
  DeleteShipyardStruct.fields.type = 10; // Delete Shipyard
  DeleteShipyardStruct.fields.shipyardId = shipyardId;
  seaUdpClient.send(Buffer.from(buf), 4000, 'localhost', err => {
    if (err) {
      console.error('sea udp client error:', err);
    }
  });
};
