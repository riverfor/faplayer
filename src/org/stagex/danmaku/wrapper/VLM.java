package org.stagex.danmaku.wrapper;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.SocketChannel;
import java.util.Iterator;

import android.util.Log;

public class VLM {

	private static VLM mInstance;

	private VLI mCallbackHandler = null;

	private Thread mVLMThread = null;

	private ByteBuffer mSendBuffer = ByteBuffer.allocate(512);
	private ByteBuffer mRecvBuffer = ByteBuffer.allocate(512);
	private Selector mSelector = null;
	private SocketChannel mClientChannel = null;

	private byte[] mCharByte = new byte[4];
	private int mCharIndex = 0;
	private int mCharLength = 0;
	private StringBuffer mLineBuffer = new StringBuffer();

	protected VLM() {
	}

	protected void triggerCallback(String line) {
		if (line == null || line.length() == 0 || mCallbackHandler == null)
			return;
		// TODO: quote
		String[] temp = line.split("\\s+");
		if (temp.length != 3) {
			Log.d("faplayer", String.format("unable to process: \"%s\"", line));
			return;
		}
		String name = temp[0];
		String key = temp[1];
		String value = temp[2];
		if (name.compareTo(VLI.MODULE_NAME_INPUT) == 0) {
			if (key.compareTo(VLI.MODULE_INPUT_TIME) == 0) {
				long val = Long.parseLong(value);
				mCallbackHandler.onInputTimeChange(val);
			} else if (key.compareTo(VLI.MODULE_INPUT_STATE) == 0) {
				int val = Integer.parseInt(value);
				mCallbackHandler.onInputStateChange(val);
			} else if (key.compareTo(VLI.MODULE_INPUT_LENGTH) == 0) {
				long val = Long.parseLong(value);
				mCallbackHandler.onInputLengthChange(val);
			} else if (key.compareTo(VLI.MODULE_INPUT_CAN_PAUSE) == 0) {
				boolean val = Integer.parseInt(value) > 0;
				mCallbackHandler.onInputCanPauseChange(val);
			} else if (key.compareTo(VLI.MODULE_INPUT_CAN_SEEK) == 0) {
				boolean val = Integer.parseInt(value) > 0;
				mCallbackHandler.onInputCanSeekChange(val);
			} else if (key.compareTo(VLI.MODULE_INPUT_AUDIO_ES) == 0) {
				String[] vals = value.split("/");
				if (vals.length == 2) {
					int index = Integer.parseInt(vals[0]);
					int count = Integer.parseInt(vals[1]);
					mCallbackHandler.onAudioEsChange(index, count);
				}
			} else if (key.compareTo(VLI.MODULE_INPUT_SPU_ES) == 0) {
				String[] vals = value.split("/");
				if (vals.length == 2) {
					int index = Integer.parseInt(vals[0]);
					int count = Integer.parseInt(vals[1]);
					mCallbackHandler.onSpuEsChange(index, count);
				}
			}
		} else if (name.compareTo(VLI.MODULE_NAME_VIDEO) == 0) {
			if (key.compareTo(VLI.MODULE_VIDEO_SIZE) == 0) {
				String[] vals = value.split("x");
				if (vals.length == 2) {
					int width = Integer.parseInt(vals[0]);
					int height = Integer.parseInt(vals[1]);
					mCallbackHandler.onVideoSizeChange(width, height);
				}
			}
		} else if (name.compareTo(VLI.MODULE_NAME_VOUT) == 0) {
			if (key.compareTo(VLI.MODULE_VOUT_FULL_SCREEN) == 0) {
				boolean on = Integer.parseInt(value) != 0;
				mCallbackHandler.onFullScreenChange(on);
			} else if (key.compareTo(VLI.MODULE_VOUT_ASPECT_RATIO) == 0) {

			}
		} else {
			mCallbackHandler.onVlcEvent(name, key, value);
		}
	}

	protected boolean wantRead() {
		return true;
	}

	protected boolean wantWrite() {
		synchronized (mSendBuffer) {
			return (mSendBuffer.position() > 0);
		}
	}

