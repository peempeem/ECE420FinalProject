package com.example.ece420finalproject;

import org.opencv.android.OpenCVLoader;

import android.Manifest;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.Size;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.camera.core.CameraSelector;
import androidx.camera.core.ImageAnalysis;
import androidx.camera.core.resolutionselector.AspectRatioStrategy;
import androidx.camera.core.resolutionselector.ResolutionSelector;
import androidx.camera.lifecycle.ProcessCameraProvider;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;
import androidx.lifecycle.LifecycleOwner;

import com.google.common.util.concurrent.ListenableFuture;

import java.nio.ByteBuffer;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("ece420finalproject");
    }
    private static final String TAG = "ECE420FinalProject";

    private ExecutorService analysisExecutor = null;
    private ImageView viewFinder;
    private SeekBar transpose;
    private Button capture;
    private Button showAccumulator;
    private Button showAudioStats;
    private Button calibrateCamera;

    private enum DisplayState {
        PREROLLING,
        CAPTURED1,
        CAPTURED2,
        ACCUMULATOR,
        AUDIOSTATS,
        CALIBRATE
    }

    private DisplayState displayState = DisplayState.PREROLLING;
    private boolean processed = false;

    private native void initCPP();
    private native void pauseCPP();
    private native void resumeCPP();
    private native void endCPP();
    private native void processImage(Bitmap bitmap);
    private native void accumulator(Bitmap bitmap);
    private native void audioStats(Bitmap bitmap);
    private native void beginCalibration();
    private native void calibrationStep(Bitmap bitmap);
    private native void endCalibration();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        initCPP();
        setContentView(R.layout.activity_main);

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED ||
                ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED)
            ActivityCompat.requestPermissions(MainActivity.this, new String[] { Manifest.permission.CAMERA, Manifest.permission.RECORD_AUDIO }, 101);

        viewFinder = findViewById(R.id.viewFinder);
        viewFinder.setScaleType(ImageView.ScaleType.FIT_START);
        viewFinder.setBackgroundColor(getResources().getColor(android.R.color.black));

        transpose = findViewById(R.id.transpose);
        capture = findViewById(R.id.captureButton);
        showAccumulator = findViewById(R.id.showAccumulator);
        showAudioStats = findViewById(R.id.showAudioStats);
        calibrateCamera = findViewById(R.id.calibrateCamera);

        capture.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (displayState == DisplayState.PREROLLING)
                    displayState = DisplayState.CAPTURED1;
                else
                {
                    displayState = DisplayState.PREROLLING;
                    capture.setText("Take Photo");
                }

            }
        });

        showAccumulator.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                displayState = DisplayState.ACCUMULATOR;
            }
        });

        showAudioStats.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                displayState = DisplayState.AUDIOSTATS;
            }
        });

        calibrateCamera.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (displayState != DisplayState.CALIBRATE) {
                    beginCalibration();
                    calibrateCamera.setText("END CALIBRATION");
                    displayState = DisplayState.CALIBRATE;
                }
                else {
                    endCalibration();
                    calibrateCamera.setText("CALIBRATE CAMERA");
                    displayState = DisplayState.PREROLLING;
                }
            }
        });

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        WindowInsetsControllerCompat windowInsetsController = WindowCompat.getInsetsController(getWindow(), getWindow().getDecorView());
        windowInsetsController.setSystemBarsBehavior(WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
        windowInsetsController.hide(WindowInsetsCompat.Type.systemBars());

        final ListenableFuture<ProcessCameraProvider> cameraProviderFuture = ProcessCameraProvider.getInstance(this);
        cameraProviderFuture.addListener(new Runnable() {
            @Override
            public void run() {
                try {
                    ProcessCameraProvider cameraProvider = cameraProviderFuture.get();
                    bindCameraProcess(cameraProvider);
                } catch (ExecutionException | InterruptedException e) {

                }
            }
        }, ContextCompat.getMainExecutor(this));
    }

    private void bindCameraProcess(@NonNull ProcessCameraProvider cameraProvider) {
        CameraSelector cameraSelector = new CameraSelector.Builder()
                .requireLensFacing(CameraSelector.LENS_FACING_BACK)
                .build();

        DisplayMetrics displayMetrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);

        ImageAnalysis imageAnalysis = new ImageAnalysis.Builder()
                .setOutputImageFormat(ImageAnalysis.OUTPUT_IMAGE_FORMAT_RGBA_8888)
                .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
                .setTargetResolution(new Size(960, 720))
                .build();

        analysisExecutor = Executors.newSingleThreadExecutor();
        imageAnalysis.setAnalyzer(analysisExecutor, imageProxy -> {

            Bitmap bitmap;

            switch (displayState)
            {
                case PREROLLING:
                    bitmap = imageProxy.toBitmap();
                    break;

                case CAPTURED1:
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            capture.setText("Processing");
                            capture.setEnabled(false);
                        }
                    });

                    bitmap = imageProxy.toBitmap();
                    processImage(bitmap);
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            processed = true;
                            displayState = DisplayState.CAPTURED2;
                            capture.setText("Retake Photo");
                            capture.setEnabled(true);
                        }
                    });
                    break;

                case ACCUMULATOR:
                    bitmap = Bitmap.createBitmap(imageProxy.getWidth(), imageProxy.getHeight(), Bitmap.Config.ARGB_8888);
                    accumulator(bitmap);
                    break;

                case AUDIOSTATS:
                    bitmap = Bitmap.createBitmap(imageProxy.getWidth(), imageProxy.getHeight(), Bitmap.Config.ARGB_8888);
                    audioStats(bitmap);
                    break;

                case CALIBRATE:
                    bitmap = imageProxy.toBitmap();
                    calibrationStep(bitmap);
                    break;

                default:
                    imageProxy.close();
                    return;
            }

            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    viewFinder.setImageBitmap(bitmap);
                }
            });
            imageProxy.close();
        });

        cameraProvider.bindToLifecycle(this, cameraSelector, imageAnalysis);
    }

    @Override
    public void onPause() {
        super.onPause();
        pauseCPP();
    }

    @Override
    public void onResume() {
        super.onResume();
        resumeCPP();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (analysisExecutor != null)
            analysisExecutor.shutdown();
        endCPP();
    }
}