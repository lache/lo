const Sqlite3 = require('better-sqlite3')
const db = new Sqlite3('ttl.db')

const insertShipyard = db.prepare(
  `INSERT INTO shipyard (name, x, y, owner_id) VALUES (?, ?, ?, ?)`
)
const deleteShipyard = db.prepare(`DELETE FROM shipyard WHERE shipyard_id = ?`)
const listShipyard = db.prepare(
  `SELECT shipyard_id, name, x, y, owner_id FROM shipyard`
)
const findShipyard = db.prepare(`SELECT
  shipyard_id, name, x, y
FROM shipyard s
WHERE s.shipyard_id = ?`)

module.exports = {
  insertShipyard,
  deleteShipyard,
  listShipyard,
  findShipyard
}
