const Sqlite3 = require('better-sqlite3')
const db = new Sqlite3('ttl.db')
const fs = require('fs')

const initialize = () => {
  const schemaSql = fs.readFileSync('./src/sql/ttl.schema.sqlite3.sql', 'utf8')
  db.exec(schemaSql)

  const dataSql = fs.readFileSync('./src/sql/ttl.data.sqlite3.sql', 'utf8')
  db.exec(dataSql)
}

module.exports = {
  initialize
}
