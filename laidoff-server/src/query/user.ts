import { db } from './db';

export const insertUser = db.prepare(
  `INSERT INTO user (guid, name) VALUES (?, ?)`,
);
export const findUserGuid = db.prepare(
  `SELECT guid FROM user WHERE user_id = ?`,
);
export const findUser = db.prepare(`SELECT
  u.user_id, u.guid, u.name AS user_name, u.gold,
  s.ship_id, s.name AS ship_name, s.x, s.y, s.angle, s.oil
FROM user u
  LEFT JOIN ship s ON u.user_id = s.user_id
WHERE u.guid = ?
LIMIT 1`);
export const earnGold = db.prepare(
  `UPDATE user SET gold = gold + ? WHERE guid = ?`,
);
export const earnGoldUser = db.prepare(
  `UPDATE user SET gold = gold + ? WHERE user_id = ?`,
);
export const spendGold = db.prepare(
  `UPDATE user SET gold = gold - ? WHERE guid = ?`,
);
