import { Application } from 'express';
import * as url from 'url';
import * as db from '../db';

export default (app: Application) => {
  app.get('/deleteRoute', (req, res) => {
    let resultMsg = '';
    let errMsg = '';
    const ship = db.findShip(req.query.shipId);
    if (ship && ship.shiproute_id) {
      db.setShipShiproute(ship.ship_id, undefined);
      resultMsg = '항로 초기화 성공';
      const affectedRows = db.deleteShiproute(ship.shiproute_id);
      if (affectedRows !== 1) {
        errMsg = '항로 데이터 경고';
      }
    } else {
      errMsg = '항로 초기화 실패';
    }
    const qs = url.format({
      pathname: '',
      query: {
        resultMsg,
        errMsg,
      },
    });
    res.redirect(`script:ttl_refresh('${qs}')`);
  });
};
