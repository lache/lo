import { Application } from 'express';
import * as url from 'url';
import captainData from '../data/captain';
import * as db from '../db';

export default (app: Application) => {
  app.get('/openHireCaptain', (req, res) => {
    const ship = db.findShip(req.query.shipId);
    if (ship) {
      const captainTemplateCount = captainData.keys.length;
      const randomCaptainIndex = Math.floor(
        Math.random() * captainTemplateCount,
      );
      const captainTemplate =
        captainData.data[captainData.keys[randomCaptainIndex]];
      return res.render('openhirecaptain', {
        ship,
        captainTemplate,
      });
    } else {
      res.redirect(
        url.format({
          pathname: '/idle',
          query: {
            errMsg: '선박을 찾을 수 없습니다',
          },
        }),
      );
    }
  });
};
