import { Request, Response } from 'express';
import * as url from 'url';
import { v1 as uuidv1 } from 'uuid';
import captainData from './data/captain';
import shipData from './data/ship';
import * as db from './db';
import * as dbUser from './dbuser';
import { SeaportBase } from './query/model';
import * as shipStatUtil from './shipstatutil';

export const openShipHandler = (req: Request, res: Response) => {
  const u = dbUser.findOrCreateUser(req.get('X-U') || req.query.u || uuidv1());
  const ship = db.findShip(req.query.shipId);
  if (ship) {
    let seaport1: SeaportBase;
    let seaport2: SeaportBase;
    const dockedShipyard = ship.docked_shipyard_id
      ? db.findShipyard(ship.docked_shipyard_id)
      : null;
    if (ship.shiproute_id) {
      const shiproute = db.findShipShiproute(ship.ship_id);
      seaport1 = db.findPort(shiproute.port1_id);
      seaport2 = db.findPort(shiproute.port2_id);
    } else {
      seaport1 = db.findPort(req.query.seaport1Id);
      seaport2 = db.findPort(req.query.seaport2Id);
    }
    const captain = ship.captain_id ? db.findCaptain(ship.captain_id) : null;
    const captainTemplate = captain
      ? captainData.data[captain.template_id]
      : undefined;
    const shipTemplate = ship.template_id
      ? shipData.data[ship.template_id]
      : shipData.data[1];
    const shipStat = shipStatUtil.addShipStat(
      shipStatUtil.templateToStat(shipTemplate),
      shipStatUtil.templateToStat(captainTemplate),
    );
    return res.render('openship', {
      user: u,
      ship,
      dockedShipyard,
      seaport1,
      seaport2,
      shipTemplate,
      captain,
      captainTemplate,
      shipStat,
      resultMsg: req.query.resultMsg,
      errMsg: req.query.errMsg,
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
};
