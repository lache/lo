import { Application } from 'express';
import * as url from 'url';
import * as db from '../db';
import * as deleteShip from '../toseaserver/deleteship';
import * as queryNearestShipyardForShip from '../toseaserver/querynearestshipyardforship';

export default (app: Application) => {
  app.get('/moveToNearestShipyard', async (req, res) => {
    let resultMsg = '';
    let errMsg = '';
    const shipId = req.query.shipId;
    const reply = await queryNearestShipyardForShip.send(
      req.app.get('seaUdpClient'),
      shipId,
    );
    const nearestShipyardId = reply.shipyardId;
    if (nearestShipyardId >= 0) {
      const shipyard = db.findShipyard(nearestShipyardId);
      if (shipyard) {
        const dockedShips = db.listShipDockedAtShipyardToArray(
          nearestShipyardId,
        );
        if (dockedShips.length < 4) {
          deleteShip.send(req.app.get('seaUdpClient'), shipId);
          db.setShipDockedShipyardId(shipId, nearestShipyardId);
          resultMsg = '정박 성공';
        } else {
          errMsg = '가장 가까운 조선소 정박 초과';
        }
      } else {
        errMsg = '조선소 조회 오류';
      }
    } else {
      errMsg = '가장 가까운 조선소 조회 오류';
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
