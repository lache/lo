const shipUtil = require('../shiputil')

module.exports = app => {
  app.get('/openShip', shipUtil.openShipHandler)
}
