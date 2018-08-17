import { DataTemplate } from './data/datautil';

export interface ShipStat {
  moveSpeed: number;
  durability: number;
  maxLoad: number;
  loadingSpeed: number;
}

export const templateToStat = (template?: DataTemplate): ShipStat =>
  template
    ? {
        moveSpeed: +template.moveSpeed,
        durability: +template.durability,
        maxLoad: +template.maxLoad,
        loadingSpeed: +template.loadingSpeed,
      }
    : {
        moveSpeed: 0,
        durability: 0,
        maxLoad: 0,
        loadingSpeed: 0,
      };

export const addShipStat = (op1?: ShipStat, op2?: ShipStat): ShipStat => {
  return {
    moveSpeed:
      ((op1 ? op1.moveSpeed : 0) || 0) + ((op2 ? op2.moveSpeed : 0) || 0),
    durability:
      ((op1 ? op1.durability : 0) || 0) + ((op2 ? op2.durability : 0) || 0),
    maxLoad: ((op1 ? op1.maxLoad : 0) || 0) + ((op2 ? op2.maxLoad : 0) || 0),
    loadingSpeed:
      ((op1 ? op1.loadingSpeed : 0) || 0) + ((op2 ? op2.loadingSpeed : 0) || 0),
  };
};
