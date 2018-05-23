package com.popsongremix.laidoff;

import java.util.concurrent.atomic.AtomicLong;

class DownloadTaskParams {
    String fileAbsolutePath;
    String remotePath;
    String localFilename;
    boolean writeEtag;
    AtomicLong sequenceNumber;
    long totalSequenceNumber;
    UpdateResTask urt;
}
