import { Application } from 'express';
import { v1 as uuidv1 } from 'uuid';
import * as db from '../db';
import * as dbUser from '../dbuser';
import * as travelto from '../toseaserver/travelto';

export default (app: Application) => {
  app.get('/traveltoport', (req, res) => {
    const u = dbUser.findOrCreateUser(req.query.u || uuidv1());
    const p = db.findPort(req.query.seaport || 1);
    console.log('travel to port', p.seaport_id, p.x, p.y);
    travelto.send(req.app.get('seaUdpClient'), u.guid, p.x, p.y);
    return res.render('idle', { user: u });
  });
};
