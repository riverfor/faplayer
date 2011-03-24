package org.stagex.danmaku.comment;

public class Comment {

	public static int TYPE_FLY = 1;
	public static int TYPE_TOP = 2;
	public static int TYPE_BOT = 4;

	
	private long mTime = -1;
	private int mType = -1;
	private int mSize = -1;
	private int mColor = -1;
	private String mText = null;

	private int mWidth = -1;
	private int mHeight = -1;

	private long mDuration = -1;

	private int mHashCode = -1;
	private String mHashString = null;

	public Comment() {
		mTime = -1;
		mType = -1;
		mSize = -1;
		mColor = -1;
		mText = "";
	}

	public Comment(long time, int type, int size, int color, String text) {
		mTime = time;
		mType = type;
		mSize = size;
		mColor = color;
		mText = text;
	}

	public long getTime() {
		return mTime;
	}

	public void setTime(long time) {
		mTime = time;
	}

	public int getType() {
		return mType;
	}

	public void setType(int type) {
		mType = type;
	}

	public int getSize() {
		return mSize;
	}

	public void setSize(int size) {
		mSize = size;
	}

	public int getColor() {
		return mColor;
	}

	public void setColor(int color) {
		mColor = color;
	}

	public String getText() {
		return mText;
	}

	public void setText(String text) {
		// XXX: replace illegal characters
		mText = text;
	}

	public int getWidth() {
		return mWidth;
	}

	public void setWidth(int width) {
		mWidth = width;
	}

	public int getHeight() {
		return mHeight;
	}

	public void setHeight(int height) {
		mHeight = height;
	}

	public long getDuration() {
		return mDuration;
	}

	public void setDuration(long duration) {
		mDuration = duration;
	}

	public int getHashCode() {
		if (mHashCode != -1) {
			return mHashCode;
		}
		String id = String.format("%d%d%s", mSize, mColor, mText);
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
		return String.format("%d|%d|%d|%d|%s|%d|%d|%d", mTime, mType, mSize,
				mColor, mText, mWidth, mHeight, mDuration);
	}

}
