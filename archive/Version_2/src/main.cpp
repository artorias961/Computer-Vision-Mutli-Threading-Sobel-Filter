#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // Load an image
    cv::Mat image = cv::imread("pictures/test.jpg");

    if (image.empty()) {
        std::cout << "Could not read the image!" << std::endl;
        return -1;
    }

    // Convert to grayscale
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    // Apply threshold
    cv::Mat thresh;
    cv::threshold(
        gray,          // input
        thresh,        // output
        128,           // threshold value
        255,           // max value
        cv::THRESH_BINARY
    );

    // Show results
    cv::imshow("Original", image);
    cv::imshow("Grayscale", gray);
    cv::imshow("Threshold", thresh);

    cv::waitKey(0);
    return 0;
}
