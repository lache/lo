import { Application } from 'express';
import * as shipyard from '../shipyard';

export default (app: Application) => {
  app.get('/purchaseNewShipyard', async (req, res) => {
    await shipyard.purchaseNewShipyard(req, res);
  });
};
