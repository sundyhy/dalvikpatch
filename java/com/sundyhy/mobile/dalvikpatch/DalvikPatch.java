package com.sundyhy.mobile.dalvikpatch;


import android.os.Build;

public class DalvikPatch {

    public static void patch() {
        if (Build.VERSION.SDK_INT >= 19) { // 4.4就先不搞了
            return;
        }

        try {
            System.loadLibrary("dhdalvikpatch");

            boolean isDalvik = isDalvik();
            if (isDalvik) {
                adjustLinearAlloc();
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private static native boolean isDalvik();
    private static native int getMapLength();
    private static native void adjustLinearAlloc();
}
