const link = require('../link')

module.exports = app => {
  app.get('/link', async (req, res) => {
    await link.link(req, res, 0)
  })
}