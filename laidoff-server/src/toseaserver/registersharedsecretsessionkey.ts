import { Socket } from 'dgram';
import { RegisterSharedSecretSessionKeyStruct } from '../message';
import {
  issueNewReplyId,
  ReplyPacket,
  sendAndReplyFromSea,
} from '../replywaiter';

interface RegisterSharedSecretSessionKeyReply extends ReplyPacket {
}

export const send = async (seaUdpClient: Socket, accountId: string, keyStr: string) => {
  const buf = RegisterSharedSecretSessionKeyStruct.buffer();
  for (let i = 0; i < buf.length; i++) {
    buf[i] = 0;
  }
  const replyId = issueNewReplyId();
  RegisterSharedSecretSessionKeyStruct.fields.type = 12;
  RegisterSharedSecretSessionKeyStruct.fields.accountId = accountId;
  RegisterSharedSecretSessionKeyStruct.fields.keyStr = keyStr;
  RegisterSharedSecretSessionKeyStruct.fields.keyStrLen = keyStr.length;
  RegisterSharedSecretSessionKeyStruct.fields.replyId = replyId;
  await sendAndReplyFromSea<RegisterSharedSecretSessionKeyReply>(
    seaUdpClient,
    buf,
    replyId,
    err => {
      if (err) {
        console.error('sea udp SpawnShipStruct client error:', err);
      }
    },
  );
  console.log('Shared session key sent to sea-server.')
};
