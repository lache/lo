import { Application } from 'express';
import { v1 as uuidv1 } from 'uuid';
import * as db from '../db';
import * as dbUser from '../dbuser';
import * as teleportto from '../toseaserver/teleportto';

export default (app: Application) => {
  app.get('/teleporttoport', (req, res) => {
    const u = dbUser.findOrCreateUser(req.query.u || uuidv1());
    const p = db.findPort(req.query.seaport || 1);
    console.log('teleport to port', p.seaport_id, p.x, p.y);
    teleportto.send(app.get('seaUdpClient'), u.guid, p.x, p.y);
    return res.render('idle', { user: u });
  });
};
