const argv = require('yargs').argv
const init = require('./init')
const express = require('express')
const moment = require('moment')
const numeral = require('numeral')
const dgram = require('dgram')
const seaUdpClient = dgram.createSocket('udp4')
const app = express()

if (argv.init) {
  init.initialize()
  console.log(`Initialized.`)
  process.exit(0)
}

app.locals.moment = moment
app.locals.numeral = numeral
app.use(express.static('./src/html'))
app.set('views', './src/views')
app.set('view engine', 'pug')
app.set('seaUdpClient', seaUdpClient)
require('./httphandler/confirmnewroute')(app)
require('./httphandler/deleteroute')(app)
require('./httphandler/demolishport')(app)
require('./httphandler/demolishshipyard')(app)
require('./httphandler/idle')(app)
require('./httphandler/link')(app)
require('./httphandler/linkland')(app)
require('./httphandler/loan')(app)
require('./httphandler/mission')(app)
require('./httphandler/movetonearestshipyard')(app)
require('./httphandler/openship')(app)
require('./httphandler/openshipyard')(app)
require('./httphandler/port')(app)
require('./httphandler/purchasenewport')(app)
require('./httphandler/purchasenewshipyard')(app)
require('./httphandler/purchaseshipatshipyard')(app)
require('./httphandler/sellport')(app)
require('./httphandler/sellship')(app)
require('./httphandler/sellvessel')(app)
require('./httphandler/start')(app)
require('./httphandler/startroute')(app)
require('./httphandler/success')(app)
require('./httphandler/teleporttoport')(app)
require('./httphandler/traveltoport')(app)
require('./httphandler/vessel')(app)

seaUdpClient.on('message', async (buf, remote) => {
  if (buf[0] === 1) {
    require('./fromseaserver/spawnshipreply').receive(buf, remote)
  } else if (buf[0] === 2) {
    await require('./fromseaserver/recoverallships').receive(
      seaUdpClient,
      buf,
      remote
    )
  } else if (buf[0] === 3) {
    require('./fromseaserver/arrival').receive(buf, remote)
  } else if (buf[0] === 4) {
    require('./fromseaserver/spawnportreply').receive(buf, remote)
  } else if (buf[0] === 5) {
    require('./fromseaserver/spawnshipyardreply').receive(buf, remote)
  } else if (buf[0] === 6) {
    require('./fromseaserver/querynearestshipyardforshipreply').receive(
      buf,
      remote
    )
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
