import { Application } from 'express';
import * as db from '../db';
import { numbers } from '../utils';

export default (app: Application) => {
  app.get('/createAccount', (req, res) => {
    const [accountId] = numbers(req, 'X-Account-Id');
    const s = req.get('X-Account-S');
    const v = req.get('X-Account-V');
    console.log('X-Account-Id');
    console.log(accountId);
    console.log('X-Account-S');
    console.log(s);
    console.log('X-Account-V');
    console.log(v);

    if (!accountId || !s || !v) {
      res.json({ id: 0, result: 'fail' });
    } else {
      const result = db.createAccount(accountId, s, v);
      res.json({ id: accountId, result: result ? 'ok' : 'fail' });
    }
  });
};
