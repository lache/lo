const Sqlite3 = require('better-sqlite3')
const db = new Sqlite3('ttl.db')

const insertAccount = db.prepare(
  `INSERT INTO account (account_id, s, v) VALUES (?, ?, ?)`
)

module.exports = {
  insertAccount
}