	protected void doRead() throws IOException {
		synchronized (mRecvBuffer) {
			mRecvBuffer.clear();
			int read = mClientChannel.read(mRecvBuffer);
			if (read == 0)
				return;
			mRecvBuffer.flip();
			while (mRecvBuffer.position() != mRecvBuffer.limit()) {
				// utf-8
				byte ch = mRecvBuffer.get();
				if (mCharIndex == 0) {
					if (ch == '\r' || ch == '\n' || ch == '\0') {
						if (mLineBuffer.length() > 0) {
							String line = mLineBuffer.toString();
							mLineBuffer.setLength(0);
							triggerCallback(line);
							continue;
						}
					}
					mCharLength = 8;
					int temp = (ch ^ 0xff);
					while (temp != 0) {
						temp = (temp >> 1);
						mCharLength -= 1;
					}
					if (mCharLength == 1 || mCharLength > 4) {
						continue;
					}
				}
				mCharByte[mCharIndex++] = ch;
				if (mCharIndex >= mCharLength) {
					String character = new String(mCharByte, 0,
							mCharLength == 0 ? 1 : mCharLength, "utf-8");
					mLineBuffer.append(character);
					mCharIndex = 0;
				}
			}
		}
	}

	protected void doWrite() throws IOException {
		synchronized (mSendBuffer) {
			int position = mSendBuffer.position();
			mSendBuffer.flip();
			int written = 0;
			written = mClientChannel.write(mSendBuffer);
			if (written < position) {
				mSendBuffer.position(position - written);
			}
			mSendBuffer.compact();
		}
	}

	protected void writeBytes(String str) {
		if (str == null || str.length() == 0)
			return;
		if (!str.endsWith("\n"))
			str += "\n";
		byte[] data = null;
		try {
			data = str.getBytes("utf-8");
		} catch (UnsupportedEncodingException e) {
		}
		writeBytes(data);
	}

	protected void writeBytes(byte[] data) {
		if (data != null && data.length > 0) {
			synchronized (mSendBuffer) {
				mSendBuffer.put(data, 0,
						mSendBuffer.remaining() >= data.length ? data.length
								: mSendBuffer.remaining());
			}
		}
		if (mSelector != null)
			mSelector.wakeup();
	}

	public static VLM getInstance() {
		if (mInstance == null)
			mInstance = new VLM();
		return mInstance;
	}

	public void create(final String[] args) {
		if (args == null || args.length != 2 || mVLMThread != null)
			return;
		mVLMThread = new Thread(new Runnable() {
			@Override
			public void run() {
				for (;;) {
					mRecvBuffer.clear();
					mSendBuffer.clear();
					try {
						mSelector = Selector.open();
						mClientChannel = SocketChannel
								.open(new InetSocketAddress(args[0], Integer
										.parseInt(args[1])));
						mClientChannel.configureBlocking(false);
						mClientChannel.register(mSelector, SelectionKey.OP_READ
								| SelectionKey.OP_WRITE);
						while (true) {
							mClientChannel
									.register(
											mSelector,
											(wantRead() ? SelectionKey.OP_READ
													: 0)
													| (wantWrite() ? SelectionKey.OP_WRITE
															: 0));
							// TODO: when the connection hangs up unexpectedly,
							// figure it out
							int n = mSelector.select();
							if (n < 0)
								break;
							Iterator<SelectionKey> it = mSelector
									.selectedKeys().iterator();
							while (it.hasNext()) {
								SelectionKey key = (SelectionKey) it.next();
								it.remove();
								if (key.isReadable()) {
									doRead();
								}
								if (key.isWritable()) {
									doWrite();
								}
							}
						}
					} catch (IOException e) {
						try {
							if (mClientChannel != null)
								mClientChannel.close();
							if (mSelector != null)
								mSelector.close();
						} catch (IOException ex) {
						}
						try {
							Thread.sleep(100);
						} catch (InterruptedException ex) {
						}
					}
				}
			}
		});
		mVLMThread.start();
	}

	public void destroy() {
		if (mVLMThread.isAlive()) {
			mVLMThread.interrupt();
			mVLMThread = null;
		}
	}

	public void setCallbackHandler(VLI handler) {
		mCallbackHandler = handler;
	}

	public void open(String file) {
		String line;
		line = String.format("open \"%s\"", file);
		writeBytes(line);
	}

	public void close() {
		String line;
		line = String.format("close");
		writeBytes(line);
	}

	public void play() {
		String line;
		line = String.format("play");
		writeBytes(line);
	}

	public void seek(long position) {
		String line;
		line = String.format("time %d", position);
		writeBytes(line);
	}

	public void pause() {
		String line;
		line = String.format("pause");
		writeBytes(line);
	}

	public void stop() {
		String line;
		line = String.format("stop");
		writeBytes(line);
	}

	public void setFullScreen(boolean on) {
		String line;
		line = String.format("fullscreen %d", on ? 1 : 0);
		writeBytes(line);
	}

	public void setAudioEs(int index) {
		String line;
		line = String.format("audio-es %d", index);
		writeBytes(line);
	}

	public void setSpuEs(int index) {
		String line;
		line = String.format("spu-es %d", index);
		writeBytes(line);
	}
}
