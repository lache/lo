import { Application } from 'express';
import * as url from 'url';
import * as db from '../db';
import * as deletePort from '../toseaserver/deleteport';

export default (app: Application) => {
  app.get('/demolishPort', (req, res) => {
    // const u = dbuser.findOrCreateUser(req.get('X-U') || req.query.u || uuidv1())
    if (req.query.portId) {
      const port = db.findPort(req.query.portId);
      if (port) {
        db.deletePort(req.query.portId);
        deletePort.send(req.app.get('seaUdpClient'), port.seaport_id);
      }
    }
    res.redirect(
      url.format({
        pathname: '/idle',
      }),
    );
  });
};
