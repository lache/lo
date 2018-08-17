import * as db from './db';
import { UserShip } from './query/model';

const userCache: { [guid: string]: UserShip } = {};

export const findOrCreateUser = (guid: string): UserShip => {
  if (guid in userCache) {
    return userCache[guid];
  }
  const userInDb = db.findUser(guid);
  // console.log(userInDb)
  if (userInDb !== undefined) {
    userCache[guid] = userInDb;
    return userInDb;
  }
  db.createUser(guid);
  return findOrCreateUser(guid);
};

export const invalidateUserCache = (u: { guid: string }) => {
  delete userCache[u.guid];
};
