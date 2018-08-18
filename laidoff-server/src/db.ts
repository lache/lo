import * as raname from 'random-name';
import * as queryAccount from './query/account';
import * as queryCaptain from './query/captain';
import { lastId } from './query/db';
import {
  Account,
  Captain,
  Seaport,
  SeaportBase,
  Ship,
  ShipDockedAtShipyard,
  ShipShiproute,
  Shipyard,
  ShipyardBase,
  UserShip,
} from './query/model';
import * as querySeaport from './query/seaport';
import * as queryShip from './query/ship';
import * as queryShiproute from './query/shiproute';
import * as queryShipyard from './query/shipyard';
import * as queryUser from './query/user';

export const createUser = (guid: string) => {
  const userName = `${raname.first()} ${raname.last()}`;
  const user = queryUser.insertUser.run(guid, userName);
  // no initial ship
  // const shipName = `${raname.middle()} ${raname.middle()}`
  // const userId = lastId(user);
  // query.insertShip.run(userId, shipName)
  return lastId(user);
};
export const createPort = (
  portName: string,
  x: number,
  y: number,
  userId: number,
  expectLand: number,
) => {
  const port = querySeaport.insertPort.run(portName, x, y, userId, expectLand);
  return lastId(port);
};
export const createShipyard = (
  shipyardName: string,
  x: number,
  y: number,
  userId: number,
) => {
  const shipyard = queryShipyard.insertShipyard.run(shipyardName, x, y, userId);
  return lastId(shipyard);
};
export const createShip = (
  userId: number,
  shipName: string,
  shipType: number,
) => {
  const ship = queryShip.insertShip.run(userId, shipName, shipType);
  return lastId(ship);
};
export const deleteShip = (shipId: number) => {
  return queryShip.deleteShip.run(shipId).changes;
};
export const createShiproute = (port1Id: number, port2Id: number) => {
  const shiproute = queryShiproute.insertShiproute.run(port1Id, port2Id);
  return lastId(shiproute);
};
export const deleteShiproute = (shiprouteId: number) => {
  return queryShiproute.deleteShiproute.run(shiprouteId).changes;
};
export const setShipShiproute = (shipId: number, shiprouteId?: number) => {
  queryShip.setShipShiproute.run(shiprouteId, shipId);
};
export const setShipDockedShipyardId = (
  shipId: number,
  dockedShipyardId?: number,
) => {
  return queryShip.setShipDockedShipyardId.run(dockedShipyardId, shipId)
    .changes;
};
export const setShipCaptainId = (shipId: number, captainId?: number) =>
  queryShip.setShipCaptainId.run(captainId, shipId);
export const setShipTemplateId = (shipId: number, templateId: number) =>
  queryShip.setShipTemplateId.run(templateId, shipId);
export const findShipShiproute = (shipId: number): ShipShiproute =>
  queryShip.findShipShiproute.get(shipId);

export const listShipShiproute = (onRow: (each: ShipShiproute) => void) => {
  // noinspection JSUnresolvedFunction
  for (const row of queryShip.listShipShiproute.all()) {
    onRow(row);
  }
};
export const listShipShiprouteToArray = () => {
  const rows: ShipShiproute[] = [];
  listShipShiproute(row => {
    rows.push(row);
  });
  return rows;
};
export const listPort = async (onRow: (each: Seaport) => void) => {
  // noinspection JSUnresolvedFunction
  for (const row of querySeaport.listPort.all()) {
    await onRow(row);
  }
};
export const listPortToArray = async () => {
  const rows: Seaport[] = [];
  await listPort(row => {
    rows.push(row);
  });
  return rows;
};
export const listShipyard = async (onRow: (each: Shipyard) => void) => {
  // noinspection JSUnresolvedFunction
  for (const row of queryShipyard.listShipyard.all()) {
    await onRow(row);
  }
};
export const listShipyardToArray = async () => {
  const rows: Shipyard[] = [];
  await listShipyard(row => {
    rows.push(row);
  });
  return rows;
};
export const listShipDockedAtShipyard = (
  shipyardId: number,
  onRow: (each: ShipDockedAtShipyard) => void,
) => {
  // noinspection JSUnresolvedFunction
  for (const row of queryShip.listShipDockedAtShipyard.all(shipyardId)) {
    onRow(row);
  }
};
export const listShipDockedAtShipyardToArray = (shipyardId: number) => {
  const rows: ShipDockedAtShipyard[] = [];
  listShipDockedAtShipyard(shipyardId, row => {
    rows.push(row);
  });
  return rows;
};
export const findShip = (shipId: number): Ship =>
  queryShip.findShip.get(shipId);
export const findUser = (guid: string): UserShip =>
  queryUser.findUser.get(guid);
export const findUserGuid = (userId: number): { guid: string } =>
  queryUser.findUserGuid.get(userId);
export const earnGold = (guid: string, reward: number) => {
  // not yet implemented
  // query.earnGold.run(reward, guid)
};
export const earnGoldUser = (userId: number, reward: number) => {
  // not yet implemented
  // query.earnGoldUser.run(reward, userId)
};
export const spendGold = (guid: string, cost: number) => {
  // not yet implemented
  // query.spendGold.run(cost, guid)
};
export const findUserShipsScrollDown = (
  userId: number,
  lastUserId: number,
  count: number,
): Ship[] => queryShip.findUserShipsScrollDown.all(userId, lastUserId, count);
export const findUserShipsScrollUp = (
  userId: number,
  firstUserId: number,
  count: number,
): Ship[] => queryShip.findUserShipsScrollUp.all(userId, firstUserId, count);
export const findPort = (portId: number): SeaportBase =>
  querySeaport.findPort.get(portId);
export const findShipyard = (shipyardId: number): ShipyardBase =>
  queryShipyard.findShipyard.get(shipyardId);
export const findPortsScrollDown = (
  userId: number,
  lastRegionId: number,
  count: number,
): SeaportBase[] => {
  return querySeaport.findPortsScrollDown.all(lastRegionId, count);
};
export const findPortsScrollUp = (
  userId: number,
  lastRegionId: number,
  count: number,
): SeaportBase[] => {
  return querySeaport.findPortsScrollUp.all(lastRegionId, count);
};
export const deletePort = (portId: number) =>
  querySeaport.deletePort.run(portId);
export const deleteShipyard = (shipyardId: number) => {
  queryShipyard.deleteShipyard.run(shipyardId);
  queryShip.deleteShipDockedAtShipyard.run(shipyardId);
};
export const insertCaptain = (
  userId: number,
  name: string,
  captainTemplateId: number,
) => {
  const captain = queryCaptain.insertCaptain.run(
    userId,
    name,
    captainTemplateId,
  );
  return lastId(captain);
};
export const findCaptain = (captainId: number): Captain =>
  queryCaptain.findCaptain.get(captainId);
export const deleteCaptain = (captainId: number) =>
  queryCaptain.deleteCaptain.run(captainId);

export const createAccount = (accountId: string, s: string, v: string) => {
  try {
    const account = queryAccount.insertAccount.run(accountId, s, v);
    return lastId(account);
  } catch (e) {
    if (e.code !== 'SQLITE_CONSTRAINT_PRIMARYKEY') {
      throw e;
    }
    console.log(`Duplicated account ID: '${accountId}'`);
    return null;
  }
};
export const findAccount = (accountId: string): Account =>
  queryAccount.findAccount.get(accountId);
