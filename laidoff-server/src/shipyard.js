const db = require('./db')
const dbUser = require('./dbuser')
const url = require('url')
const raname = require('random-name')
const spawnShipyard = require('./toseaserver/spawnshipyard')
const uuidv1 = require('uuid/v1')

const execCreateShipyard = async (
  seaUdpClient,
  u,
  selectedLng,
  selectedLat
) => {
  const shipyardName = `Shipyard ${raname.first()}`
  // create db entry first
  const shipyardId = db.createShipyard(
    shipyardName,
    selectedLng,
    selectedLat,
    u.user_id
  )
  // send spawn command to sea-server
  const reply = await spawnShipyard.send(
    seaUdpClient,
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

const purchaseNewShipyard = async (req, res) => {
  const u = dbUser.findOrCreateUser(req.get('X-U') || uuidv1())
  const xc0 = req.get('X-D-XC0')
  const yc0 = req.get('X-D-YC0')
  console.log(`purchaseNewShipyard at [${xc0}, ${yc0}]`)
  let resultMsg = ''
  let errMsg = ''
  const r0 = await execCreateShipyard(req.app.get('seaUdpClient'), u, xc0, yc0)
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

module.exports = {
  purchaseNewShipyard
}
