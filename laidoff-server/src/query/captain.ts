import { db } from './db';

export const insertCaptain = db.prepare(
  `INSERT INTO captain (user_id, name, template_id) VALUES (?, ?, ?)`,
);
export const deleteCaptain = db.prepare(
  `DELETE FROM captain WHERE captain_id = ?`,
);
export const findCaptain = db.prepare(
  `SELECT captain_id, name, template_id, user_id FROM captain WHERE captain_id = ?`,
);
