package com.popsongremix.laidoff;

import android.content.Intent;
import android.os.IBinder;
import androidx.annotation.Nullable;

public class LaidoffFirebaseInstanceIDService extends android.app.Service {
    public static native void setPushToken(String text);

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
//
//    @Override
//    public void onTokenRefresh() {
//        // Get updated InstanceID token.
//        String refreshedToken = FirebaseInstanceId.getInstance().getToken();
//        Log.d(LaidoffNativeActivity.LOG_TAG, "Refreshed token: " + refreshedToken);
//        setPushToken(refreshedToken);
//
//        // If you want to send messages to this application instance or
//        // manage this apps subscriptions on the server side, send the
//        // Instance ID token to your app server.
//        //sendRegistrationToServer(refreshedToken);
//    }
}
