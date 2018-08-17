import { db } from './db';

export const insertAccount = db.prepare(
  `INSERT INTO account (account_id, s, v) VALUES (?, ?, ?)`,
);
export const findAccount = db.prepare(
  `SELECT s, v FROM account WHERE account_id = ?`,
);
