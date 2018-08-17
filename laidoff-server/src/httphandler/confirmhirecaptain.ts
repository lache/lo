import { Application } from 'express';
import * as raname from 'random-name';
import * as url from 'url';
import captainData from '../data/captain';
import * as db from '../db';
import * as dbUser from '../dbuser';

export default (app: Application) => {
  app.get('/confirmHireCaptain', (req, res) => {
    let resultMsg = '';
    let errMsg = '';

    const guid = req.get('X-U');
    const u = guid ? dbUser.findOrCreateUser(guid) : undefined;
    if (u) {
      const ship = db.findShip(req.query.shipId);
      if (ship) {
        const captainTemplate = captainData.data[req.query.captainTemplateId];
        if (captainTemplate) {
          const captainId = db.insertCaptain(
            u.user_id,
            raname.first(),
            req.query.captainTemplateId,
          );
          db.setShipCaptainId(ship.ship_id, captainId);
          resultMsg = '선장 고용 성공';
        } else {
          errMsg = '선장 고용 실패 - 데이터 오류';
        }
        res.redirect(
          url.format({
            pathname: '/openShip',
            query: {
              shipId: ship.ship_id,
              resultMsg,
              errMsg,
            },
          }),
        );
      } else {
        res.redirect(
          url.format({
            pathname: '/idle',
            query: {
              errMsg: '선박을 찾을 수 없습니다',
            },
          }),
        );
      }
    }
  });
};
