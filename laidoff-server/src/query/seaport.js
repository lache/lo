const Sqlite3 = require('better-sqlite3')
const db = new Sqlite3('ttl.db')

const insertPort = db.prepare(
  `INSERT INTO seaport (name, x, y, user_id, seaport_type) VALUES (?, ?, ?, ?, ?)`
)
const findPort = db.prepare(`SELECT
  seaport_id, name, x, y
FROM seaport r
WHERE r.seaport_id = ?`)
const findPorts = db.prepare(`SELECT
  seaport_id, name, x, y
FROM seaport LIMIT 7`)
const findPortsScrollDown = db.prepare(`SELECT
  seaport_id, name, x, y
  FROM seaport
  WHERE seaport_id > ? ORDER BY seaport_id LIMIT ?`)
const findPortsScrollUp = db.prepare(`SELECT
  seaport_id, name, x, y
  FROM seaport
  WHERE seaport_id < ? ORDER BY seaport_id DESC LIMIT ?`)
const listPort = db.prepare(
  `SELECT seaport_id, name, x, y, user_id, seaport_type FROM seaport`
)
const deletePort = db.prepare(`DELETE FROM seaport WHERE seaport_id = ?`)

module.exports = {
  findPort,
  insertPort,
  findPorts,
  findPortsScrollDown,
  findPortsScrollUp,
  listPort,
  deletePort
}
