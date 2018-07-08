const addShipStat = (op1, op2) => {
  return {
    moveSpeed:
      ((op1 ? op1.moveSpeed : 0) || 0) + ((op2 ? op2.moveSpeed : 0) || 0),
    durability:
      ((op1 ? op1.durability : 0) || 0) + ((op2 ? op2.durability : 0) || 0),
    maxLoad: ((op1 ? op1.maxLoad : 0) || 0) + ((op2 ? op2.maxLoad : 0) || 0),
    loadingSpeed:
      ((op1 ? op1.loadingSpeed : 0) || 0) + ((op2 ? op2.loadingSpeed : 0) || 0)
  }
}

module.exports = {
  addShipStat
}
