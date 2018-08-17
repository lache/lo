import { Application } from 'express';
import * as shipUtil from '../shiputil';

export default (app: Application) => {
  app.get('/openShip', shipUtil.openShipHandler);
};
