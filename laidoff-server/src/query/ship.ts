import { db } from './db';

export const insertShip = db.prepare(
  `INSERT INTO ship (user_id, name, ship_type) VALUES (?, ?, ?)`,
);
export const deleteShip = db.prepare(`DELETE FROM ship WHERE ship_id = ?`);
export const setShipShiproute = db.prepare(
  `UPDATE ship SET shiproute_id = ? WHERE ship_id = ?`,
);
export const listShipShiproute = db.prepare(
  `SELECT
    ship_id, ship_type, docked_shipyard_id, template_id,
    sr.shiproute_id, port1_id, port2_id
  FROM ship s
    JOIN shiproute sr ON s.shiproute_id=sr.shiproute_id`,
);
export const findShipShiproute = db.prepare(
  `SELECT 
    ship_id, ship_type, docked_shipyard_id, template_id,
    sr.shiproute_id, port1_id, port2_id
  FROM ship s 
    JOIN shiproute sr ON s.shiproute_id=sr.shiproute_id 
  WHERE s.ship_id = ?`,
);
export const findShip = db.prepare(`SELECT * from ship WHERE ship_id = ?`);
export const findUserShipsScrollDown = db.prepare(
  `SELECT * from ship WHERE user_id = ? AND ship_id > ? ORDER BY ship_id LIMIT ?`,
);
export const findUserShipsScrollUp = db.prepare(
  `SELECT * from ship WHERE user_id = ? AND ship_id < ? ORDER BY ship_id DESC LIMIT ?`,
);
export const setShipDockedShipyardId = db.prepare(
  `UPDATE ship SET docked_shipyard_id = ? WHERE ship_id = ?`,
);
export const listShipDockedAtShipyard = db.prepare(
  `SELECT ship_id, name, user_id FROM ship WHERE docked_shipyard_id = ?`,
);
export const deleteShipDockedAtShipyard = db.prepare(
  `DELETE FROM ship WHERE docked_shipyard_id = ?`,
);
export const setShipCaptainId = db.prepare(
  `UPDATE ship SET captain_id = ? WHERE ship_id = ?`,
);
export const setShipTemplateId = db.prepare(
  `UPDATE ship SET template_id = ? WHERE ship_id = ?`,
);
