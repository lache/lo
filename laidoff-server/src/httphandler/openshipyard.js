const db = require('../db')
const dbUser = require('../dbuser')
const uuidv1 = require('uuid/v1')
const url = require('url')

module.exports = app => {
  app.get('/openShipyard', (req, res) => {
    const u = dbUser.findOrCreateUser(req.get('X-U') || req.query.u || uuidv1())
    const shipyard = db.findShipyard(req.query.shipyardId)
    const dockedShips = db.listShipDockedAtShipyardToArray(req.query.shipyardId)
    if (shipyard) {
      return res.render('openshipyard', {
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
}
