import { Request, Response } from 'express';
import * as url from 'url';
import { v1 as uuidv1 } from 'uuid';
import * as dbUser from './dbuser';
import * as seaport from './seaport';
import * as ship from './ship';
import { numbers } from './utils';

export const link = async (req: Request, res: Response, expectLand: number) => {
  const u = dbUser.findOrCreateUser(req.get('X-U') || uuidv1());
  const [xc0, yc0, xc1, yc1] = numbers(
    req,
    'X-D-XC0',
    'X-D-YC0',
    'X-D-XC1',
    'X-D-YC1',
  );

  console.log(`Link [${xc0}, ${yc0}]-[${xc1}, ${yc1}]`);
  let resultMsg = '';
  let errMsg = '';
  const r0 = await seaport.execCreatePort(
    req.app.get('seaUdpClient'),
    u,
    xc0,
    yc0,
    expectLand,
  );
  if (r0.seaportId && r0.seaportId > 0) {
    const r1 = await seaport.execCreatePort(
      req.app.get('seaUdpClient'),
      u,
      xc1,
      yc1,
      expectLand,
    );
    if (r1.seaportId && r1.seaportId > 0) {
      const shipDbId = await ship.execCreateShipWithRoute(
        req.app.get('seaUdpClient'),
        u.user_id,
        xc0,
        yc0,
        expectLand,
        r0.seaportId,
        r1.seaportId,
        1,
      );
      if (shipDbId > 0) {
        resultMsg = '새 항로 등록 성공';
      } else {
        errMsg = '항로 등록 실패';
      }
    } else {
      errMsg = '도착 항구 건설 실패';
    }
  } else {
    errMsg = '출발 항구 건설 실패';
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
