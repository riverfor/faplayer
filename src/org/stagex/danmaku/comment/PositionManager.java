package org.stagex.danmaku.comment;

import java.util.ArrayList;
import java.util.LinkedList;
import java.util.ListIterator;

import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.Canvas;
import android.util.Log;

public class PositionManager {

	private int mStageWidth = -1;
	private int mStageHeight = -1;

	private Canvas mCanvas = null;
	private Bitmap mStage = null;

	private LinkedList<Position> mFlyUsed = new LinkedList<Position>();
	private LinkedList<Position> mTopUsed = new LinkedList<Position>();
	private LinkedList<Position> mBotUsed = new LinkedList<Position>();

	protected void setTopPosition(LinkedList<Position> list, Position p) {
		int n = list.size();
		if (n > 0) {
			// check if there is space before first
			Position first = list.getFirst();
			if (first.y >= p.height) {
				p.y = 0;
				list.addFirst(p);
				return;
			}
			// check if there is space between two
			ListIterator<Position> it = (ListIterator<Position>) list
					.iterator();
			Position p1 = null, p2 = it.next();
			for (int i = 0; i < n - 1; i++) {
				p1 = p2;
				p2 = it.next();
				int space = p2.y - p1.y - p1.height;
				if (space >= p.height) {
					p.y = p1.y + p1.height;
					it.add(p);
					return;
				}
			}
			// check if there is space after last
			Position last = list.getLast();
			if (mStageHeight - last.y - last.height >= p.height) {
				p.y = last.y + last.height;
				list.addLast(p);
				return;
			} else {
				// XXX:
				p.y = (first.y + first.height + p.height) % mStageHeight;
				list.addLast(p);
				return;
			}
		} else {
			p.y = 0;
			list.addFirst(p);
		}
	}

	protected void setBotPosition(LinkedList<Position> l, Position p) {
	}

	public PositionManager() {

	}

	public int count() {
		return (mFlyUsed.size() + mTopUsed.size() + mBotUsed.size());
	}

	public void feed(Comment c) {
		if (mStageWidth < 0 || mStageHeight < 0) {
			switch (c.site) {
			case Comment.SITE_ACFUN: {
				mStageWidth = Comment.STAGE_WIDTH_ACFUN;
				mStageHeight = Comment.STAGE_HEIGHT_ACFUN;
				break;
			}
			case Comment.SITE_BILIBILI: {
				mStageWidth = Comment.STAGE_WIDTH_BILIBILI;
				mStageHeight = Comment.STAGE_HEIGHT_BILIBILI;
				break;
			}
			case Comment.SITE_ICHIBA: {
				mStageWidth = Comment.STAGE_WIDTH_ICHIBA;
				mStageHeight = Comment.STAGE_HEIGHT_ICHIBA;
				break;
			}
			default: {
				assert (false);
				break;
			}
			}
			mStage = Bitmap.createBitmap(mStageWidth, mStageHeight,
					Config.ARGB_8888);
			mCanvas = new Canvas(mStage);
		}
		Position p = new Position();
		p.id = c.getHashString();
		p.duration = c.getDuration();
		p.width = c.getWidth();
		p.height = c.getHeight();
		switch (c.type) {
		case Comment.TYPE_FLY: {
			p.x = mStageWidth;
			setTopPosition(mFlyUsed, p);
			break;
		}
		case Comment.TYPE_TOP: {
			p.x = (mStageWidth - p.width) / 2;
			setTopPosition(mTopUsed, p);
			break;
		}
		case Comment.TYPE_BOT: {
			p.x = (mStageWidth - p.width) / 2;
			setBotPosition(mBotUsed, p);
			break;
		}
		default: {
			assert (false);
			break;
		}
		}
	}

	public void play(long time) {
		ListIterator<Position> it = null;
		// calculate fly positions
		it = (ListIterator<Position>) mFlyUsed.iterator();
		while (it.hasNext()) {
			Position p = it.next();
			if (p.time + p.duration < time) {
				it.remove();
				continue;
			}
			p.x = (int) (mStageWidth - (time - p.time)
					* (p.width + mStageWidth) / (p.duration));
		}
		// clean top and bottom positions
		it = (ListIterator<Position>) mTopUsed.iterator();
		while (it.hasNext()) {
			Position p = it.next();
			if (p.time + p.duration < time) {
				it.remove();
			}
		}
		it = (ListIterator<Position>) mBotUsed.iterator();
		while (it.hasNext()) {
			Position p = it.next();
			if (p.time + p.duration < time) {
				it.remove();
			}
		}
	}

	public void reset() {
		mFlyUsed.clear();
		mTopUsed.clear();
		mBotUsed.clear();
	}

	public Bitmap snapshot() {
		// TODO:
		return mStage;
	}

}
