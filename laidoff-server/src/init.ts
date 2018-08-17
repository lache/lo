import * as fs from 'fs';
import { db } from './query/db';

export const initialize = () => {
  const schemaSql = fs.readFileSync('./src/sql/ttl.schema.sqlite3.sql', 'utf8');
  db.exec(schemaSql);

  const dataSql = fs.readFileSync('./src/sql/ttl.data.sqlite3.sql', 'utf8');
  db.exec(dataSql);
};
