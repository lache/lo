const raname = require('random-name')
const querySeaport = require('./query/seaport')
const queryShip = require('./query/ship')
const queryShiproute = require('./query/shiproute')
const queryShipyard = require('./query/shipyard')
const queryUser = require('./query/user')
const queryCaptain = require('./query/captain')

const createUser = guid => {
  const userName = `${raname.first()} ${raname.last()}`
  const user = queryUser.insertUser.run(guid, userName)
  // no initial ship
  // const shipName = `${raname.middle()} ${raname.middle()}`
  // query.insertShip.run(user.lastInsertROWID, shipName)
  return user.lastInsertROWID
}
const createPort = (portName, x, y, userId, expectLand) => {
  const port = querySeaport.insertPort.run(portName, x, y, userId, expectLand)
  return port.lastInsertROWID
}
const createShipyard = (shipyardName, x, y, userId) => {
  const shipyard = queryShipyard.insertShipyard.run(shipyardName, x, y, userId)
  return shipyard.lastInsertROWID
}
const createShip = (userId, shipName, shipType) => {
  const ship = queryShip.insertShip.run(userId, shipName, shipType)
  return ship.lastInsertROWID
}
const deleteShip = shipId => {
  return queryShip.deleteShip.run(shipId).changes
}
const createShiproute = (port1Id, port2Id) => {
  const shiproute = queryShiproute.insertShiproute.run(port1Id, port2Id)
  return shiproute.lastInsertROWID
}
const deleteShiproute = shiprouteId => {
  return queryShiproute.deleteShiproute.run(shiprouteId).changes
}
const setShipShiproute = (shipId, shiprouteId) => {
  queryShip.setShipShiproute.run(shiprouteId, shipId)
}
const setShipDockedShipyardId = (shipId, dockedShipyardId) => {
  return queryShip.setShipDockedShipyardId.run(dockedShipyardId, shipId).changes
}
const setShipCaptainId = (shipId, captainId) => {
  queryShip.setShipCaptainId.run(captainId, shipId)
}
const setShipTemplateId = (shipId, templateId) => {
  queryShip.setShipTemplateId.run(templateId, shipId)
}
const findShipShiproute = shipId => {
  return queryShip.findShipShiproute.get(shipId)
}
const listShipShiproute = onRow => {
  // noinspection JSUnresolvedFunction
  for (const row of queryShip.listShipShiproute.iterate()) {
    onRow(row)
  }
}
const listShipShiprouteToArray = () => {
  const rows = []
  listShipShiproute(row => {
    rows.push(row)
  })
  return rows
}
const listPort = async onRow => {
  // noinspection JSUnresolvedFunction
  for (const row of querySeaport.listPort.iterate()) {
    await onRow(row)
  }
}
const listPortToArray = async () => {
  const rows = []
  await listPort(row => {
    rows.push(row)
  })
  return rows
}
const listShipyard = async onRow => {
  // noinspection JSUnresolvedFunction
  for (const row of queryShipyard.listShipyard.iterate()) {
    await onRow(row)
  }
}
const listShipyardToArray = async () => {
  const rows = []
  await listShipyard(row => {
    rows.push(row)
  })
  return rows
}
const listShipDockedAtShipyard = (shipyardId, onRow) => {
  // noinspection JSUnresolvedFunction
  for (const row of queryShip.listShipDockedAtShipyard.iterate(shipyardId)) {
    onRow(row)
  }
}
const listShipDockedAtShipyardToArray = shipyardId => {
  const rows = []
  listShipDockedAtShipyard(shipyardId, row => {
    rows.push(row)
  })
  return rows
}
const findShip = shipId => queryShip.findShip.get(shipId)
const findUser = guid => queryUser.findUser.get(guid)
const findUserGuid = userId => queryUser.findUserGuid.get(userId)
const earnGold = (guid, reward) => {} // query.earnGold.run(reward, guid)
const earnGoldUser = (userId, reward) => {} // query.earnGoldUser.run(reward, userId)
const spendGold = (guid, cost) => {} // query.spendGold.run(cost, guid)
const findUserShipsScrollDown = (userId, lastUserId, count) => {
  return queryShip.findUserShipsScrollDown.all(userId, lastUserId, count)
}
const findUserShipsScrollUp = (userId, firstUserId, count) => {
  return queryShip.findUserShipsScrollUp.all(userId, firstUserId, count)
}
const findMission = missionId => queryMission.findMission.get(missionId)
const findMissions = () => {
  const result = queryMission.findMissions.all()
  const rows = []
  let row = []
  let index = 0
  for (let each of result) {
    row.push(each)
    if (++index % 2 === 0) {
      rows.push(row)
      row = []
    }
  }
  if (row.length > 0) {
    rows.push(row)
  }
  console.log(rows)
  return rows
}

const findPort = portId => querySeaport.findPort.get(portId)
const findShipyard = shipyardId => queryShipyard.findShipyard.get(shipyardId)
const findPortsScrollDown = (userId, lastRegionId, count) => {
  return querySeaport.findPortsScrollDown.all(lastRegionId, count)
}
const findPortsScrollUp = (userId, lastRegionId, count) => {
  return querySeaport.findPortsScrollUp.all(lastRegionId, count)
}
const deletePort = portId => {
  querySeaport.deletePort.run(portId)
}
const deleteShipyard = shipyardId => {
  queryShipyard.deleteShipyard.run(shipyardId)
  queryShip.deleteShipDockedAtShipyard.run(shipyardId)
}
const insertCaptain = (userId, name, captainTemplateId) => {
  const captain = queryCaptain.insertCaptain.run(
    userId,
    name,
    captainTemplateId
  )
  return captain.lastInsertROWID
}
const findCaptain = captainId => queryCaptain.findCaptain.get(captainId)
const deleteCaptain = captainId => queryCaptain.deleteCaptain.run(captainId)

module.exports = {
  createUser,
  createPort,
  createShipyard,
  createShip,
  deleteShip,
  createShiproute,
  deleteShiproute,
  setShipShiproute,
  setShipDockedShipyardId,
  setShipCaptainId,
  setShipTemplateId,
  findShipShiproute,
  listShipShiproute,
  listShipShiprouteToArray,
  listPort,
  listPortToArray,
  listShipyard,
  listShipyardToArray,
  listShipDockedAtShipyard,
  listShipDockedAtShipyardToArray,
  findShip,
  findUser,
  findUserGuid,
  earnGold,
  earnGoldUser,
  spendGold,
  findUserShipsScrollDown,
  findUserShipsScrollUp,
  findMission,
  findMissions,
  findPort,
  findShipyard,
  findPortsScrollDown,
  findPortsScrollUp,
  deletePort,
  deleteShipyard,
  insertCaptain,
  findCaptain,
  deleteCaptain
}
