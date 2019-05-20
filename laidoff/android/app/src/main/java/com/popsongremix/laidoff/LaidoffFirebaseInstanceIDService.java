package com.popsongremix.laidoff;

public class LaidoffFirebaseInstanceIDService {
    public static native void setPushToken(String text);
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
