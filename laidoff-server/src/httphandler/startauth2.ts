import { Application } from 'express';
import * as url from 'url';
import * as db from '../db';
import * as registerSharedSecretSessionKey from '../toseaserver/registersharedsecretsessionkey';
const srp = require('../../build/Debug/module'); // tslint:disable-line

// noinspection JSUnusedGlobalSymbols
export default (app: Application) => {
  app.get('/startAuth2', async (req, res) => {
    const accountId = req.get('X-Account-Id')!;
    const m = req.get('X-Account-M');
    const b = req.get('X-Account-B');
    const resultMsg = '';
    let errMsg = '';
    const account = db.findAccount(accountId);
    if (account && m) {
      console.log('X-Account-Id');
      console.log(accountId);
      console.log('X-Account-M');
      console.log(m);

      const ver = req.app.get('verifierMap')[accountId + ':' + b];
      if (ver) {
        const mBuf = Buffer.from(m, 'hex');
        const hamkBuf = srp.VerifierVerifySession(ver, mBuf);
        if (hamkBuf.length > 0) {
          const hamkStr = hamkBuf.toString('hex');
          console.log(`HAMK: ${hamkStr}`);
          const keyBuf = srp.VerifierGetSessionKey(ver);
          const keyStr = keyBuf.toString('hex');
          console.log(`Session key: ${keyStr}`);
          console.log('User authentication ok');
          req.app.get('sessionKeyMap')[accountId] = keyStr;
          res.setHeader('Content-Type', 'application/json');
          res.send(JSON.stringify({ HAMK: hamkStr }));
          await registerSharedSecretSessionKey.send(app.get('seaUdpClient'), accountId, keyStr)
        } else {
          console.error('User authentication failed!');
          errMsg = '인증 실패';
          res.redirect(
            url.format({
              pathname: '/idle',
              query: {
                resultMsg,
                errMsg,
              },
            }),
          );
        }
      } else {
        console.error('No pending verifier exist');
        errMsg = '잘못된 접근!';
        res.redirect(
          url.format({
            pathname: '/idle',
            query: {
              resultMsg,
              errMsg,
            },
          }),
        );
      }
    } else {
      errMsg = '계정 조회 실패!';
      res.redirect(
        url.format({
          pathname: '/idle',
          query: {
            resultMsg,
            errMsg,
          },
        }),
      );
    }
  });
};
