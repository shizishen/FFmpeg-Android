package cc.dewdrop.ffplayer;

import static android.content.ContentValues.TAG;

import static cc.dewdrop.ffplayer.utils.FFUtils.analyStreams;
import static cc.dewdrop.ffplayer.utils.FFUtils.ffmpegInfo;
import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Environment;
import android.os.Bundle;
import android.provider.Settings;
import android.util.Log;
import android.view.View;
import android.widget.TextView;


import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;

import com.android.application.R;

import java.io.File;
import java.net.URL;

import cc.dewdrop.ffplayer.widget.FFVideoView;

public class MainActivity extends AppCompatActivity {

    String url = "https://app.punyapat.me/mjpeg-server/mjpeg";

    static {
        System.loadLibrary("native-lib");
    }

    private TextView mTextView;
    private TextView videoInfoTextView;
    private FFVideoView mVideoView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        requestInternalStoragePermission();


        mTextView = findViewById(R.id.sample_text);
        mVideoView = findViewById(R.id.videoView);
        videoInfoTextView = findViewById(R.id.av_info);
        mTextView.setText(ffmpegInfo());

    }

    public void onButtonClick(View view) {
        int id = view.getId();
        if (id == R.id.button_play){
            String filePath = "/storage/emulated/0/test/test1.mp4";
            File file = new File(filePath);
            String folderUrl = file.getAbsolutePath();
            //String videoPath = Environment.getExternalStorageDirectory() + "/DCIM/Camera/VID_20160527_153309.mp4";
            mVideoView.playVideo(folderUrl);
            videoInfoTextView.setText(analyStreams(folderUrl));
        }


    }



    private final ActivityResultLauncher<Intent> requestPermissionLauncher = registerForActivityResult(
            new ActivityResultContracts.StartActivityForResult(),
            result -> {
                if (result.getResultCode() == Activity.RESULT_OK) {
                    // 用户已授予访问权限
                    // 在此处理访问内部存储的逻辑
                } else {
                    // 用户拒绝了访问权限
                    // 在此处理权限拒绝的逻辑
                }
            }
    );
    private void requestInternalStoragePermission() {
        Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
        Uri uri = Uri.fromParts("package", getPackageName(), null);
        intent.setData(uri);
        requestPermissionLauncher.launch(intent);
    }

}
