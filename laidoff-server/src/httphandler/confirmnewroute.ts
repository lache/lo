import { Application } from 'express';
import * as url from 'url';
import * as db from '../db';

export default (app: Application) => {
  app.get('/confirmNewRoute', (req, res) => {
    let resultMsg = '';
    let errMsg = '';
    const ship = db.findShip(req.query.shipId);
    while (true) {
      if (ship) {
        const seaport1 = db.findPort(req.query.seaport1Id);
        if (seaport1) {
          const seaport2 = db.findPort(req.query.seaport2Id);
          if (seaport2) {
            if (req.query.seaport1Id === req.query.seaport2Id) {
              errMsg = '중복 항구 오류';
              break;
            }
            const shiprouteId = db.createShiproute(
              seaport1.seaport_id,
              seaport2.seaport_id,
            );
            db.setShipShiproute(ship.ship_id, shiprouteId);
            resultMsg = '항로 확정 성공';
            break;
          } else {
            errMsg = '항로 확정 실패 - 항구2오류';
            break;
          }
        } else {
          errMsg = '항로 확정 실패 - 항구1오류';
          break;
        }
      } else {
        errMsg = '항로 확정 실패 - 선박오류';
        break;
      }
      // break
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
