export interface ShipBase {
  ship_id: number;
  name: string;
  ship_name: string;
  x: number;
  y: number;
  angle: number;
  oil: number;
}
export interface Ship extends ShipBase {
  user_id: number;
  updated: Date; // ?
  shiproute_id: number;
  ship_type: number;
  docked_shipyard_id: number;
  captain_id: number;
  template_id: number;
}

export interface User {
  user_id: number;
  guid: string;
  name: string;
  user_name: string;
  gold: number;
}

export interface UserShip extends ShipBase, User {}

export interface ShipShiproute {
  // In ship
  ship_id: number;
  ship_type: number;
  docked_shipyard_id: number;
  shiproute_id: number;
  template_id: number;

  // In shiproute
  port1_id: number;
  port2_id: number;
}

export interface SeaportBase {
  seaport_id: number;
  name: string;
  x: number;
  y: number;
}

export interface Seaport extends SeaportBase {
  user_id: number;
  seaport_type: number;
}

export interface ShipyardBase {
  shipyard_id: number;
  name: string;
  x: number;
  y: number;
}

export interface Shipyard extends ShipyardBase {
  user_id: number;
}

export interface ShipDockedAtShipyard {
  ship_id: number;
  name: string;
  user_id: number;
}

export interface Captain {
  captain_id: number;
  name: string;
  template_id: number;
  user_id: number;
}

export interface Account {
  s: string;
  v: string;
}
