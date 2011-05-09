package org.stagex.danmaku.wrapper;

public interface VLI {

	public final static int EVENT_INPUT_STATE = 0;
	public final static int EVENT_INPUT_DEAD = 1;
	public final static int EVENT_INPUT_TIME = 4;
	public final static int EVENT_INPUT_LENGTH = 5;
	public final static int EVENT_INPUT_ES = 9;
	public final static int EVENT_INPUT_AOUT = 22;
	public final static int EVENT_INPUT_VOUT = 23;
	public final static int EVENT_INPUT_MISC = 99;

	public final static int EVENT_VIDEO_SIZE = 101;
	public final static int EVENT_VIDEO_PLACE = 102;

	public final static int EVENT_FULL_SCREEN = 201;
	
	public final static int EVENT_ASPECT_RATIO = 301;

	public final static int EVENT_AUDIO_ES = 401;
	public final static int EVENT_SPU_ES = 402;

	public final static int EVENT_VIDEO_SURFACE_CREATED = 1001;
	public final static int EVENT_VIDEO_SURFACE_CHANGED = 1002;
	public final static int EVENT_VIDEO_SURFACE_DESTROYED = 1003;

	public final static int EVENT_PLAYER_PREPAIRING_BGN = 4001;
	public final static int EVENT_PLAYER_PREPAIRING_END = 4002;

	public final static int INPUT_STATE_INIT = 0;
	public final static int INPUT_STATE_OPEN = 1;
	public final static int INPUT_STATE_PLAY = 2;
	public final static int INPUT_STATE_PAUSE = 3;
	public final static int INPUT_STATE_END = 4;
	public final static int INPUT_STATE_ERROR = 5;

	public final static int INPUT_MISC_CAN_PAUSE = 0;
	public final static int INPUT_MISC_CAN_SEEK = 2;

	public final static String MODULE_NAME_INPUT = "input";
	public final static String MODULE_INPUT_CAN_PAUSE = "can-pause";
	public final static String MODULE_INPUT_CAN_SEEK = "can-seek";
	public final static String MODULE_INPUT_LENGTH = "length";
	public final static String MODULE_INPUT_TIME = "time";
	public final static String MODULE_INPUT_STATE = "state";
	public final static String MODULE_INPUT_AUDIO_ES = "audio-es";
	public final static String MODULE_INPUT_SPU_ES = "spu-es";
	public final static String MODULE_INPUT_VIDEO_ES = "video-es";
	public final static String MODULE_NAME_VIDEO = "video";
	public final static String MODULE_VIDEO_SIZE = "size";
	public final static String MODULE_VIDEO_PLACE = "place";
	public final static String MODULE_NAME_VOUT = "vout";
	public final static String MODULE_VOUT_FULL_SCREEN = "fullscreen";
	public final static String MODULE_VOUT_ASPECT_RATIO = "aspect-ratio";

	public void onInputCanPauseChange(boolean value);

	public void onInputCanSeekChange(boolean value);

	public void onInputLengthChange(long value);

	public void onInputTimeChange(long value);

	public void onInputStateChange(int value);

	public void onVideoSizeChange(int width, int height);

	public void onVideoPlaceChange(int x, int y, int width, int height);

	public void onFullScreenChange(boolean on);

	public void onAspectRatioChange(int selected);

	public void onAudioEsChange(int index, int count);

	public void onSpuEsChange(int index, int count);

	public void onVlcEvent(String name, String key, String value);

}
