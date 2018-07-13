package com.popsongremix.laidoff;

import android.app.AlertDialog;
import android.app.NativeActivity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.SoundPool;
import android.os.Build;
import android.os.Bundle;
import android.text.InputType;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.widget.EditText;

import com.google.firebase.iid.FirebaseInstanceId;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Locale;

public class LaidoffNativeActivity extends NativeActivity {
    static {
        System.loadLibrary("zmq");
        System.loadLibrary("czmq");
        System.loadLibrary("native-activity");
    }

    public static native String signalResourceReady(Class<LaidoffNativeActivity> and9NativeActivityClass, int downloadAssets);

    public static native int pushTextureData(int width, int height, int[] data, int texAtlasIndex);

    public static native void initRegisterAsset();
    public static native void registerAsset(String assetPath, int startOffset, int length);

    private static native void sendApkPath(String apkPath, String filesPath, String packageVersion);

    public static native void setPushTokenAndSend(String text, long pLwcLong);

    @SuppressWarnings("SameParameterValue")
    public static native void setWindowSize(int width, int height, long pLwcLong);

    public static native void sendInputText(String text);
    public static native void sendChatInputText(String text);

    private static LaidoffNativeActivity INSTANCE;
    public static final String LOG_TAG = "and9";
    private SoundPool mSoundPool;
    private int mSoundCollapse;
    private int mSoundCollision;
    private int mSoundDamage;
    private int mSoundDash1;
    private int mSoundDash2;
    private int mSoundDefeat;
    private int mSoundIntroBgm;
    private int mSoundVictory;
    private int mSoundSwoosh;
    private int mSoundClick;
    private int mSoundReady;
    private int mSoundSteady;
    private int mSoundGo;
    private static MediaPlayer mBgmPlayer;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        INSTANCE = this;
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        // enable immersive mode
        enableImmersiveMode();
        Log.i(LOG_TAG, "Device Android Version: " + Build.VERSION.SDK_INT);
        AssetsLoader assetsLoader = new AssetsLoader(this);
        Log.i(LOG_TAG, "APK Path: " + assetsLoader.GetAPKPath());
        initRegisterAsset();
        assetsLoader.registerAllAssetsOfType("action");
        assetsLoader.registerAllAssetsOfType("armature");
        assetsLoader.registerAllAssetsOfType("atlas");
        assetsLoader.registerAllAssetsOfType("conf");
        assetsLoader.registerAllAssetsOfType("d");
        assetsLoader.registerAllAssetsOfType("fanim");
        assetsLoader.registerAllAssetsOfType("field");
        assetsLoader.registerAllAssetsOfType("fnt");
        assetsLoader.registerAllAssetsOfType("fvbo");
        assetsLoader.registerAllAssetsOfType("glsl");
        assetsLoader.registerAllAssetsOfType("ktx");
        assetsLoader.registerAllAssetsOfType("l");
        assetsLoader.registerAllAssetsOfType("nav");
        assetsLoader.registerAllAssetsOfType("ogg");
        assetsLoader.registerAllAssetsOfType("pkm");
        assetsLoader.registerAllAssetsOfType("svbo");
        assetsLoader.registerAllAssetsOfType("tex");
        assetsLoader.registerAllAssetsOfType("vbo");
        assetsLoader.registerAllAssetsOfType("css");
        assetsLoader.registerAllAssetsOfType("html");
        assetsLoader.registerAllAssetsOfType("cvbo");
        assetsLoader.registerAllAssetsOfType("ttldata");
		assetsLoader.registerAllAssetsOfType("mvbo");
        sendApkPath(assetsLoader.GetAPKPath(), getApplicationContext().getFilesDir().getAbsolutePath(), getPackageVersion());

