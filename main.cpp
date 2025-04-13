#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    cv::Mat image(100, 100, CV_8UC3, cv::Scalar(0, 255, 0));  // Green image
    if (image.empty()) {
        std::cerr << "Image not created!" << std::endl;
        return -1;
    }

    cv::imshow("Test Image", image);
    cv::waitKey(0);
    return 0;
}