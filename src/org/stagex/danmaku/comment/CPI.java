package org.stagex.danmaku.comment;

import android.graphics.Bitmap;

public interface CPI {
	
	public final static int EVENT_COMMENT_SNAPSHOT = 200;
	
	public void onSnapshotReady(Bitmap bitmap);
	
}
