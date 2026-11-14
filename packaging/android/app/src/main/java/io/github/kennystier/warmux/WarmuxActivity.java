package io.github.kennystier.warmux;

import android.content.Context;
import android.net.wifi.WifiManager;
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
