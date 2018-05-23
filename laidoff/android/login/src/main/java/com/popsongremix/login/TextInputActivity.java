package com.popsongremix.login;

import android.app.Activity;
import android.content.Context;

import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.AdView;

import java.nio.ByteBuffer;
import java.nio.charset.Charset;

/**
 * A login screen that offers login via email/password.
 */
public class TextInputActivity extends Activity {

    private EditText mInputText;
    private Button mConfirmButton;

    //public static native void sendInputText(String text);
    private AdView mAdView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //this.requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.activity_text_input);
        // AdMob
//        mAdView = (AdView) findViewById(R.id.adView);
//        AdRequest adRequest = new AdRequest.Builder().build();
//        mAdView.loadAd(adRequest);

        mInputText = (EditText) findViewById(R.id.input_text);
        mInputText.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView textView, int id, KeyEvent keyEvent) {
                if (id == EditorInfo.IME_ACTION_DONE || id == EditorInfo.IME_NULL) {
                    confirmInputText();
                    return true;
                }
                return false;
            }
        });
        mInputText.requestFocus();
        InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
        if (imm != null) {
            imm.showSoftInput(mInputText, InputMethodManager.SHOW_FORCED);
        }

        mConfirmButton = (Button)findViewById(R.id.confirm_button);
        mConfirmButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                confirmInputText();
            }
        });
    }

    private void confirmInputText() {
        //sendInputText(mInputText.getText().toString());
        finish();
    }
}

