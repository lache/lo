import * as dgram from 'dgram';
import * as express from 'express';
import * as glob from 'glob';
import * as moment from 'moment';
import * as numeral from 'numeral';
import * as path from 'path';
import * as yargs from 'yargs';
import * as init from './init';

const argv = yargs.argv;
const app = express();
const seaUdpClient = dgram.createSocket('udp4');

if (argv.init) {
  init.initialize();
  console.log(`Initialized.`);
  process.exit(0);
}

app.locals.moment = moment;
app.locals.numeral = numeral;
app.use(express.static('./src/html'));
app.set('views', './src/views');
app.set('view engine', 'pug');
app.set('seaUdpClient', seaUdpClient);
app.set('verifierMap', {}); // Account ID + ':' + SRP 'B' --> SRP Verifier object
app.set('sessionKeyMap', {}); // Account ID --> Shared secret key string

// register all HTTP handlers
// It should be executed after tsc.
const dirname = __dirname.replace(/\\/gi, '/');
const files = glob.sync(path.join(__dirname, 'httphandler', '*.js'));
for (const file of files) {
  const requireModuleName = file
    .replace(/\\/gi, '/')
    .replace(dirname, '.')
    .replace(/\.js$/, '');
  const r = require(requireModuleName);
  r.default(app);
}

seaUdpClient.on('message', async (buf, remote) => {
  if (buf[0] === 1) {
    require('./fromseaserver/spawnshipreply').receive(buf, remote);
  } else if (buf[0] === 2) {
    await require('./fromseaserver/recoverallships').receive(
      seaUdpClient,
      buf,
      remote,
      app.get('sessionKeyMap')
    );
  } else if (buf[0] === 3) {
    require('./fromseaserver/arrival').receive(buf, remote);
  } else if (buf[0] === 4) {
    require('./fromseaserver/spawnportreply').receive(buf, remote);
  } else if (buf[0] === 5) {
    require('./fromseaserver/spawnshipyardreply').receive(buf, remote);
  } else if (buf[0] === 6) {
    require('./fromseaserver/querynearestshipyardforshipreply').receive(
      buf,
      remote,
    );
  } else if (buf[0] === 7) {
    require('./fromseaserver/registersharedsecretsessionkeyreply').receive(buf, remote);
  }
});

seaUdpClient.on('listening', () => {
  const addressInfo = seaUdpClient.address();
  const address =
    typeof addressInfo === 'string'
      ? addressInfo
      : `${addressInfo.address}:${addressInfo.port}`;
  console.log(`UDP server listening on ${address}!`);
});

const udpPort = argv.udpport || 3003;
seaUdpClient.bind(udpPort);

const port = argv.port || 3000;
app.listen(port, () => {
  console.log(`TCP server listening on ${port} port!`);
});
