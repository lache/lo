package com.popsongremix.login;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.util.Log;
import android.widget.Toast;

import com.google.android.gms.auth.api.Auth;
import com.google.android.gms.auth.api.signin.GoogleSignIn;
import com.google.android.gms.auth.api.signin.GoogleSignInAccount;
import com.google.android.gms.auth.api.signin.GoogleSignInClient;
import com.google.android.gms.auth.api.signin.GoogleSignInOptions;
import com.google.android.gms.auth.api.signin.GoogleSignInResult;
import com.google.android.gms.common.api.ApiException;
import com.google.android.gms.common.api.CommonStatusCodes;
import com.google.android.gms.games.Games;
import com.google.android.gms.games.Player;
import com.google.android.gms.games.PlayersClient;
import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.Task;
import com.google.firebase.auth.AuthCredential;
import com.google.firebase.auth.AuthResult;
import com.google.firebase.auth.FirebaseAuth;
import com.google.firebase.auth.GoogleAuthProvider;

import java.io.IOException;
import java.io.InputStream;
import java.security.KeyManagementException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.security.UnrecoverableKeyException;
import java.security.cert.CertificateException;

import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManager;
import javax.net.ssl.TrustManagerFactory;
import javax.net.ssl.X509TrustManager;

import okhttp3.OkHttpClient;

public class SignInActivity extends Activity {
    static int RC_SIGN_IN = 20000;
    public static final String LOG_TAG = "SignIn";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        startSignIn(this);
    }

    public static void startSignIn(Activity activity) {
        GoogleSignInOptions googleSignInOptions = new GoogleSignInOptions.Builder(GoogleSignInOptions.DEFAULT_GAMES_SIGN_IN)
                //.requestIdToken(getString(R.string.web_client_id)) // require additional consent step
                //.requestEmail() // require additional consent step
                //.requestProfile() // require additional consent step
                //.requestServerAuthCode(getString(R.string.web_client_id)) // require additional consent step
                .build();
        signInSilently(activity, googleSignInOptions);
//        if (!isSignedIn()) {
//
//        } else {
//            onSignedInAccountAcquired(GoogleSignIn.getLastSignedInAccount(this));
//        }
    }

    private static void signInSilently(final Activity activity, final GoogleSignInOptions googleSignInOptions) {
        GoogleSignInClient signInClient = GoogleSignIn.getClient(activity, googleSignInOptions);
        signInClient.silentSignIn().addOnCompleteListener(activity,
                new OnCompleteListener<GoogleSignInAccount>() {
                    @Override
                    public void onComplete(@NonNull Task<GoogleSignInAccount> task) {
                        if (task.isSuccessful()) {
                            // The signed in account is stored in the task's result.
                            GoogleSignInAccount signedInAccount = task.getResult();
                            //Log.i(LOG_TAG, signedInAccount.getDisplayName());
                            onSignedInAccountAcquired(activity, signedInAccount);
                        } else {
                            // Player will need to sign-in explicitly using via UI
                            ApiException e = (ApiException)task.getException();
                            if (e != null) {
                                if (e.getStatusCode() == CommonStatusCodes.SIGN_IN_REQUIRED) {
                                    Log.i(LOG_TAG, "Silent sign-in failed. Try to start user interaction sign-in activity...");
                                    startSignInIntent(activity, googleSignInOptions);
                                } else {
                                    Log.e(LOG_TAG, String.format("Silent sign-in failed with unknown status code %d", e.getStatusCode()));
                                }
                            } else {
                                Log.e(LOG_TAG, "Silent sign-in failed. ApiException null error.");
                            }
                        }
                    }
                });
    }

    private static void startSignInIntent(Activity activity, GoogleSignInOptions googleSignInOptions) {
        GoogleSignInClient signInClient = GoogleSignIn.getClient(activity, googleSignInOptions);
        Intent intent = signInClient.getSignInIntent();
        activity.startActivityForResult(intent, RC_SIGN_IN);
    }

