import { db } from './db';

export const insertShipyard = db.prepare(
  `INSERT INTO shipyard (name, x, y, user_id) VALUES (?, ?, ?, ?)`,
);
export const deleteShipyard = db.prepare(
  `DELETE FROM shipyard WHERE shipyard_id = ?`,
);
export const listShipyard = db.prepare(
  `SELECT shipyard_id, name, x, y, user_id FROM shipyard`,
);
export const findShipyard = db.prepare(`SELECT
  shipyard_id, name, x, y
FROM shipyard s
WHERE s.shipyard_id = ?`);
