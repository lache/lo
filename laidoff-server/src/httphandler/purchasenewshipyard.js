const shipyard = require('../shipyard')

module.exports = app => {
  app.get('/purchaseNewShipyard', async (req, res) => {
    await shipyard.purchaseNewShipyard(req, res, 0)
  })
}
