#include <opencv2/opencv.hpp>
#include <iostream>

int main() {

    // ------------------------------------------------------------------
    // Load an image from disk
    // imread() loads the image in BGR format by default
    // ------------------------------------------------------------------
    cv::Mat image = cv::imread("pictures/test.jpg");

    // ------------------------------------------------------------------
    // Check if the image was loaded successfully
    // If the file path is wrong or file is missing, image will be empty
    // ------------------------------------------------------------------
    if (image.empty()) {
        std::cout << "Could not read the image!" << std::endl;
        return -1;
    }

    // ------------------------------------------------------------------
    // Display the loaded image in a window
    // ------------------------------------------------------------------
    cv::imshow("Test Window", image);

    // ------------------------------------------------------------------
    // Wait indefinitely until a key is pressed
    // Required so the window does not close immediately
    // ------------------------------------------------------------------
    cv::waitKey(0);

    return 0;
}
