import { Application } from 'express';
import { link } from '../link';

export default (app: Application) => {
  app.get('/linkland', async (req, res) => {
    await link(req, res, 1);
  });
};
