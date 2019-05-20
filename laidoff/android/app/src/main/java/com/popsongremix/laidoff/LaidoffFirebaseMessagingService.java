package com.popsongremix.laidoff;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.widget.Toast;

import com.google.firebase.messaging.FirebaseMessagingService;
import com.google.firebase.messaging.RemoteMessage;

import java.util.Map;

public class LaidoffFirebaseMessagingService extends FirebaseMessagingService {
    @Override
    public void onMessageReceived(RemoteMessage remoteMessage) {
        // ************* This handler only called when the app is in "FOREGROUND". *************

        // TODO(developer): Handle FCM messages here.
        // Not getting messages here? See why this may be: https://goo.gl/39bRNJ
        Log.d(LaidoffNativeActivity.LOG_TAG, "From: " + remoteMessage.getFrom());

        // Check if message contains a data payload.
        if (remoteMessage.getData().size() > 0) {
            Log.d(LaidoffNativeActivity.LOG_TAG, "Message data payload: " + remoteMessage.getData());

            if (/* Check if data needs to be processed by long running job */ true) {
                // For long-running tasks (10 seconds or more) use Firebase Job Dispatcher.
                //scheduleJob();
            } else {
                // Handle message within 10 seconds
                //handleNow();
            }
            Map<String, String> remoteData = remoteMessage.getData();
            if (remoteData.containsKey("msg")) {
                // The line below throws 'Not an UI thread' exception!
                //Toast.makeText(this, remoteData.get("msg"), Toast.LENGTH_SHORT).show();
            }
        }

        // Check if message contains a notification payload.
        if (remoteMessage.getNotification() != null) {
            Log.d(LaidoffNativeActivity.LOG_TAG, "Message Notification Body: " + remoteMessage.getNotification().getBody());
            Handler handler = new Handler(Looper.getMainLooper());
            final String notificationBody = remoteMessage.getNotification().getBody();
            handler.post(new Runnable() {
                public void run() {
                    Toast.makeText(getApplicationContext(), notificationBody, Toast.LENGTH_SHORT).show();
                }
            });
        }

        // Also if you intend on generating your own notifications as a result of a received FCM
        // message, here is where that should be initiated. See sendNotification method below.
    }

    @Override
    public void onNewToken(String s) {
        super.onNewToken(s);

        Log.d(LaidoffNativeActivity.LOG_TAG, "Refreshed token: " + s);
        LaidoffFirebaseInstanceIDService.setPushToken(s);
    }
}
