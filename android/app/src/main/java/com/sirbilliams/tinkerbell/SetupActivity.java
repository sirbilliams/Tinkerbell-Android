package com.sirbilliams.tinkerbell;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.RandomAccessFile;
import java.nio.channels.FileChannel;

/**
 * Launcher. Installs the game from the player's own disc image (ISO 9660 with Joliet names): copies the
 * disc's setup/game folder into the app's files folder, checks it, and starts the game.
 */
public class SetupActivity extends Activity {
    private static final int REQ_ISO = 1;
    private static final String DISC_FOLDER = "setup/game";

    private final Handler ui = new Handler(Looper.getMainLooper());
    private TextView status;
    private ProgressBar bar;
    private Button play, install, share;
    private volatile boolean busy, cancel;

    private File gameDir() {
        File base = getExternalFilesDir(null);
        if (base == null) base = getFilesDir();
        return new File(base, "game");
    }

    @Override
    protected void onCreate(Bundle b) {
        super.onCreate(b);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setGravity(Gravity.CENTER);
        root.setPadding(48, 24, 48, 24);
        root.setBackgroundColor(Color.rgb(11, 46, 42));

        TextView title = new TextView(this);
        title.setText("Tinker Bell");
        title.setTextSize(34);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        title.setTextColor(Color.rgb(255, 226, 122));
        title.setGravity(Gravity.CENTER);
        root.addView(title);

        status = new TextView(this);
        status.setTextSize(16);
        status.setTextColor(Color.WHITE);
        status.setGravity(Gravity.CENTER);
        status.setPadding(0, 16, 0, 16);
        root.addView(status);

        bar = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
        bar.setMax(1000);
        bar.setVisibility(View.GONE);
        root.addView(bar, new LinearLayout.LayoutParams(900, LinearLayout.LayoutParams.WRAP_CONTENT));

        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER);
        play = button("Play", v -> play());
        install = button("Install from ISO", v -> pickIso());
        share = button("Share log", v -> shareLog());
        row.addView(play);
        row.addView(install);
        row.addView(share);
        root.addView(row);
        setContentView(root);
        refresh();
    }

    private Button button(String text, View.OnClickListener l) {
        Button x = new Button(this);
        x.setText(text);
        x.setOnClickListener(l);
        return x;
    }

    private void refresh() {
        boolean ok = IsoImage.installOk(gameDir());
        status.setText(ok ? "The game is installed.\nPress Play."
                : "Install the game from your own Tinker Bell disc image (.iso).\n"
                + "Choose the ISO; its setup/game folder is copied into the app.");
        play.setEnabled(ok && !busy);
        install.setEnabled(!busy);
        install.setText(ok ? "Reinstall from ISO" : "Install from ISO");
        share.setEnabled(!busy);
    }

    private void play() {
        Intent i = new Intent(this, GameActivity.class);
        i.putExtra(GameActivity.EXTRA_GAME_DIR, gameDir().getPath());
        startActivity(i);
    }

    private void pickIso() {
        Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        i.addCategory(Intent.CATEGORY_OPENABLE);
        i.setType("*/*");
        startActivityForResult(i, REQ_ISO);
    }

    @Override
    protected void onActivityResult(int req, int res, Intent data) {
        super.onActivityResult(req, res, data);
        if (req == REQ_ISO && res == RESULT_OK && data != null && data.getData() != null) install(data.getData());
    }

    private void install(final Uri uri) {
        busy = true;
        cancel = false;
        bar.setVisibility(View.VISIBLE);
        bar.setProgress(0);
        status.setText("Reading the disc image...");
        refresh();
        new Thread(() -> {
            String err = null;
            try {
                err = doInstall(uri);
            } catch (Throwable t) {
                err = "Install failed: " + t;
            }
            final String e = err;
            ui.post(() -> {
                busy = false;
                bar.setVisibility(View.GONE);
                refresh();
                if (e != null) status.setText(e);
            });
        }, "install").start();
    }

    /** Returns null on success, else the message for the player. */
    private String doInstall(Uri uri) throws IOException {
        try (ParcelFileDescriptor pfd = getContentResolver().openFileDescriptor(uri, "r");
             FileInputStream in = new FileInputStream(pfd.getFileDescriptor())) {
            FileChannel ch = in.getChannel();
            if (!IsoImage.looksLikeIso(ch)) return "That file is not a CD image (ISO 9660).";
            IsoImage iso = new IsoImage(ch);
            IsoImage.Entry src = iso.find(DISC_FOLDER);
            if (src == null) return "This disc has no setup/game folder: is it the Tinker Bell disc?";
            final long total = Math.max(1, iso.totalSize(src));

            File dest = gameDir(), tmp = new File(dest.getParentFile(), "game.tmp");
            deleteTree(tmp);
            tmp.mkdirs();
            long[] done = new long[1];
            final long[] lastUi = new long[1];
            boolean cancelled = iso.copyTree(src, "", rel -> {
                File f = new File(tmp, rel);
                f.getParentFile().mkdirs();
                return new BufferedOutputStream(new FileOutputStream(f), 1 << 16);
            }, n -> {
                if (n - lastUi[0] > (1 << 20) || n == total) {
                    lastUi[0] = n;
                    final int p = (int) (n * 1000 / total);
                    final String s = "Copying game files... " + (n >> 20) + " / " + (total >> 20) + " MB";
                    ui.post(() -> { bar.setProgress(p); status.setText(s); });
                }
                return cancel;
            }, done);
            if (cancelled) { deleteTree(tmp); return "Cancelled."; }
            if (!IsoImage.installOk(tmp)) {
                deleteTree(tmp);
                return "The copy is missing \"Tinker Bell.exe\", data or audio: is this the right disc?";
            }
            // keep saves from an earlier install
            File oldSave = new File(dest, "save");
            deleteTreeExcept(dest, oldSave);
            if (oldSave.isDirectory()) oldSave.renameTo(new File(tmp, "save"));
            deleteTree(dest);
            if (!tmp.renameTo(dest)) return "Could not move the installed files into place.";
        }
        return null;
    }

    private static void deleteTreeExcept(File dir, File keep) {
        File[] fs = dir.listFiles();
        if (fs == null) return;
        for (File f : fs) if (!f.equals(keep)) deleteTree(f);
    }

    private static void deleteTree(File f) {
        File[] fs = f.listFiles();
        if (fs != null) for (File c : fs) deleteTree(c);
        f.delete();
    }

    /** Sends the end of the game's log (tinkerbell.log in the install folder, see tb_log_path()). */
    private void shareLog() {
        File log = new File(gameDir(), "tinkerbell.log");
        if (!log.isFile()) { status.setText("No log yet: play the game first."); return; }
        String text;
        try (RandomAccessFile f = new RandomAccessFile(log, "r")) {
            long len = f.length(), keep = Math.min(len, 200 * 1024);
            byte[] buf = new byte[(int) keep];
            f.seek(len - keep);
            f.readFully(buf);
            text = new String(buf, "UTF-8");
        } catch (IOException e) {
            status.setText("Could not read the log: " + e);
            return;
        }
        Intent i = new Intent(Intent.ACTION_SEND);
        i.setType("text/plain");
        i.putExtra(Intent.EXTRA_SUBJECT, "Tinker Bell log");
        i.putExtra(Intent.EXTRA_TEXT, text);
        startActivity(Intent.createChooser(i, "Share log"));
    }

    @Override
    public void onBackPressed() {
        if (busy) cancel = true; else super.onBackPressed();
    }
}
