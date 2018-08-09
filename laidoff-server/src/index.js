const argv = require('yargs').argv
const init = require('./init')
const express = require('express')
const moment = require('moment')
const numeral = require('numeral')
const dgram = require('dgram')
const seaUdpClient = dgram.createSocket('udp4')
const app = express()
const glob = require('glob')
const addon = require('../build/Release/module')
const value = 8

console.log(`${value} times 2 equals`, addon.my_function(value, 9))

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

// register all HTTP handlers
glob('./src/httphandler/*.js', (err, files) => {
  if (!err) {
    for (const file of files) {
      const requireModuleName = file
        .replace(/^\.\/src\//, './')
        .replace(/\.js$/, '')
      require(requireModuleName)(app)
    }
  } else {
    console.error(err)
  }
})

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
  console.log(`UDP server listening on ${address.address}:${address.port}!`)
})

const udpPort = argv.udpport || 3003
seaUdpClient.bind(udpPort)

const port = argv.port || 3000
app.listen(port, () => {
  console.log(`TCP server listening on ${port} port!`)
})
