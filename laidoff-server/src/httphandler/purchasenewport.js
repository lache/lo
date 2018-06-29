const seaport = require('../seaport')

module.exports = app => {
  app.get('/purchaseNewPort', async (req, res) => {
    await seaport.purchaseNewPort(req, res, 0)
  })
}