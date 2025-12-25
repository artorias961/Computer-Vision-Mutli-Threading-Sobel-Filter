#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // Load an image (replace with your own file if needed)
    cv::Mat image = cv::imread("pictures/test.jpg");

    if (image.empty()) {
        std::cout << "Could not read the image!" << std::endl;
        return -1;
    }

    cv::imshow("Test Window", image);

    cv::waitKey(0);  // Wait for any key
    return 0;
}