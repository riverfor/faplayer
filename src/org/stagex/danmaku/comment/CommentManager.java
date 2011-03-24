package org.stagex.danmaku.comment;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Hashtable;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;

import org.stagex.danmaku.site.CommentParserFactory;

import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.FontMetrics;
import android.util.Log;
import android.view.Surface;
import android.view.Surface.OutOfResourcesException;

public class CommentManager {

	private static CommentManager mInstance = null;

	private Paint mPaint = new Paint();

	private Surface mSurface = null;
	private Object mSurfaceLock = new Object();

	private int mWidth = -1;
	private int mHeight = -1;

	private int mStageWidth = -1;
	private int mStageHeight = -1;

	private ReentrantLock mReadyLock = new ReentrantLock();
	private Condition mReadyCond = mReadyLock.newCondition();

	private boolean mExit = false;
	private boolean mPause = true;

	private ReentrantLock mPauseLock = new ReentrantLock();
	private Condition mPauseCond = mPauseLock.newCondition();

	private HashMap<String, Bitmap> mCommentBitmap = new HashMap<String, Bitmap>();

	private ArrayList<Comment> mFlyComment = new ArrayList<Comment>();
	private ArrayList<Comment> mTopComment = new ArrayList<Comment>();
	private ArrayList<Comment> mBotComment = new ArrayList<Comment>();

	private class Position {
		public String id = null;
		public int x = -1;
		public int y = -1;
		public int width = -1;
		public int height = -1;
		public long start = -1;
		public long duration = -1;
	}

	private long mStartTime = -1;

	private Thread mRendererThread = null;

	private class RendererThread extends Thread {
		@Override
		public void run() {
			// wait until it is ready to loop
			try {
				mReadyLock.lock();
				try {
					while (!((mWidth > 0) && (mHeight > 0) && (mStageWidth > 0) && (mStageHeight > 0))) {
						mReadyCond.await();
					}
				} catch (InterruptedException e) {
				}
			} finally {
				mReadyLock.unlock();
			}
			mStartTime = System.currentTimeMillis();
			while (!mExit) {
				// used for frame control
				long rendererBegin = System.currentTimeMillis();
				// wait if it is paused
				boolean pauseTest = false;
				long pauseBegin = 0, pauseEnd = 0;
				mPauseLock.lock();
				pauseTest = mPause;
				if (pauseTest) {
					pauseBegin = System.currentTimeMillis();
				}
				while (mPause) {
					try {
						mPauseCond.await();
					} catch (InterruptedException e) {
					}
				}
				if (pauseTest) {
					pauseEnd = System.currentTimeMillis();
					if (mStartTime != -1) {
						mStartTime += (pauseEnd - pauseBegin);
					}
				}
				mPauseLock.unlock();
				// check whether exit is requested
				if (mExit) {
					break;
				}
				// let's draw the whole screen
				long currentTime = System.currentTimeMillis() - mStartTime;

				// check whether we can sleep
				long rendererEnd = System.currentTimeMillis();
				long rendererEclipsed = (rendererEnd - rendererBegin);
				if (pauseTest) {
					rendererEclipsed -= (pauseEnd - pauseBegin);
				}
				Log.d("faplayer", String.format("rendered one frame in %d ms",
						rendererEclipsed));
				long timeToSleep = (1000 / 25) - rendererEclipsed;
				if (timeToSleep > 0) {
					try {
						Thread.sleep(timeToSleep);
					} catch (InterruptedException e) {
					}
				}
			}
		}
	}

	public static CommentManager getInstance() {
		if (mInstance == null) {
			mInstance = new CommentManager();
		}
		return mInstance;
	}

	protected CommentManager() {
	}

	protected Bitmap getCommentBitmap(Comment comment) {
		mPaint.setTextSize(comment.getSize());
		mPaint.setColor(comment.getColor());
		String text = comment.getText();
		FontMetrics metrics = mPaint.getFontMetrics();
		int width = (int) Math.ceil(mPaint.measureText(text) + 2);
		int height = (int) Math.ceil(metrics.descent - metrics.ascent + 2);
		Bitmap bitmap = Bitmap.createBitmap(width, height, Config.ARGB_8888);
		if (bitmap == null)
			return null;
		Canvas canvas = new Canvas(bitmap);
		canvas.drawText(text, 1, 1, mPaint);
		return bitmap;
	}

	public void attachSurface(Surface surface, int width, int height) {
		synchronized (mSurfaceLock) {
			mSurface = surface;
			try {
				mReadyLock.lock();
				mWidth = width;
				mHeight = height;
				mReadyCond.signal();
			} finally {
				mReadyLock.unlock();
			}
		}
	}

	public void detachSurface() {
		synchronized (mSurfaceLock) {
			mSurface = null;
			mWidth = -1;
			mHeight = -1;
		}
	}

	public void setStageSize(int width, int height) {
		try {
			mReadyLock.lock();
			mStageWidth = width;
			mStageHeight = height;
			mReadyCond.signal();
		} finally {
			mReadyLock.unlock();
		}
	}

	public void open(String uri) {
		close();
		ArrayList<Comment> yeah = CommentParserFactory.parse(uri);
		if (yeah == null || yeah.size() == 0) {
			return;
		}
		for (Comment c : yeah) {
			if (c.getType() == Comment.TYPE_FLY) {
				mFlyComment.add(c);
			} else if (c.getType() == Comment.TYPE_TOP) {
				mTopComment.add(c);
			} else if (c.getType() == Comment.TYPE_BOT) {
				mBotComment.add(c);
			}
			// XXX: danmaku implementation may be different
			String key = c.getHashString();
			Bitmap value = mCommentBitmap.get(key);
			if (value == null) {
				value = getCommentBitmap(c);
				mCommentBitmap.put(key, value);
			}
			// XXX: need resize
			c.setWidth(value.getWidth());
			c.setHeight(value.getHeight());
			Log.d("faplayer", c.toString());
		}
		Comparator<Comment> comparator = new Comparator<Comment>() {
			@Override
			public int compare(Comment c1, Comment c2) {
				return (int) (c1.getTime() - c2.getTime());
			}
		};
		Collections.sort(mFlyComment, comparator);
		Collections.sort(mTopComment, comparator);
		Collections.sort(mBotComment, comparator);
		mRendererThread = new RendererThread();
		mRendererThread.start();
	}

	public void seek(long time) {
	}

	public void pause() {
		mPauseLock.lock();
		mPause = true;
		mPauseCond.signal();
		mPauseLock.unlock();
	}

	public void play() {
		mPauseLock.lock();
		mPause = false;
		mPauseCond.signal();
		mPauseLock.unlock();
	}

	public void close() {
		if (mRendererThread != null) {
			mExit = true;
			try {
				mReadyLock.lock();
				mReadyCond.signal();
			} finally {
				mReadyLock.unlock();
			}
			play();
			if (mRendererThread.isAlive()) {
				try {
					mRendererThread.join();
				} catch (InterruptedException e) {
				}
			}
			mRendererThread = null;
		}
		mFlyComment.clear();
		mTopComment.clear();
		mBotComment.clear();
		mCommentBitmap.clear();
	}

}
