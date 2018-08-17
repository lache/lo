import { Application } from 'express';
import * as seaport from '../seaport';

export default (app: Application) => {
  app.get('/purchaseNewPort', async (req, res) => {
    await seaport.purchaseNewPort(req, res, 0);
  });
};
