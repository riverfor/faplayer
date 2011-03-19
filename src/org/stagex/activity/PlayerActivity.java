package org.stagex.activity;

import java.util.ArrayList;

import org.stagex.R;
import org.videolan.VLC;
import org.videolan.VLI;
import org.videolan.VLM;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

public class PlayerActivity extends Activity implements VLI {

	private SurfaceView mSurfaceView;
	private TextView mTextViewTime;
	private SeekBar mSeekBar;
	private TextView mTextViewLength;
	private ImageButton mImageButtonPrev;
	private ImageButton mImageButtonPlay;
	private ImageButton mImageButtonNext;

	private SurfaceHolder mSurfaceHolder;

	private ArrayList<String> mPlayList = null;
	private int mCurrentIndex = -1;

	private int mCurrentState = -1;
	private int mCurrentTime = -1;
	private int mCurrentLength = -1;

	private int mCanPause = -1;
	@SuppressWarnings("unused")
	private int mCanRewind = -1;
	private int mCanSeek = -1;

	private int mAudioChannelCount = -1;

	private int mDisplayWidth = -1;
	private int mDisplayHeight = -1;
	private int mVideoWidth = -1;
	private int mVideoHeight = -1;

	private Handler mEventHandler = new Handler() {
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case VLI.EVENT_SURFACE_CREATED: {
				/* this is hopefully called only once */
				Surface surface = (Surface) msg.obj;
				VLC.attachVideoOutput(surface);
				if (mPlayList != null && mCurrentIndex >= 0
						&& mCurrentIndex < mPlayList.size()) {
					VLM.getInstance().open(mPlayList.get(mCurrentIndex));
				}
				break;
			}
			case VLI.EVENT_SURFACE_CHANGED: {
				/* this is hopefully called only once */
				Surface surface = (Surface) msg.obj;
				VLC.attachVideoOutput(surface);
				mDisplayWidth = msg.arg1;
				mDisplayHeight = msg.arg2;
				break;
			}
			case VLI.EVENT_SURFACE_DESTROYED: {
				VLC.detachVideoOutput();
				break;
			}
			case VLI.EVENT_INPUT_STATE: {
				int state = msg.arg1;
				switch (state) {
				case VLI.EVENT_INPUT_STATE_PLAY: {
					mImageButtonPlay.setImageResource(R.drawable.pause);
					break;
				}
				case VLI.EVENT_INPUT_STATE_PAUSE: {
					mImageButtonPlay.setImageResource(R.drawable.play);
					break;
				}
				case VLI.EVENT_INPUT_STATE_END: {
					mImageButtonPlay.setImageResource(R.drawable.play);
					break;
				}
				case VLI.EVENT_INPUT_STATE_ERROR: {
					break;
				}
				default:
					break;
				}
				mCurrentState = msg.arg1;
				break;
			}
			case VLI.EVENT_INPUT_DEAD: {
				break;
			}
			case VLI.EVENT_INPUT_POSITION: {
				int val = msg.arg1 / 10;
				if (val != mCurrentTime) {
					int hour = val / 3600;
					val -= hour * 3600;
					int minute = val / 60;
					val -= minute * 60;
					int second = val;
					String time = String.format("%02d:%02d:%02d", hour, minute,
							second);
					mTextViewTime.setText(time);
					mCurrentTime = msg.arg1;
					if (mCurrentLength > 0) {
						mSeekBar.setProgress(mCurrentTime);
					}
				}
				break;
			}
			case VLI.EVENT_INPUT_LENGTH: {
				int val = msg.arg1 / 10;
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
					mSeekBar.setMax(mCurrentLength);
				}
				break;
			}
			case VLI.EVENT_INPUT_AOUT: {
				mAudioChannelCount = msg.arg1;
				break;
			}
			case VLI.EVENT_INPUT_VOUT: {
				mVideoWidth = msg.arg1;
				mVideoHeight = msg.arg2;
				break;
			}
			case VLI.EVENT_INPUT_MISC: {
				switch (msg.arg1) {
				case VLI.EVENT_INPUT_MISC_CAN_PAUSE: {
					mCanPause = msg.arg2;
					break;
				}
				case VLI.EVENT_INPUT_MISC_CAN_REWIND: {
					mCanRewind = msg.arg2;
					break;
				}
				case VLI.EVENT_INPUT_MISC_CAN_SEEK: {
					mCanSeek = msg.arg2;
					break;
				}
				default:
					break;
				}
			}
			default:
				break;
			}
		}
	};

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(R.layout.player);

		mSurfaceView = (SurfaceView) findViewById(R.id.video);
		mSurfaceHolder = mSurfaceView.getHolder();
		mSurfaceHolder.addCallback(new SurfaceHolder.Callback() {

			@Override
			public void surfaceDestroyed(SurfaceHolder holder) {
				Message msg = new Message();
				msg.what = VLI.EVENT_SURFACE_DESTROYED;
				mEventHandler.dispatchMessage(msg);
			}

			@Override
			public void surfaceCreated(SurfaceHolder holder) {
				Message msg = new Message();
				msg.what = VLI.EVENT_SURFACE_CREATED;
				msg.obj = holder.getSurface();
				mEventHandler.dispatchMessage(msg);
			}

			@Override
			public void surfaceChanged(SurfaceHolder holder, int format,
					int width, int height) {
				Message msg = new Message();
				msg.what = VLI.EVENT_SURFACE_CHANGED;
				msg.arg1 = width;
				msg.arg2 = height;
				msg.obj = holder.getSurface();
				mEventHandler.dispatchMessage(msg);
			}
		});
		mTextViewTime = (TextView) findViewById(R.id.time);
		mSeekBar = (SeekBar) findViewById(R.id.seekbar);
		mSeekBar.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {

			@Override
			public void onProgressChanged(SeekBar seekBar, int progress,
					boolean fromUser) {
			}

			@Override
			public void onStartTrackingTouch(SeekBar seekBar) {
			}

			@Override
			public void onStopTrackingTouch(SeekBar seekBar) {
				if (mCanSeek > 0) {
					long position = seekBar.getProgress() * 1000 * 100;
					VLM.getInstance().seek(position);
				}
			}
		});
		mTextViewLength = (TextView) findViewById(R.id.length);
		mImageButtonPrev = (ImageButton) findViewById(R.id.prev);
		mImageButtonPrev.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mCurrentIndex != -1 && mPlayList != null
						&& mPlayList.size() > 1) {
					mCurrentIndex--;
					if (mCurrentIndex < 0)
						mCurrentIndex = 0;
					VLM.getInstance().open(mPlayList.get(mCurrentIndex));
				}
			}
		});
		mImageButtonPlay = (ImageButton) findViewById(R.id.play);
		mImageButtonPlay.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mCanPause > 0) {
					if (mCurrentState == VLI.EVENT_INPUT_STATE_PLAY)
						VLM.getInstance().pause();
					else if (mCurrentState == VLI.EVENT_INPUT_STATE_PAUSE)
						VLM.getInstance().play();
				}
			}
		});
		mImageButtonNext = (ImageButton) findViewById(R.id.next);
		mImageButtonNext.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mCurrentIndex != -1 && mPlayList != null
						&& mPlayList.size() > 1) {
					mCurrentIndex++;
					if (mCurrentIndex >= mPlayList.size())
						mCurrentIndex %= mPlayList.size();
					VLM.getInstance().open(mPlayList.get(mCurrentIndex));
				}
			}
		});

		VLM.getInstance().setCallbackHandler(this);

		Bundle bundle = getIntent().getExtras().getBundle("playlist");
		mPlayList = bundle.getStringArrayList("list");
		mCurrentIndex = bundle.getInt("index");

	}

	@Override
	public void onDestroy() {
		super.onDestroy();

		VLM.getInstance().close();

	}

	@Override
	public void onStart() {
		super.onStart();

		VLM.getInstance().play();
	}

	@Override
	public void onStop() {
		super.onStop();

		VLM.getInstance().pause();

	}

	@Override
	public void onInputCanPauseChange(boolean value) {
		Message msg = new Message();
		msg.what = VLI.EVENT_INPUT_MISC;
		msg.arg1 = VLI.EVENT_INPUT_MISC_CAN_PAUSE;
		msg.arg2 = value ? 1 : 0;
		mEventHandler.sendMessage(msg);
	}

	@Override
	public void onInputCanRewindChange(boolean value) {
		Message msg = new Message();
		msg.what = VLI.EVENT_INPUT_MISC;
		msg.arg1 = VLI.EVENT_INPUT_MISC_CAN_REWIND;
		msg.arg2 = value ? 1 : 0;
		mEventHandler.sendMessage(msg);
	}

	@Override
	public void onInputCanSeekChange(boolean value) {
		Message msg = new Message();
		msg.what = VLI.EVENT_INPUT_MISC;
		msg.arg1 = VLI.EVENT_INPUT_MISC_CAN_SEEK;
		msg.arg2 = value ? 1 : 0;
		mEventHandler.sendMessage(msg);
	}

	@Override
	public void onInputLengthChange(long length) {
		Message msg = new Message();
		msg.what = VLI.EVENT_INPUT_LENGTH;
		msg.arg1 = (int) (length / 1000 / 100);
		mEventHandler.sendMessage(msg);
	}

	@Override
	public void onInputPositionChange(long position) {
		Message msg = new Message();
		msg.what = VLI.EVENT_INPUT_POSITION;
		msg.arg1 = (int) (position / 1000 / 100);
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
	public void onVlcEvent(String name, String key, String value) {
		Log.d("faplayer", String.format(
				"unable to process: \"%s\" \"%s\" \"%s\"", name, key, value));
	}

}
