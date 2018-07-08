const captainData = require('../data/captain')
const db = require('../db')
const url = require('url')

module.exports = app => {
  app.get('/openHireCaptain', (req, res) => {
    const ship = db.findShip(req.query.shipId)
    if (ship) {
      const captainTemplateCount = captainData.keys.length
      const randomCaptainIndex = Math.floor(
        Math.random() * captainTemplateCount
      )
      const captainTemplate =
        captainData.data[captainData.keys[randomCaptainIndex]]
      return res.render('openhirecaptain', {
        ship,
        captainTemplate
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
}