import { Socket } from 'dgram';
import { Request, Response } from 'express';
import * as raname from 'random-name';
import * as url from 'url';
import { v1 as uuidv1 } from 'uuid';
import * as db from './db';
import * as dbUser from './dbuser';
import * as spawnPort from './toseaserver/spawnport';
import { numbers } from './utils';

interface ExecCreatePortResult {
  seaportId?: number;
  err?: string;
}

export const execCreatePort = async (
  seaUdpClient: Socket,
  u: { user_id: number; guid: string },
  selectedLng: number,
  selectedLat: number,
  expectLand: number,
): Promise<ExecCreatePortResult> => {
  const portName = `Port ${raname.first()}`;
  // create db entry first
  const seaportId = db.createPort(
    portName,
    selectedLng,
    selectedLat,
    u.user_id,
    expectLand,
  );
  // send spawn command to sea-server
  const reply = await spawnPort.send(
    seaUdpClient,
    seaportId,
    portName,
    selectedLng,
    selectedLat,
    u.user_id,
    expectLand,
  );
  if (reply.dbId === seaportId) {
    if (reply.existing === 0) {
      // successfully created
      db.spendGold(u.guid, 10000);
      return {
        seaportId,
        err: undefined,
      };
    } else {
      console.error(
        'Port with the same ID already exists on sea-server (is this possible?!)',
      );
      return {
        seaportId,
        err: 'db inconsistent',
      };
    }
  }
  // something went wrong; delete from db
  db.deletePort(seaportId);
  // print useful information
  if (reply.dbId > 0 && reply.dbId !== seaportId) {
    console.log(
      `port cannot be created: port ID ${
        reply.dbId
      } already exists on that location`,
    );
    return {
      seaportId: reply.dbId,
      err: 'already exists',
    };
  } else if (reply.tooClose) {
    console.error('port cannot be created: too close');
    return {
      err: 'too close',
    };
  } else {
    console.error('port cannot be created: unknown case');
    return {
      err: 'unknown error',
    };
  }
};

export const purchaseNewPort = async (
  req: Request,
  res: Response,
  expectLand: number,
) => {
  const u = dbUser.findOrCreateUser(req.get('X-U') || uuidv1());
  const [xc0, yc0] = numbers(req, 'X-D-XC0', 'X-D-YC0');

  console.log(`purchaseNewPort at [${xc0}, ${yc0}]`);
  let resultMsg = '';
  let errMsg = '';
  const r0 = await execCreatePort(
    req.app.get('seaUdpClient'),
    u,
    +xc0,
    +yc0,
    expectLand,
  );
  if (r0.seaportId && r0.seaportId > 0 && !r0.err) {
    resultMsg = '새 항구 건설 완료';
  } else {
    errMsg = '새 항구 건설 실패';
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
