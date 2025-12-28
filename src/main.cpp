// main.cpp
// ------------------------------------------------------------
// Reads an animated GIF frame-by-frame, applies MANUAL Sobel
// using 4 Windows threads (image split into 4 quadrants),
// loops playback until a key is pressed, and saves results as
// MP4 videos inside "output/".
// ------------------------------------------------------------

#include <opencv2/opencv.hpp>
#include <iostream>
#include <cmath>
#include <filesystem>
#include <windows.h>

// ------------------------------------------------------------
// Thread argument struct (shared Mats + region bounds)
// ------------------------------------------------------------
struct SobelTask {
    const cv::Mat* gray;        // input grayscale (CV_8U)
    cv::Mat* gx;                // output Gx (CV_32F)
    cv::Mat* gy;                // output Gy (CV_32F)
    cv::Mat* mag;               // output magnitude (CV_32F)
    cv::Mat* theta;             // output direction (CV_32F)

    int x0, x1;                 // region bounds in x: [x0, x1)
    int y0, y1;                 // region bounds in y: [y0, y1)

    const int (*kx)[3];         // Sobel kernel X (3x3)
    const int (*ky)[3];         // Sobel kernel Y (3x3)
};

// ------------------------------------------------------------
// Windows thread function
// ------------------------------------------------------------
DWORD WINAPI SobelWorker(LPVOID param) {
    SobelTask* t = reinterpret_cast<SobelTask*>(param);

    const cv::Mat& gray = *(t->gray);
    cv::Mat& gx = *(t->gx);
    cv::Mat& gy = *(t->gy);
    cv::Mat& mag = *(t->mag);
    cv::Mat& theta = *(t->theta);

    // --------------------------------------------------------
    // Clamp region to safe convolution area (skip borders)
    // We need neighbors (x±1, y±1), so avoid 0 and last index.
    // --------------------------------------------------------
    int startY = std::max(t->y0, 1);
    int endY   = std::min(t->y1, gray.rows - 1); // exclusive end, keep y < rows-1
    int startX = std::max(t->x0, 1);
    int endX   = std::min(t->x1, gray.cols - 1);

    // --------------------------------------------------------
    // Manual Sobel convolution on this region
    // --------------------------------------------------------
    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {

            float sumX = 0.0f;
            float sumY = 0.0f;

            // Apply 3x3 kernels around pixel (x, y)
            for (int j = -1; j <= 1; j++) {
                for (int i = -1; i <= 1; i++) {
                    uchar p = gray.at<uchar>(y + j, x + i);
                    sumX += p * t->kx[j + 1][i + 1];
                    sumY += p * t->ky[j + 1][i + 1];
                }
            }

            gx.at<float>(y, x) = sumX;
            gy.at<float>(y, x) = sumY;
            mag.at<float>(y, x) = std::sqrt(sumX * sumX + sumY * sumY);
            theta.at<float>(y, x) = std::atan2(sumY, sumX); // radians [-pi, pi]
        }
    }

    return 0;
}

