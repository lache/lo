import { Socket } from 'dgram';
import { AddressInfo } from 'net';
import * as db from '../db';
import * as spawnPort from '../toseaserver/spawnport';
import * as spawnShip from '../toseaserver/spawnship';
import * as spawnShipyard from '../toseaserver/spawnshipyard';

export const receive = async (
  seaUdpClient: Socket,
  buf: any,
  remote: AddressInfo,
) => {
  // RecoverAllShips
  console.log(
    `RecoverAllShips from ${remote.address}:${remote.port} (len=${buf.length})`,
  );
  console.log('A new sea-server instance requested recovering.');
  console.log('Recovering in progress...');
  // recovering ports
  let portCount = 0;
  const ports = await db.listPortToArray();
  for (const row of ports) {
    await spawnPort.send(
      seaUdpClient,
      row.seaport_id,
      row.name,
      row.x,
      row.y,
      row.user_id,
      row.seaport_type,
    );
    portCount++;
  }
  console.log(`  ${portCount} port(s) recovered...`);
  // recovering shipyards
  let shipyardCount = 0;
  const shipyards = await db.listShipyardToArray();
  for (const row of shipyards) {
    await spawnShipyard.send(
      seaUdpClient,
      row.shipyard_id,
      row.name,
      row.x,
      row.y,
      row.user_id,
    );
    shipyardCount++;
  }
  console.log(`  ${shipyardCount} shipyard(s) recovered...`);
  // recovering ships
  let shipShiprouteCount = 0;
  let shipDockedCount = 0;
  const ships = db.listShipShiprouteToArray();
  for (const row of ships) {
    if (!row.docked_shipyard_id) {
      await spawnShip.send(
        seaUdpClient,
        row.ship_id,
        0,
        0,
        row.port1_id,
        row.port2_id,
        row.ship_type,
        row.template_id,
      );
      shipShiprouteCount++;
    } else {
      shipDockedCount++;
    }
  }
  console.log(
    `  ${shipShiprouteCount} ship(s) recovered... (${shipDockedCount} docked ships excluded)`,
  );
  console.log(`Recovering Done.`);
};
