// main.cpp
// ------------------------------------------------------------
// Reads an animated GIF frame-by-frame, applies a MANUAL Sobel
// operator (Gx, Gy, Magnitude, Theta), loops playback until a
// key is pressed, and saves each result as MP4 videos inside
// an "output/" directory.
// ------------------------------------------------------------

#include <opencv2/opencv.hpp>
#include <iostream>
#include <cmath>
#include <filesystem>

int main() {

    // ------------------------------------------------------------
    // Input / output configuration
    // ------------------------------------------------------------
    const std::string gifPath = "pictures/silk_song.gif"; // Input GIF
    const std::string outDir  = "output";                  // Output folder
    const int fps      = 30;                                // Output video FPS
    const int delayMs = 30;                                 // Playback delay (ms)

    // ------------------------------------------------------------
    // Create output directory if it does not exist
    // ------------------------------------------------------------
    std::filesystem::create_directories(outDir);

    // ------------------------------------------------------------
    // Open the animated GIF as a video stream
    // (GIFs are treated as videos in OpenCV)
    // ------------------------------------------------------------
    cv::VideoCapture cap(gifPath);
    if (!cap.isOpened()) {
        std::cout << "Could not open the GIF! Check path: "
                  << gifPath << std::endl;
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
    // Grayscale images are converted to BGR before writing
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
    const int kx[3][3] = {
        {-1,  0, +1},
        {-2,  0, +2},
        {-1,  0, +1}
    };

    const int ky[3][3] = {
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
        cv::Mat gx(gray.size(), CV_32F, cv::Scalar(0));     // Horizontal gradient
        cv::Mat gy(gray.size(), CV_32F, cv::Scalar(0));     // Vertical gradient
        cv::Mat mag(gray.size(), CV_32F, cv::Scalar(0));    // Gradient magnitude
        cv::Mat theta(gray.size(), CV_32F, cv::Scalar(0));  // Gradient direction (radians)

        // --------------------------------------------------------
        // 3) Manual Sobel convolution (skip borders)
        // --------------------------------------------------------
        for (int y = 1; y < gray.rows - 1; y++) {
            for (int x = 1; x < gray.cols - 1; x++) {

                float sumX = 0.0f;
                float sumY = 0.0f;

                // Apply 3x3 Sobel kernels
                for (int j = -1; j <= 1; j++) {
                    for (int i = -1; i <= 1; i++) {
                        uchar p = gray.at<uchar>(y + j, x + i);
                        sumX += p * kx[j + 1][i + 1];
                        sumY += p * ky[j + 1][i + 1];
                    }
                }

                // Store results
                gx.at<float>(y, x) = sumX;
                gy.at<float>(y, x) = sumY;
                mag.at<float>(y, x) = std::sqrt(sumX * sumX + sumY * sumY);
                theta.at<float>(y, x) = std::atan2(sumY, sumX);
            }
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
        cv::imshow("Sobel Magnitude", mag8);

        // Press ANY key to stop playback
        if (cv::waitKey(delayMs) != -1)
            break;
    }

    return 0;
}
