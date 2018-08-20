import * as Struct from 'struct';
// import * as dgram from 'dgram';

/*
Not yet used.
const HttpResponseStruct = Struct()
  .word16Sle('size')
  .word16Sle('type')
  .chars('body', 8192 - 4)
*/

export const FullStateObjectStruct = Struct()
  .word32Sle('id')
  .floatle('x')
  .floatle('y')
  .floatle('a');
FullStateObjectStruct.allocate();

export const FullStateStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .array('objects', 64, FullStateObjectStruct);
FullStateStruct.allocate();

export const SpawnStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('expectedDbId')
  .floatle('x')
  .floatle('y');
SpawnStruct.allocate();

export const TravelToStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .chars('id', 64)
  .floatle('x')
  .floatle('y');
TravelToStruct.allocate();

export const TeleportToStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .chars('id', 64)
  .floatle('x')
  .floatle('y');
TeleportToStruct.allocate();

export const SpawnShipStruct = Struct()
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
  .word32Sle('shipTemplateId');
SpawnShipStruct.allocate();

export const SpawnShipReplyStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('dbId')
  .word32Sle('port1Id')
  .word32Sle('port2Id')
  .word32Sle('routed')
  .word32Sle('replyId');
SpawnShipReplyStruct.allocate();

export const ArrivalStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('shipId');
ArrivalStruct.allocate();

export const DeleteShipStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('shipId');
DeleteShipStruct.allocate();

export const SpawnPortStruct = Struct()
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
  .word32Sle('expectLand');
SpawnPortStruct.allocate();

export const SpawnPortReplyStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('dbId')
  .word32Sle('replyId')
  .word32Sle('existing')
  .word32Sle('tooClose');
SpawnPortReplyStruct.allocate();

export const SpawnShipyardStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('expectedDbId')
  .chars('name', 64)
  .word32Sle('x')
  .word32Sle('y')
  .word32Sle('ownerId')
  .word32Sle('replyId');
SpawnShipyardStruct.allocate();

export const SpawnShipyardReplyStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('dbId')
  .word32Sle('replyId')
  .word32Sle('existing');
SpawnShipyardReplyStruct.allocate();

export const NamePortStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('portId')
  .chars('name', 64)
  .word32Sle('x')
  .word32Sle('y')
  .word32Sle('ownerId')
  .word32Sle('portType');
NamePortStruct.allocate();

export const DeletePortStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('portId');
DeletePortStruct.allocate();

export const DeleteShipyardStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('shipyardId');
DeleteShipyardStruct.allocate();

export const QueryNearestShipyardForShipStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('shipId')
  .word32Sle('replyId');
QueryNearestShipyardForShipStruct.allocate();

export const QueryNearestShipyardForShipReplyStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('replyId')
  .word32Sle('shipId')
  .word32Sle('shipyardId');
QueryNearestShipyardForShipReplyStruct.allocate();

export const RegisterSharedSecretSessionKeyStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('replyId')
  .chars('accountId', 32)
  .chars('keyStr', 64) // hex-string(0x??) 64 characters --- (32-byte) "NOT NULL TERMINATED"
  .word32Sle('keyStrLen');
RegisterSharedSecretSessionKeyStruct.allocate();

export const RegisterSharedSecretSessionKeyReplyStruct = Struct()
  .word8Sle('type')
  .word8Sle('padding0')
  .word8Sle('padding1')
  .word8Sle('padding2')
  .word32Sle('replyId');
RegisterSharedSecretSessionKeyReplyStruct.allocate();
