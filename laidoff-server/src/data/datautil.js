const XLSX = require('xlsx')
const fs = require('fs')

const loadSheet = (fileName, sheetName) => {
  const xlsxBuf = fs.readFileSync(fileName)
  const workbook = XLSX.read(xlsxBuf, { type: 'buffer' })
  const shipSheet = workbook.Sheets[sheetName]
  let totalRange = XLSX.utils.decode_range(shipSheet['!ref'])
  // console.log(totalRange)
  // console.log(shipSheet[XLSX.utils.encode_cell({ c: 0, r: 0 })])
  const shipData = {}
  const shipDataHeaders = []
  const shipDataKeys = []
  let shipDataRow = null
  for (let R = totalRange.s.r; R <= totalRange.e.r; ++R) {
    for (let C = totalRange.s.c; C <= totalRange.e.c; ++C) {
      const cellAddress = { c: C, r: R }
      /* if an A1-style address is needed, encode the address */
      const cellRef = XLSX.utils.encode_cell(cellAddress)
      if (shipSheet[cellRef]) {
        const v = shipSheet[cellRef].v
        if (R === 0) {
          shipDataHeaders.push(v)
        } else {
          if (C === 0) {
            shipDataRow = shipData[v] = {}
            shipDataKeys.push(v)
          }
          shipDataRow[shipDataHeaders[C]] = v
        }
      }
    }
  }

  return {
    keys: shipDataKeys,
    data: shipData
  }
}

module.exports = {
  loadSheet
}
