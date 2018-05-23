package com.popsongremix.laidoff;

import android.os.AsyncTask;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;

class DownloadTask extends AsyncTask<DownloadTaskParams, Void, GetFileResult> {
    private DownloadTaskParams dtp;

    private static String convertStreamToString(InputStream is) throws Exception {
        BufferedReader reader = new BufferedReader(new InputStreamReader(is));
        StringBuilder sb = new StringBuilder();
        String line;
        while (null != (line = reader.readLine())) {
            sb.append(line).append("\n");
        }
        reader.close();
        return sb.toString();
    }

    static String getStringFromFile(File fl) throws Exception {
        FileInputStream fin = new FileInputStream(fl);
        String ret = convertStreamToString(fin);
        //Make sure you close all streams.
        fin.close();
        return ret;
    }

    static GetFileResult getFile(String fileAbsolutePath, String remotePath, String localFilename, boolean writeEtag) throws Exception {
        //set the download URL, a url that points to a file on the internet
        //this is the file to be downloaded
        URL url = new URL(remotePath);

        //create the new connection
        HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();

        //set up some things on the connection
        urlConnection.setRequestMethod("HEAD");
        urlConnection.setDoOutput(false);
        urlConnection.connect();
        String etagServer = urlConnection.getHeaderField("Etag").trim();
            Log.i(LaidoffNativeActivity.LOG_TAG, String.format("%s - server Etag: %s", remotePath, etagServer));

        File etagFile = new File(fileAbsolutePath, localFilename + ".Etag");

        boolean doDownload = false;
        if (etagFile.exists()) {
            String etagLocal = DownloadTask.getStringFromFile(etagFile).trim();
            if (etagLocal.compareTo(etagServer) != 0) {
                doDownload = true;
            }
        } else {
            doDownload = true;
        }

        //set the path where we want to save the file
        //in this case, going to save it on the root directory of the
        //sd card.

        //create a new file, specifying the path, and the filename
        //which we want to save the file as.
        File file = new File(fileAbsolutePath, localFilename);

        if (doDownload) {
            urlConnection = (HttpURLConnection) url.openConnection();

            //set up some things on the connection
            urlConnection.setRequestMethod("GET");
            urlConnection.setDoOutput(false);
            urlConnection.connect();

            //this will be used to write the downloaded data into the file we created
            FileOutputStream fileOutput = new FileOutputStream(file);

            //this will be used in reading the data from the internet
            InputStream inputStream = urlConnection.getInputStream();

            //this is the total size of the file
            int totalSize = urlConnection.getContentLength();
            //variable to store total downloaded bytes
            int downloadedSize = 0;

            //create a buffer...
            byte[] buffer = new byte[1024];
            int bufferLength; //used to store a temporary size of the buffer

            //now, read through the input buffer and write the contents to the file
            while ( (bufferLength = inputStream.read(buffer)) > 0 ) {
                //add the data in the buffer to the file in the file output stream (the file on the sd card
                fileOutput.write(buffer, 0, bufferLength);
                //add up the size so we know how much is downloaded
                downloadedSize += bufferLength;
                //this is where you would do something to report the progress, like this maybe
                updateProgress(file.getAbsolutePath(), downloadedSize, totalSize);

            }
            //close the output stream when done
            fileOutput.close();

            // Write etag file
            if (writeEtag) {
                writeEtagToFile(etagServer, etagFile);
            }
        }

        GetFileResult gfr = new GetFileResult();
        gfr.file = file;
        gfr.etag = etagServer;
        gfr.newlyDownloaded = doDownload;
        return gfr;
    }

    static void writeEtagToFile(String etag, File file) throws IOException {
        FileOutputStream etagFileOutput = new FileOutputStream(file);
        //noinspection TryFinallyCanBeTryWithResources
        try {
            etagFileOutput.write(etag.getBytes());
        } finally {
            etagFileOutput.close();
        }
    }

    @Override
    protected GetFileResult doInBackground(DownloadTaskParams... params) {
        try {
            dtp = params[0];
            return getFile(dtp.fileAbsolutePath, dtp.remotePath, dtp.localFilename, dtp.writeEtag);
        } catch (Exception e) {
            e.printStackTrace();
        }

        return null;
    }

    @Override
    protected void onPostExecute(GetFileResult result) {
        // TODO: check this.exception
        // TODO: do something with the result

        writeEtag();
    }

    private void writeEtag() {
        if (dtp.sequenceNumber.incrementAndGet() == dtp.totalSequenceNumber) {
            Log.i(LaidoffNativeActivity.LOG_TAG, "Resource update finished.");
            dtp.urt.writeListEtag();
            dtp.urt.onResourceLoadFinished(true);
        }
    }

    @SuppressWarnings("UnusedParameters")
    private static void updateProgress(String filename, int downloadedSize, int totalSize) {
        //Log.i(LaidoffNativeActivity.LOG_TAG, String.format("Downloading %s... (%d/%d bytes)", filename, downloadedSize, totalSize));
    }
}