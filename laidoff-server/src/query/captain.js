const Sqlite3 = require('better-sqlite3')
const db = new Sqlite3('ttl.db')

const insertCaptain = db.prepare(
  `INSERT INTO captain (owner_user_id, name, template_id) VALUES (?, ?, ?)`
)
const deleteCaptain = db.prepare(`DELETE FROM captain WHERE captain_id = ?`)
const findCaptain = db.prepare(
  `SELECT name, template_id, owner_user_id FROM captain WHERE captain_id = ?`
)

module.exports = {
  insertCaptain,
  deleteCaptain,
  findCaptain
}
