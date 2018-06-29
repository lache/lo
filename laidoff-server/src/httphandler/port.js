const db = require('../db')
const dbUser = require('../dbuser')
const uuidv1 = require('uuid/v1')

module.exports = app => {
  app.get('/port', (req, res) => {
    const u = dbUser.findOrCreateUser(req.query.u || uuidv1())
    const limit = 5
    let p
    if (req.query.firstKey) {
      // user pressed 'next' page button
      p = db.findPortsScrollDown(u.user_id, req.query.firstKey, limit)
      if (p.length === 0) {
        // no next elements available. stay on the current page
        p = db.findPortsScrollDown(
          u.user_id,
          req.query.currentFirstKey - 1,
          limit
        )
      }
    } else if (req.query.lastKey) {
      // user pressed 'prev' page button
      p = db.findPortsScrollUp(u.user_id, req.query.lastKey, limit)
      p.reverse()
      if (p.length === 0) {
        // no prev elements available. stay on the current page
        p = db.findPortsScrollDown(
          u.user_id,
          req.query.currentFirstKey - 1,
          limit
        )
      }
    } else if (
      req.query.currentFirstKey &&
      req.query.currentFirstKey !== 'undefined'
    ) {
      // refresh current page
      p = db.findPortsScrollDown(
        u.user_id,
        req.query.currentFirstKey - 1,
        limit
      )
      if (p.length === 0) {
        // the only element in this page is removed
        // go to previous page
        p = db.findPortsScrollUp(u.user_id, req.query.currentFirstKey, limit)
        p.reverse()
      }
    } else {
      // default: fetch first page
      p = db.findPortsScrollDown(u.user_id, 0, limit)
    }
    const firstKey = p.length > 0 ? p[0].seaport_id : undefined
    const lastKey = p.length > 0 ? p[p.length - 1].seaport_id : undefined

    return res.render('port', {
      user: u,
      rows: p,
      firstKey: firstKey,
      lastKey: lastKey,
      errMsg: req.query.errMsg
    })
  })
}
