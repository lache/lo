package com.popsongremix.laidoff;

import android.app.Activity;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetFileDescriptor;
import android.util.Log;

import java.io.IOException;

/**
 * Created by gasbank on 2017-03-26.
 */

public class AssetsLoader {

    private Activity activity;

    public AssetsLoader(Activity activity) {
        this.activity = activity;
    }

    int GetAssetOffset(String filename) {
        int filesize=0;
        try {
            android.content.res.AssetFileDescriptor fd = activity.getAssets().openFd(filename);

            filesize=(int)fd.getStartOffset();
        } catch(java.io.IOException e) {
            Log.e("someTag", "IOException");
        }
        return filesize;
    }

    String GetAPKPath() {

        String PathToAPK;
        ApplicationInfo appInfo = null;
        PackageManager packMgmr = activity.getPackageManager();
        try {
            appInfo = packMgmr.getApplicationInfo("com.popsongremix.laidoff", 0);
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
            throw new RuntimeException("Unable to locate APK...");
        }

        PathToAPK = appInfo.sourceDir;

        return PathToAPK; // this.getPackageResourcePath();
    }

    public void registerAllAssetsOfType(String assetType) {
        try {
            for (String assetSubpath : activity.getAssets().list(assetType)) {

                String assetPath = assetType + "/" + assetSubpath;

                Log.i("And9", "Registering asset " + assetPath);

                try {
                    AssetFileDescriptor afd = activity.getAssets().openFd(assetPath);

                    LaidoffNativeActivity.registerAsset(assetPath, (int) afd.getStartOffset(), (int) afd.getLength());
                } catch (IOException e) {
                    // may 'assetPath' directs a directory(should directs a file)
                    e.printStackTrace();
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
