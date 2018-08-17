import * as Sqlite3 from 'better-sqlite3';

export const db = new Sqlite3('ttl.db');

export interface StatementPolyfill {
  iterate(...params: any[]): any[];
}

// Sadly, there is no type for better-sqlite3 4.1.0 but only 3.1.0.
// Because of this, we cannot use iterate function with its type.
export const iterable = (statement: any /* Statement */, ...args: any[]) =>
  (statement as StatementPolyfill).iterate(args);

export const lastId = (result: { lastInsertROWID: any }): number => {
  if (typeof result.lastInsertROWID === 'number') {
    return result.lastInsertROWID;
  }
  throw new Error(`Invalid numeric value: ${JSON.stringify(result)}`);
};
