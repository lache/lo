import { Application } from 'express';
import { v1 as uuidv1 } from 'uuid';
import * as dbUser from '../dbuser';

export default (app: Application) => {
  app.get('/idle', (req, res) => {
    const u = dbUser.findOrCreateUser(
      req.get('X-U') || req.query.u || uuidv1(),
    );
    return res.render('idle', {
      user: u,
      resultMsg: req.query.resultMsg,
      errMsg: req.query.errMsg,
      menuToggle: req.query.mt === 'false',
    });
  });
};
