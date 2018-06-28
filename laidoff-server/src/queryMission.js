const Sqlite3 = require('better-sqlite3')
const db = new Sqlite3('ttl.db')

const findMission = db.prepare(`SELECT
  mission_id, reward,
  dept.name AS dept_name, dept.x AS dept_x, dept.y AS dept_y,
  arvl.name AS arvl_name, arvl.x AS arvl_x, arvl.y AS arvl_y,
  (dept.x - arvl.x)*(dept.x - arvl.x)+(dept.y-arvl.y)*(dept.y-arvl.y) AS dist
FROM mission m
  JOIN seaport dept ON m.departure_id=dept.seaport_id
  JOIN seaport arvl ON m.arrival_id=arvl.seaport_id
WHERE m.mission_id = ?`)
const findMissions = db.prepare(`SELECT
  mission_id, reward,
  dept.name AS dept_name, dept.x AS dept_x, dept.y AS dept_y,
  arvl.name AS arvl_name, arvl.x AS arvl_x, arvl.y AS arvl_y,
  (dept.x - arvl.x)*(dept.x - arvl.x)+(dept.y-arvl.y)*(dept.y-arvl.y) AS dist
FROM mission m
  JOIN seaport dept ON m.departure_id=dept.seaport_id
  JOIN seaport arvl ON m.arrival_id=arvl.seaport_id`)

module.exports = {
  findMission,
  findMissions
}
