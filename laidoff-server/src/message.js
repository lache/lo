const Struct = require('struct')
// const dgram = require("dgram");

/*
Not yet used.
const HttpResponseStruct = Struct()
  .word16Sle('size')
  .word16Sle('type')
  .chars('body', 8192 - 4)
*/

const FullStateObjectStruct = Struct()
  .word32Sle('id')
  .floatle('x')
  .floatle('y')
  .floatle('a')
FullStateObjectStruct.allocate()

const FullStateStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .array('objects', 64, FullStateObjectStruct)
FullStateStruct.allocate()

const SpawnStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('expectedDbId')
  .floatle('x')
  .floatle('y')
SpawnStruct.allocate()

const TravelToStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .chars('id', 64)
  .floatle('x')
  .floatle('y')
TravelToStruct.allocate()

const TeleportToStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .chars('id', 64)
  .floatle('x')
  .floatle('y')
TeleportToStruct.allocate()

const SpawnShipStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('expectedDbId')
  .floatle('x')
  .floatle('y')
  .word32Sle('port1Id')
  .word32Sle('port2Id')
  .word32Sle('replyId')
  .word32Sle('expectLand')
SpawnShipStruct.allocate()

const SpawnShipReplyStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('dbId')
  .word32Sle('port1Id')
  .word32Sle('port2Id')
  .word32Sle('routed')
  .word32Sle('replyId')
SpawnShipReplyStruct.allocate()

const ArrivalStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('shipId')
ArrivalStruct.allocate()

const DeleteShipStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('shipId')
DeleteShipStruct.allocate()

const SpawnPortStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('expectedDbId')
  .chars('name', 64)
  .word32Sle('x')
  .word32Sle('y')
  .word32Sle('ownerId')
  .word32Sle('replyId')
  .word32Sle('expectLand')
SpawnPortStruct.allocate()

const SpawnPortReplyStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('dbId')
  .word32Sle('replyId')
  .word32Sle('existing')
SpawnPortReplyStruct.allocate()

const NamePortStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('portId')
  .chars('name', 64)
  .word32Sle('x')
  .word32Sle('y')
  .word32Sle('ownerId')
  .word32Sle('portType')
NamePortStruct.allocate()

const DeletePortStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('portId')
DeletePortStruct.allocate()

module.exports = {
  SpawnStruct,
  TravelToStruct,
  TeleportToStruct,
  SpawnShipStruct,
  SpawnShipReplyStruct,
  ArrivalStruct,
  DeleteShipStruct,
  SpawnPortStruct,
  SpawnPortReplyStruct,
  NamePortStruct,
  DeletePortStruct
}
