// main.cpp
// ------------------------------------------------------------
// Load a single image (test.jpg), run MANUAL Sobel (Gx, Gy,
// Magnitude, Theta) using Windows threads.
// The image is divided into 4 regions (quadrants), each processed
// by one thread.
// Results are displayed and saved to output/.
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
    const cv::Mat* gray;   // input grayscale (CV_8U)
    cv::Mat* gx;           // output Gx (CV_32F)
    cv::Mat* gy;           // output Gy (CV_32F)
    cv::Mat* mag;          // output magnitude (CV_32F)
    cv::Mat* theta;        // output direction (CV_32F)

    int x0, x1;            // region bounds in x: [x0, x1)
    int y0, y1;            // region bounds in y: [y0, y1)

    const int (*kx)[3];    // Sobel kernel X (3x3)
    const int (*ky)[3];    // Sobel kernel Y (3x3)
};

// ------------------------------------------------------------
// Thread procedure
// Must match WINAPI signature for CreateThread
// ------------------------------------------------------------
DWORD WINAPI SobelWorker(LPVOID param) {
    SobelTask* t = reinterpret_cast<SobelTask*>(param);

    const cv::Mat& gray = *(t->gray);
    cv::Mat& gx = *(t->gx);
    cv::Mat& gy = *(t->gy);
    cv::Mat& mag = *(t->mag);
    cv::Mat& theta = *(t->theta);

    // Clamp region to safe convolution bounds (skip borders)
    int startY = std::max(t->y0, 1);
    int endY   = std::min(t->y1, gray.rows - 1); // exclusive end, but we keep < rows-1
    int startX = std::max(t->x0, 1);
    int endX   = std::min(t->x1, gray.cols - 1);

    // Manual Sobel convolution on this region
    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {

            float sumX = 0.0f;
            float sumY = 0.0f;

            // Apply 3x3 kernels
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
    // Paths
    // ------------------------------------------------------------
    const std::string imagePath = "pictures/test.jpg";
    const std::string outDir = "output";

    // Create output folder
    std::filesystem::create_directories(outDir);

    // ------------------------------------------------------------
    // Load input image (BGR)
    // ------------------------------------------------------------
    cv::Mat image = cv::imread(imagePath);
    if (image.empty()) {
        std::cout << "Could not read image: " << imagePath << std::endl;
        return -1;
    }

    // ------------------------------------------------------------
    // Convert to grayscale (CV_8U)
    // ------------------------------------------------------------
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    // ------------------------------------------------------------
    // Allocate output mats (float to avoid overflow)
    // ------------------------------------------------------------
    cv::Mat gx(gray.size(), CV_32F, cv::Scalar(0));
    cv::Mat gy(gray.size(), CV_32F, cv::Scalar(0));
    cv::Mat mag(gray.size(), CV_32F, cv::Scalar(0));
    cv::Mat theta(gray.size(), CV_32F, cv::Scalar(0));

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
    // Divide the image into 4 quadrants
    // Region bounds are [x0, x1) and [y0, y1)
    // ------------------------------------------------------------
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

    // ------------------------------------------------------------
    // Create 4 Windows threads
    // ------------------------------------------------------------
    HANDLE threads[4];
    DWORD threadIds[4];

    for (int i = 0; i < 4; i++) {
        threads[i] = CreateThread(
            nullptr,           // default security
            0,                 // default stack size
            SobelWorker,       // thread function
            &tasks[i],         // argument
            0,                 // run immediately
            &threadIds[i]      // thread id (optional)
        );

        if (threads[i] == nullptr) {
            std::cout << "Failed to create thread " << i << std::endl;
            return -1;
        }
    }

    // Wait for all threads to finish
    WaitForMultipleObjects(4, threads, TRUE, INFINITE);

    // Close thread handles
    for (int i = 0; i < 4; i++) {
        CloseHandle(threads[i]);
    }

    // ------------------------------------------------------------
    // Normalize results to 8-bit for display & saving
    // ------------------------------------------------------------
    cv::Mat gx8, gy8, mag8, theta8;
    cv::normalize(cv::abs(gx), gx8, 0, 255, cv::NORM_MINMAX);
    cv::normalize(cv::abs(gy), gy8, 0, 255, cv::NORM_MINMAX);
    cv::normalize(mag,        mag8, 0, 255, cv::NORM_MINMAX);
    cv::normalize(theta,    theta8, 0, 255, cv::NORM_MINMAX);

    gx8.convertTo(gx8, CV_8U);
    gy8.convertTo(gy8, CV_8U);
    mag8.convertTo(mag8, CV_8U);
    theta8.convertTo(theta8, CV_8U);

    // ------------------------------------------------------------
    // Save results
    // ------------------------------------------------------------
    cv::imwrite(outDir + "/original.png", image);
    cv::imwrite(outDir + "/gray.png", gray);
    cv::imwrite(outDir + "/gx.png", gx8);
    cv::imwrite(outDir + "/gy.png", gy8);
    cv::imwrite(outDir + "/magnitude.png", mag8);
    cv::imwrite(outDir + "/theta.png", theta8);

    // ------------------------------------------------------------
    // Display results
    // ------------------------------------------------------------
    cv::imshow("Original", image);
    cv::imshow("Grayscale", gray);
    cv::imshow("Sobel Gx (manual, 4 threads)", gx8);
    cv::imshow("Sobel Gy (manual, 4 threads)", gy8);
    cv::imshow("Sobel Magnitude (manual, 4 threads)", mag8);
    cv::imshow("Theta (Direction)", theta8);

    cv::waitKey(0);
    return 0;
}
