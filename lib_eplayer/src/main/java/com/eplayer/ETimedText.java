package com.eplayer;

import android.graphics.Rect;

public class ETimedText {

    private Rect mTextBounds = null;
    private String mTextChars = null;
    //注释
    public ETimedText(Rect bounds, String text) {
        mTextBounds = bounds;
        mTextChars = text;
    }

    public Rect getBounds() {
        return mTextBounds;
    }

    public String getText() {
        return mTextChars;
    }
}
