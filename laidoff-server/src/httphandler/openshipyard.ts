import { Application } from 'express';
import * as url from 'url';
import { v1 as uuidv1 } from 'uuid';
import shipData from '../data/ship';
import * as db from '../db';
import * as dbUser from '../dbuser';

export default (app: Application) => {
  app.get('/openShipyard', (req, res) => {
    const u = dbUser.findOrCreateUser(
      req.get('X-U') || req.query.u || uuidv1(),
    );
    const shipyard = db.findShipyard(req.query.shipyardId);
    const dockedShips = db.listShipDockedAtShipyardToArray(
      req.query.shipyardId,
    );
    if (shipyard) {
      return res.render('openshipyard', {
        user: u,
        shipyard,
        dockedShips,
        shipData,
        resultMsg: req.query.resultMsg,
        errMsg: req.query.errMsg,
      });
    } else {
      res.redirect(
        url.format({
          pathname: '/idle',
          query: {
            errMsg: '조선소를 찾을 수 없습니다',
          },
        }),
      );
    }
  });
};
