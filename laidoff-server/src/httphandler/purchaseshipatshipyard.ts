import { Application } from 'express';
import * as raname from 'random-name';
import * as url from 'url';
import { v1 as uuidv1 } from 'uuid';
import * as db from '../db';
import * as dbUser from '../dbuser';
// const shipUtil = require('../shiputil')

export default (app: Application) => {
  app.get('/purchaseShipAtShipyard', (req, res) => {
    const u = dbUser.findOrCreateUser(
      req.get('X-U') || req.query.u || uuidv1(),
    );
    const shipyard = db.findShipyard(req.query.shipyardId);
    let resultMsg = '';
    let errMsg = '';
    if (shipyard) {
      const dockedShips = db.listShipDockedAtShipyardToArray(
        shipyard.shipyard_id,
      );
      if (dockedShips.length < 4) {
        const shipName = `${raname.middle()} ${raname.middle()}`;
        const shipId = db.createShip(u.user_id, shipName, 0);
        db.setShipDockedShipyardId(shipId, req.query.shipyardId);
        db.setShipTemplateId(shipId, req.query.shipTemplateId);
        resultMsg = '새 선박 구입 성공';
        // shipUtil.openShipHandler(req, res)
        res.redirect(
          url.format({
            pathname: '/openShip',
            query: {
              shipId,
              resultMsg,
            },
          }),
        );
        return;
      } else {
        errMsg = '정박중 선박 초과';
      }
    } else {
      errMsg = '새 선박 구입 실패';
    }
    res.redirect(
      url.format({
        pathname: '/openShipyard',
        query: {
          shipyardId: shipyard.shipyard_id,
          resultMsg,
          errMsg,
        },
      }),
    );
  });
};
