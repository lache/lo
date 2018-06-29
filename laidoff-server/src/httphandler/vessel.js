const db = require('../db')
const dbUser = require('../dbuser')
const uuidv1 = require('uuid/v1')

module.exports = app => {
  app.get('/vessel', (req, res) => {
    const u = dbUser.findOrCreateUser(req.query.u || uuidv1())
    const limit = 5
    let s
    if (req.query.firstKey) {
      // user pressed 'next' page button
      s = db.findUserShipsScrollDown(u.user_id, req.query.firstKey, limit)
      if (s.length === 0) {
        // no next elements available. stay on the current page
        s = db.findUserShipsScrollDown(
          u.user_id,
          req.query.currentFirstKey - 1,
          limit
        )
      }
    } else if (req.query.lastKey) {
      // user pressed 'prev' page button
      s = db.findUserShipsScrollUp(u.user_id, req.query.lastKey, limit)
      s.reverse()
      if (s.length === 0) {
        // no prev elements available. stay on the current page
        s = db.findUserShipsScrollDown(
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
      s = db.findUserShipsScrollDown(
        u.user_id,
        req.query.currentFirstKey - 1,
        limit
      )
      if (s.length === 0) {
        // the only element in this page is removed
        // go to previous page
        s = db.findUserShipsScrollUp(
          u.user_id,
          req.query.currentFirstKey,
          limit
        )
        s.reverse()
      }
    } else {
      // default: fetch first page
      s = db.findUserShipsScrollDown(u.user_id, 0, limit)
    }
    const firstKey = s.length > 0 ? s[0].ship_id : undefined
    const lastKey = s.length > 0 ? s[s.length - 1].ship_id : undefined
    return res.render('vessel', {
      user: u,
      rows: s,
      firstKey: firstKey,
      lastKey: lastKey
    })
  })
}
