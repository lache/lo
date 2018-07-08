const XLSX = require('xlsx')
const fs = require('fs')

const loadSheet = (fileName, sheetName) => {
  const xlsxBuf = fs.readFileSync(fileName)
  const workbook = XLSX.read(xlsxBuf, { type: 'buffer' })
  const sheet = workbook.Sheets[sheetName]
  let totalRange = XLSX.utils.decode_range(sheet['!ref'])
  // console.log(totalRange)
  // console.log(shipSheet[XLSX.utils.encode_cell({ c: 0, r: 0 })])
  const data = {}
  const dataHeaders = []
  const dataKeys = []
  let dataRow = null
  for (let R = totalRange.s.r; R <= totalRange.e.r; ++R) {
    for (let C = totalRange.s.c; C <= totalRange.e.c; ++C) {
      const cellAddress = { c: C, r: R }
      /* if an A1-style address is needed, encode the address */
      const cellRef = XLSX.utils.encode_cell(cellAddress)
      if (sheet[cellRef]) {
        const v = sheet[cellRef].v
        if (R === 0) {
          dataHeaders.push(v)
        } else {
          if (C === 0) {
            dataRow = data[v] = {}
            dataKeys.push(v)
          }
          dataRow[dataHeaders[C]] = v
        }
      }
    }
  }

  return {
    keys: dataKeys,
    data: data
  }
}

module.exports = {
  loadSheet
}
