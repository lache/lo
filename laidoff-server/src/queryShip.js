const Sqlite3 = require('better-sqlite3')
const db = new Sqlite3('ttl.db')

const insertShip = db.prepare(
  `INSERT INTO ship (user_id, name, ship_type) VALUES (?, ?, ?)`
)
const deleteShip = db.prepare(`DELETE FROM ship WHERE ship_id = ?`)
const setShipShiproute = db.prepare(
  `UPDATE ship SET shiproute_id = ? WHERE ship_id = ?`
)
const listShipShiproute = db.prepare(
  `SELECT ship_id, port1_id, port2_id, ship_type, docked_shipyard_id FROM ship s JOIN shiproute sr ON s.shiproute_id=sr.shiproute_id`
)
const findShipShiproute = db.prepare(
  `SELECT ship_id, docked_shipyard_id, port1_id, port2_id, ship_type, sr.shiproute_id FROM ship s JOIN shiproute sr ON s.shiproute_id=sr.shiproute_id WHERE s.ship_id = ?`
)
const findShip = db.prepare(`SELECT * from ship WHERE ship_id = ?`)
const findUserShipsScrollDown = db.prepare(
  `SELECT * from ship WHERE user_id = ? AND ship_id > ? ORDER BY ship_id LIMIT ?`
)
const findUserShipsScrollUp = db.prepare(
  `SELECT * from ship WHERE user_id = ? AND ship_id < ? ORDER BY ship_id DESC LIMIT ?`
)
const setShipDockedShipyardId = db.prepare(
  `UPDATE ship SET docked_shipyard_id = ? WHERE ship_id = ?`
)
const listShipDockedAtShipyard = db.prepare(
  `SELECT ship_id, name, user_id FROM ship WHERE docked_shipyard_id = ?`
)
const deleteShipDockedAtShipyard = db.prepare(
  `DELETE FROM ship WHERE docked_shipyard_id = ?`
)

module.exports = {
  insertShip,
  setShipShiproute,
  listShipShiproute,
  findShipShiproute,
  findShip,
  findUserShipsScrollDown,
  findUserShipsScrollUp,
  deleteShip,
  setShipDockedShipyardId,
  listShipDockedAtShipyard,
  deleteShipDockedAtShipyard
}