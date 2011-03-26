package org.stagex.danmaku.comment;

import java.util.HashMap;

import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.FontMetricsInt;

public class Comment {

	public final static int SITE_ACFUN = 1;
	public final static int SITE_BILIBILI = 2;
	public final static int SITE_ICHIBA = 3;

	public final static int STAGE_WIDTH_ACFUN = 848;
	public final static int STAGE_HEIGHT_ACFUN = 480;
	public final static int STAGE_WIDTH_BILIBILI = 848;
	public final static int STAGE_HEIGHT_BILIBILI = 480;
	public final static int STAGE_WIDTH_ICHIBA = 848;
	public final static int STAGE_HEIGHT_ICHIBA = 480;

	public final static int TYPE_FLY = 1;
	public final static int TYPE_TOP = 2;
	public final static int TYPE_BOT = 4;

	public static Paint mPaint = new Paint();
	public static HashMap<String, Bitmap> mBitmap = new HashMap<String, Bitmap>();

	public int site = -1;
	public long time = -1;
	public int type = -1;
	public int size = -1;
	public int color = -1;
	public String text = null;

	private long mDuration = -1;

	private int mWidth = -1;
	private int mHeight = -1;

	private int mHashCode = -1;
	private String mHashString = null;

	public static Bitmap getBitmap(String key) {
		return mBitmap.get(key);
	}

	public Comment() {
	}

	protected void draw() {
		String key = getHashString();
		Bitmap value = mBitmap.get(key);
		if (value == null) {
			mPaint.setColor(color);
			mPaint.setTextSize(size);
			FontMetricsInt metrics = mPaint.getFontMetricsInt();
			mWidth = (int) Math.ceil(mPaint.measureText(text));
			mHeight = metrics.descent - metrics.ascent;
			value = Bitmap.createBitmap(mWidth, mHeight, Config.ARGB_8888);
			Canvas canvas = new Canvas(value);
			canvas.drawText(text, 0, 0, mPaint);
			mBitmap.put(key, value);
		}
		switch (site) {
		case Comment.SITE_ACFUN: {
			break;
		}
		case Comment.SITE_BILIBILI: {
			break;
		}
		case Comment.SITE_ICHIBA: {
			if (type == Comment.TYPE_FLY) {
				mDuration = ((mWidth + Comment.STAGE_WIDTH_ICHIBA) * 2500)
						/ (Comment.STAGE_WIDTH_ICHIBA);
			} else {
				mDuration = 4000;
			}
			break;
		}
		default: {
			assert (false);
			break;
		}
		}
	}

	public long getDuration() {
		if (mDuration < 0) {
			draw();
		}
		return mDuration;
	}

	public int getWidth() {
		if (mWidth < 0) {
			draw();
		}
		return mWidth;
	}

	public int getHeight() {
		if (mHeight < 0) {
			draw();
		}
		return mHeight;
	}

	public int getHashCode() {
		if (mHashCode != -1) {
			return mHashCode;
		}
		String id = String.format("%d%d%s", size, color, text);
		byte[] target = id.getBytes();
		mHashCode = 1315423911;
		for (int i = 0; i < target.length; i++) {
			byte val = target[i];
			mHashCode ^= ((mHashCode << 5) + val + (mHashCode >> 2));
		}
		mHashCode &= 0x7fffffff;
		return mHashCode;
	}

	public String getHashString() {
		if (mHashString != null) {
			return mHashString;
		}
		int hash = getHashCode();
		mHashString = Integer.toHexString(hash);
		return mHashString;
	}

	public String toString() {
		return String.format("%d|%d|%d|%d|%s", time, type, size, color, text);
	}

}
