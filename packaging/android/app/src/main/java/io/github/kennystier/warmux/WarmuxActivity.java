package io.github.kennystier.warmux;

import org.libsdl.app.SDLActivity;

/** Starts the game: SDL loads libSDL2.so and libmain.so and calls main(). */
public class WarmuxActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "main" };
    }
}
