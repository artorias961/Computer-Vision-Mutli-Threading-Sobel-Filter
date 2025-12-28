#include <opencv2/opencv.hpp>
#include <iostream>

int main() {

    // ------------------------------------------------------------------
    // Load input image (BGR format by default)
    // ------------------------------------------------------------------
    cv::Mat image = cv::imread("pictures/test.jpg");

    // Check if image was loaded successfully
    if (image.empty()) {
        std::cout << "Could not read the image!" << std::endl;
        return -1;
    }

    // ------------------------------------------------------------------
    // Convert image from BGR to Grayscale
    // Thresholding operates on single-channel images
    // ------------------------------------------------------------------
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    // ------------------------------------------------------------------
    // Apply binary thresholding
    // Pixels > threshold value → max value (white)
    // Pixels <= threshold value → 0 (black)
    // ------------------------------------------------------------------
    cv::Mat thresh;
    cv::threshold(
        gray,              // Input grayscale image
        thresh,            // Output binary image
        128,               // Threshold value
        255,               // Maximum value assigned to pixels
        cv::THRESH_BINARY  // Binary threshold type
    );

    // ------------------------------------------------------------------
    // Display results
    // ------------------------------------------------------------------
    cv::imshow("Original", image);
    cv::imshow("Grayscale", gray);
    cv::imshow("Threshold", thresh);

    // Wait indefinitely until a key is pressed
    cv::waitKey(0);
    return 0;
}
