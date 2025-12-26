#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // Load an image (replace with your own file if needed)
    cv::Mat image = cv::imread("pictures/test.jpg");

    // Check for errors if image is empty
    if (image.empty()) {
        std::cout << "Could not read the image!" << std::endl;
        return -1;
    }

    // Display image
    cv::imshow("Test Window", image);

    // Wait for any key
    cv::waitKey(0);  
    return 0;
}

