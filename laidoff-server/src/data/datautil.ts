import * as fs from 'fs';
import * as XLSX from 'xlsx';

export interface DataTemplate {
  [header: string]: string;
}

export interface DataTemplateStore {
  [key: string]: DataTemplate;
}

export const loadSheet = (fileName: string, sheetName: string) => {
  const xlsxBuf = fs.readFileSync(fileName);
  const workbook = XLSX.read(xlsxBuf, { type: 'buffer' });
  const sheet = workbook.Sheets[sheetName];
  const totalRange = XLSX.utils.decode_range(sheet['!ref']!);
  // console.log(totalRange)
  // console.log(shipSheet[XLSX.utils.encode_cell({ c: 0, r: 0 })])
  const data: DataTemplateStore = {};
  const dataHeaders: string[] = [];
  const dataKeys: string[] = [];
  for (let R = totalRange.s.r; R <= totalRange.e.r; ++R) {
    for (let C = totalRange.s.c; C <= totalRange.e.c; ++C) {
      const cellAddress = { c: C, r: R };
      /* if an A1-style address is needed, encode the address */
      const cellRef = XLSX.utils.encode_cell(cellAddress);
      if (sheet[cellRef]) {
        const v: string = sheet[cellRef].v;
        if (R === 0) {
          dataHeaders.push(v);
        } else {
          if (C === 0) {
            dataKeys.push(v);
          }
          data[v][dataHeaders[C]] = v;
        }
      }
    }
  }

  return {
    keys: dataKeys,
    data,
  };
};
