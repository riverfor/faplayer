package org.stagex.danmaku.wrapper;

import android.view.Surface;

public class VLC {

	static {
		System.loadLibrary("vlccore");
	}

	private static VLC mInstance = null;

	private int mHandle = 0;

	private Thread mVLCMain;

	protected VLC() {
	}

	public static native int setenv(String key, String val, boolean overwrite);

	public static VLC getInstance() {
		if (mInstance == null)
			mInstance = new VLC();
		return mInstance;
	}

	private native void run(String[] args);

	public native void attachSurface(Surface surface, int width, int height);

	public native void detachSurface();

	public void create(final String[] args) {
		if (mVLCMain != null && mVLCMain.isAlive())
			return;
		mVLCMain = new Thread(new Runnable() {
			@Override
			public void run() {
				VLC.this.run(args);
			}
		});
		mVLCMain.start();
	}

	public void destroy() {

	}
}
