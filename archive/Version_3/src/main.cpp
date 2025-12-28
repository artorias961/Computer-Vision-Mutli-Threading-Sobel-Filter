#include <opencv2/opencv.hpp>
#include <iostream>
#include <cmath>

int main() {

    // Load input image (BGR by default)
    cv::Mat image = cv::imread("pictures/test.jpg");
    if (image.empty()) {
        std::cout << "Could not read the image!" << std::endl;
        return -1;
    }

    // ------------------------------------------------------------------
    // 1) Convert input image to grayscale (required for Sobel)
    // ------------------------------------------------------------------
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    // ------------------------------------------------------------------
    // 2) Allocate output matrices
    //    Use CV_32F to avoid overflow during convolution
    // ------------------------------------------------------------------
    cv::Mat gx(gray.size(), CV_32F, cv::Scalar(0));    // Horizontal gradient
    cv::Mat gy(gray.size(), CV_32F, cv::Scalar(0));    // Vertical gradient
    cv::Mat mag(gray.size(), CV_32F, cv::Scalar(0));   // Gradient magnitude
    cv::Mat theta(gray.size(), CV_32F, cv::Scalar(0)); // Gradient direction (radians)

    // ------------------------------------------------------------------
    // 3) Define Sobel kernels (Wikipedia formulation)
    // ------------------------------------------------------------------
    int kx[3][3] = {
        {-1,  0, +1},
        {-2,  0, +2},
        {-1,  0, +1}
    };

    int ky[3][3] = {
        {-1, -2, -1},
        { 0,  0,  0},
        {+1, +2, +1}
    };

    // ------------------------------------------------------------------
    // 4) Manual convolution
    //    Skip image borders to avoid out-of-bounds access
    // ------------------------------------------------------------------
    for (int y = 1; y < gray.rows - 1; y++) {
        for (int x = 1; x < gray.cols - 1; x++) {

            float sumX = 0.0f;  // Accumulator for Gx
            float sumY = 0.0f;  // Accumulator for Gy

            // Apply 3×3 Sobel kernels
            for (int j = -1; j <= 1; j++) {
                for (int i = -1; i <= 1; i++) {

                    // Pixel value from grayscale image
                    uchar p = gray.at<uchar>(y + j, x + i);

                    // Convolution with Sobel kernels
                    sumX += p * kx[j + 1][i + 1];
                    sumY += p * ky[j + 1][i + 1];
                }
            }

            // Store horizontal and vertical gradients
            gx.at<float>(y, x) = sumX;
            gy.at<float>(y, x) = sumY;

            // Compute gradient magnitude: sqrt(Gx^2 + Gy^2)
            mag.at<float>(y, x) = std::sqrt(sumX * sumX + sumY * sumY);

            // Compute gradient direction (theta) in radians
            // Range: [-pi, +pi]
            theta.at<float>(y, x) = std::atan2(sumY, sumX);
        }
    }

    // ------------------------------------------------------------------
    // 5) Normalize results for visualization (0–255)
    // ------------------------------------------------------------------
    cv::Mat gx8, gy8, mag8, theta8;

    cv::normalize(cv::abs(gx), gx8, 0, 255, cv::NORM_MINMAX);
    cv::normalize(cv::abs(gy), gy8, 0, 255, cv::NORM_MINMAX);
    cv::normalize(mag,        mag8, 0, 255, cv::NORM_MINMAX);
    cv::normalize(theta,    theta8, 0, 255, cv::NORM_MINMAX);

    gx8.convertTo(gx8, CV_8U);
    gy8.convertTo(gy8, CV_8U);
    mag8.convertTo(mag8, CV_8U);
    theta8.convertTo(theta8, CV_8U);

    // ------------------------------------------------------------------
    // 6) Display results
    // ------------------------------------------------------------------
    cv::imshow("Original", image);
    cv::imshow("Grayscale", gray);
    cv::imshow("Sobel Gx (manual)", gx8);
    cv::imshow("Sobel Gy (manual)", gy8);
    cv::imshow("Sobel Magnitude", mag8);
    cv::imshow("Theta (Direction)", theta8);

    cv::waitKey(0);
    return 0;
}
