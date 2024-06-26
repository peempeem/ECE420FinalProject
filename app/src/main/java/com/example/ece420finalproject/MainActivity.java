package com.example.ece420finalproject;

import org.opencv.android.OpenCVLoader;
import org.w3c.dom.Text;

import android.Manifest;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.os.Bundle;
import android.os.Handler;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.Size;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;
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
import java.util.concurrent.Semaphore;

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("ece420finalproject");
    }
    private static final String TAG = "ECE420FinalProject";

    private ExecutorService analysisExecutor = null;
    private Thread playbackThread = null;
    private Bitmap bitmap;
    private ImageView viewFinder;
    private SeekBar transpose;
    private Button capture;
    private Button restartPlayback;
    private Button calibrateCamera;
    private TextView detectedKeyText;
    private TextView transposedKeyText;

    private enum DisplayState {
        PREROLLING,
        CAPTURED,
        CALIBRATE,
        APP1,
        APP2,
        END
    }

    private DisplayState displayState = DisplayState.PREROLLING;

    private native void initCPP();
    private native void pauseCPP();
    private native void resumeCPP();
    private native void endCPP();
    private native void processImage(Bitmap bitmap);
    private native void beginCalibration();
    private native void calibrationStep(Bitmap bitmap);
    private native void endCalibration();
    private native void restartPlayback();
    private native void stepPlayback(Bitmap bitmap);
    private native void transpose(int transNumber);
    private native String detectedKey();
    private native String transposedKey();

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
        restartPlayback = findViewById(R.id.restartPlayback);
        calibrateCamera = findViewById(R.id.calibrateCamera);

        detectedKeyText = findViewById(R.id.detectedKeyText);
        transposedKeyText = findViewById(R.id.transposedKeyText);

        capture.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (displayState == DisplayState.PREROLLING)
                    displayState = DisplayState.CAPTURED;
                else
                {
                    displayState = DisplayState.PREROLLING;
                    capture.setText("Take Photo");
                }
            }
        });

        restartPlayback.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                displayState = DisplayState.APP1;
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

        transpose.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                transpose(progress - 6);
                transposedKeyText.setText(transposedKey());
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

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

            switch (displayState)
            {
                case PREROLLING:
                    bitmap = imageProxy.toBitmap();
                    break;

                case CAPTURED:
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
                            displayState = DisplayState.APP1;
                            capture.setText("Retake Photo");
                            capture.setEnabled(true);
                        }
                    });
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

        playbackThread = new Thread()
        {
            private final Semaphore mutex = new Semaphore(1);
            @Override
            public void run() {
                try {
                    while (displayState != DisplayState.END) {
                        if (displayState == DisplayState.APP1)
                        {
                            restartPlayback();
                            runOnUiThread(new Runnable() {
                                @Override
                                public void run() {
                                    if (!detectedKey().equals("None")) {
                                        detectedKeyText.setText(detectedKey());
                                        detectedKeyText.setTextColor(getResources().getColor(R.color.green));
                                        transposedKeyText.setText(transposedKey());
                                        transposedKeyText.setTextColor(getResources().getColor(R.color.green));
                                    }
                                    else {
                                        detectedKeyText.setText("None");
                                        detectedKeyText.setTextColor(getResources().getColor(R.color.red));
                                        transposedKeyText.setText("None");
                                        transposedKeyText.setTextColor(getResources().getColor(R.color.red));
                                    }

                                }
                            });
                            displayState = DisplayState.APP2;
                        }
                        if (displayState == DisplayState.APP2) {
                            mutex.acquire();
                            stepPlayback(bitmap);
                            mutex.release();

                            runOnUiThread(new Runnable() {
                                @Override
                                public void run() {
                                    try {
                                        mutex.acquire();
                                    } catch (Exception e) {e.printStackTrace(); }
                                    Bitmap tempBMP = Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), null, true);
                                    mutex.release();
                                    viewFinder.setImageBitmap(tempBMP);
                                }
                            });
                        }
                        sleep(16);
                    }
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        };
        playbackThread.start();
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
        displayState = DisplayState.END;
        if (analysisExecutor != null)
            analysisExecutor.shutdown();
        endCPP();
        if (playbackThread != null)
        {
            try {
                playbackThread.join(1000);
            }
            catch (Exception e) {}
        }
    }
}