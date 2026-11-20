package io.github.kennystier.warmux;

import android.content.Context;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Bundle;
import android.view.KeyEvent;
import android.window.OnBackInvokedDispatcher;
import org.libsdl.app.SDLActivity;

/** Starts the game: SDL loads libSDL2.so and libmain.so and calls main(). */
public class WarmuxActivity extends SDLActivity {
    // Many Wi-Fi drivers drop broadcast packets unless an app holds this;
    // a hosted game needs them to hear LAN discovery probes
    private WifiManager.MulticastLock multicastLock;

    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "main" };
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // Android 13+ handles Back itself for apps that target it (predictive
        // back): it no longer reaches onBackPressed() or SDL, and closes the
        // app. Hand it to the game as the Back key instead, which the menus
        // use to close a form or leave a menu, and a battle to open its menu.
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU && !mBrokenLibraries) {
            getOnBackInvokedDispatcher().registerOnBackInvokedCallback(
                OnBackInvokedDispatcher.PRIORITY_DEFAULT, () -> {
                    try {
                        onNativeKeyDown(KeyEvent.KEYCODE_BACK);
                        onNativeKeyUp(KeyEvent.KEYCODE_BACK);
                    } catch (UnsatisfiedLinkError e) {
                        // The game is not running (yet)
                    }
                });
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        try {
            if (multicastLock == null) {
                WifiManager wifi = (WifiManager) getApplicationContext()
                        .getSystemService(Context.WIFI_SERVICE);
                if (wifi != null) {
                    multicastLock = wifi.createMulticastLock("warmux-lan");
                    multicastLock.setReferenceCounted(false);
                }
            }
            if (multicastLock != null)
                multicastLock.acquire();
        } catch (RuntimeException e) {
            // No Wi-Fi: nothing to discover
        }
    }

    @Override
    protected void onPause() {
        if (multicastLock != null && multicastLock.isHeld())
            multicastLock.release();
        super.onPause();
    }
}
