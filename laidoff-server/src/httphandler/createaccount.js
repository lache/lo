const url = require('url')

module.exports = app => {
  app.get('/createAccount', (req, res) => {
    console.log('X-Account-Id')
    console.log(req.get('X-Account-Id'))
    res.redirect(
      url.format({
        pathname: '/idle'
      })
    )
  })
}
