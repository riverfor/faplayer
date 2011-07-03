package org.stagex.danmaku.activity;

import java.io.File;
import java.util.ArrayList;

import org.stagex.danmaku.R;
import org.stagex.danmaku.helper.SystemUtility;
import org.stagex.danmaku.player.AbsMediaPlayer;
import org.stagex.danmaku.player.DefMediaPlayer;
import org.stagex.danmaku.player.MsgMediaPlayer;
import org.stagex.danmaku.player.VlcMediaPlayer;

import android.app.Activity;
import android.content.Intent;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

public class PlayerActivity extends Activity implements
		AbsMediaPlayer.OnBufferingUpdateListener,
		AbsMediaPlayer.OnCompletionListener, AbsMediaPlayer.OnErrorListener,
		AbsMediaPlayer.OnInfoListener, AbsMediaPlayer.OnPreparedListener,
		AbsMediaPlayer.OnProgressUpdateListener,
		AbsMediaPlayer.OnVideoSizeChangedListener, OnTouchListener,
		OnClickListener, OnSeekBarChangeListener {

	static final String LOGTAG = "DANMAKU-PlayerActivity";

	/* */
	private static final int DEF_SURFACE_CREATED = 0x1001;
	private static final int DEF_SURFACE_CHANGED = 0x1002;
	private static final int DEF_SURFACE_DESTROYED = 0x1003;
	private static final int VLC_SURFACE_CREATED = 0x2001;
	private static final int VLC_SURFACE_CHANGED = 0x2002;
	private static final int VLC_SURFACE_DESTROYED = 0x2003;
	private static final int MSG_SURFACE_CREATED = 0x3001;
	private static final int MSG_SURFACE_CHANGED = 0x3002;
	private static final int MSG_SURFACE_DESTROYED = 0x3003;

	private static final int SURFACE_NONE = 0;
	private static final int SURFACE_FILL = 1;
	private static final int SURFACE_ORIG = 2;
	private static final int SURFACE_4_3 = 3;
	private static final int SURFACE_16_9 = 4;
	private static final int SURFACE_16_10 = 5;
	private static final int SURFACE_MAX = 6;

	private static final int MEDIA_PLAYER_BUFFERING_UPDATE = 0x4001;
	private static final int MEDIA_PLAYER_COMPLETION = 0x4002;
	private static final int MEDIA_PLAYER_ERROR = 0x4003;
	private static final int MEDIA_PLAYER_INFO = 0x4004;
	private static final int MEDIA_PLAYER_PREPARED = 0x4005;
	private static final int MEDIA_PLAYER_PROGRESS_UPDATE = 0x4006;
	private static final int MEDIA_PLAYER_VIDEO_SIZE_CHANGED = 0x4007;

	/* the media player */
	private AbsMediaPlayer mMediaPlayer = null;
	/* the message player */
	private AbsMediaPlayer mMessagePlayer = null;

	/* */
	private ArrayList<String> mPlayListArray = null;
	private int mPlayListSelected = -1;

	/* GUI evnet handler */
	private Handler mEventHandler;

	/* player misc */
	private ProgressBar mProgressBarPreparing;

	/* player controls */
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

	private LinearLayout mLinearLayoutControlBar;

	/* player video */
	private SurfaceView mSurfaceViewMsg;
	private SurfaceHolder mSurfaceHolderMsg;
	private SurfaceView mSurfaceViewDef;
	private SurfaceHolder mSurfaceHolderDef;
	private SurfaceView mSurfaceViewVlc;
	private SurfaceHolder mSurfaceHolderVlc;

	/* misc */
	private boolean mMediaPlayerPrepared = false;
	private boolean mMediaPlayerError = false;
	private boolean mMessagePlayerPrepared = false;
	private boolean mMessagePlayerError = false;
	private int mTime = -1;
	private int mLength = -1;
	private boolean mCanSeek = true;
	private int mAspectRatio = 0;

	private int mAudioTrackIndex = 0;
	private int mAudioTrackCount = 0;
	private int mSubtitleTrackIndex = 0;
	private int mSubtitleTrackCount = 0;

	private static boolean isDefMediaPlayer(Object obj) {
		return obj.getClass().getName()
				.compareTo(DefMediaPlayer.class.getName()) == 0;
	}

	private static boolean isVlcMediaPlayer(Object obj) {
		return obj.getClass().getName()
				.compareTo(VlcMediaPlayer.class.getName()) == 0;
	}

	private static boolean isMsgMediaPlayer(Object obj) {
		return obj.getClass().getName()
				.compareTo(MsgMediaPlayer.class.getName()) == 0;
	}

	protected void initializeEvents() {
		mEventHandler = new Handler() {
			public void handleMessage(Message msg) {
				switch (msg.what) {
				case DEF_SURFACE_CREATED: {
					createMediaPlayer(true,
							mPlayListArray.get(mPlayListSelected),
							mSurfaceHolderDef);
					break;
				}
				case DEF_SURFACE_CHANGED: {
					break;
				}
				case DEF_SURFACE_DESTROYED: {
					destroyMediaPlayer(true);
					break;
				}
				case VLC_SURFACE_CREATED: {
					createMediaPlayer(false,
							mPlayListArray.get(mPlayListSelected),
							mSurfaceHolderVlc);
					break;
				}
				case VLC_SURFACE_CHANGED: {
					break;
				}
				case VLC_SURFACE_DESTROYED: {
					destroyMediaPlayer(false);
					break;
				}
				case MSG_SURFACE_CREATED: {
					createMessagePlayer(mPlayListArray.get(mPlayListSelected),
							mSurfaceHolderMsg);
					break;
				}
				case MSG_SURFACE_CHANGED: {
					break;
				}
				case MSG_SURFACE_DESTROYED: {
					destroyMessagePlayer();
					break;
				}
				case MEDIA_PLAYER_BUFFERING_UPDATE: {
					mProgressBarPreparing
							.setVisibility(msg.arg1 < 100 ? View.VISIBLE
									: View.GONE);
					break;
				}
				case MEDIA_PLAYER_COMPLETION: {
					break;
				}
				case MEDIA_PLAYER_ERROR: {
					/* fall back to VlcMediaPlayer if possible */
					if (isDefMediaPlayer(msg.obj)) {
						selectMediaPlayer(
								mPlayListArray.get(mPlayListSelected), true);
					} else if (isVlcMediaPlayer(msg.obj)) {
						/* really won't play */
						mMediaPlayerError = true;
						mProgressBarPreparing.setVisibility(View.GONE);
						if (mMessagePlayerError) {
							finish();
						}
						/* cause to destroy media player */
						mSurfaceViewVlc.setVisibility(View.GONE);
					} else if (isMsgMediaPlayer(msg.obj)) {
						mMessagePlayerError = true;
						mProgressBarPreparing.setVisibility(View.GONE);
						if (mMediaPlayerError) {
							finish();
						}
					}
					break;
				}
				case MEDIA_PLAYER_INFO: {
					if (msg.arg1 == MediaPlayer.MEDIA_INFO_NOT_SEEKABLE) {
						mCanSeek = false;
					}
					break;
				}
				case MEDIA_PLAYER_PREPARED: {
					if (isDefMediaPlayer(msg.obj) || isVlcMediaPlayer(msg.obj)) {
						mMediaPlayerPrepared = true;
					} else if (isMsgMediaPlayer(msg.obj)) {
						mMessagePlayerPrepared = true;
					}
					startIfAllPrepared();
					break;
				}
				case MEDIA_PLAYER_PROGRESS_UPDATE: {
					/*
					 * if there is media player, use it, else use progress
					 * reported by message player
					 */
					if ((mMediaPlayer != null && !isMsgMediaPlayer(msg.obj))
							|| (mMediaPlayer == null && isMsgMediaPlayer(msg.obj))) {
						int length = msg.arg2;
						if (length >= 0) {
							mLength = length;
							mTextViewLength.setText(SystemUtility
									.getTimeString(mLength));
							mSeekBarProgress.setMax(mLength);
						}
						int time = msg.arg1;
						if (time >= 0) {
							mTime = time;
							mTextViewTime.setText(SystemUtility
									.getTimeString(mTime));
							mSeekBarProgress.setProgress(mTime);
						}
					}
					break;
				}
				case MEDIA_PLAYER_VIDEO_SIZE_CHANGED: {
					AbsMediaPlayer player = (AbsMediaPlayer) msg.obj;
					SurfaceView surface = isDefMediaPlayer(player) ? mSurfaceViewDef
							: (isVlcMediaPlayer(player) ? mSurfaceViewVlc
									: mSurfaceViewMsg);
					int ar = mAspectRatio;
					changeSurfaceSize(player, surface, ar);
					break;
				}
				default:
					break;
				}
			}
		};
	}

	protected void initializeControls() {
		/* SufaceView used by VLC is a normal surface */
		mSurfaceViewVlc = (SurfaceView) findViewById(R.id.player_surface_vlc);
		mSurfaceHolderVlc = mSurfaceViewVlc.getHolder();
		mSurfaceHolderVlc.addCallback(new SurfaceHolder.Callback() {
			@Override
			public void surfaceCreated(SurfaceHolder holder) {
				Message msg = new Message();
				msg.what = VLC_SURFACE_CREATED;
				msg.obj = holder;
				mEventHandler.dispatchMessage(msg);
			}

			@Override
			public void surfaceChanged(SurfaceHolder holder, int format,
					int width, int height) {
				Message msg = new Message();
				msg.what = VLC_SURFACE_CHANGED;
				msg.obj = holder;
				msg.arg1 = width;
				msg.arg2 = height;
				mEventHandler.dispatchMessage(msg);
			}

			@Override
			public void surfaceDestroyed(SurfaceHolder holder) {
				Message msg = new Message();
				msg.what = VLC_SURFACE_DESTROYED;
				msg.obj = holder;
				mEventHandler.dispatchMessage(msg);
			}
		});
		mSurfaceViewVlc.setOnTouchListener(this);
		/* SurfaceView used by MediaPlayer is a PUSH_BUFFERS surface */
		mSurfaceViewDef = (SurfaceView) findViewById(R.id.player_surface_def);
		mSurfaceViewDef.setOnTouchListener(this);
		mSurfaceHolderDef = mSurfaceViewDef.getHolder();
		mSurfaceHolderDef.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
		mSurfaceHolderDef.addCallback(new SurfaceHolder.Callback() {
			@Override
			public void surfaceCreated(SurfaceHolder holder) {
				Message msg = new Message();
				msg.what = DEF_SURFACE_CREATED;
				msg.obj = holder;
				mEventHandler.dispatchMessage(msg);
			}

			@Override
			public void surfaceChanged(SurfaceHolder holder, int format,
					int width, int height) {
				Message msg = new Message();
				msg.what = DEF_SURFACE_CHANGED;
				msg.obj = holder;
				msg.arg1 = width;
				msg.arg2 = height;
				mEventHandler.dispatchMessage(msg);
			}

			@Override
			public void surfaceDestroyed(SurfaceHolder holder) {
				Message msg = new Message();
				msg.what = DEF_SURFACE_DESTROYED;
				msg.obj = holder;
				mEventHandler.dispatchMessage(msg);
			}
		});
		/* SurfaceView used by message player is a normal surface */
		mSurfaceViewMsg = (SurfaceView) findViewById(R.id.player_surface_msg);
		mSurfaceViewMsg.setOnTouchListener(this);
		mSurfaceHolderMsg = mSurfaceViewMsg.getHolder();
		mSurfaceHolderMsg.addCallback(new SurfaceHolder.Callback() {
			@Override
			public void surfaceCreated(SurfaceHolder holder) {
				Message msg = new Message();
				msg.what = MSG_SURFACE_CREATED;
				msg.obj = holder;
				mEventHandler.dispatchMessage(msg);
			}

			@Override
			public void surfaceChanged(SurfaceHolder holder, int format,
					int width, int height) {
				Message msg = new Message();
				msg.what = MSG_SURFACE_CHANGED;
				msg.obj = holder;
				msg.arg1 = width;
				msg.arg2 = height;
				mEventHandler.dispatchMessage(msg);
			}

			@Override
			public void surfaceDestroyed(SurfaceHolder holder) {
				Message msg = new Message();
				msg.what = MSG_SURFACE_DESTROYED;
				msg.obj = holder;
				mEventHandler.dispatchMessage(msg);
			}
		});

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

		mLinearLayoutControlBar = (LinearLayout) findViewById(R.id.player_control_bar);

		mProgressBarPreparing = (ProgressBar) findViewById(R.id.player_prepairing);
	}

	protected void initializeData() {
		Intent intent = getIntent();
		String action = intent.getAction();
		if (action != null && action.equals(Intent.ACTION_VIEW)) {
			String one = intent.getDataString();
			mPlayListSelected = 0;
			mPlayListArray = new ArrayList<String>();
			mPlayListArray.add(one);
		} else {
			mPlayListSelected = intent.getIntExtra("selected", 0);
			mPlayListArray = intent.getStringArrayListExtra("playlist");
		}
		if (mPlayListArray == null || mPlayListArray.size() == 0) {
			Log.e(LOGTAG, "initializeData(): empty");
			finish();
			return;
		}
		// TODO: specify message file URI
	}

	protected void resetInterface() {
		int resource = -1;
		/* initial status */
		mMediaPlayerPrepared = false;
		mMediaPlayerError = false;
		mTime = -1;
		mLength = -1;
		mCanSeek = true;
		mAspectRatio = 0;
		/* */
		mImageButtonToggleMessage.setVisibility(View.GONE);
		mImageButtonSwitchAudio.setVisibility(View.GONE);
		mImageButtonSwitchSubtitle.setVisibility(View.GONE);
		mImageButtonPrevious
				.setVisibility((mPlayListArray.size() == 1) ? View.GONE
						: View.VISIBLE);
		mImageButtonTogglePlay.setVisibility(View.VISIBLE);
		resource = SystemUtility.getDrawableId("btn_play_0");
		mImageButtonTogglePlay.setBackgroundResource(resource);
		mImageButtonNext.setVisibility((mPlayListArray.size() == 1) ? View.GONE
				: View.VISIBLE);
		mImageButtonSwitchAspectRatio.setVisibility(View.VISIBLE);
		resource = SystemUtility.getDrawableId("btn_aspect_ratio_0");
		mImageButtonSwitchAspectRatio.setBackgroundResource(resource);
		/* */
		mLinearLayoutControlBar.setVisibility(View.GONE);
		/* */
		mProgressBarPreparing.setVisibility(View.GONE);
	}

	protected void selectMediaPlayer(String uri, boolean forceVlc) {
		/* TODO: do this through configuration */
		boolean useDefault = true;
		int indexOfDot = uri.lastIndexOf('.');
		if (indexOfDot != -1) {
			String extension = uri.substring(indexOfDot).toLowerCase();
			if (extension.compareTo(".flv") == 0
					|| extension.compareTo(".hlv") == 0
					|| extension.compareTo(".m3u8") == 0
					|| extension.compareTo(".mkv") == 0
					|| extension.compareTo(".rm") == 0
					|| extension.compareTo(".rmvb") == 0) {
				useDefault = false;
			}
		}
		if (forceVlc) {
			useDefault = false;
		}
		mSurfaceViewDef.setVisibility(useDefault ? View.VISIBLE : View.GONE);
		mSurfaceViewVlc.setVisibility(useDefault ? View.GONE : View.VISIBLE);
	}

	protected void createMediaPlayer(boolean useDefault, String uri,
			SurfaceHolder holder) {
		Log.d(LOGTAG, "createMediaPlayer() " + uri);
		/* */
		resetInterface();
		/* */
		mMediaPlayer = AbsMediaPlayer.getMediaPlayer(useDefault);
		mMediaPlayer.setOnBufferingUpdateListener(this);
		mMediaPlayer.setOnCompletionListener(this);
		mMediaPlayer.setOnErrorListener(this);
		mMediaPlayer.setOnInfoListener(this);
		mMediaPlayer.setOnPreparedListener(this);
		mMediaPlayer.setOnProgressUpdateListener(this);
		mMediaPlayer.setOnVideoSizeChangedListener(this);
		mMediaPlayer.reset();
		mMediaPlayer.setDisplay(holder);
		mMediaPlayer.setDataSource(uri);
		mMediaPlayer.prepareAsync();
	}

	protected void destroyMediaPlayer(boolean isDefault) {
		assert (mMediaPlayer != null);
		boolean testDefault = mMediaPlayer.getClass().getName()
				.equals(DefMediaPlayer.class.getName());
		if (isDefault == testDefault) {
			mMediaPlayer.release();
			mMediaPlayer = null;
		}
	}

	protected void detectMessagePlayer(String uri) {
		/* TODO: try specified file */
		/* try local file */
		if (uri.startsWith("file://")) {
			String path = Uri.decode(uri).substring(7);
			int indexOfDot = path.lastIndexOf('.');
			String name = indexOfDot != -1 ? (path.substring(0, indexOfDot))
					: path;
			String xml = String.format("%s.xml", name);
			File test = new File(xml);
			boolean hasMessage = test.isFile();
			mSurfaceViewMsg
					.setVisibility(hasMessage ? View.VISIBLE : View.GONE);
		}
	}

	protected void createMessagePlayer(String uri, SurfaceHolder holder) {
		Log.d(LOGTAG, "createMessagePlayer() " + uri);
		/* */
		mMessagePlayerPrepared = false;
		mMessagePlayerError = false;
		/* */
		mMessagePlayer = AbsMediaPlayer.getMessagePlayer();
		mMessagePlayer.setOnBufferingUpdateListener(this);
		mMessagePlayer.setOnCompletionListener(this);
		mMessagePlayer.setOnErrorListener(this);
		mMessagePlayer.setOnInfoListener(this);
		mMessagePlayer.setOnPreparedListener(this);
		mMessagePlayer.setOnProgressUpdateListener(this);
		mMessagePlayer.setOnVideoSizeChangedListener(this);
		mMessagePlayer.reset();
		mMessagePlayer.setDisplay(holder);
		mMessagePlayer.setDataSource(uri);
		mMessagePlayer.prepareAsync();
	}

	protected void destroyMessagePlayer() {
		assert (mMessagePlayer != null);
		mMessagePlayer.release();
		mMessagePlayer = null;
	}

	protected void startIfAllPrepared() {
		if ((mMediaPlayer != null && !mMediaPlayerPrepared)
				|| (mMessagePlayer != null && !mMessagePlayerPrepared))
			return;
		if (mMediaPlayer != null)
			mMediaPlayer.start();
		if (mMessagePlayer != null)
			mMessagePlayer.start();
		int resource = SystemUtility.getDrawableId("btn_play_1");
		mImageButtonTogglePlay.setBackgroundResource(resource);
	}

	protected void changeSurfaceSize(AbsMediaPlayer player,
			SurfaceView surface, int ar) {
		int videoWidth = player.getVideoWidth();
		int videoHeight = player.getVideoHeight();
		if (videoWidth <= 0 || videoHeight <= 0) {
			return;
		}
		SurfaceHolder holder = surface.getHolder();
		holder.setFixedSize(videoWidth, videoHeight);
		int displayWidth = getWindowManager().getDefaultDisplay().getWidth();
		int displayHeight = getWindowManager().getDefaultDisplay().getHeight();
		int targetWidth = -1;
		int targetHeight = -1;
		switch (ar) {
		case SURFACE_NONE: {
			targetWidth = videoWidth;
			targetHeight = videoHeight;
			break;
		}
		case SURFACE_FILL: {
			break;
		}
		case SURFACE_ORIG: {
			displayWidth = videoWidth;
			displayHeight = videoHeight;
			break;
		}
		case SURFACE_4_3: {
			targetWidth = 4;
			targetHeight = 3;
			break;
		}
		case SURFACE_16_9: {
			targetWidth = 16;
			targetHeight = 9;
			break;
		}
		case SURFACE_16_10: {
			targetWidth = 16;
			targetHeight = 10;
			break;
		}
		default:
			break;
		}
		if (targetWidth > 0 && targetHeight > 0) {
			double ard = (double) displayWidth / (double) displayHeight;
			double art = (double) targetWidth / (double) targetHeight;
			if (ard > art) {
				displayWidth = displayHeight * targetWidth / targetHeight;
			} else {
				displayHeight = displayWidth * targetHeight / targetWidth;
			}
		}
		LayoutParams lp = surface.getLayoutParams();
		lp.width = displayWidth;
		lp.height = displayHeight;
		surface.setLayoutParams(lp);
		surface.invalidate();
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		initializeEvents();
		setContentView(R.layout.player);
		initializeControls();
		initializeData();
		String uri = mPlayListArray.get(mPlayListSelected);
		detectMessagePlayer(uri);
		selectMediaPlayer(uri, false);
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
		if (mMediaPlayer != null) {
			mMediaPlayer.stop();
			mMediaPlayer.release();
		}
		if (mMessagePlayer != null) {
			mMessagePlayer.stop();
			mMessagePlayer.release();
		}
	}

	@Override
	public void onStart() {
		super.onStart();
	}

	@Override
	public void onStop() {
		super.onStop();
		if (mMediaPlayer != null) {
			mMediaPlayer.pause();
		}
		if (mMessagePlayer != null) {
			mMessagePlayer.pause();
		}
	}

	@Override
	public boolean onTouch(View v, MotionEvent event) {
		if ((mMediaPlayer != null && !mMediaPlayerPrepared)
				|| (mMessagePlayer != null && !mMessagePlayerPrepared)) {
			return true;
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
			int visibility = mSurfaceViewMsg.getVisibility();
			mSurfaceViewMsg
					.setVisibility(visibility == View.VISIBLE ? View.INVISIBLE
							: View.VISIBLE);
			break;
		}
		case R.id.player_button_switch_audio: {

			break;
		}
		case R.id.player_button_switch_subtitle: {

			break;
		}
		case R.id.player_button_previous: {

			break;
		}
		case R.id.player_button_toggle_play: {
			// TODO: message player
			boolean playing = mMediaPlayer.isPlaying();
			if (playing) {
				mMediaPlayer.pause();
			} else {
				mMediaPlayer.start();
			}
			String name = String.format("btn_play_%d", !playing ? 1 : 0);
			int resouce = SystemUtility.getDrawableId(name);
			mImageButtonTogglePlay.setBackgroundResource(resouce);
			break;
		}
		case R.id.player_button_next: {
			break;
		}
		case R.id.player_button_switch_aspect_ratio: {
			mAspectRatio = (mAspectRatio + 1) % SURFACE_MAX;
			changeSurfaceSize(mMessagePlayer, mSurfaceViewMsg, mAspectRatio);
			String name = String.format("btn_aspect_ratio_%d", mAspectRatio);
			int resource = SystemUtility.getDrawableId(name);
			mImageButtonSwitchAspectRatio.setBackgroundResource(resource);
			break;
		}
		default:
			break;
		}
	}

	@Override
	public void onProgressChanged(SeekBar seekBar, int progress,
			boolean fromUser) {
		/* not used */
	}

	@Override
	public void onStartTrackingTouch(SeekBar seekBar) {
		/* not used */
	}

	@Override
	public void onStopTrackingTouch(SeekBar seekBar) {
		int id = seekBar.getId();
		switch (id) {
		case R.id.player_seekbar_progress: {
			if (mCanSeek && mLength > 0) {
				int position = seekBar.getProgress();
				mMediaPlayer.seekTo(position);
			}
			break;
		}
		default:
			break;
		}
	}

	@Override
	public void onBufferingUpdate(AbsMediaPlayer mp, int percent) {
		Message msg = new Message();
		msg.obj = mp;
		msg.what = MEDIA_PLAYER_BUFFERING_UPDATE;
		msg.arg1 = percent;
		mEventHandler.sendMessage(msg);
	}

	@Override
	public void onCompletion(AbsMediaPlayer mp) {
		Message msg = new Message();
		msg.obj = mp;
		msg.what = MEDIA_PLAYER_COMPLETION;
		mEventHandler.sendMessage(msg);
	}

	@Override
	public boolean onError(AbsMediaPlayer mp, int what, int extra) {
		Message msg = new Message();
		msg.obj = mp;
		msg.what = MEDIA_PLAYER_ERROR;
		msg.arg1 = what;
		msg.arg2 = extra;
		mEventHandler.sendMessage(msg);
		return true;
	}

	@Override
	public boolean onInfo(AbsMediaPlayer mp, int what, int extra) {
		Message msg = new Message();
		msg.obj = mp;
		msg.what = MEDIA_PLAYER_INFO;
		msg.arg1 = what;
		msg.arg2 = extra;
		mEventHandler.sendMessage(msg);
		return true;
	}

	@Override
	public void onPrepared(AbsMediaPlayer mp) {
		Message msg = new Message();
		msg.obj = mp;
		msg.what = MEDIA_PLAYER_PREPARED;
		mEventHandler.sendMessage(msg);
	}

	@Override
	public void onProgressUpdate(AbsMediaPlayer mp, int time, int length) {
		Message msg = new Message();
		msg.obj = mp;
		msg.what = MEDIA_PLAYER_PROGRESS_UPDATE;
		msg.arg1 = time;
		msg.arg2 = length;
		mEventHandler.sendMessage(msg);
	}

	@Override
	public void onVideoSizeChangedListener(AbsMediaPlayer mp, int width,
			int height) {
		Message msg = new Message();
		msg.obj = mp;
		msg.what = MEDIA_PLAYER_VIDEO_SIZE_CHANGED;
		msg.arg1 = width;
		msg.arg2 = height;
		mEventHandler.sendMessage(msg);
	}
}
