import { Application } from 'express';
import * as uuidv1 from 'uuid';
import * as dbUser from '../dbuser';

export default (app: Application) => {
  app.get('/start', (req, res) => {
    const u = dbUser.findOrCreateUser(req.query.u || uuidv1());
    return res.render('start', { user: u });
  });
};
