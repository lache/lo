import { db } from './db';

export const insertPort = db.prepare(
  `INSERT INTO seaport (name, x, y, user_id, seaport_type) VALUES (?, ?, ?, ?, ?)`,
);
export const findPort = db.prepare(`SELECT
  seaport_id, name, x, y
FROM seaport r
WHERE r.seaport_id = ?`);
export const findPorts = db.prepare(`SELECT
  seaport_id, name, x, y
FROM seaport LIMIT 7`);
export const findPortsScrollDown = db.prepare(`SELECT
  seaport_id, name, x, y
  FROM seaport
  WHERE seaport_id > ? ORDER BY seaport_id LIMIT ?`);
export const findPortsScrollUp = db.prepare(`SELECT
  seaport_id, name, x, y
  FROM seaport
  WHERE seaport_id < ? ORDER BY seaport_id DESC LIMIT ?`);
export const listPort = db.prepare(
  `SELECT seaport_id, name, x, y, user_id, seaport_type FROM seaport`,
);
export const deletePort = db.prepare(
  `DELETE FROM seaport WHERE seaport_id = ?`,
);