//    private boolean isSignedIn() {
//        return GoogleSignIn.getLastSignedInAccount(this) != null;
//    }

    @Override
    protected void onResume() {
        super.onResume();
        //signInSilently(googleSignInOptions);
    }

    @Override
    public void onStart() {
        super.onStart();
        // Check for existing Google Sign In account, if the user is already signed in
        // the GoogleSignInAccount will be non-null.
        GoogleSignInAccount account = GoogleSignIn.getLastSignedInAccount(this);
        if (account != null) {
            printGoogleAccountDebugInfo(account);
        } else {
            Log.i(LOG_TAG, "onStart(): Google account not available.");
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        onSignInActivityResult(this, requestCode, data);
    }

    public static void onSignInActivityResult(Activity activity, int requestCode, Intent data) {
        if (requestCode == RC_SIGN_IN) {
            GoogleSignInResult result = Auth.GoogleSignInApi.getSignInResultFromIntent(data);
            if (result.isSuccess()) {
                // The signed in account is stored in the result.
                GoogleSignInAccount signedInAccount = result.getSignInAccount();
                onSignedInAccountAcquired(activity, signedInAccount);
            } else {
                String message = result.getStatus().getStatusMessage();
                if (message == null || message.isEmpty()) {
                    message = "Unknown sign-in error!";
                }
                new AlertDialog.Builder(activity).setMessage(message)
                        .setNeutralButton(android.R.string.ok, null).show();
            }
        }
    }

    private static void onSignedInAccountAcquired(final Activity activity, final GoogleSignInAccount signedInAccount) {
        Log.i(LOG_TAG, "Account Display Name: " + signedInAccount.getDisplayName());
        Log.i(LOG_TAG, "Account Photo URL: " + signedInAccount.getPhotoUrl());
        Log.i(LOG_TAG, "Account Email: " + signedInAccount.getEmail());
        Log.i(LOG_TAG, "Account Family Name: " + signedInAccount.getFamilyName());
        Log.i(LOG_TAG, "Account Given Name: " + signedInAccount.getGivenName());
        Log.i(LOG_TAG, "Account ID Token: " + signedInAccount.getIdToken());
        Log.i(LOG_TAG, "Account ID: " + signedInAccount.getId());
        Log.i(LOG_TAG, "Account Server Auth Code: " + signedInAccount.getServerAuthCode());
        if (signedInAccount.getIdToken() != null) {
            sendIdTokenSecurely(activity, signedInAccount.getIdToken());
            AuthCredential credential = GoogleAuthProvider.getCredential(signedInAccount.getIdToken(),null);
            // Signed-in successfully. Register this account to firebase.
            FirebaseAuth.getInstance().signInWithCredential(credential)
                    .addOnCompleteListener(
                            new OnCompleteListener<AuthResult>() {
                                @Override
                                public void onComplete(@NonNull Task<AuthResult> task) {
                                    // check task.isSuccessful()
                                    if (task.isSuccessful()) {
                                        Log.i(LOG_TAG, "Registered to firebase successfully.");
                                        Toast.makeText(activity, "Registered to firebase successfully.", Toast.LENGTH_LONG).show();
                                    } else if (task.getException() != null) {
                                        Log.e(LOG_TAG, "Registered to firebase failed with error: " + task.getException().getMessage());
                                    }
                                    getGamePlayer(activity, signedInAccount);
                                }
                            });
        } else {
            getGamePlayer(activity, signedInAccount);
        }
    }

    private static void getGamePlayer(final Activity activity, GoogleSignInAccount signedInAccount) {
        // Get Google Play Games profile info
        PlayersClient pc = Games.getPlayersClient(activity, signedInAccount);
        pc.getCurrentPlayer().addOnCompleteListener(activity,
                new OnCompleteListener<Player>() {
                    @Override
                    public void onComplete(@NonNull Task<Player> task) {
                        if (task.isSuccessful()) {
                            Player player = task.getResult();
                            Log.i(LOG_TAG, "Player Name: " + player.getName());
                            Log.i(LOG_TAG, "Player Display Name: " + player.getDisplayName());
                            Log.i(LOG_TAG, "Player ID: " + player.getPlayerId());
                            Log.i(LOG_TAG, "Player Title: " + player.getTitle());
                            Log.i(LOG_TAG, "Player Hi Res Image URI: " + player.getHiResImageUri());
                            Log.i(LOG_TAG, "Player Icon Image URI: " + player.getIconImageUri());
                            Log.i(LOG_TAG, "Player Banner Image Landscape URI: " + player.getBannerImageLandscapeUri());
                            Log.i(LOG_TAG, "Player Banner Image Portrait URI: " + player.getBannerImagePortraitUri());
                            Log.i(LOG_TAG, "Player Has Icon Image: " + player.hasIconImage());


                        } else {
                            Log.e(LOG_TAG, "getCurrentPlayer() error: " + task.toString());
                        }
                    }
                });
    }

    private void printGoogleAccountDebugInfo(GoogleSignInAccount account) {
        Log.i(LOG_TAG, "handleSignInResult - Google account name: " + account.getDisplayName());
        Log.i(LOG_TAG, "handleSignInResult - Google Photo URL: " + account.getPhotoUrl());
    }

    private static void sendIdTokenSecurely(Activity activity, String idToken) {
        KeyStore keyStore = readKeyStore(activity); //your method to obtain KeyStore
        SSLContext sslContext = null;
        try {
            sslContext = SSLContext.getInstance("SSL");
        } catch (NoSuchAlgorithmException e) {
            e.printStackTrace();
        }
        TrustManagerFactory trustManagerFactory = null;
        try {
            trustManagerFactory = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
        } catch (NoSuchAlgorithmException e) {
            e.printStackTrace();
        }
        try {
            if (trustManagerFactory != null) {
                trustManagerFactory.init(keyStore);
            }
        } catch (KeyStoreException e) {
            e.printStackTrace();
        }
        KeyManagerFactory keyManagerFactory = null;
        try {
            keyManagerFactory = KeyManagerFactory.getInstance(KeyManagerFactory.getDefaultAlgorithm());
        } catch (NoSuchAlgorithmException e) {
            e.printStackTrace();
        }
        try {
            if (keyManagerFactory != null) {
                keyManagerFactory.init(keyStore, activity.getString(R.string.keystore_password).toCharArray());
            }
        } catch (KeyStoreException e) {
            e.printStackTrace();
        } catch (NoSuchAlgorithmException e) {
            e.printStackTrace();
        } catch (UnrecoverableKeyException e) {
            e.printStackTrace();
        }
        try {
            if (sslContext != null && keyManagerFactory != null && trustManagerFactory != null) {
                sslContext.init(keyManagerFactory.getKeyManagers(),trustManagerFactory.getTrustManagers(), new SecureRandom());
            }
        } catch (KeyManagementException e) {
            e.printStackTrace();
        }

        TrustManager[] tms = new TrustManager[0];
        if (trustManagerFactory != null) {
            tms = trustManagerFactory.getTrustManagers();
        }
        X509TrustManager x509TrustManager = (X509TrustManager)tms[0];
        OkHttpClient client = null;
        if (sslContext != null) {
            client = new OkHttpClient.Builder()
                    .sslSocketFactory(sslContext.getSocketFactory(), x509TrustManager)
                    .build();
        }
        Bundle b = activity.getIntent().getExtras();
        AuthTask.AuthRequestBody authRequestBody = new AuthTask.AuthRequestBody();
        if(b != null) {
            authRequestBody.v1 = b.getInt("userId[0]");
            authRequestBody.v2 = b.getInt("userId[1]");
            authRequestBody.v3 = b.getInt("userId[2]");
            authRequestBody.v4 = b.getInt("userId[3]");
        }
        authRequestBody.idToken = idToken;
        postAsync(client, activity.getString(R.string.google_auth_service_addr), authRequestBody);
    }

    private static void postAsync(OkHttpClient client, String url, AuthTask.AuthRequestBody authRequestBody) {
        AuthTask.AuthTaskParam authTaskParam = new AuthTask.AuthTaskParam();
        authTaskParam.client = client;
        authTaskParam.url = url;
        authTaskParam.body = authRequestBody;
        new AuthTask().execute(authTaskParam);
    }

    static KeyStore readKeyStore(Activity activity) {
        KeyStore ks = null;
        try {
            ks = KeyStore.getInstance(KeyStore.getDefaultType());
        } catch (KeyStoreException e) {
            e.printStackTrace();
        }

        InputStream fis = null;
        try {
            fis = activity.getResources().openRawResource(R.raw.popsongremix_com);
            try {
                assert ks != null;
                ks.load(fis, activity.getString(R.string.keystore_password).toCharArray());
            } catch (IOException e) {
                e.printStackTrace();
            } catch (NoSuchAlgorithmException e) {
                e.printStackTrace();
            } catch (CertificateException e) {
                e.printStackTrace();
            }
        } finally {
            if (fis != null) {
                try {
                    fis.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return ks;
    }
}
