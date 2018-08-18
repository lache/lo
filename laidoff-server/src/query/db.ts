import * as Sqlite3 from 'better-sqlite3';

export const db = new Sqlite3('ttl.db');

export const lastId = (result: { lastInsertROWID: any }): number => {
  if (typeof result.lastInsertROWID === 'number') {
    return result.lastInsertROWID;
  }
  throw new Error(`Invalid numeric value: ${JSON.stringify(result)}`);
};
