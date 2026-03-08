package com.vibeogame.chickenpotato;

import android.app.NativeActivity;
import android.content.Context;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.FrameLayout;

public class GameActivity extends NativeActivity {
    private EditText hiddenInput;
    private volatile String keyboardInputBuffer = "";
    private final Object keyboardInputLock = new Object();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setupHiddenInput();
        enableImmersiveMode();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            enableImmersiveMode();
        }
    }

    private void setupHiddenInput() {
        runOnUiThread(() -> {
            if (hiddenInput != null) return;

            View root = getWindow().getDecorView();
            ViewGroup parent = (root instanceof ViewGroup) ? (ViewGroup) root : null;
            if (parent == null) return;

            hiddenInput = new EditText(this);
            hiddenInput.setLayoutParams(new FrameLayout.LayoutParams(1, 1));
            hiddenInput.setAlpha(0.0f);
            hiddenInput.setFocusable(true);
            hiddenInput.setFocusableInTouchMode(true);
            hiddenInput.setSingleLine(true);
            hiddenInput.setImeOptions(android.view.inputmethod.EditorInfo.IME_ACTION_DONE);
            hiddenInput.setInputType(android.text.InputType.TYPE_CLASS_TEXT
                    | android.text.InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS
                    | android.text.InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);

            hiddenInput.addTextChangedListener(new TextWatcher() {
                @Override
                public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

                @Override
                public void onTextChanged(CharSequence s, int start, int before, int count) {
                    if (count > 0) {
                        String newChars = s.subSequence(start, start + count).toString();
                        synchronized (keyboardInputLock) {
                            keyboardInputBuffer += newChars;
                        }
                    }
                }

                @Override
                public void afterTextChanged(Editable s) {}
            });

            parent.addView(hiddenInput);
        });
    }

    public void enableImmersiveMode() {
        runOnUiThread(() -> {
            Window window = getWindow();
            window.addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);

            View decor = window.getDecorView();
            int flags = View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
            decor.setSystemUiVisibility(flags);
        });
    }

    public void showGameKeyboard() {
        runOnUiThread(() -> {
            if (hiddenInput == null) return;

            hiddenInput.setText("");
            hiddenInput.requestFocus();
            InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
            if (imm != null) {
                imm.showSoftInput(hiddenInput, InputMethodManager.SHOW_FORCED);
            }
        });
    }

    public void hideGameKeyboard() {
        runOnUiThread(() -> {
            if (hiddenInput != null) {
                InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
                if (imm != null) {
                    imm.hideSoftInputFromWindow(hiddenInput.getWindowToken(), 0);
                }
                hiddenInput.clearFocus();
            }
            enableImmersiveMode();
        });
    }

    // Called from native thread - thread-safe via synchronized lock
    public String getKeyboardInput() {
        synchronized (keyboardInputLock) {
            String result = keyboardInputBuffer;
            keyboardInputBuffer = "";
            return result;
        }
    }
}
