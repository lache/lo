import { Application } from 'express';
import * as url from 'url';
import * as uuidv1 from 'uuid';
import * as db from '../db';
import * as dbUser from '../dbuser';
import * as deletePort from '../toseaserver/deleteport';

export default (app: Application) => {
  app.get('/sell_port', (req, res) => {
    const u = dbUser.findOrCreateUser(req.query.u || uuidv1());
    if (req.query.r) {
      const port = db.findPort(req.query.r);
      if (port) {
        db.deletePort(req.query.r);
        deletePort.send(req.app.get('seaUdpClient'), req.query.r);
      }
    }
    res.redirect(
      url.format({
        pathname: '/port',
        query: {
          u: u.guid,
          currentFirstKey: req.query.currentFirstKey,
        },
      }),
    );
  });
};
