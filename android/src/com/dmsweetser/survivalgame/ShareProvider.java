package com.dmsweetser.survivalgame;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;

import java.io.File;
import java.io.FileNotFoundException;

// Serves the one share-card PNG from the app cache to whichever app the
// player shares to. (No AndroidX FileProvider in this hand-built APK,
// so this tiny provider does the same job.)
// NOTE: this class MUST stay public and in its own file — Android
// instantiates providers reflectively at process start, and a
// package-private provider class crashes the app on launch
// (IllegalAccessException). That bug shipped once (Oct 29) — the
// class originally sat package-private inside MainActivity.java.
public class ShareProvider extends ContentProvider {
    @Override
    public boolean onCreate() { return true; }

    @Override
    public ParcelFileDescriptor openFile(Uri uri, String mode) throws FileNotFoundException {
        File f = new File(getContext().getCacheDir(), "share.png");
        if (!f.exists()) throw new FileNotFoundException("no share image yet");
        return ParcelFileDescriptor.open(f, ParcelFileDescriptor.MODE_READ_ONLY);
    }

    @Override
    public String getType(Uri uri) { return "image/png"; }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) { return null; }

    @Override
    public Uri insert(Uri uri, ContentValues values) { return null; }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) { return 0; }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) { return 0; }
}
