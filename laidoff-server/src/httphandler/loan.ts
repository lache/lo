import { Application } from 'express';
import { v1 as uuidv1 } from 'uuid';
import * as dbUser from '../dbuser';

export default (app: Application) => {
  app.get('/loan', (req, res) => {
    const u = dbUser.findOrCreateUser(req.query.u || uuidv1());
    return res.render('loan', { user: u });
  });
};
