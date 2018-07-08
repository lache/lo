const db = require('./db')
const dbUser = require('./dbuser')
const uuidv1 = require('uuid/v1')
const raname = require('random-name')
const spawnPort = require('./toseaserver/spawnport')
const url = require('url')

const execCreatePort = async (
  seaUdpClient,
  u,
  selectedLng,
  selectedLat,
  expectLand
) => {
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
  const reply = await spawnPort.send(
    seaUdpClient,
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

const purchaseNewPort = async (req, res, expectLand) => {
  const u = dbUser.findOrCreateUser(req.get('X-U') || uuidv1())
  const xc0 = req.get('X-D-XC0')
  const yc0 = req.get('X-D-YC0')
  console.log(`purchaseNewPort at [${xc0}, ${yc0}]`)
  let resultMsg = ''
  let errMsg = ''
  const r0 = await execCreatePort(
    req.app.get('seaUdpClient'),
    u,
    xc0,
    yc0,
    expectLand
  )
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

module.exports = {
  purchaseNewPort,
  execCreatePort
}