        //noinspection deprecation
        mSoundPool = new SoundPool(15, AudioManager.STREAM_MUSIC, 0);
        mSoundPool.setOnLoadCompleteListener(new SoundPool.OnLoadCompleteListener() {
            @Override
            public void onLoadComplete(SoundPool soundPool, int sampleId,
                                       int status) {
                //loaded = true;

                //mSoundPool.play(sampleId, 1, 1, 0, 0, 1);
            }
        });
        mSoundCollapse = mSoundPool.load(getApplicationContext(), R.raw.collapse, 1);
        mSoundCollision = mSoundPool.load(getApplicationContext(), R.raw.collision, 1);
        mSoundDamage = mSoundPool.load(getApplicationContext(), R.raw.damage, 1);
        mSoundDash1 = mSoundPool.load(getApplicationContext(), R.raw.dash1, 1);
        mSoundDash2 = mSoundPool.load(getApplicationContext(), R.raw.dash2, 1);
        mSoundDefeat = mSoundPool.load(getApplicationContext(), R.raw.defeat, 1);
        mSoundIntroBgm = mSoundPool.load(getApplicationContext(), R.raw.introbgm, 1);
        mSoundVictory = mSoundPool.load(getApplicationContext(), R.raw.victory, 1);
        mSoundSwoosh = mSoundPool.load(getApplicationContext(), R.raw.swoosh, 1);
        mSoundClick = mSoundPool.load(getApplicationContext(), R.raw.click, 1);
        mSoundReady = mSoundPool.load(getApplicationContext(), R.raw.ready, 1);
        mSoundSteady = mSoundPool.load(getApplicationContext(), R.raw.steady, 1);
        mSoundGo = mSoundPool.load(getApplicationContext(), R.raw.go, 1);

        mBgmPlayer = MediaPlayer.create(getApplicationContext(), R.raw.introbgm);
        mBgmPlayer.setLooping(true);

