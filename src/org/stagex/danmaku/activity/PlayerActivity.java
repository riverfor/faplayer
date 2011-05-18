package org.stagex.danmaku.activity;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;

import org.stagex.danmaku.R;
import org.stagex.danmaku.comment.CPI;
import org.stagex.danmaku.comment.CommentDrawable;
import org.stagex.danmaku.comment.CommentManager;
import org.stagex.danmaku.helper.SystemUtility;
import org.stagex.danmaku.wrapper.VLC;
import org.stagex.danmaku.wrapper.VLI;
import org.stagex.danmaku.wrapper.VLM;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

public class PlayerActivity extends Activity implements VLI, CPI,
		OnTouchListener, OnClickListener, OnSeekBarChangeListener {

	private VLC mVLC = VLC.getInstance();
	private VLM mVLM = VLM.getInstance();

	private Handler mEventHandler;

	// player misc
	private ProgressBar mProgressBarPrepairing;

	private boolean mPrepairDone = false;

	private ReentrantLock mPrepairLock = new ReentrantLock();
	private Condition mPrepairCond = mPrepairLock.newCondition();
	private Thread mPrepairThread = null;

	// player controls
	private TextView mTextViewTime;
	private SeekBar mSeekBarProgress;
	private TextView mTextViewLength;
	private ImageButton mImageButtonToggleMessage;
	private ImageButton mImageButtonSwitchAudio;
	private ImageButton mImageButtonSwitchSubtitle;
	private ImageButton mImageButtonPrevious;
	private ImageButton mImageButtonTogglePlay;
	private ImageButton mImageButtonNext;
	private ImageButton mImageButtonSwitchAspectRatio;
	private ImageButton mImageButtonToggleFullScreen;

	private LinearLayout mLinearLayoutControlBar;

	// player video
	private View mViewMessage;
	private SurfaceView mSurfaceViewVideo;
	private SurfaceHolder mSurfaceHolderVideo;

	private boolean mVideoSurfaceReady = false;

	private CommentManager mCommentManager = new CommentManager();

	//
	private ArrayList<String> mPlayList = null;
	private int mCurrentIndex = -1;

	private String mUriVideo;
	private String mUriMessage;

	//
	private int mCurrentState = -1;
	private int mCurrentTime = -1;
	private int mCurrentLength = -1;

	private int mCanPause = -1;
	private int mCanSeek = -1;

	private boolean mFullScreen = false;
	private int mAspectRatio = 0;

	private int mVideoWidth = -1;
	private int mVideoHeight = -1;

	private int mVideoSurfaceWidth = -1;
	private int mVideoSurfaceHeight = -1;

	private int mVideoPlaceX = 0;
	private int mVideoPlaceY = 0;
	private int mVideoPlaceW = 0;
	private int mVideoPlaceH = 0;

	private int mAudioTrackIndex = 0;
	private int mAudioTrackCount = 0;
	private int mSubtitleTrackIndex = 0;
	private int mSubtitleTrackCount = 0;

	protected void initializeEvents() {
		mEventHandler = new Handler() {
			public void handleMessage(Message msg) {
				switch (msg.what) {
				case CPI.EVENT_COMMENT_SNAPSHOT: {
					Bitmap bitmap = (Bitmap) msg.obj;
					CommentDrawable snapshot = (CommentDrawable) mViewMessage
							.getBackground();
					snapshot.setSize(mViewMessage.getWidth(),
							mViewMessage.getHeight());
					snapshot.setBitmap(bitmap);
					mViewMessage.invalidate();
					break;
				}
				case CPI.EVENT_COMMENT_LOADED: {
					if (msg.arg1 != 0) {
						mImageButtonToggleMessage.setVisibility(View.VISIBLE);
					}
					break;
				}
				case VLI.EVENT_INPUT_STATE: {
					int id = -1;

					mCurrentState = msg.arg1;
					switch (mCurrentState) {
					case VLI.INPUT_STATE_PLAY: {
						mCommentManager.play();
						id = R.drawable.btn_play_1;
						break;
					}
					case VLI.INPUT_STATE_PAUSE: {
						mCommentManager.pause();
						id = R.drawable.btn_play_0;
						break;
					}
					case VLI.INPUT_STATE_END: {
						mCommentManager.pause();
						id = R.drawable.btn_play_0;
						break;
					}
					case VLI.INPUT_STATE_ERROR: {
						id = R.drawable.btn_play_0;
						break;
					}
					default:
						break;
					}
					if (id != -1) {
						mImageButtonTogglePlay.setBackgroundResource(id);
					}
					break;
				}
				case VLI.EVENT_INPUT_DEAD: {
					break;
				}
				case VLI.EVENT_INPUT_TIME: {
					int val = msg.arg1 / 1000;
					if (val != mCurrentTime) {
						int hour = val / 3600;
						val -= hour * 3600;
						int minute = val / 60;
						val -= minute * 60;
						int second = val;
						String time = String.format("%02d:%02d:%02d", hour,
								minute, second);
						mTextViewTime.setText(time);
						mCurrentTime = msg.arg1;
						if (mCurrentLength > 0) {
							mSeekBarProgress.setProgress(mCurrentTime);
						}
					}
					// mCommentManager.seek(msg.arg1);
					break;
				}
				case VLI.EVENT_INPUT_LENGTH: {
					int val = msg.arg1 / 1000;
					int hour = val / 3600;
					val -= hour * 3600;
					int minute = val / 60;
					val -= minute * 60;
					int second = val;
					String time = String.format("%02d:%02d:%02d", hour, minute,
							second);
					mTextViewLength.setText(time);
					mCurrentLength = msg.arg1;
					if (mCurrentLength > 0) {
						mSeekBarProgress.setMax(mCurrentLength);
					}
					break;
				}
				case VLI.EVENT_INPUT_MISC: {
					switch (msg.arg1) {
					case VLI.INPUT_MISC_CAN_PAUSE: {
						mCanPause = msg.arg2;
						break;
					}
					case VLI.INPUT_MISC_CAN_SEEK: {
						mCanSeek = msg.arg2;
						break;
					}
					default:
						break;
					}
					break;
				}
				case VLI.EVENT_VIDEO_SIZE: {
					mVideoWidth = msg.arg1;
					mVideoHeight = msg.arg2;
					break;
				}
				case VLI.EVENT_VIDEO_PLACE: {
					mVideoPlaceX = msg.arg1 >> 16;
					mVideoPlaceY = msg.arg1 & 0xffff;
					mVideoPlaceW = msg.arg2 >> 16;
					mVideoPlaceH = msg.arg2 & 0xffff;
					break;
				}
				case VLI.EVENT_FULL_SCREEN: {
					mFullScreen = msg.arg1 != 0;
					int id = mFullScreen ? R.drawable.btn_full_screen_1
							: R.drawable.btn_full_screen_0;
					mImageButtonToggleFullScreen.setBackgroundResource(id);
					break;
				}
				case VLI.EVENT_ASPECT_RATIO: {
					int id = SystemUtility.getDrawableId(String.format(
							"btn_aspect_ratio_%d", msg.arg1));
					mImageButtonSwitchAspectRatio.setBackgroundResource(id);
					break;
				}
				case VLI.EVENT_AUDIO_ES: {
					int index = msg.arg1;
					int count = msg.arg2;
					if (count < 3) {
						mImageButtonSwitchAudio.setVisibility(View.GONE);
					} else {
						setAudioTrackImage(index, count);
					}
					mAudioTrackIndex = index;
					mAudioTrackCount = count;
					break;
				}
				case VLI.EVENT_SPU_ES: {
					int index = msg.arg1;
					int count = msg.arg2;
					// a chance to disable subtitle
					if (count < 2) {
						mImageButtonSwitchSubtitle.setVisibility(View.GONE);
					} else {
						setSubtitleTrackImage(index, count);
					}
					mSubtitleTrackIndex = index;
					mSubtitleTrackCount = count;
					break;
				}
				case VLI.EVENT_PLAYER_PREPAIRING_BGN: {
					mPrepairDone = false;
					mProgressBarPrepairing.setVisibility(View.VISIBLE);
					break;
				}
				case VLI.EVENT_PLAYER_PREPAIRING_END: {
					mPrepairDone = true;
					mProgressBarPrepairing.setVisibility(View.INVISIBLE);
					break;
				}
				case VLI.EVENT_VIDEO_SURFACE_CREATED: {
					/* this is hopefully called only once */
					break;
				}
				case VLI.EVENT_VIDEO_SURFACE_CHANGED: {
					/* this is hopefully called only once */
					mVideoSurfaceWidth = msg.arg1;
					mVideoSurfaceHeight = msg.arg2;
					mVLC.attachSurface((Surface) msg.obj, mVideoSurfaceWidth,
							mVideoSurfaceHeight);
					try {
						mPrepairLock.lock();
						mVideoSurfaceReady = true;
						mPrepairCond.signal();
					} finally {
						mPrepairLock.unlock();
					}
					break;
				}
				case VLI.EVENT_VIDEO_SURFACE_DESTROYED: {
					/* this is hopefully called only once */
					mVLC.detachSurface();
					break;
				}
				default:
					break;
				}
			}
		};
	}

	protected void initializeControls() {
		mSurfaceViewVideo = (SurfaceView) findViewById(R.id.player_surface_video);
		mSurfaceHolderVideo = mSurfaceViewVideo.getHolder();
		mSurfaceHolderVideo.addCallback(new SurfaceHolder.Callback() {
			@Override
			public void surfaceCreated(SurfaceHolder holder) {
				Message msg = new Message();
				msg.what = VLI.EVENT_VIDEO_SURFACE_CREATED;
				msg.obj = holder.getSurface();
				mEventHandler.dispatchMessage(msg);
			}

			@Override
			public void surfaceChanged(SurfaceHolder holder, int format,
					int width, int height) {
				Message msg = new Message();
				msg.what = VLI.EVENT_VIDEO_SURFACE_CHANGED;
				msg.arg1 = width;
				msg.arg2 = height;
				msg.obj = holder.getSurface();
				mEventHandler.dispatchMessage(msg);
			}

			@Override
			public void surfaceDestroyed(SurfaceHolder holder) {
				Message msg = new Message();
				msg.what = VLI.EVENT_VIDEO_SURFACE_DESTROYED;
				mEventHandler.dispatchMessage(msg);
			}
		});
		// mSurfaceViewVideo.setOnTouchListener(this);
		mViewMessage = (View) findViewById(R.id.player_view_message);
		mViewMessage.setBackgroundDrawable(new CommentDrawable());
		mViewMessage.setOnTouchListener(this);

		mTextViewTime = (TextView) findViewById(R.id.player_text_position);
		mSeekBarProgress = (SeekBar) findViewById(R.id.player_seekbar_progress);
		mSeekBarProgress.setOnSeekBarChangeListener(this);
		mTextViewLength = (TextView) findViewById(R.id.player_text_length);
		mImageButtonToggleMessage = (ImageButton) findViewById(R.id.player_button_toggle_message);
		mImageButtonToggleMessage.setOnClickListener(this);
		mImageButtonSwitchAudio = (ImageButton) findViewById(R.id.player_button_switch_audio);
		mImageButtonSwitchAudio.setOnClickListener(this);
		mImageButtonSwitchSubtitle = (ImageButton) findViewById(R.id.player_button_switch_subtitle);
		mImageButtonSwitchSubtitle.setOnClickListener(this);
		mImageButtonPrevious = (ImageButton) findViewById(R.id.player_button_previous);
		mImageButtonPrevious.setOnClickListener(this);
		mImageButtonTogglePlay = (ImageButton) findViewById(R.id.player_button_toggle_play);
		mImageButtonTogglePlay.setOnClickListener(this);
		mImageButtonNext = (ImageButton) findViewById(R.id.player_button_next);
		mImageButtonNext.setOnClickListener(this);
		mImageButtonSwitchAspectRatio = (ImageButton) findViewById(R.id.player_button_switch_aspect_ratio);
		mImageButtonSwitchAspectRatio.setOnClickListener(this);
		mImageButtonToggleFullScreen = (ImageButton) findViewById(R.id.player_button_toggle_full_screen);
		mImageButtonToggleFullScreen.setOnClickListener(this);

		mLinearLayoutControlBar = (LinearLayout) findViewById(R.id.player_control_bar);

		mProgressBarPrepairing = (ProgressBar) findViewById(R.id.player_prepairing);
	}

	protected void setMediaSource(String uri) {
		if (mPrepairThread != null && mPrepairThread.isAlive())
			return;
		mUriVideo = uri;
		mUriMessage = uri.substring(0, uri.lastIndexOf(".")) + ".xml";
		mPrepairThread = (new Thread(new Runnable() {
			@Override
			public void run() {
				mEventHandler.sendEmptyMessage(VLI.EVENT_PLAYER_PREPAIRING_BGN);
				// wait until the surface is ready
				mPrepairLock.lock();
				try {
					try {
						while (mVideoSurfaceReady == false) {
							mPrepairCond.await();
						}
					} catch (InterruptedException e) {
					}
				} finally {
					mPrepairLock.unlock();
				}
				mCommentManager.open(mUriMessage);
				mVLM.open(mUriVideo);
				mEventHandler.sendEmptyMessage(VLI.EVENT_PLAYER_PREPAIRING_END);
			}
		}));
		mPrepairThread.start();
	}

	protected void setAudioTrackImage(int index, int count) {
		String text = String.format("%01d/%01d", index, count);
		Bitmap image = BitmapFactory.decodeResource(getResources(),
				R.drawable.btn_switch_audio).copy(Config.ARGB_8888, true);
		Canvas canvas = new Canvas(image);
		Paint paint = new Paint();
		paint.setTextSize(image.getWidth() / 3);
		paint.setTextAlign(Align.CENTER);
		canvas.drawText(text, 0, image.getHeight() / 2, paint);
		mImageButtonSwitchAudio.setImageBitmap(image);
		mImageButtonSwitchAudio.setVisibility(View.VISIBLE);
		mImageButtonSwitchAudio.invalidate();
	}

	protected void setSubtitleTrackImage(int index, int count) {
		String text = String.format("%01d/%01d", index, count);
		Bitmap image = BitmapFactory.decodeResource(getResources(),
				R.drawable.btn_switch_subtitle).copy(Config.ARGB_8888, true);
		Canvas canvas = new Canvas(image);
		Paint paint = new Paint();
		paint.setTextSize(image.getWidth() / 3);
		paint.setTextAlign(Align.CENTER);
		canvas.drawText(text, 0, image.getHeight() / 2, paint);
		mImageButtonSwitchSubtitle.setImageBitmap(image);
		mImageButtonSwitchSubtitle.setVisibility(View.VISIBLE);
		mImageButtonSwitchSubtitle.invalidate();
	}

	@Override
	public void onInputCanPauseChange(boolean value) {
		Message msg = new Message();
		msg.what = VLI.EVENT_INPUT_MISC;
		msg.arg1 = VLI.INPUT_MISC_CAN_PAUSE;
		msg.arg2 = value ? 1 : 0;
		mEventHandler.sendMessage(msg);
	}

	@Override
	public void onInputCanSeekChange(boolean value) {
		Message msg = new Message();
		msg.what = VLI.EVENT_INPUT_MISC;
		msg.arg1 = VLI.INPUT_MISC_CAN_SEEK;
		msg.arg2 = value ? 1 : 0;
		mEventHandler.sendMessage(msg);
	}

	@Override
	public void onInputLengthChange(long length) {
		if (length < 0)
			return;
		Message msg = new Message();
		msg.what = VLI.EVENT_INPUT_LENGTH;
		msg.arg1 = (int) (length / 1000);
		mEventHandler.sendMessage(msg);
	}

	@Override
	public void onInputTimeChange(long position) {
		Message msg = new Message();
		msg.what = VLI.EVENT_INPUT_TIME;
		msg.arg1 = (int) (position / 1000);
		mEventHandler.sendMessage(msg);
	}

	@Override
	public void onInputStateChange(int state) {
		Message msg = new Message();
		msg.what = VLI.EVENT_INPUT_STATE;
		msg.arg1 = state;
		mEventHandler.sendMessage(msg);
	}

	@Override
	public void onAudioEsChange(int index, int count) {
		Message msg = new Message();
		msg.what = VLI.EVENT_AUDIO_ES;
		msg.arg1 = index;
		msg.arg2 = count;
		mEventHandler.sendMessage(msg);
	}

	@Override
	public void onSpuEsChange(int index, int count) {
		Message msg = new Message();
		msg.what = VLI.EVENT_SPU_ES;
		msg.arg1 = index;
		msg.arg2 = count;
		mEventHandler.sendMessage(msg);
	}

	@Override
	public void onVideoSizeChange(int width, int height) {
		Message msg = new Message();
		msg.what = VLI.EVENT_VIDEO_SIZE;
		msg.arg1 = width;
		msg.arg2 = height;
		mEventHandler.sendMessage(msg);
	}

	@Override
	public void onVideoPlaceChange(int x, int y, int width, int height) {
		Message msg = new Message();
		msg.what = VLI.EVENT_VIDEO_PLACE;
		msg.arg1 = x << 16 | y;
		msg.arg2 = width << 16 | height;
		mEventHandler.sendMessage(msg);
	}

	@Override
	public void onFullScreenChange(boolean on) {
		Message msg = new Message();
		msg.what = VLI.EVENT_FULL_SCREEN;
		msg.arg1 = on ? 1 : 0;
		mEventHandler.sendMessage(msg);
	}

	@Override
	public void onAspectRatioChange(int selected) {
		Message msg = new Message();
		msg.what = VLI.EVENT_ASPECT_RATIO;
		msg.arg1 = selected;
		mEventHandler.sendMessage(msg);
	}

	@Override
	public void onCommentLoadComplete(boolean success) {
		Message msg = new Message();
		msg.what = CPI.EVENT_COMMENT_LOADED;
		msg.arg1 = success ? 1 : 0;
		mEventHandler.sendMessage(msg);
	}

	@Override
	public void onCommentSnapshotReady(Bitmap bitmap) {
		Message msg = new Message();
		msg.what = CPI.EVENT_COMMENT_SNAPSHOT;
		msg.obj = bitmap;
		mEventHandler.sendMessage(msg);
	}

	@Override
	public void onVlcEvent(String name, String key, String value) {
		Log.d("faplayer", String.format(
				"unable to process: \"%s\" \"%s\" \"%s\"", name, key, value));
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		initializeEvents();
		setContentView(R.layout.player);
		initializeControls();
		mVLM.setCallbackHandler(this);
		mCommentManager.setCallbackHandler(this);
		Bundle bundle = getIntent().getExtras().getBundle("playlist");
		mPlayList = bundle.getStringArrayList("list");
		mCurrentIndex = bundle.getInt("index");

		if (mPlayList != null && mCurrentIndex >= 0
				&& mCurrentIndex < mPlayList.size()) {
			String uri = mPlayList.get(mCurrentIndex);
			setMediaSource(uri);
		}
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
		mCommentManager.close();
		mVLM.close();
	}

	@Override
	public void onStart() {
		super.onStart();
		mCommentManager.play();
		mVLM.play();
	}

	@Override
	public void onStop() {
		super.onStop();
		mCommentManager.pause();
		mVLM.pause();
	}

	@Override
	public boolean onTouch(View v, MotionEvent event) {
		if (!mPrepairDone) {
			return false;
		}
		int action = event.getAction();
		if (action == MotionEvent.ACTION_DOWN) {
			int visibility = mLinearLayoutControlBar.getVisibility();
			if (visibility != View.VISIBLE) {
				mLinearLayoutControlBar.setVisibility(View.VISIBLE);
			} else {
				mLinearLayoutControlBar.setVisibility(View.GONE);
			}
			return true;
		}
		return false;
	}

	@Override
	public void onClick(View v) {
		int id = v.getId();
		switch (id) {
		case R.id.player_button_toggle_message: {
			int visibility = mViewMessage.getVisibility();
			mViewMessage
					.setVisibility(visibility == View.VISIBLE ? View.INVISIBLE
							: View.VISIBLE);
			break;
		}
		case R.id.player_button_switch_audio: {
			mVLM.setSpuEs((mAudioTrackIndex + 1) % mAudioTrackCount);
			break;
		}
		case R.id.player_button_switch_subtitle: {
			mVLM.setSpuEs((mSubtitleTrackIndex + 1) % mSubtitleTrackCount);
			break;
		}
		case R.id.player_button_previous: {
			if (mCurrentIndex != -1 && mPlayList != null
					&& mPlayList.size() > 1) {
				mCurrentIndex--;
				if (mCurrentIndex < 0)
					mCurrentIndex = 0;
				String uri = mPlayList.get(mCurrentIndex);
				setMediaSource(uri);
			}
			break;
		}
		case R.id.player_button_toggle_play: {
			if (mCanPause > 0) {
				if (mCurrentState == VLI.INPUT_STATE_PLAY)
					mVLM.pause();
				else if (mCurrentState == VLI.INPUT_STATE_PAUSE)
					mVLM.play();
			}
			break;
		}
		case R.id.player_button_next: {
			if (mCurrentIndex != -1 && mPlayList != null
					&& mPlayList.size() > 1) {
				mCurrentIndex++;
				if (mCurrentIndex >= mPlayList.size())
					mCurrentIndex %= mPlayList.size();
				String uri = mPlayList.get(mCurrentIndex);
				setMediaSource(uri);
			}
			break;
		}
		case R.id.player_button_switch_aspect_ratio: {

			break;
		}
		case R.id.player_button_toggle_full_screen: {
			mVLM.setFullScreen(!mFullScreen);
			break;
		}
		default:
			break;
		}
	}

	@Override
	public void onProgressChanged(SeekBar seekBar, int progress,
			boolean fromUser) {
	}

	@Override
	public void onStartTrackingTouch(SeekBar seekBar) {
	}

	@Override
	public void onStopTrackingTouch(SeekBar seekBar) {
		int id = seekBar.getId();
		switch (id) {
		case R.id.player_seekbar_progress: {
			if (mCanSeek > 0) {
				long position = seekBar.getProgress();
				mCommentManager.seek(position);
				mVLM.seek(position * 1000);
			}
			break;
		}
		default:
			break;
		}
	}
}
