package org.stagex.danmaku.player;

import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.view.SurfaceHolder;

public class MsgMediaPlayer extends AbsMediaPlayer {

	protected OnBufferingUpdateListener mOnBufferingUpdateListener = null;
	protected OnPreparedListener mOnPreparedListener = null;
	protected OnVideoSizeChangedListener mOnVideoSizeChangedListener = null;

	private SurfaceHolder mDisplay = null;
	private ReentrantLock mDisplayLock = new ReentrantLock();
	private Condition mDisplayCond = mDisplayLock.newCondition();

	/* example only, not safe!!! */
	private Thread mRenderer = new Thread(new Runnable() {
		@Override
		public void run() {
			// TODO Auto-generated method stub
			while (mDisplay == null) {
				mDisplayLock.lock();
				try {
					mDisplayCond.await();
				} catch (InterruptedException e) {
				}
				mDisplayLock.unlock();
			}
			while (true) {
				String text = String.format("%d", System.currentTimeMillis());
				Paint paint = new Paint();
				paint.setColor(0x7f00ff7f);
				paint.setTextSize(25);
				Canvas canvas = mDisplay.lockCanvas();
				int w = canvas.getWidth();
				int h = canvas.getHeight();
				canvas.drawText(text, 25, 25, paint);
				mDisplay.unlockCanvasAndPost(canvas);
				try {
					Thread.sleep(100);
				} catch (InterruptedException e) {
					break;
				}
			}
		}
	});

	protected MsgMediaPlayer() {
		mRenderer.start();
	}

	@Override
	public int getCurrentPosition() {
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public int getDuration() {
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public int getVideoHeight() {
		// TODO Auto-generated method stub
		return 384;
	}

	@Override
	public int getVideoWidth() {
		// TODO Auto-generated method stub
		return 512;
	}

	@Override
	public boolean isLooping() {
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	public boolean isPlaying() {
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	public void pause() {
		// TODO Auto-generated method stub

	}

	@Override
	public void prepare() {
		// TODO Auto-generated method stub

	}

	@Override
	public void prepareAsync() {
		// TODO Auto-generated method stub
		if (mOnBufferingUpdateListener != null) {
			mOnBufferingUpdateListener.onBufferingUpdate(this, 100);
		}
		if (mOnPreparedListener != null) {
			mOnPreparedListener.onPrepared(this);
		}
	}

	@Override
	public void release() {
		// TODO Auto-generated method stub

	}

	@Override
	public void reset() {
		// TODO Auto-generated method stub

	}

	@Override
	public void seekTo(int msec) {
		// TODO Auto-generated method stub

	}

	@Override
	public void setDataSource(String path) {
		// TODO Auto-generated method stub
	}

	@Override
	public void setDisplay(SurfaceHolder holder) {
		holder.setFormat(PixelFormat.TRANSLUCENT);
		if (mOnVideoSizeChangedListener != null) {
			mOnVideoSizeChangedListener.onVideoSizeChangedListener(this, 512,
					384);
		}
		mDisplayLock.lock();
		mDisplay = holder;
		mDisplay.addCallback(new SurfaceHolder.Callback() {

			@Override
			public void surfaceDestroyed(SurfaceHolder holder) {
				// TODO Auto-generated method stub
				try {
					mRenderer.interrupt();
				} catch (Exception e) {

				}
			}

			@Override
			public void surfaceCreated(SurfaceHolder holder) {
				// TODO Auto-generated method stub

			}

			@Override
			public void surfaceChanged(SurfaceHolder holder, int format,
					int width, int height) {
				// TODO Auto-generated method stub

			}
		});
		mDisplayCond.signal();
		mDisplayLock.unlock();
	}

	@Override
	public void setLooping(boolean looping) {
		// TODO Auto-generated method stub

	}

	@Override
	public void setOnBufferingUpdateListener(OnBufferingUpdateListener listener) {
		mOnBufferingUpdateListener = listener;
	}

	@Override
	public void setOnCompletionListener(OnCompletionListener listener) {
		// TODO Auto-generated method stub

	}

	@Override
	public void setOnErrorListener(OnErrorListener listener) {
		// TODO Auto-generated method stub

	}

	@Override
	public void setOnInfoListener(OnInfoListener listener) {
		/* not used */
	}

	@Override
	public void setOnPreparedListener(OnPreparedListener listener) {
		mOnPreparedListener = listener;
	}

	@Override
	public void setOnProgressUpdateListener(OnProgressUpdateListener listener) {
		// TODO Auto-generated method stub

	}

	@Override
	public void setOnVideoSizeChangedListener(
			OnVideoSizeChangedListener listener) {
		mOnVideoSizeChangedListener = listener;
	}

	@Override
	public void start() {
		// TODO Auto-generated method stub

	}

	@Override
	public void stop() {
		// TODO Auto-generated method stub

	}

	@Override
	public int getAudioTrackCount() {
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public int getAudioTrack() {
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public void setAudioTrack(int index) {
		// TODO Auto-generated method stub

	}

	@Override
	public int getSubtitleTrackCount() {
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public int getSubtitleTrack() {
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public void setSubtitleTrack(int index) {
		// TODO Auto-generated method stub

	}

}
