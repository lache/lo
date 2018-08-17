import { Socket } from 'dgram';
import * as raname from 'random-name';
import * as db from './db';
import * as spawnShip from './toseaserver/spawnship';

export const execCreateShipWithRoute = async (
  seaUdpClient: Socket,
  userId: number,
  xc0: number,
  yc0: number,
  expectLand: number,
  r0: number,
  r1: number,
  shipTemplateId: number,
) => {
  const shipName = `${raname.middle()} ${raname.middle()}`;
  const dbId = db.createShip(userId, shipName, expectLand);
  db.setShipTemplateId(dbId, shipTemplateId);
  const p0 = db.findPort(r0);
  const p1 = db.findPort(r1);
  if (p0 && p1) {
    const reply = await spawnShip.send(
      seaUdpClient,
      dbId,
      xc0,
      yc0,
      p0.seaport_id,
      p1.seaport_id,
      expectLand,
      shipTemplateId,
    );
    if (reply.dbId === dbId) {
      return dbId;
    } else {
      console.error(`Spawn ship request id and result id mismatch`);
      db.deleteShip(dbId);
    }
  } else {
    console.error(`Ports cannot be found - ${p0}, ${p1}`);
    db.deleteShip(dbId);
  }
  return 0;
};
