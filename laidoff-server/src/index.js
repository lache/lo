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
const dgram = require('dgram')
const seaUdpClient = dgram.createSocket('udp4')
const message = require('./message')
const db = require('./db')
const url = require('url')
const app = express()
app.locals.moment = moment
app.locals.numeral = numeral

app.use(express.static('./src/html'))
app.set('views', './src/views')
app.set('view engine', 'pug')

const userCache = {}
const findOrCreateUser = guid => {
  if (guid in userCache) {
    return userCache[guid]
  }
  const userInDb = db.findUser(guid)
  // console.log(userInDb)
  if (userInDb !== undefined) {
    userCache[guid] = userInDb
    return userInDb
  }
  db.createUser(guid)
  return findOrCreateUser(guid)
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
  const ship = db.findShip(req.query.shipId)
  if (ship) {
    if (ship.shiproute_id) {
      db.deleteShiproute(ship.shiproute_id)
    }
    const deletedRowCount = db.deleteShip(ship.ship_id)
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

const requestDespawnShipToSeaServer = shipId => {
  const buf = message.DeleteShipStruct.buffer()
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0
  }
  message.DeleteShipStruct.fields.type = 5 // Delete Ship
  message.DeleteShipStruct.fields.shipId = shipId
  seaUdpClient.send(Buffer.from(buf), 4000, 'localhost', err => {
    if (err) {
      console.error('sea udp client error:', err)
    }
  })
}

app.get('/sell_vessel', (req, res) => {
  const u = findOrCreateUser(req.query.u || uuidv1())
  if (req.query.s) {
    db.deleteShip(req.query.s)
    requestDespawnShipToSeaServer(req.query.s)
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
    s = db.findUserShipsScrollDown(u.user_id, req.query.firstKey, limit)
    if (s.length === 0) {
      // no next elements available. stay on the current page
      s = db.findUserShipsScrollDown(
        u.user_id,
        req.query.currentFirstKey - 1,
        limit
      )
    }
  } else if (req.query.lastKey) {
    // user pressed 'prev' page button
    s = db.findUserShipsScrollUp(u.user_id, req.query.lastKey, limit)
    s.reverse()
    if (s.length === 0) {
      // no prev elements available. stay on the current page
      s = db.findUserShipsScrollDown(
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
    s = db.findUserShipsScrollDown(
      u.user_id,
      req.query.currentFirstKey - 1,
      limit
    )
    if (s.length === 0) {
      // the only element in this page is removed
      // go to previous page
      s = db.findUserShipsScrollUp(u.user_id, req.query.currentFirstKey, limit)
      s.reverse()
    }
  } else {
    // default: fetch first page
    s = db.findUserShipsScrollDown(u.user_id, 0, limit)
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
  const m = db.findMissions()
  return res.render('mission', { user: u, rows: m })
})

app.get('/start', (req, res) => {
  const u = findOrCreateUser(req.query.u || uuidv1())
  const m = db.findMission(req.query.mission || 1)
  return res.render('start', { user: u, mission: m })
})

app.get('/success', (req, res) => {
  const u = findOrCreateUser(req.query.u || uuidv1())
  const m = db.findMission(req.query.mission || 1)
  db.earnGold(u.guid, m.reward)
  delete userCache[u.guid]
  return res.render('success', { user: u, mission: m })
})

app.get('/port', (req, res) => {
  const u = findOrCreateUser(req.query.u || uuidv1())
  const limit = 5
  let p
  if (req.query.firstKey) {
    // user pressed 'next' page button
    p = db.findPortsScrollDown(u.user_id, req.query.firstKey, limit)
    if (p.length === 0) {
      // no next elements available. stay on the current page
      p = db.findPortsScrollDown(
        u.user_id,
        req.query.currentFirstKey - 1,
        limit
      )
    }
  } else if (req.query.lastKey) {
    // user pressed 'prev' page button
    p = db.findPortsScrollUp(u.user_id, req.query.lastKey, limit)
    p.reverse()
    if (p.length === 0) {
      // no prev elements available. stay on the current page
      p = db.findPortsScrollDown(
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
    p = db.findPortsScrollDown(u.user_id, req.query.currentFirstKey - 1, limit)
    if (p.length === 0) {
      // the only element in this page is removed
      // go to previous page
      p = db.findPortsScrollUp(u.user_id, req.query.currentFirstKey, limit)
      p.reverse()
    }
  } else {
    // default: fetch first page
    p = db.findPortsScrollDown(u.user_id, 0, limit)
  }
  const firstKey = p.length > 0 ? p[0].seaport_id : undefined
  const lastKey = p.length > 0 ? p[p.length - 1].seaport_id : undefined

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
  const p = db.findPort(req.query.seaport || 1)
  console.log('travel to port', p.seaport_id, p.x, p.y)
  travelTo(u.guid, p.x, p.y)
  return res.render('idle', { user: u })
})

app.get('/teleporttoport', (req, res) => {
  const u = findOrCreateUser(req.query.u || uuidv1())
  const p = db.findPort(req.query.seaport || 1)
  console.log('teleport to port', p.seaport_id, p.x, p.y)
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
    const port = db.findPort(req.query.portId)
    if (port) {
      db.deletePort(req.query.portId)
      const buf = message.DeletePortStruct.buffer()
      for (let i = 0; i < buf.length; i++) {
        buf[i] = 0
      }
      message.DeletePortStruct.fields.type = 8 // Delete Port
      message.DeletePortStruct.fields.portId = port.seaport_id
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
    const shipyard = db.findShipyard(req.query.shipyardId)
    if (shipyard) {
      db.deleteShipyard(req.query.shipyardId)
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
  const seaportId = db.createPort(
    portName,
    selectedLng,
    selectedLat,
    u.user_id,
    expectLand
  )
  // send spawn command to sea-server
  const reply = await sendSpawnPort(
    seaportId,
    portName,
    selectedLng,
    selectedLat,
    u.user_id,
    expectLand
  )
  if (reply.dbId === seaportId) {
    if (reply.existing === 0) {
      // successfully created
      db.spendGold(u.guid, 10000)
      return {
        seaportId: seaportId,
        err: null
      }
    } else {
      console.error(
        'Port with the same ID already exists on sea-server (is this possible?!)'
      )
      return {
        seaportId: seaportId,
        err: 'db inconsistent'
      }
    }
  }
  // something went wrong; delete from db
  db.deletePort(seaportId)
  // print useful information
  if (reply.dbId > 0 && reply.dbId !== seaportId) {
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
  const shipyardId = db.createShipyard(
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
      db.spendGold(u.guid, 10000)
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
  db.deleteShipyard(shipyardId)
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
  const dbId = db.createShip(userId, shipName, expectLand)
  const p0 = db.findPort(r0)
  const p1 = db.findPort(r1)
  if (p0 && p1) {
    const reply = await sendSpawnShip(
      dbId,
      xc0,
      yc0,
      p0.seaport_id,
      p1.seaport_id,
      expectLand
    )
    if (reply.dbId === dbId) {
      return dbId
    } else {
      console.error(`Spawn ship request id and result id mismatch`)
      db.deleteShip(dbId)
    }
  } else {
    console.error(`Ports cannot be found - ${p0}, ${p1}`)
    db.deleteShip(dbId)
  }
  return 0
}

app.get('/sell_port', (req, res) => {
  const u = findOrCreateUser(req.query.u || uuidv1())
  if (req.query.r) {
    const port = db.findPort(req.query.r)
    if (port) {
      db.deletePort(req.query.r)
      const buf = message.DeletePortStruct.buffer()
      for (let i = 0; i < buf.length; i++) {
        buf[i] = 0
      }
      message.DeletePortStruct.fields.type = 8 // Delete Port
      message.DeletePortStruct.fields.portId = port.seaport_id
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
  const shipyard = db.findShipyard(req.query.shipyardId)
  const dockedShips = db.listShipDockedAtShipyardToArray(req.query.shipyardId)
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
  const ship = db.findShip(req.query.shipId)
  if (ship) {
    let seaport1, seaport2
    const dockedShipyard = ship.docked_shipyard_id
      ? db.findShipyard(ship.docked_shipyard_id)
      : null
    if (ship.shiproute_id) {
      const shiproute = db.findShipShiproute(ship.ship_id)
      seaport1 = db.findPort(shiproute.port1_id)
      seaport2 = db.findPort(shiproute.port2_id)
    } else {
      seaport1 = db.findPort(req.query.seaport1Id)
      seaport2 = db.findPort(req.query.seaport2Id)
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
  const shipyard = db.findShipyard(req.query.shipyardId)
  let resultMsg, errMsg
  if (shipyard) {
    const dockedShips = db.listShipDockedAtShipyardToArray(shipyard.shipyard_id)
    if (dockedShips.length < 4) {
      const shipName = `${raname.middle()} ${raname.middle()}`
      const shipId = db.createShip(u.user_id, shipName, 0)
      db.setShipDockedShipyardId(shipId, req.query.shipyardId)
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
  const ship = db.findShip(req.query.shipId)
  while (1) {
    if (ship) {
      const seaport1 = db.findPort(req.query.seaport1Id)
      if (seaport1) {
        const seaport2 = db.findPort(req.query.seaport2Id)
        if (seaport2) {
          if (req.query.seaport1Id === req.query.seaport2Id) {
            errMsg = '중복 항구 오류'
            break
          }
          const shiprouteId = db.createShiproute(
            seaport1.seaport_id,
            seaport2.seaport_id
          )
          db.setShipShiproute(ship.ship_id, shiprouteId)
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
  const ship = db.findShip(req.query.shipId)
  if (ship && ship.shiproute_id) {
    db.setShipShiproute(ship.ship_id, null)
    resultMsg = '항로 초기화 성공'
    const affectedRows = db.deleteShiproute(ship.shiproute_id)
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
  const shiproute = db.findShipShiproute(req.query.shipId)
  if (shiproute) {
    if (shiproute.shiproute_id > 0) {
      if (shiproute.docked_shipyard_id > 0) {
        const affectedRows = db.setShipDockedShipyardId(shiproute.ship_id, null)
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

const sendQueryNearestShipyardForShip = async shipId => {
  const buf = message.QueryNearestShipyardForShipStruct.buffer()
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0
  }
  const replyId = issueNewReplyId()
  message.QueryNearestShipyardForShipStruct.fields.type = 11
  message.QueryNearestShipyardForShipStruct.fields.shipId = shipId
  message.QueryNearestShipyardForShipStruct.fields.replyId = replyId
  return sendAndReplyFromSea(buf, replyId, err => {
    if (err) {
      console.error('sea udp SpawnShipStruct client error:', err)
    }
  })
}

app.get('/moveToNearestShipyard', async (req, res) => {
  let resultMsg, errMsg
  const shipId = req.query.shipId
  const reply = await sendQueryNearestShipyardForShip(shipId)
  const nearestShipyardId = reply.shipyardId
  if (nearestShipyardId >= 0) {
    const shipyard = db.findShipyard(nearestShipyardId)
    if (shipyard) {
      const dockedShips = db.listShipDockedAtShipyardToArray(nearestShipyardId)
      if (dockedShips.length < 4) {
        requestDespawnShipToSeaServer(shipId)
        db.setShipDockedShipyardId(shipId, nearestShipyardId)
        resultMsg = '정박 성공'
      } else {
        errMsg = '가장 가까운 조선소 정박 초과'
      }
    } else {
      errMsg = '조선소 조회 오류'
    }
  } else {
    errMsg = '가장 가까운 조선소 조회 오류'
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
    const shiprouteId = db.createShiproute(
      message.SpawnShipReplyStruct.fields.port1Id,
      message.SpawnShipReplyStruct.fields.port2Id
    )
    db.setShipShiproute(message.SpawnShipReplyStruct.fields.dbId, shiprouteId)
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
    const ports = await db.listPortToArray()
    for (let i = 0; i < ports.length; i++) {
      const row = ports[i]
      await sendSpawnPort(
        row.seaport_id,
        row.name,
        row.x,
        row.y,
        row.owner_id,
        row.seaport_type
      )
      portCount++
    }
    console.log(`  ${portCount} port(s) recovered...`)
    // recovering shipyards
    let shipyardCount = 0
    const shipyards = await db.listShipyardToArray()
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
    const ships = db.listShipShiprouteToArray()
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
    const ship = db.findShip(message.ArrivalStruct.fields.shipId)
    if (ship) {
      db.earnGoldUser(ship.user_id, 1)
      const guid = db.findUserGuid(ship.user_id).guid
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
  } else if (buf[0] === 6) {
    // QueryNearestShipyardForShipReplyStruct
    console.log(
      `QueryNearestShipyardForShipReplyStruct from ${remote.address}:${
        remote.port
      } (len=${buf.length})`
    )
    message.QueryNearestShipyardForShipReplyStruct._setBuff(buf)
    notifyWaiter(message.QueryNearestShipyardForShipReplyStruct)
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
