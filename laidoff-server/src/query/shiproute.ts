import { db } from './db';

export const insertShiproute = db.prepare(
  `INSERT INTO shiproute (port1_id, port2_id) VALUES (?, ?)`,
);
export const deleteShiproute = db.prepare(
  `DELETE FROM shiproute WHERE shiproute_id = ?`,
);
