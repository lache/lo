const Sqlite3 = require('better-sqlite3')
const db = new Sqlite3('ttl.db')

const insertShiproute = db.prepare(
  `INSERT INTO shiproute (port1_id, port2_id) VALUES (?, ?)`
)
const deleteShiproute = db.prepare(
  `DELETE FROM shiproute WHERE shiproute_id = ?`
)

module.exports = {
  insertShiproute,
  deleteShiproute
}
