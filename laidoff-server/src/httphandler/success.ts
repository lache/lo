import { Application } from 'express';
import { v1 as uuidv1 } from 'uuid';
import * as dbUser from '../dbuser';

export default (app: Application) => {
  app.get('/success', (req, res) => {
    const u = dbUser.findOrCreateUser(req.query.u || uuidv1());
    // const m = db.findMission(req.query.mission || 1);
    // db.earnGold(u.guid, m.reward);
    dbUser.invalidateUserCache(u);
    return res.render('success', { user: u });
  });
};
