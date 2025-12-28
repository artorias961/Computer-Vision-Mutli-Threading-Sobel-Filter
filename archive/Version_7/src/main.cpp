// main.cpp
// ------------------------------------------------------------
// SOBEL 3D on an animated GIF (treat video as volume I(x,y,t)).
//
// Computes:
//   Gx = dI/dx (with smoothing in y and t)
//   Gy = dI/dy (with smoothing in x and t)
//   Gt = dI/dt (with smoothing in x and y)
// and magnitude3D = sqrt(Gx^2 + Gy^2 + Gt^2)
//
// Loops playback until key press, saves MP4 outputs to output/.
// ------------------------------------------------------------

#include <opencv2/opencv.hpp>
#include <iostream>
#include <cmath>
#include <filesystem>

static inline float sqr(float v) { return v * v; }

int main() {

    // ------------------------------------------------------------
    // Input / output configuration
    // ------------------------------------------------------------
    const std::string gifPath = "pictures/silk_song.gif";
    const std::string outDir  = "output";
    const int fps     = 30;
    const int delayMs = 30;

    std::filesystem::create_directories(outDir);

    // ------------------------------------------------------------
    // Open GIF as a video stream
    // ------------------------------------------------------------
    cv::VideoCapture cap(gifPath);
    if (!cap.isOpened()) {
        std::cout << "Could not open GIF: " << gifPath << std::endl;
        return -1;
    }

    // Read one frame to determine frame size
    cv::Mat tmp;
    cap >> tmp;
    if (tmp.empty()) {
        std::cout << "GIF has no frames." << std::endl;
        return -1;
    }
    const cv::Size frameSize = tmp.size();

    // MP4 writers (use BGR frames for compatibility)
    int fourcc = cv::VideoWriter::fourcc('m','p','4','v');

    cv::VideoWriter outGt      (outDir + "/sobel3d_gt.mp4",       fourcc, fps, frameSize, true);
    cv::VideoWriter outMag3D   (outDir + "/sobel3d_mag.mp4",      fourcc, fps, frameSize, true);
    cv::VideoWriter outOriginal(outDir + "/original.mp4",         fourcc, fps, frameSize, true);

    if (!outGt.isOpened() || !outMag3D.isOpened() || !outOriginal.isOpened()) {
        std::cout << "Failed to open MP4 outputs. Check OpenCV/FFMPEG support." << std::endl;
        return -1;
    }

    // ------------------------------------------------------------
    // 3D Sobel (separable) components
    // smooth = [1 2 1], deriv = [-1 0 +1]
    // This is the same idea as 2D Sobel: derivative in one axis + smoothing in others.
    // ------------------------------------------------------------
    const int smooth[3] = { 1, 2, 1 };
    const int deriv [3] = { -1, 0, 1 };

    // ------------------------------------------------------------
    // We need 3 consecutive grayscale frames: prev, curr, next
    // to compute temporal derivative (Gt).
    // ------------------------------------------------------------
    auto reset_and_prime = [&]() -> bool {
        cap.release();
        cap.open(gifPath);
        if (!cap.isOpened()) return false;
        return true;
    };

    // Rewind to start
    cap.set(cv::CAP_PROP_POS_FRAMES, 0);

    cv::Mat framePrevBgr, frameCurrBgr, frameNextBgr;
    cv::Mat prev, curr, next; // grayscale CV_8U

    // Prime the buffer: prev, curr, next
    cap >> framePrevBgr;
    cap >> frameCurrBgr;
    cap >> frameNextBgr;

    if (framePrevBgr.empty() || frameCurrBgr.empty() || frameNextBgr.empty()) {
        std::cout << "GIF must have at least 3 frames for Sobel 3D." << std::endl;
        return -1;
    }

    cv::cvtColor(framePrevBgr, prev, cv::COLOR_BGR2GRAY);
    cv::cvtColor(frameCurrBgr, curr, cv::COLOR_BGR2GRAY);
    cv::cvtColor(frameNextBgr, next, cv::COLOR_BGR2GRAY);

    // ------------------------------------------------------------
    // Main loop (process "curr" using prev/curr/next)
    // ------------------------------------------------------------
    while (true) {

        // Allocate outputs (float)
        cv::Mat gx(curr.size(), CV_32F, cv::Scalar(0));
        cv::Mat gy(curr.size(), CV_32F, cv::Scalar(0));
        cv::Mat gt(curr.size(), CV_32F, cv::Scalar(0));
        cv::Mat mag3d(curr.size(), CV_32F, cv::Scalar(0));

        // --------------------------------------------------------
        // 3D convolution over a 3x3x3 neighborhood
        // We skip borders in x/y because we need (x±1, y±1).
        // Time borders are handled by using prev/curr/next already.
        // --------------------------------------------------------
        for (int y = 1; y < curr.rows - 1; y++) {
            for (int x = 1; x < curr.cols - 1; x++) {

                float sumX = 0.0f;
                float sumY = 0.0f;
                float sumT = 0.0f;

                // dt: -1 (prev), 0 (curr), +1 (next)
                for (int dt = -1; dt <= 1; dt++) {
                    const cv::Mat* It =
                        (dt == -1) ? &prev :
                        (dt ==  0) ? &curr : &next;

                    int wt_s = smooth[dt + 1];
                    int wt_d = deriv[dt + 1];

                    // dy: -1..+1
                    for (int dy = -1; dy <= 1; dy++) {
                        int wy_s = smooth[dy + 1];
                        int wy_d = deriv[dy + 1];

                        // dx: -1..+1
                        for (int dx = -1; dx <= 1; dx++) {
                            int wx_s = smooth[dx + 1];
                            int wx_d = deriv[dx + 1];

                            float p = static_cast<float>(It->at<uchar>(y + dy, x + dx));

                            // Gx: derivative in x, smooth in y and t
                            sumX += p * (wx_d * wy_s * wt_s);

                            // Gy: derivative in y, smooth in x and t
                            sumY += p * (wx_s * wy_d * wt_s);

                            // Gt: derivative in t, smooth in x and y
                            sumT += p * (wx_s * wy_s * wt_d);
                        }
                    }
                }

                gx.at<float>(y, x) = sumX;
                gy.at<float>(y, x) = sumY;
                gt.at<float>(y, x) = sumT;

                mag3d.at<float>(y, x) = std::sqrt(sqr(sumX) + sqr(sumY) + sqr(sumT));
            }
        }

        // --------------------------------------------------------
        // Normalize for visualization (8-bit)
        // --------------------------------------------------------
        cv::Mat gt8, mag3d8;
        cv::normalize(cv::abs(gt),    gt8,    0, 255, cv::NORM_MINMAX);
        cv::normalize(mag3d,       mag3d8,    0, 255, cv::NORM_MINMAX);

        gt8.convertTo(gt8, CV_8U);
        mag3d8.convertTo(mag3d8, CV_8U);

        // Convert to BGR for MP4 writing
        cv::Mat gtBgr, mag3dBgr;
        cv::cvtColor(gt8,    gtBgr,    cv::COLOR_GRAY2BGR);
        cv::cvtColor(mag3d8, mag3dBgr, cv::COLOR_GRAY2BGR);

        // --------------------------------------------------------
        // Write outputs for this "curr" time slice
        // --------------------------------------------------------
        outOriginal.write(frameCurrBgr);
        outGt.write(gtBgr);
        outMag3D.write(mag3dBgr);

        // Display
        cv::imshow("Original (curr)", frameCurrBgr);
        cv::imshow("Sobel3D |Gt| (temporal)", gt8);
        cv::imshow("Sobel3D Magnitude", mag3d8);

        // Stop on any key
        if (cv::waitKey(delayMs) != -1)
            break;

        // --------------------------------------------------------
        // Advance time window: prev <- curr <- next <- new
        // --------------------------------------------------------
        framePrevBgr = frameCurrBgr;
        frameCurrBgr = frameNextBgr;
        prev = curr;
        curr = next;

        cap >> frameNextBgr;

        // If GIF ends, restart and re-prime buffers
        if (frameNextBgr.empty()) {
            if (!reset_and_prime()) break;

            cap >> framePrevBgr;
            cap >> frameCurrBgr;
            cap >> frameNextBgr;

            if (framePrevBgr.empty() || frameCurrBgr.empty() || frameNextBgr.empty())
                break;

            cv::cvtColor(framePrevBgr, prev, cv::COLOR_BGR2GRAY);
            cv::cvtColor(frameCurrBgr, curr, cv::COLOR_BGR2GRAY);
            cv::cvtColor(frameNextBgr, next, cv::COLOR_BGR2GRAY);

            continue;
        }

        cv::cvtColor(frameNextBgr, next, cv::COLOR_BGR2GRAY);
    }

    return 0;
}
