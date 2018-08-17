import { Request } from 'express';

export const numbers = (req: Request, ...names: string[]): number[] => {
  const values: number[] = [];
  for (const name of names) {
    const maybeValue = req.get(name);
    if (!maybeValue) {
      throw new Error(`No ${name} in request[${JSON.stringify(req)}]`);
    }
    values.push(+maybeValue);
  }
  return values;
};
