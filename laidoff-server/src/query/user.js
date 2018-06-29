const Sqlite3 = require('better-sqlite3')
const db = new Sqlite3('ttl.db')

const insertUser = db.prepare(`INSERT INTO user (guid, name) VALUES (?, ?)`)
const findUserGuid = db.prepare(`SELECT guid FROM user WHERE user_id = ?`)
const findUser = db.prepare(`SELECT
  u.user_id, u.guid, u.name AS user_name, u.gold,
  s.ship_id, s.name AS ship_name, s.x, s.y, s.angle, s.oil
FROM user u
  LEFT JOIN ship s ON u.user_id = s.user_id
WHERE u.guid = ?
LIMIT 1`)
const earnGold = db.prepare(`UPDATE user SET gold = gold + ? WHERE guid = ?`)
const earnGoldUser = db.prepare(
  `UPDATE user SET gold = gold + ? WHERE user_id = ?`
)
const spendGold = db.prepare(`UPDATE user SET gold = gold - ? WHERE guid = ?`)

module.exports = {
  insertUser,
  findUser,
  earnGold,
  earnGoldUser,
  spendGold,
  findUserGuid
}
