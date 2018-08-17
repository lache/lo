import { Application } from 'express';
import * as url from 'url';
import * as db from '../db';
import * as dbUser from '../dbuser';

export default (app: Application) => {
  app.get('/fireCaptain', (req, res) => {
    let resultMsg = '';
    let errMsg = '';
    const guid = req.get('X-U');
    const u = guid ? dbUser.findOrCreateUser(guid) : undefined;
    if (u) {
      const ship = db.findShip(req.query.shipId);
      if (ship) {
        if (ship.captain_id) {
          const captain = db.findCaptain(ship.captain_id);
          if (captain) {
            db.deleteCaptain(captain.captain_id);
            db.setShipCaptainId(ship.ship_id, undefined);
            resultMsg = '해고 성공!!!';
          } else {
            errMsg = '선장이 존재하지 않음';
          }
        } else {
          errMsg = '선장이 없는 선박';
        }
      } else {
        errMsg = '선박 없음';
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
      return;
    } else {
      errMsg = '플레이어 없음';
    }
    res.redirect(
      url.format({
        pathname: '/idle',
        query: {
          errMsg,
        },
      }),
    );
  });
};
