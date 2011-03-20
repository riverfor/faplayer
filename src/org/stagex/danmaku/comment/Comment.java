package org.stagex.danmaku.comment;

public class Comment {

	public static int TYPE_FLY = 1;
	public static int TYPE_TOP = 2;
	public static int TYPE_BOT = 4;

	private int mTime;
	private int mType;
	private int mSize;
	private int mColor;
	private String mText;

	public Comment() {
		mTime = -1;
		mType = -1;
		mSize = -1;
		mColor = -1;
		mText = "";
	}

	public Comment(int time, int type, int size, int color, String text) {
		mTime = time;
		mType = type;
		mSize = size;
		mColor = color;
		mText = text;
	}

	public long getTime() {
		return mTime;
	}

	public void setTime(int time) {
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
		mText = text;
	}

	public String toString() {
		return String.format("%d|%d|%d|%d|%s", mTime, mType, mSize, mColor,
				mText);
	}

}