        if (BuildConfig.DOWNLOAD_ASSETS) {
            // Download latest assets from server
            downloadResFromServer();
        } else {
            // or.. use embedded assets
            UpdateResTask.onResourceLoadFinished(false);
        }
    }

    private void enableImmersiveMode() {
        //getWindow().getDecorView().setKeepScreenOn(true);
        getWindow().addFlags(
                //WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED
                //WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD
                WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON|
                WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON
                //WindowManager.LayoutParams.FLAG_ALLOW_LOCK_WHILE_SCREEN_ON
        );
        final View decorView = getWindow().getDecorView();
        decorView.setOnSystemUiVisibilityChangeListener
                (new View.OnSystemUiVisibilityChangeListener() {
                    @Override
                    public void onSystemUiVisibilityChange(int visibility) {
                        Log.i(LOG_TAG, "onSystemUiVisibilityChange - decorView window size width: " + decorView.getWidth());
                        Log.i(LOG_TAG, "onSystemUiVisibilityChange - decorView window size height: " + decorView.getHeight());
                        setWindowSize(decorView.getWidth(), decorView.getHeight(), 0);
                        // Note that system bars will only be "visible" if none of the
                        // LOW_PROFILE, HIDE_NAVIGATION, or FULLSCREEN flags are set.
//                        if ((visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0) {
//                            // TODO: The system bars are visible. Make any desired
//                            // adjustments to your UI, such as showing the action bar or
//                            // other navigational controls.
//                        } else {
//                            // TODO: The system bars are NOT visible. Make any desired
//                            // adjustments to your UI, such as hiding the action bar or
//                            // other navigational controls.
//                        }
                    }
                });
    }

    public void playCollapse() {
        mSoundPool.play(mSoundCollapse, 1, 1, 0, 0, 1);
    }

    public void playCollision() {
        mSoundPool.play(mSoundCollision, 1, 1, 0, 0, 1);
    }

    public void playDamage() {
        mSoundPool.play(mSoundDamage, 1, 1, 0, 0, 1);
    }

    public void playDash1() {
        mSoundPool.play(mSoundDash1, 1, 1, 0, 0, 1);
    }

    public void playDash2() {
        mSoundPool.play(mSoundDash2, 1, 1, 0, 0, 1);
    }

    public void playDefeat() {
        mSoundPool.play(mSoundDefeat, 1, 1, 0, 0, 1);
    }

    public void playIntroBgm() {
        mSoundPool.play(mSoundIntroBgm, 1, 1, 0, 0, 1);
    }

    public void playVictory() {
        mSoundPool.play(mSoundVictory, 1, 1, 0, 0, 1);
    }

    public void playSwoosh() {
        mSoundPool.play(mSoundSwoosh, 1, 1, 0, 0, 1);
    }

    public void playClick() {
        mSoundPool.play(mSoundClick, 1, 1, 0, 0, 1);
    }

    public void playReady() {
        mSoundPool.play(mSoundReady, 1, 1, 0, 0, 1);
    }

    public void playSteady() {
        mSoundPool.play(mSoundSteady, 1, 1, 0, 0, 1);
    }

    public void playGo() {
        mSoundPool.play(mSoundGo, 1, 1, 0, 0, 1);
    }

    private void downloadResFromServer() {

        File files = getApplicationContext().getFilesDir();

        File[] fileList = files.listFiles();

        Log.i(LOG_TAG, String.format("Download cache dir: %s (%d files)", files.getAbsolutePath(), fileList.length));

        for (File aFileList : fileList) {
            //Date d = new Date(fileList[i].lastModified());
            Log.i(LOG_TAG, String.format(" - file: %s", aFileList.getAbsolutePath()));
        }

        UpdateResTaskParam updateResTaskParam = new UpdateResTaskParam();
        updateResTaskParam.fileAbsolutePath = files.getAbsolutePath();
        updateResTaskParam.remoteAssetsBasePath = getString(R.string.remote_assets_base_path);
        //updateResTaskParam.remoteAssetsBasePath = "http://192.168.0.28:19876/assets";
        updateResTaskParam.remoteApkBasePath = getString(R.string.remote_apk_base_path);
        //updateResTaskParam.remoteAssetsBasePath = "http://222.110.4.119:18080";
        updateResTaskParam.remoteListFilePath = getString(R.string.list_file_name);
        updateResTaskParam.localListFilename = getString(R.string.list_file_name);
        updateResTaskParam.activity = this;

        new UpdateResTask(this).execute(updateResTaskParam);
    }

    @SuppressWarnings("unused")
    static public int loadBitmap(String assetName) {
        return loadBitmapWithIndex(0, assetName);
    }

    static public int loadBitmapWithIndex(@SuppressWarnings("SameParameterValue") int i, String assetName) {
        Bitmap bitmap = getBitmapFromAsset(INSTANCE.getApplicationContext(), assetName);

        Log.i(LOG_TAG, String.format(Locale.US, "Tex(asset name) %s Bitmap width: %d", assetName, bitmap.getWidth()));
        Log.i(LOG_TAG, String.format(Locale.US, "Tex(asset name) %s Bitmap height: %d", assetName, bitmap.getHeight()));

        int[] pixels = new int[bitmap.getWidth() * bitmap.getHeight()];
        bitmap.getPixels(pixels, 0, bitmap.getWidth(), 0, 0, bitmap.getWidth(), bitmap.getHeight());
        int bytes_allocated_on_native = pushTextureData(bitmap.getWidth(), bitmap.getHeight(), pixels, i);

        Log.i(LOG_TAG, String.format(Locale.US, "Tex(asset name) %s Bitmap copied to native side %d bytes", assetName, bytes_allocated_on_native));

        return bitmap.getWidth() * bitmap.getHeight();
    }

    public static Bitmap getBitmapFromAsset(Context context, String filePath) {
        AssetManager assetManager = context.getAssets();

        InputStream inputStr;
        Bitmap bitmap = null;
        try {
            boolean fromDownloaded = true;
            //noinspection ConstantConditions
            if (fromDownloaded) {

                String filenameOnly = filePath.substring(filePath.lastIndexOf("/") + 1);

                File f = new File(context.getFilesDir().getAbsoluteFile(), filenameOnly);

                inputStr = new FileInputStream(f);
            } else {
                inputStr = assetManager.open(filePath);
            }

            bitmap = BitmapFactory.decodeStream(inputStr);
        } catch (IOException e) {
            // handle exception
        }

        return bitmap;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.d(LOG_TAG, "onDestroy()");
    }

    @Override
    public void onPause() {
        super.onPause();
        Log.d(LOG_TAG, "onPause()");
        mBgmPlayer.pause();
    }

    @Override
    public void onResume() {
        super.onResume();
        Log.d(LOG_TAG, "onResume()");
        mBgmPlayer.start();
    }


    @SuppressWarnings("unused")
    public static void startTextInputActivity(String dummy) {
        INSTANCE.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                //Intent intent = new Intent(INSTANCE, TextInputActivity.class);
                //INSTANCE.startActivity(intent);
                INSTANCE.showNicknameInputDialog();
            }
        });
    }

    @SuppressWarnings("unused")
    public static void startChatTextInputActivity(String dummy) {
        INSTANCE.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Intent intent = new Intent(INSTANCE, ChatActivity.class);
                INSTANCE.startActivity(intent);
            }
        });
    }

    @SuppressWarnings("unused")
    public static void startRewardVideo(String dummy) {
//        INSTANCE.runOnUiThread(new Runnable() {
//            @Override
//            public void run() {
//                INSTANCE.loadRewardedVideoAd();
//            }
//        });
    }

    @SuppressWarnings("unused")
    public static void startSignIn(final int v1, final int v2, final int v3, final int v4) {
//        INSTANCE.runOnUiThread(new Runnable() {
//            @Override
//            public void run() {
//                Intent intent = new Intent(INSTANCE, SignInActivity.class);
//                Bundle b = new Bundle();
//                b.putInt("userId[0]", v1);
//                b.putInt("userId[1]", v2);
//                b.putInt("userId[2]", v3);
//                b.putInt("userId[3]", v4);
//                intent.putExtras(b); //Put your id to your next Intent
//                INSTANCE.startActivity(intent);
//            }
//        });
    }

    @SuppressWarnings("unused")
    public static void startSignOut(String dummy) {
//        INSTANCE.runOnUiThread(new Runnable() {
//            @Override
//            public void run() {
//                INSTANCE.signOut();
//            }
//        });
    }

    @SuppressWarnings("unused")
    public static void requestPushToken(long pLwc) {
        setPushTokenAndSend(FirebaseInstanceId.getInstance().getToken(), pLwc);
    }

    @SuppressWarnings("unused")
    public static void startCollapseSound(String dummy) {
        INSTANCE.playCollapse();
    }

    @SuppressWarnings("unused")
    public static void startCollisionSound(String dummy) {
        INSTANCE.playCollision();
    }

    @SuppressWarnings("unused")
    public static void startDamageSound(String dummy) {
        INSTANCE.playDamage();
    }

    @SuppressWarnings("unused")
    public static void startDash1Sound(String dummy) {
        INSTANCE.playDash1();
    }

    @SuppressWarnings("unused")
    public static void startDash2Sound(String dummy) {
        INSTANCE.playDash2();
    }

    @SuppressWarnings("unused")
    public static void startDefeatSound(String dummy) {
        INSTANCE.playDefeat();
    }

    @SuppressWarnings("unused")
    public static void startIntroBgmSound(String dummy) {
        INSTANCE.playIntroBgm();
    }

    @SuppressWarnings("unused")
    public static void startVictorySound(String dummy) {
        INSTANCE.playVictory();
    }

    @SuppressWarnings("unused")
    public static void startSwooshSound(String dummy) {
        INSTANCE.playSwoosh();
    }

    @SuppressWarnings("unused")
    public static void startClickSound(String dummy) {
        INSTANCE.playClick();
    }

    @SuppressWarnings("unused")
    public static void startReadySound(String dummy) {
        INSTANCE.playReady();
    }

    @SuppressWarnings("unused")
    public static void startSteadySound(String dummy) {
        INSTANCE.playSteady();
    }

    @SuppressWarnings("unused")
    public static void startGoSound(String dummy) {
        INSTANCE.playGo();
    }

    @SuppressWarnings("unused")
    public static int getCurrentOrientation(String dummy) {
        return INSTANCE.getResources().getConfiguration().orientation;
    }

    public static String getPackageVersion() {
        String packageVersionName = "0.0.0";
        try {
            PackageInfo packageInfo = INSTANCE.getPackageManager().getPackageInfo(INSTANCE.getPackageName(), 0);
            packageVersionName = packageInfo.versionName;
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
        }
        return packageVersionName;
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        enableImmersiveModeOnWindowFocusChanged(hasFocus);
    }

    private void enableImmersiveModeOnWindowFocusChanged(boolean hasFocus) {
        if (hasFocus) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
                final View decorView = getWindow().getDecorView();
                decorView.setSystemUiVisibility(
                        View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                                | View.SYSTEM_UI_FLAG_LOW_PROFILE
                                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION // hide nav bar
                                | View.SYSTEM_UI_FLAG_FULLSCREEN // hide status bar
                                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                );
                Log.i(LOG_TAG, "onWindowFocusChanged - decorView window width: " + decorView.getWidth());
                Log.i(LOG_TAG, "onWindowFocusChanged - decorView window height: " + decorView.getHeight());
                setWindowSize(decorView.getWidth(), decorView.getHeight(), 0);
            }
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
    }

    public void showNicknameInputDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(getString(R.string.enter_new_nickname));
        // Set up the input
        final EditText input = new EditText(this);
        input.setImeOptions(input.getImeOptions() | EditorInfo.IME_FLAG_NO_EXTRACT_UI | EditorInfo.IME_ACTION_DONE);
        // Specify the type of input expected; this, for example, sets the input as a password, and will mask the text
        input.setInputType(InputType.TYPE_CLASS_TEXT);
        //input.requestFocus();
        input.clearFocus();
        builder.setView(input);
        // Set up the buttons
        builder.setPositiveButton(getString(R.string.change_nickname), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                sendInputText(input.getText().toString());
            }
        });
        builder.show();
    }
}
