package com.sirbilliams.tinkerbell;

import android.os.Bundle;
import android.system.ErrnoException;
import android.system.Os;
import android.view.WindowManager;

import org.libsdl.app.SDLActivity;

/** Hosts the game: SDLActivity loads libSDL2.so and libtinkerbell.so, then calls SDL_main (platform/core.cpp). */
public class GameActivity extends SDLActivity {
    /** Intent extra: the install folder (the copy of the disc's setup/game). */
    static final String EXTRA_GAME_DIR = "game_dir";

    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "tinkerbell" };
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // The native side reads the install folder from this variable (platform/core.cpp), so it must be set
        // before SDL starts the game thread.
        String dir = getIntent() != null ? getIntent().getStringExtra(EXTRA_GAME_DIR) : null;
        try {
            if (dir != null) Os.setenv("TB_ANDROID_GAME_DIR", dir, true);
        } catch (ErrnoException e) {
            // the native side then logs "game dir ." and fails to find the exe
        }
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }
}
