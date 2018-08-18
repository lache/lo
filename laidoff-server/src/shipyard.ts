import { Socket } from 'dgram';
import { Request, Response } from 'express';
import * as raname from 'random-name';
import * as url from 'url';
import { v1 as uuidv1 } from 'uuid';
import * as db from './db';
import * as dbUser from './dbuser';
import * as spawnShipyard from './toseaserver/spawnshipyard';
import { numbers } from './utils';

export const execCreateShipyard = async (
  seaUdpClient: Socket,
  u: { user_id: number; guid: string },
  selectedLng: number,
  selectedLat: number,
) => {
  const shipyardName = `Shipyard ${raname.first()}`;
  // create db entry first
  const shipyardId = db.createShipyard(
    shipyardName,
    selectedLng,
    selectedLat,
    u.user_id,
  );
  // send spawn command to sea-server
  const reply = await spawnShipyard.send(
    seaUdpClient,
    shipyardId,
    shipyardName,
    selectedLng,
    selectedLat,
    u.user_id,
  );
  if (reply.dbId === shipyardId) {
    if (reply.existing === 0) {
      // successfully created
      db.spendGold(u.guid, 10000);
      return {
        shipyardId,
        err: null,
      };
    } else {
      console.error(
        'Shipyard with the same ID already exists on sea-server (is this possible?!)',
      );
      return {
        shipyardId,
        err: 'db inconsistent',
      };
    }
  }
  // something went wrong; delete from db
  db.deleteShipyard(shipyardId);
  // print useful information
  if (reply.dbId > 0 && reply.dbId !== shipyardId) {
    console.log(
      `shipyard cannot be created: shipyard ID ${
        reply.dbId
      } already exists on that location`,
    );
    return {
      seaportId: reply.dbId,
      err: 'already exists',
    };
  } else if (reply.tooClose) {
    console.error('shipyard cannot be created: too close');
    return {
      err: 'too close',
    };
  } else {
    console.error('shipyard cannot be created: unknown case');
    return {
      err: 'unknown error',
    };
  }
};

export const purchaseNewShipyard = async (req: Request, res: Response) => {
  const u = dbUser.findOrCreateUser(req.get('X-U') || uuidv1());
  const [xc0, yc0] = numbers(req, 'X-D-XC0', 'X-D-YC0');

  console.log(`purchaseNewShipyard at [${xc0}, ${yc0}]`);
  let resultMsg = '';
  let errMsg = '';
  const r0 = await execCreateShipyard(req.app.get('seaUdpClient'), u, xc0, yc0);
  if (r0.shipyardId && r0.shipyardId > 0 && !r0.err) {
    resultMsg = '새 조선소 건설 완료';
  } else {
    errMsg = '새 조선소 건설 실패';
  }
  res.redirect(
    url.format({
      pathname: '/idle',
      query: {
        u: u.guid,
        resultMsg,
        errMsg,
      },
    }),
  );
};

module.exports = {
  purchaseNewShipyard,
};
