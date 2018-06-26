const argv = require('yargs').argv
const init = require('./init')

if (argv.init) {
  init.initialize()
  console.log(`Initialized.`)
  process.exit(0)
}

const express = require('express')
const raname = require('random-name')
const uuidv1 = require('uuid/v1')
const moment = require('moment')
const numeral = require('numeral')
const query = require('./query')
const dgram = require('dgram')
const seaUdpClient = dgram.createSocket('udp4')
const message = require('./message')
const url = require('url')
const app = express()
app.locals.moment = moment
app.locals.numeral = numeral

app.use(express.static('./src/html'))
app.set('views', './src/views')
app.set('view engine', 'pug')

const userCache = {}

const createUser = guid => {
  const userName = `${raname.first()} ${raname.last()}`
  const user = query.insertUser.run(guid, userName)
  // no initial ship
  // const shipName = `${raname.middle()} ${raname.middle()}`
  // query.insertShip.run(user.lastInsertROWID, shipName)
  return user.lastInsertROWID
}
const createPort = (portName, x, y, userId, expectLand) => {
  const port = query.insertPort.run(portName, x, y, userId, expectLand)
  return port.lastInsertROWID
}
const createShipyard = (shipyardName, x, y, userId) => {
  const shipyard = query.insertShipyard.run(shipyardName, x, y, userId)
  return shipyard.lastInsertROWID
}
const createShip = (userId, shipName, shipType) => {
  const ship = query.insertShip.run(userId, shipName, shipType)
  return ship.lastInsertROWID
}
const deleteShip = shipId => {
  return query.deleteShip.run(shipId).changes
}
const createShiproute = (port1Id, port2Id) => {
  const shiproute = query.insertShiproute.run(port1Id, port2Id)
  return shiproute.lastInsertROWID
}
const deleteShiproute = shiprouteId => {
  return query.deleteShiproute.run(shiprouteId).changes
}
const setShipShiproute = (shipId, shiprouteId) => {
  query.setShipShiproute.run(shiprouteId, shipId)
}
const setShipDockedShipyardId = (shipId, dockedShipyardId) => {
  return query.setShipDockedShipyardId.run(dockedShipyardId, shipId).changes
}
const findShipShiproute = shipId => {
  return query.findShipShiproute.get(shipId)
}
const listShipShiproute = onRow => {
  for (const row of query.listShipShiproute.iterate()) {
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
  for (const row of query.listPort.iterate()) {
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
  for (const row of query.listShipyard.iterate()) {
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
  for (const row of query.listShipDockedAtShipyard.iterate(shipyardId)) {
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
const findShip = shipId => query.findShip.get(shipId)
const findUser = guid => query.findUser.get(guid)
const findUserGuid = userId => query.findUserGuid.get(userId)
const earnGold = (guid, reward) => {} // query.earnGold.run(reward, guid)
const earnGoldUser = (userId, reward) => {} // query.earnGoldUser.run(reward, userId)
const spendGold = (guid, cost) => {} // query.spendGold.run(cost, guid)
const findOrCreateUser = guid => {
  if (guid in userCache) {
    return userCache[guid]
  }
  const userInDb = findUser(guid)
  // console.log(userInDb)
  if (userInDb !== undefined) {
    userCache[guid] = userInDb
    return userInDb
  }
  createUser(guid)
  return findOrCreateUser(guid)
}
const findUserShipsScrollDown = (userId, lastUserId, count) => {
  return query.findUserShipsScrollDown.all(userId, lastUserId, count)
}
const findUserShipsScrollUp = (userId, firstUserId, count) => {
  return query.findUserShipsScrollUp.all(userId, firstUserId, count)
}
const findMission = missionId => query.findMission.get(missionId)
const findMissions = () => {
  const result = query.findMissions.all()
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

const findPort = portId => query.findPort.get(portId)
const findShipyard = shipyardId => query.findShipyard.get(shipyardId)
const findPortsScrollDown = (userId, lastRegionId, count) => {
  return query.findPortsScrollDown.all(lastRegionId, count)
}
const findPortsScrollUp = (userId, lastRegionId, count) => {
  return query.findPortsScrollUp.all(lastRegionId, count)
}
const deletePort = portId => {
  query.deletePort.run(portId)
}
const deleteShipyard = shipyardId => {
  query.deleteShipyard.run(shipyardId)
  query.deleteShipDockedAtShipyard.run(shipyardId)
}
const travelTo = (id, x, y) => {
  const buf = message.TeleportToStruct.buffer()
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0
  }
  message.TeleportToStruct.fields.type = 2
  message.TeleportToStruct.fields.id = id
  message.TeleportToStruct.fields.x = x
  message.TeleportToStruct.fields.y = y
  seaUdpClient.send(Buffer.from(buf), 4000, 'localhost', err => {
    if (err) {
      console.error('sea udp client error:', err)
    }
  })
}

const teleportTo = (id, x, y) => {
  const buf = message.TeleportToStruct.buffer()
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0
  }
  message.TeleportToStruct.fields.type = 3
  message.TeleportToStruct.fields.id = id
  message.TeleportToStruct.fields.x = x
  message.TeleportToStruct.fields.y = y
  seaUdpClient.send(Buffer.from(buf), 4000, 'localhost', err => {
    if (err) {
      console.error('sea udp client error:', err)
    }
  })
}

app.get('/idle', (req, res) => {
  const u = findOrCreateUser(req.get('X-U') || req.query.u || uuidv1())
  return res.render('idle', {
    user: u,
    resultMsg: req.query.resultMsg,
    errMsg: req.query.errMsg,
    menuToggle: req.query.mt === 'false'
  })
})

app.get('/loan', (req, res) => {
  const u = findOrCreateUser(req.query.u || uuidv1())
  return res.render('loan', { user: u })
})

app.get('/sellShip', (req, res) => {
  // const u = findOrCreateUser(req.query.u || uuidv1())
  let resultMsg, errMsg
  const ship = findShip(req.query.shipId)
  if (ship) {
    if (ship.shiproute_id) {
      deleteShiproute(ship.shiproute_id)
    }
    const deletedRowCount = deleteShip(ship.ship_id)
    if (deletedRowCount === 1) {
      const buf = message.DeleteShipStruct.buffer()
      for (let i = 0; i < buf.length; i++) {
        buf[i] = 0
      }
      message.DeleteShipStruct.fields.type = 5 // Delete Ship
      message.DeleteShipStruct.fields.shipId = req.query.shipId
      seaUdpClient.send(Buffer.from(buf), 4000, 'localhost', err => {
        if (err) {
          console.error('sea udp client error:', err)
        }
      })
      resultMsg = '선박 판매 성공'
    } else if (deletedRowCount === 0) {
      resultMsg = '선박 판매 성공?!'
    } else {
      errMsg = '선박 판매 실패'
    }
  }
  // res.redirect(
  //   url.format({
  //     pathname: '/idle',
  //     query: {
  //       resultMsg: resultMsg,
  //       errMsg: errMsg
  //     }
  //   })
  // )
  const qs = url.format({
    pathname: '',
    query: {
      resultMsg: resultMsg,
      errMsg: errMsg
    }
  })
  res.redirect(`script:ttl_go_back('${qs}')`)
})

app.get('/sell_vessel', (req, res) => {
  const u = findOrCreateUser(req.query.u || uuidv1())
  if (req.query.s) {
    deleteShip(req.query.s)
    const buf = message.DeleteShipStruct.buffer()
    for (let i = 0; i < buf.length; i++) {
      buf[i] = 0
    }
    message.DeleteShipStruct.fields.type = 5 // Delete Ship
    message.DeleteShipStruct.fields.shipId = req.query.s
    seaUdpClient.send(Buffer.from(buf), 4000, 'localhost', err => {
      if (err) {
        console.error('sea udp client error:', err)
      }
    })
  }
  res.redirect(
    url.format({
      pathname: '/vessel',
      query: {
        u: u.guid,
        currentFirstKey: req.query.currentFirstKey
      }
    })
  )
})

app.get('/vessel', (req, res) => {
  const u = findOrCreateUser(req.query.u || uuidv1())
  const limit = 5
  let s
  if (req.query.firstKey) {
    // user pressed 'next' page button
    s = findUserShipsScrollDown(u.user_id, req.query.firstKey, limit)
    if (s.length === 0) {
      // no next elements available. stay on the current page
      s = findUserShipsScrollDown(
        u.user_id,
        req.query.currentFirstKey - 1,
        limit
      )
    }
  } else if (req.query.lastKey) {
    // user pressed 'prev' page button
    s = findUserShipsScrollUp(u.user_id, req.query.lastKey, limit)
    s.reverse()
    if (s.length === 0) {
      // no prev elements available. stay on the current page
      s = findUserShipsScrollDown(
        u.user_id,
        req.query.currentFirstKey - 1,
        limit
      )
    }
  } else if (
    req.query.currentFirstKey &&
    req.query.currentFirstKey !== 'undefined'
  ) {
    // refresh current page
    s = findUserShipsScrollDown(u.user_id, req.query.currentFirstKey - 1, limit)
    if (s.length === 0) {
      // the only element in this page is removed
      // go to previous page
      s = findUserShipsScrollUp(u.user_id, req.query.currentFirstKey, limit)
      s.reverse()
    }
  } else {
    // default: fetch first page
    s = findUserShipsScrollDown(u.user_id, 0, limit)
  }
  const firstKey = s.length > 0 ? s[0].ship_id : undefined
  const lastKey = s.length > 0 ? s[s.length - 1].ship_id : undefined
  return res.render('vessel', {
    user: u,
    rows: s,
    firstKey: firstKey,
    lastKey: lastKey
  })
})

app.get('/mission', (req, res) => {
  const u = findOrCreateUser(req.query.u || uuidv1())
  const m = findMissions()
  return res.render('mission', { user: u, rows: m })
})

app.get('/start', (req, res) => {
  const u = findOrCreateUser(req.query.u || uuidv1())
  const m = findMission(req.query.mission || 1)
  return res.render('start', { user: u, mission: m })
})

app.get('/success', (req, res) => {
  const u = findOrCreateUser(req.query.u || uuidv1())
  const m = findMission(req.query.mission || 1)
  earnGold(u.guid, m.reward)
  delete userCache[u.guid]
  return res.render('success', { user: u, mission: m })
})

app.get('/port', (req, res) => {
  const u = findOrCreateUser(req.query.u || uuidv1())
  const limit = 5
  let p
  if (req.query.firstKey) {
    // user pressed 'next' page button
    p = findPortsScrollDown(u.user_id, req.query.firstKey, limit)
    if (p.length === 0) {
      // no next elements available. stay on the current page
      p = findPortsScrollDown(u.user_id, req.query.currentFirstKey - 1, limit)
    }
  } else if (req.query.lastKey) {
    // user pressed 'prev' page button
    p = findPortsScrollUp(u.user_id, req.query.lastKey, limit)
    p.reverse()
    if (p.length === 0) {
      // no prev elements available. stay on the current page
      p = findPortsScrollDown(u.user_id, req.query.currentFirstKey - 1, limit)
    }
  } else if (
    req.query.currentFirstKey &&
    req.query.currentFirstKey !== 'undefined'
  ) {
    // refresh current page
    p = findPortsScrollDown(u.user_id, req.query.currentFirstKey - 1, limit)
    if (p.length === 0) {
      // the only element in this page is removed
      // go to previous page
      p = findPortsScrollUp(u.user_id, req.query.currentFirstKey, limit)
      p.reverse()
    }
  } else {
    // default: fetch first page
    p = findPortsScrollDown(u.user_id, 0, limit)
  }
  const firstKey = p.length > 0 ? p[0].region_id : undefined
  const lastKey = p.length > 0 ? p[p.length - 1].region_id : undefined

  return res.render('port', {
    user: u,
    rows: p,
    firstKey: firstKey,
    lastKey: lastKey,
    errMsg: req.query.errMsg
  })
})

app.get('/traveltoport', (req, res) => {
  const u = findOrCreateUser(req.query.u || uuidv1())
  const p = findPort(req.query.region || 1)
  console.log('travel to port', p.region_id, p.x, p.y)
  travelTo(u.guid, p.x, p.y)
  return res.render('idle', { user: u })
})

app.get('/teleporttoport', (req, res) => {
  const u = findOrCreateUser(req.query.u || uuidv1())
  const p = findPort(req.query.region || 1)
  console.log('teleport to port', p.region_id, p.x, p.y)
  teleportTo(u.guid, p.x, p.y)
  return res.render('idle', { user: u })
})

const purchaseNewPort = async (req, res, expectLand) => {
  const u = findOrCreateUser(req.get('X-U') || uuidv1())
  const xc0 = req.get('X-D-XC0')
  const yc0 = req.get('X-D-YC0')
  console.log(`purchaseNewPort at [${xc0}, ${yc0}]`)
  let resultMsg = ''
  let errMsg = ''
  const r0 = await execCreatePort(u, xc0, yc0, expectLand)
  if (r0.seaportId > 0 && r0.err === null) {
    resultMsg = '새 항구 건설 완료'
  } else {
    errMsg = '새 항구 건설 실패'
  }
  res.redirect(
    url.format({
      pathname: '/idle',
      query: {
        u: u.guid,
        resultMsg: resultMsg,
        errMsg: errMsg
      }
    })
  )
}

const purchaseNewShipyard = async (req, res) => {
  const u = findOrCreateUser(req.get('X-U') || uuidv1())
  const xc0 = req.get('X-D-XC0')
  const yc0 = req.get('X-D-YC0')
  console.log(`purchaseNewShipyard at [${xc0}, ${yc0}]`)
  let resultMsg = ''
  let errMsg = ''
  const r0 = await execCreateShipyard(u, xc0, yc0)
  if (r0.shipyardId > 0 && r0.err === null) {
    resultMsg = '새 조선소 건설 완료'
  } else {
    errMsg = '새 조선소 건설 실패'
  }
  res.redirect(
    url.format({
      pathname: '/idle',
      query: {
        u: u.guid,
        resultMsg: resultMsg,
        errMsg: errMsg
      }
    })
  )
}

app.get('/purchaseNewPort', async (req, res) => {
  await purchaseNewPort(req, res, 0)
})

app.get('/demolishPort', (req, res) => {
  // const u = findOrCreateUser(req.get('X-U') || req.query.u || uuidv1())
  if (req.query.portId) {
    const port = findPort(req.query.portId)
    if (port) {
      deletePort(req.query.portId)
      const buf = message.DeletePortStruct.buffer()
      for (let i = 0; i < buf.length; i++) {
        buf[i] = 0
      }
      message.DeletePortStruct.fields.type = 8 // Delete Port
      message.DeletePortStruct.fields.portId = port.region_id
      seaUdpClient.send(Buffer.from(buf), 4000, 'localhost', err => {
        if (err) {
          console.error('sea udp client error:', err)
        }
      })
    }
  }
  res.redirect(
    url.format({
      pathname: '/idle'
    })
  )
})

app.get('/purchaseNewShipyard', async (req, res) => {
  await purchaseNewShipyard(req, res, 0)
})

app.get('/demolishShipyard', (req, res) => {
  const u = findOrCreateUser(req.query.u || uuidv1())
  let resultMsg = ''
  let errMsg = ''
  if (req.query.shipyardId) {
    const shipyard = findShipyard(req.query.shipyardId)
    if (shipyard) {
      deleteShipyard(req.query.shipyardId)
      const buf = message.DeleteShipyardStruct.buffer()
      for (let i = 0; i < buf.length; i++) {
        buf[i] = 0
      }
      message.DeleteShipyardStruct.fields.type = 10 // Delete Shipyard
      message.DeleteShipyardStruct.fields.shipyardId = shipyard.shipyard_id
      seaUdpClient.send(Buffer.from(buf), 4000, 'localhost', err => {
        if (err) {
          console.error('sea udp client error:', err)
        }
      })
      resultMsg = '조선소 철거 성공'
    } else {
      errMsg = '존재하지 않는 조선소'
    }
  }
  res.redirect(
    url.format({
      pathname: '/idle',
      query: {
        u: u.guid,
        resultMsg: resultMsg,
        errMsg: errMsg
      }
    })
  )
})

const sendSpawnShip = async (
  expectedDbId,
  x,
  y,
  port1Id,
  port2Id,
  expectLand
) => {
  const buf = message.SpawnShipStruct.buffer()
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0
  }
  const replyId = issueNewReplyId()
  message.SpawnShipStruct.fields.type = 4
  message.SpawnShipStruct.fields.expectedDbId = expectedDbId
  message.SpawnShipStruct.fields.x = x
  message.SpawnShipStruct.fields.y = y
  message.SpawnShipStruct.fields.port1Id = port1Id
  message.SpawnShipStruct.fields.port2Id = port2Id
  message.SpawnShipStruct.fields.replyId = replyId
  message.SpawnShipStruct.fields.expectLand = expectLand
  return sendAndReplyFromSea(buf, replyId, err => {
    if (err) {
      console.error('sea udp SpawnShipStruct client error:', err)
    }
  })
}

let replyId = 0
const issueNewReplyId = () => {
  return ++replyId
}
let replyWaiters = {}
const sendAndReplyFromSea = async (buf, replyId, onSendErr) => {
  return new Promise((resolve, reject) => {
    if (replyWaiters[replyId]) {
      console.error('Previous reply waiter exist?!')
    }
    replyWaiters[replyId] = (packet, err) => {
      if (err) {
        reject(err)
      } else {
        resolve(packet)
      }
    }
    seaUdpClient.send(Buffer.from(buf), 4000, 'localhost', onSendErr)
  })
}
const notifyWaiter = packetStruct => {
  const waiter = replyWaiters[packetStruct.fields.replyId]
  if (waiter) {
    waiter(packetStruct.fields, null)
    delete replyWaiters[packetStruct.fields.replyId]
  }
}

const sendSpawnPort = async (expectedDbId, name, x, y, ownerId, expectLand) => {
  const buf = message.SpawnPortStruct.buffer()
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0
  }
  const replyId = issueNewReplyId()
  message.SpawnPortStruct.fields.type = 6
  message.SpawnPortStruct.fields.expectedDbId = expectedDbId
  message.SpawnPortStruct.fields.name = name
  message.SpawnPortStruct.fields.x = x
  message.SpawnPortStruct.fields.y = y
  message.SpawnPortStruct.fields.ownerId = ownerId
  message.SpawnPortStruct.fields.replyId = replyId
  message.SpawnPortStruct.fields.expectLand = expectLand
  return sendAndReplyFromSea(buf, replyId, err => {
    if (err) {
      console.error('sea udp SpawnShipStruct client error:', err)
    }
  })
}

const sendSpawnShipyard = async (expectedDbId, name, x, y, ownerId) => {
  const buf = message.SpawnShipyardStruct.buffer()
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0
  }
  const replyId = issueNewReplyId()
  message.SpawnShipyardStruct.fields.type = 9
  message.SpawnShipyardStruct.fields.expectedDbId = expectedDbId
  message.SpawnShipyardStruct.fields.name = name
  message.SpawnShipyardStruct.fields.x = x
  message.SpawnShipyardStruct.fields.y = y
  message.SpawnShipyardStruct.fields.ownerId = ownerId
  message.SpawnShipyardStruct.fields.replyId = replyId
  return sendAndReplyFromSea(buf, replyId, err => {
    if (err) {
      console.error('sea udp SpawnShipyardStruct client error:', err)
    }
  })
}

const execCreatePort = async (u, selectedLng, selectedLat, expectLand) => {
  const portName = `Port ${raname.first()}`
  // create db entry first
  const regionId = createPort(
    portName,
    selectedLng,
    selectedLat,
    u.user_id,
    expectLand
  )
  // send spawn command to sea-server
  const reply = await sendSpawnPort(
    regionId,
    portName,
    selectedLng,
    selectedLat,
    u.user_id,
    expectLand
  )
  if (reply.dbId === regionId) {
    if (reply.existing === 0) {
      // successfully created
      spendGold(u.guid, 10000)
      return {
        seaportId: regionId,
        err: null
      }
    } else {
      console.error(
        'Port with the same ID already exists on sea-server (is this possible?!)'
      )
      return {
        seaportId: regionId,
        err: 'db inconsistent'
      }
    }
  }
  // something went wrong; delete from db
  deletePort(regionId)
  // print useful information
  if (reply.dbId > 0 && reply.dbId !== regionId) {
    console.log(
      `port cannot be created: port ID ${
        reply.dbId
      } already exists on that location`
    )
    return {
      seaportId: reply.dbId,
      err: 'already exists'
    }
  } else if (reply.tooClose) {
    console.error('port cannot be created: too close')
    return {
      err: 'too close'
    }
  } else {
    console.error('port cannot be created: unknown case')
    return {
      err: 'unknown error'
    }
  }
}

const execCreateShipyard = async (u, selectedLng, selectedLat) => {
  const shipyardName = `Shipyard ${raname.first()}`
  // create db entry first
  const shipyardId = createShipyard(
    shipyardName,
    selectedLng,
    selectedLat,
    u.user_id
  )
  // send spawn command to sea-server
  const reply = await sendSpawnShipyard(
    shipyardId,
    shipyardName,
    selectedLng,
    selectedLat,
    u.user_id
  )
  if (reply.dbId === shipyardId) {
    if (reply.existing === 0) {
      // successfully created
      spendGold(u.guid, 10000)
      return {
        shipyardId: shipyardId,
        err: null
      }
    } else {
      console.error(
        'Shipyard with the same ID already exists on sea-server (is this possible?!)'
      )
      return {
        shipyardId: shipyardId,
        err: 'db inconsistent'
      }
    }
  }
  // something went wrong; delete from db
  deleteShipyard(shipyardId)
  // print useful information
  if (reply.dbId > 0 && reply.dbId !== shipyardId) {
    console.log(
      `shipyard cannot be created: shipyard ID ${
        reply.dbId
      } already exists on that location`
    )
    return {
      seaportId: reply.dbId,
      err: 'already exists'
    }
  } else if (reply.tooClose) {
    console.error('shipyard cannot be created: too close')
    return {
      err: 'too close'
    }
  } else {
    console.error('shipyard cannot be created: unknown case')
    return {
      err: 'unknown error'
    }
  }
}

const execCreateShipWithRoute = async (
  userId,
  xc0,
  yc0,
  expectLand,
  r0,
  r1
) => {
  const shipName = `${raname.middle()} ${raname.middle()}`
  const dbId = createShip(userId, shipName, expectLand)
  const p0 = findPort(r0)
  const p1 = findPort(r1)
  if (p0 && p1) {
    const reply = await sendSpawnShip(
      dbId,
      xc0,
      yc0,
      p0.region_id,
      p1.region_id,
      expectLand
    )
    if (reply.dbId === dbId) {
      return dbId
    } else {
      console.error(`Spawn ship request id and result id mismatch`)
      deleteShip(dbId)
    }
  } else {
    console.error(`Ports cannot be found - ${p0}, ${p1}`)
    deleteShip(dbId)
  }
  return 0
}

app.get('/sell_port', (req, res) => {
  const u = findOrCreateUser(req.query.u || uuidv1())
  if (req.query.r) {
    const port = findPort(req.query.r)
    if (port) {
      deletePort(req.query.r)
      const buf = message.DeletePortStruct.buffer()
      for (let i = 0; i < buf.length; i++) {
        buf[i] = 0
      }
      message.DeletePortStruct.fields.type = 8 // Delete Port
      message.DeletePortStruct.fields.portId = port.region_id
      seaUdpClient.send(Buffer.from(buf), 4000, 'localhost', err => {
        if (err) {
          console.error('sea udp client error:', err)
        }
      })
    }
  }
  res.redirect(
    url.format({
      pathname: '/port',
      query: {
        u: u.guid,
        currentFirstKey: req.query.currentFirstKey
      }
    })
  )
})

const link = async (req, res, expectLand) => {
  const u = findOrCreateUser(req.get('X-U') || uuidv1())
  const xc0 = req.get('X-D-XC0')
  const yc0 = req.get('X-D-YC0')
  const xc1 = req.get('X-D-XC1')
  const yc1 = req.get('X-D-YC1')
  console.log(`Link [${xc0}, ${yc0}]-[${xc1}, ${yc1}]`)
  let resultMsg = ''
  let errMsg = ''
  const r0 = await execCreatePort(u, xc0, yc0, expectLand)
  if (r0.seaportId > 0) {
    const r1 = await execCreatePort(u, xc1, yc1, expectLand)
    if (r1.seaportId > 0) {
      const shipDbId = await execCreateShipWithRoute(
        u.user_id,
        xc0,
        yc0,
        expectLand,
        r0.seaportId,
        r1.seaportId
      )
      if (shipDbId > 0) {
        resultMsg = '새 항로 등록 성공'
      } else {
        errMsg = '항로 등록 실패'
      }
    } else {
      errMsg = '도착 항구 건설 실패'
    }
  } else {
    errMsg = '출발 항구 건설 실패'
  }
  res.redirect(
    url.format({
      pathname: '/idle',
      query: {
        u: u.guid,
        resultMsg: resultMsg,
        errMsg: errMsg
      }
    })
  )
}

app.get('/link', async (req, res) => {
  await link(req, res, 0)
})

app.get('/linkland', async (req, res) => {
  await link(req, res, 1)
})

app.get('/openShipyard', (req, res) => {
  const u = findOrCreateUser(req.get('X-U') || req.query.u || uuidv1())
  const shipyard = findShipyard(req.query.shipyardId)
  const dockedShips = listShipDockedAtShipyardToArray(req.query.shipyardId)
  if (shipyard) {
    return res.render('openShipyard', {
      user: u,
      shipyard: shipyard,
      dockedShips: dockedShips,
      resultMsg: req.query.resultMsg,
      errMsg: req.query.errMsg
    })
  } else {
    res.redirect(
      url.format({
        pathname: '/idle',
        query: {
          errMsg: '조선소를 찾을 수 없습니다'
        }
      })
    )
  }
})

app.get('/openShip', (req, res) => {
  const u = findOrCreateUser(req.get('X-U') || req.query.u || uuidv1())
  const ship = findShip(req.query.shipId)
  if (ship) {
    let seaport1, seaport2
    const dockedShipyard = ship.docked_shipyard_id
      ? findShipyard(ship.docked_shipyard_id)
      : null
    if (ship.shiproute_id) {
      const shiproute = findShipShiproute(ship.ship_id)
      seaport1 = findPort(shiproute.port1_id)
      seaport2 = findPort(shiproute.port2_id)
    } else {
      seaport1 = findPort(req.query.seaport1Id)
      seaport2 = findPort(req.query.seaport2Id)
    }
    return res.render('openShip', {
      user: u,
      ship: ship,
      dockedShipyard: dockedShipyard,
      seaport1: seaport1,
      seaport2: seaport2,
      resultMsg: req.query.resultMsg,
      errMsg: req.query.errMsg
    })
  } else {
    res.redirect(
      url.format({
        pathname: '/idle',
        query: {
          errMsg: '선박을 찾을 수 없습니다'
        }
      })
    )
  }
})

app.get('/purchaseShipAtShipyard', (req, res) => {
  const u = findOrCreateUser(req.get('X-U') || req.query.u || uuidv1())
  const shipyard = findShipyard(req.query.shipyardId)
  let resultMsg, errMsg
  if (shipyard) {
    const dockedShips = listShipDockedAtShipyardToArray(shipyard.shipyard_id)
    if (dockedShips.length < 4) {
      const shipName = `${raname.middle()} ${raname.middle()}`
      const shipId = createShip(u.user_id, shipName, 0)
      setShipDockedShipyardId(shipId, req.query.shipyardId)
      resultMsg = '새 선박 구입 성공'
    } else {
      errMsg = '정박중 선박 초과'
    }
  } else {
    errMsg = '새 선박 구입 실패'
  }
  res.redirect(
    url.format({
      pathname: '/openShipyard',
      query: {
        shipyardId: shipyard.shipyard_id,
        resultMsg: resultMsg,
        errMsg: errMsg
      }
    })
  )
})

app.get('/confirmNewRoute', (req, res) => {
  let resultMsg, errMsg
  const ship = findShip(req.query.shipId)
  while (1) {
    if (ship) {
      const seaport1 = findPort(req.query.seaport1Id)
      if (seaport1) {
        const seaport2 = findPort(req.query.seaport2Id)
        if (seaport2) {
          if (req.query.seaport1Id === req.query.seaport2Id) {
            errMsg = '중복 항구 오류'
            break
          }
          const shiprouteId = createShiproute(
            seaport1.region_id,
            seaport2.region_id
          )
          setShipShiproute(ship.ship_id, shiprouteId)
          resultMsg = '항로 확정 성공'
          break
        } else {
          errMsg = '항로 확정 실패 - 항구2오류'
          break
        }
      } else {
        errMsg = '항로 확정 실패 - 항구1오류'
        break
      }
    } else {
      errMsg = '항로 확정 실패 - 선박오류'
      break
    }
    // break
  }
  const qs = url.format({
    pathname: '',
    query: {
      resultMsg: resultMsg,
      errMsg: errMsg
    }
  })
  res.redirect(`script:ttl_refresh('${qs}')`)
})

app.get('/deleteRoute', (req, res) => {
  let resultMsg, errMsg
  const ship = findShip(req.query.shipId)
  if (ship && ship.shiproute_id) {
    setShipShiproute(ship.ship_id, null)
    resultMsg = '항로 초기화 성공'
    const affectedRows = deleteShiproute(ship.shiproute_id)
    if (affectedRows !== 1) {
      errMsg = '항로 데이터 경고'
    }
  } else {
    errMsg = '항로 초기화 실패'
  }
  const qs = url.format({
    pathname: '',
    query: {
      resultMsg: resultMsg,
      errMsg: errMsg
    }
  })
  res.redirect(`script:ttl_refresh('${qs}')`)
})

app.get('/startRoute', async (req, res) => {
  let resultMsg, errMsg
  const shiproute = findShipShiproute(req.query.shipId)
  if (shiproute) {
    if (shiproute.shiproute_id > 0) {
      if (shiproute.docked_shipyard_id > 0) {
        const affectedRows = setShipDockedShipyardId(shiproute.ship_id, null)
        if (affectedRows !== 1) {
          errMsg = '항로 데이터 경고'
        }
        resultMsg = '운항 시작 성공'
        await sendSpawnShip(
          shiproute.ship_id,
          0,
          0,
          shiproute.port1_id,
          shiproute.port2_id,
          shiproute.ship_type
        )
      } else {
        errMsg = '정박중인 선박 아님'
      }
    } else {
      errMsg = '항로 미확정 선박'
    }
  } else {
    errMsg = '선박 찾기 실패'
  }
  const qs = url.format({
    pathname: '',
    query: {
      resultMsg: resultMsg,
      errMsg: errMsg
    }
  })
  res.redirect(`script:ttl_refresh('${qs}')`)
})

seaUdpClient.on('message', async (buf, remote) => {
  if (buf[0] === 1) {
    // SpawnShipReply
    console.log(
      `SpawnShipReply from ${remote.address}:${remote.port} (len=${buf.length})`
    )
    message.SpawnShipReplyStruct._setBuff(buf)
    notifyWaiter(message.SpawnShipReplyStruct)
    // console.log('UDP type: ' + message.SpawnShipReplyStruct.fields.type)
    // console.log('UDP ship_id: ' + message.SpawnShipReplyStruct.fields.shipId)
    // console.log('UDP port1_id: ' + message.SpawnShipReplyStruct.fields.port1Id)
    // console.log('UDP port2_id: ' + message.SpawnShipReplyStruct.fields.port2Id)
    const shiprouteId = createShiproute(
      message.SpawnShipReplyStruct.fields.port1Id,
      message.SpawnShipReplyStruct.fields.port2Id
    )
    setShipShiproute(message.SpawnShipReplyStruct.fields.dbId, shiprouteId)
  } else if (buf[0] === 2) {
    // RecoverAllShips
    console.log(
      `RecoverAllShips from ${remote.address}:${remote.port} (len=${
        buf.length
      })`
    )
    console.log('A new sea-server instance requested recovering.')
    console.log('Recovering in progress...')
    // recovering ports
    let portCount = 0
    const ports = await listPortToArray()
    for (let i = 0; i < ports.length; i++) {
      const row = ports[i]
      await sendSpawnPort(
        row.region_id,
        row.name,
        row.x,
        row.y,
        row.owner_id,
        row.region_type
      )
      portCount++
    }
    console.log(`  ${portCount} port(s) recovered...`)
    // recovering shipyards
    let shipyardCount = 0
    const shipyards = await listShipyardToArray()
    for (let i = 0; i < shipyards.length; i++) {
      const row = shipyards[i]
      await sendSpawnShipyard(
        row.shipyard_id,
        row.name,
        row.x,
        row.y,
        row.owner_id
      )
      shipyardCount++
    }
    console.log(`  ${shipyardCount} shipyard(s) recovered...`)
    // recovering ships
    let shipShiprouteCount = 0
    let shipDockedCount = 0
    const ships = listShipShiprouteToArray()
    for (let i = 0; i < ships.length; i++) {
      const row = ships[i]
      if (!row.docked_shipyard_id) {
        await sendSpawnShip(
          row.ship_id,
          0,
          0,
          row.port1_id,
          row.port2_id,
          row.ship_type
        )
        shipShiprouteCount++
      } else {
        shipDockedCount++
      }
    }
    console.log(
      `  ${shipShiprouteCount} ship(s) recovered... (${shipDockedCount} docked ships excluded)`
    )
    console.log(`Recovering Done.`)
  } else if (buf[0] === 3) {
    // Arrival
    message.ArrivalStruct._setBuff(buf)
    // console.log(
    //   `Arrival ship id ${message.ArrivalStruct.fields.shipId} from ${
    //     remote.address
    //   }:${remote.port} (len=${buf.length})`
    // )
    const ship = findShip(message.ArrivalStruct.fields.shipId)
    if (ship) {
      earnGoldUser(ship.user_id, 1)
      const guid = findUserGuid(ship.user_id).guid
      delete userCache[guid]
    } else {
      console.error(
        `Could not find ship with id ${message.ArrivalStruct.fields.shipId}!`
      )
    }
  } else if (buf[0] === 4) {
    // SpawnPortReply
    console.log(
      `SpawnPortReply from ${remote.address}:${remote.port} (len=${buf.length})`
    )
    message.SpawnPortReplyStruct._setBuff(buf)
    notifyWaiter(message.SpawnPortReplyStruct)
  } else if (buf[0] === 5) {
    // SpawnShipyardReply
    console.log(
      `SpawnShipyardReply from ${remote.address}:${remote.port} (len=${
        buf.length
      })`
    )
    message.SpawnShipyardReplyStruct._setBuff(buf)
    notifyWaiter(message.SpawnShipyardReplyStruct)
  }
})

seaUdpClient.on('listening', () => {
  const address = seaUdpClient.address()
  console.log(`UDP server listening ${address.address}:${address.port}!`)
})

const udpPort = argv.udpport || 3003
seaUdpClient.bind(udpPort)

const port = argv.port || 3000
app.listen(port, () => {
  console.log(`TCP server listening on ${port} port!`)
})
