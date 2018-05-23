package com.popsongremix.login;

import android.os.AsyncTask;
import android.util.Log;

import com.google.gson.Gson;

import java.io.IOException;

import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

public class AuthTask extends AsyncTask<AuthTask.AuthTaskParam, Void, String> {
    public static final String LOG_TAG = "SignIn";

    public static class AuthRequestBody {
        public int v1;
        public int v2;
        public int v3;
        public int v4;
        public String idToken;
    }

    public static class AuthTaskParam {
        public OkHttpClient client;
        String url;
        public AuthRequestBody body;
    }

    public static final MediaType JSON
            = MediaType.parse("application/json; charset=utf-8");
    private static final MediaType TEXT_PLAIN
            = MediaType.parse("text/plain; charset=utf-8");

    @Override
    protected String doInBackground(AuthTaskParam... authTaskParams) {
        Gson gson = new Gson();
        String bodyJsonStr = gson.toJson(authTaskParams[0].body);
        RequestBody body = RequestBody.create(JSON, bodyJsonStr);
        Request request = new Request.Builder()
                .url(authTaskParams[0].url)
                .post(body)
                .build();
        Response response = null;
        try {
            response = authTaskParams[0].client.newCall(request).execute();
        } catch (IOException e) {
            e.printStackTrace();
        }
        try {
            if (response != null && response.body() != null) {
                String result = response.body().string();
                Log.i(LOG_TAG, "AuthTask result: " + result);
                return result;
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return "";
    }
}
