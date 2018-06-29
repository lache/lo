const db = require('./db')

const userCache = {}

const findOrCreateUser = guid => {
  if (guid in userCache) {
    return userCache[guid]
  }
  const userInDb = db.findUser(guid)
  // console.log(userInDb)
  if (userInDb !== undefined) {
    userCache[guid] = userInDb
    return userInDb
  }
  db.createUser(guid)
  return findOrCreateUser(guid)
}

const invalidateUserCache = u => {
  delete userCache[u.guid]
}

module.exports = {
  findOrCreateUser,
  invalidateUserCache
}
