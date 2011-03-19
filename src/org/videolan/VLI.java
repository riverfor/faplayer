package org.videolan;

public interface VLI {
	public final static int EVENT_SURFACE_CREATED = 1001;
	public final static int EVENT_SURFACE_CHANGED = 1002;
	public final static int EVENT_SURFACE_DESTROYED = 1003;

	public final static int EVENT_INPUT_STATE = 0;
	public final static int EVENT_INPUT_DEAD = 1;
	public final static int EVENT_INPUT_POSITION = 4;
	public final static int EVENT_INPUT_LENGTH = 5;
	public final static int EVENT_INPUT_AOUT = 22;
	public final static int EVENT_INPUT_VOUT = 23;
	public final static int EVENT_INPUT_MISC = 99;
	
	public final static int EVENT_INPUT_STATE_INIT = 0;
	public final static int EVENT_INPUT_STATE_OPEN = 1;
	public final static int EVENT_INPUT_STATE_PLAY = 2;
	public final static int EVENT_INPUT_STATE_PAUSE = 3;
	public final static int EVENT_INPUT_STATE_END = 4;
	public final static int EVENT_INPUT_STATE_ERROR = 5;
	
	public final static int EVENT_INPUT_MISC_CAN_PAUSE = 0;
	public final static int EVENT_INPUT_MISC_CAN_REWIND = 1;
	public final static int EVENT_INPUT_MISC_CAN_SEEK = 2;

	public final static String MODULE_NAME_INPUT = "input";
	public final static String MODULE_INPUT_KEY_CAN_PAUSE = "can-pause";
	public final static String MODULE_INPUT_KEY_CAN_REWIND = "can-rewind";
	public final static String MODULE_INPUT_KEY_CAN_SEEK = "can-seek";
	public final static String MODULE_INPUT_KEY_LENGTH = "length";
	public final static String MODULE_INPUT_KEY_POSITION = "position";
	public final static String MODULE_INPUT_KEY_STATE = "state";

	public void onInputCanPauseChange(boolean value);
	public void onInputCanRewindChange(boolean value);
	public void onInputCanSeekChange(boolean value);
	public void onInputLengthChange(long value);	
	public void onInputStateChange(int value);
	public void onInputPositionChange(long value);

	public void onVlcEvent(String name, String key, String value);
	
}
