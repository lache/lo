const link = require('../link')

module.exports = app => {
  app.get('/linkland', async (req, res) => {
    await link(req, res, 1)
  })
}
