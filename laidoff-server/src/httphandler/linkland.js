const link = require('../link')

module.exports = app => {
  app.get('/linkland', async (req, res) => {
    await link.link(req, res, 1)
  })
}
