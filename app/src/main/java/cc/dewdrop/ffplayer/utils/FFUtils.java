package cc.dewdrop.ffplayer.utils;

import android.os.Build;
import android.view.Surface;



public class FFUtils {



    public static native String urlProtocolInfo();

    public static native String avFormatInfo();

    public static native String avCodecInfo();

    public static native String avFilterInfo();

    public static native void playVideo(String videoPath, Surface surface);

    public static native String ffmpegInfo();
    public static native String analyStreams(String videoPath);
}
