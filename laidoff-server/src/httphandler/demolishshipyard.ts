import { Application } from 'express';
import * as url from 'url';
import { v1 as uuidv1 } from 'uuid';
import * as db from '../db';
import * as dbUser from '../dbuser';
import * as deleteShipyard from '../toseaserver/deleteshipyard';

export default (app: Application) => {
  app.get('/demolishShipyard', (req, res) => {
    const u = dbUser.findOrCreateUser(req.query.u || uuidv1());
    let resultMsg = '';
    let errMsg = '';
    if (req.query.shipyardId) {
      const shipyard = db.findShipyard(req.query.shipyardId);
      if (shipyard) {
        db.deleteShipyard(req.query.shipyardId);
        deleteShipyard.send(req.app.get('seaUdpClient'), req.query.shipyardId);
        resultMsg = '조선소 철거 성공';
      } else {
        errMsg = '존재하지 않는 조선소';
      }
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
  });
};
