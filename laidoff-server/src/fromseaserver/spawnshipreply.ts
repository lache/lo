import { AddressInfo } from 'net';
import * as db from '../db';
import { SpawnShipReplyStruct } from '../message';
import { notifyWaiter } from '../replywaiter';

export const receive = (buf: any, remote: AddressInfo) => {
  console.log(
    `SpawnShipReply from ${remote.address}:${remote.port} (len=${buf.length})`,
  );
  SpawnShipReplyStruct._setBuff(buf);
  notifyWaiter(SpawnShipReplyStruct);
  // console.log('UDP type: ' + SpawnShipReplyStruct.fields.type)
  // console.log('UDP ship_id: ' + SpawnShipReplyStruct.fields.shipId)
  // console.log('UDP port1_id: ' + SpawnShipReplyStruct.fields.port1Id)
  // console.log('UDP port2_id: ' + SpawnShipReplyStruct.fields.port2Id)
  const shiprouteId = db.createShiproute(
    SpawnShipReplyStruct.fields.port1Id,
    SpawnShipReplyStruct.fields.port2Id,
  );
  db.setShipShiproute(SpawnShipReplyStruct.fields.dbId, shiprouteId);
};
