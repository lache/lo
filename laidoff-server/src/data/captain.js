const XLSX = require('xlsx')
const fs = require('fs')
const xlsxBuf = fs.readFileSync('data/ttl.ods')
const workbook = XLSX.read(xlsxBuf, { type: 'buffer' })
const captainSheet = workbook.Sheets[workbook.SheetNames[0]]

let totalRange = XLSX.utils.decode_range(captainSheet['!ref'])
// console.log(totalRange)
// console.log(captainSheet[XLSX.utils.encode_cell({ c: 0, r: 0 })])
const captainData = {}
const captainDataHeaders = []
const captainDataKeys = []
let captainDataRow = null
for (let R = totalRange.s.r; R <= totalRange.e.r; ++R) {
  for (let C = totalRange.s.c; C <= totalRange.e.c; ++C) {
    const cellAddress = { c: C, r: R }
    /* if an A1-style address is needed, encode the address */
    const cellRef = XLSX.utils.encode_cell(cellAddress)
    const v = captainSheet[cellRef].v
    if (R === 0) {
      captainDataHeaders.push(v)
    } else {
      if (C === 0) {
        captainDataRow = captainData[v] = {}
        captainDataKeys.push(v)
      }
      captainDataRow[captainDataHeaders[C]] = v
    }
  }
}

module.exports = {
  keys: captainDataKeys,
  data: captainData
}
