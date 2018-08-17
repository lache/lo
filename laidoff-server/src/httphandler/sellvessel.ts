import { Application } from 'express';
import * as url from 'url';
import * as uuidv1 from 'uuid';
import * as db from '../db';
import * as dbUser from '../dbuser';
import * as despawnShip from '../toseaserver/despawnship';

export default (app: Application) => {
  app.get('/sell_vessel', (req, res) => {
    const u = dbUser.findOrCreateUser(req.query.u || uuidv1());
    if (req.query.s) {
      db.deleteShip(req.query.s);
      despawnShip.send(app.get('seaUdpClient'), req.query.s);
    }
    res.redirect(
      url.format({
        pathname: '/vessel',
        query: {
          u: u.guid,
          currentFirstKey: req.query.currentFirstKey,
        },
      }),
    );
  });
};