int main() {

    // ------------------------------------------------------------
    // Input / output configuration
    // ------------------------------------------------------------
    const std::string gifPath = "pictures/silk_song.gif"; // Input GIF
    const std::string outDir  = "output";                 // Output folder
    const int fps      = 30;                              // Output video FPS
    const int delayMs  = 30;                              // Playback delay (ms)

    // ------------------------------------------------------------
    // Create output directory if it does not exist
    // ------------------------------------------------------------
    std::filesystem::create_directories(outDir);

    // ------------------------------------------------------------
    // Open the animated GIF as a video stream
    // ------------------------------------------------------------
    cv::VideoCapture cap(gifPath);
    if (!cap.isOpened()) {
        std::cout << "Could not open the GIF! Check path: " << gifPath << std::endl;
        return -1;
    }

    // ------------------------------------------------------------
    // Read the first frame to determine frame size
    // ------------------------------------------------------------
    cv::Mat frame;
    cap >> frame;
    if (frame.empty()) {
        std::cout << "GIF contains no frames." << std::endl;
        return -1;
    }
    const cv::Size frameSize = frame.size();

    // ------------------------------------------------------------
    // Create MP4 video writers (requires OpenCV built with FFMPEG)
    // Grayscale images are converted to BGR before writing.
    // ------------------------------------------------------------
    int fourcc = cv::VideoWriter::fourcc('m','p','4','v');

    cv::VideoWriter outOriginal(outDir + "/original.mp4",  fourcc, fps, frameSize, true);
    cv::VideoWriter outGray     (outDir + "/gray.mp4",      fourcc, fps, frameSize, true);
    cv::VideoWriter outGx       (outDir + "/gx.mp4",        fourcc, fps, frameSize, true);
    cv::VideoWriter outGy       (outDir + "/gy.mp4",        fourcc, fps, frameSize, true);
    cv::VideoWriter outMag      (outDir + "/magnitude.mp4", fourcc, fps, frameSize, true);
    cv::VideoWriter outTheta    (outDir + "/theta.mp4",     fourcc, fps, frameSize, true);

    // Verify all writers opened correctly
    if (!outOriginal.isOpened() || !outGray.isOpened() ||
        !outGx.isOpened() || !outGy.isOpened() ||
        !outMag.isOpened() || !outTheta.isOpened()) {

        std::cout << "Failed to open one or more MP4 outputs. "
                     "Check OpenCV FFMPEG support." << std::endl;
        return -1;
    }

    // ------------------------------------------------------------
    // Sobel kernels (Wikipedia formulation)
    // ------------------------------------------------------------
    static const int kx[3][3] = {
        {-1,  0, +1},
        {-2,  0, +2},
        {-1,  0, +1}
    };

    static const int ky[3][3] = {
        {-1, -2, -1},
        { 0,  0,  0},
        {+1, +2, +1}
    };

    // ------------------------------------------------------------
    // Rewind GIF to the first frame before full processing
    // ------------------------------------------------------------
    cap.set(cv::CAP_PROP_POS_FRAMES, 0);

    // ------------------------------------------------------------
    // Main playback + processing loop
    // Loops until a key is pressed
    // ------------------------------------------------------------
    while (true) {

        // Read next frame
        cap >> frame;

        // If end of GIF reached, restart playback
        if (frame.empty()) {
            cap.release();
            cap.open(gifPath);
            if (!cap.isOpened())
                break;
            continue;
        }

        // --------------------------------------------------------
        // 1) Convert current frame to grayscale
        // --------------------------------------------------------
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        // --------------------------------------------------------
        // 2) Allocate Sobel output matrices (float to avoid overflow)
        // --------------------------------------------------------
        cv::Mat gx(gray.size(), CV_32F, cv::Scalar(0));
        cv::Mat gy(gray.size(), CV_32F, cv::Scalar(0));
        cv::Mat mag(gray.size(), CV_32F, cv::Scalar(0));
        cv::Mat theta(gray.size(), CV_32F, cv::Scalar(0));

        // --------------------------------------------------------
        // 3) Split image into 4 quadrants and run 4 threads
        // --------------------------------------------------------
        int midX = gray.cols / 2;
        int midY = gray.rows / 2;

        SobelTask tasks[4] = {
            // Top-left
            { &gray, &gx, &gy, &mag, &theta, 0,    midX, 0,    midY, kx, ky },
            // Top-right
            { &gray, &gx, &gy, &mag, &theta, midX, gray.cols, 0,    midY, kx, ky },
            // Bottom-left
            { &gray, &gx, &gy, &mag, &theta, 0,    midX, midY, gray.rows, kx, ky },
            // Bottom-right
            { &gray, &gx, &gy, &mag, &theta, midX, gray.cols, midY, gray.rows, kx, ky }
        };

        HANDLE threads[4];
        DWORD threadIds[4];

        for (int i = 0; i < 4; i++) {
            threads[i] = CreateThread(
                nullptr,        // default security
                0,              // default stack size
                SobelWorker,    // thread function
                &tasks[i],      // argument
                0,              // run immediately
                &threadIds[i]   // thread id (optional)
            );

            if (threads[i] == nullptr) {
                std::cout << "Failed to create thread " << i << std::endl;
                return -1;
            }
        }

        // Wait until all threads finish this frame
        WaitForMultipleObjects(4, threads, TRUE, INFINITE);

        // Close thread handles
        for (int i = 0; i < 4; i++) {
            CloseHandle(threads[i]);
        }

        // --------------------------------------------------------
        // 4) Normalize Sobel results to 8-bit for display & saving
        // --------------------------------------------------------
        cv::Mat gx8, gy8, mag8, theta8;
        cv::normalize(cv::abs(gx), gx8, 0, 255, cv::NORM_MINMAX);
        cv::normalize(cv::abs(gy), gy8, 0, 255, cv::NORM_MINMAX);
        cv::normalize(mag,        mag8, 0, 255, cv::NORM_MINMAX);
        cv::normalize(theta,    theta8, 0, 255, cv::NORM_MINMAX);

        gx8.convertTo(gx8, CV_8U);
        gy8.convertTo(gy8, CV_8U);
        mag8.convertTo(mag8, CV_8U);
        theta8.convertTo(theta8, CV_8U);

        // --------------------------------------------------------
        // 5) Convert single-channel images to BGR for MP4 writing
        // --------------------------------------------------------
        cv::Mat grayBgr, gxBgr, gyBgr, magBgr, thetaBgr;
        cv::cvtColor(gray,   grayBgr,  cv::COLOR_GRAY2BGR);
        cv::cvtColor(gx8,    gxBgr,    cv::COLOR_GRAY2BGR);
        cv::cvtColor(gy8,    gyBgr,    cv::COLOR_GRAY2BGR);
        cv::cvtColor(mag8,   magBgr,   cv::COLOR_GRAY2BGR);
        cv::cvtColor(theta8, thetaBgr, cv::COLOR_GRAY2BGR);

        // --------------------------------------------------------
        // 6) Write frames to output MP4 files
        // --------------------------------------------------------
        outOriginal.write(frame);
        outGray.write(grayBgr);
        outGx.write(gxBgr);
        outGy.write(gyBgr);
        outMag.write(magBgr);
        outTheta.write(thetaBgr);

        // --------------------------------------------------------
        // 7) Display (optional, for live feedback)
        // --------------------------------------------------------
        cv::imshow("Original", frame);
        cv::imshow("Sobel Magnitude (manual, 4 threads)", mag8);

        // Press ANY key to stop playback
        if (cv::waitKey(delayMs) != -1)
            break;
    }

    return 0;
}
