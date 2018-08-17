import { AddressInfo } from 'net';
import * as db from '../db';
import * as dbUser from '../dbuser';
import { ArrivalStruct } from '../message';

export const receive = (buf: any, remote: AddressInfo) => {
  // Arrival
  ArrivalStruct._setBuff(buf);
  // console.log(
  //   `Arrival ship id ${ArrivalStruct.fields.shipId} from ${
  //     remote.address
  //   }:${remote.port} (len=${buf.length})`
  // )
  const ship = db.findShip(ArrivalStruct.fields.shipId);
  if (ship) {
    db.earnGoldUser(ship.user_id, 1);
    const u = db.findUserGuid(ship.user_id);
    dbUser.invalidateUserCache(u);
  } else {
    console.error(
      `Could not find ship with id ${ArrivalStruct.fields.shipId}!`,
    );
  }
};
