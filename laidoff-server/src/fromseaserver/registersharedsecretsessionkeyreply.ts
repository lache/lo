import { AddressInfo } from 'net';
import { RegisterSharedSecretSessionKeyReplyStruct } from '../message';
import { notifyWaiter } from '../replywaiter';

export const receive = (buf: any, remote: AddressInfo) => {
  // RegisterSharedSecretSessionKeyReplyStruct
  console.log(
    `RegisterSharedSecretSessionKeyReplyStruct from ${remote.address}:${remote.port} (len=${
      buf.length
      })`,
  );
  RegisterSharedSecretSessionKeyReplyStruct._setBuff(buf);
  notifyWaiter(RegisterSharedSecretSessionKeyReplyStruct);
};
