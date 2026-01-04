// main.cpp
// ------------------------------------------------------------
// SOBEL 3D on a video (treat video as volume I(x,y,t)).
//
// Headless (no imshow/waitKey).
// Processes the input ONCE (no looping) so files finalize properly.
// Writes output videos. Prefers MP4; falls back to AVI if codec fails.
// ------------------------------------------------------------

#include <opencv2/opencv.hpp>
#include <iostream>
#include <cmath>
#include <filesystem>
#include <string>

static inline float sqr(float v) { return v * v; }

static bool open_writers_with_fallback(
    const std::string& outDir,
    const cv::Size& frameSize,
    double fps,
    cv::VideoWriter& outOriginal,
    cv::VideoWriter& outGt,
    cv::VideoWriter& outMag3D,
    std::string& usedExt
) {
    // Try MP4 first
    {
        int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
        usedExt = "mp4";

        outOriginal.open(outDir + "/original.mp4",    fourcc, fps, frameSize, true);
        outGt.open      (outDir + "/sobel3d_gt.mp4",  fourcc, fps, frameSize, true);
        outMag3D.open   (outDir + "/sobel3d_mag.mp4", fourcc, fps, frameSize, true);

        if (outOriginal.isOpened() && outGt.isOpened() && outMag3D.isOpened())
            return true;

        outOriginal.release();
        outGt.release();
        outMag3D.release();
    }

    // Fallback to AVI MJPG (very widely supported)
    {
        int fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
        usedExt = "avi";

        outOriginal.open(outDir + "/original.avi",    fourcc, fps, frameSize, true);
        outGt.open      (outDir + "/sobel3d_gt.avi",  fourcc, fps, frameSize, true);
        outMag3D.open   (outDir + "/sobel3d_mag.avi", fourcc, fps, frameSize, true);

        if (outOriginal.isOpened() && outGt.isOpened() && outMag3D.isOpened())
            return true;

        outOriginal.release();
        outGt.release();
        outMag3D.release();
    }

    return false;
}

int main() {
    // ----------------------------
    // Config
    // ----------------------------
    const std::string videoPath = "pictures/piplup.mp4"; // <-- change this
    const std::string outDir    = "output";

    std::filesystem::create_directories(outDir);

    // ----------------------------
    // Open input
    // ----------------------------
    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened()) {
        std::cout << "Could not open video: " << videoPath << std::endl;
        return -1;
    }

    // Get FPS (fallback)
    double fps = cap.get(cv::CAP_PROP_FPS);
    if (!(fps > 0.0 && std::isfinite(fps))) fps = 30.0;

    // Read one frame to determine size
    cv::Mat tmp;
    cap >> tmp;
    if (tmp.empty()) {
        std::cout << "Video has no frames.\n";
        return -1;
    }
    const cv::Size frameSize = tmp.size();

    // Rewind
    cap.set(cv::CAP_PROP_POS_FRAMES, 0);

    // ----------------------------
    // Open output writers (MP4 -> AVI fallback)
    // ----------------------------
    cv::VideoWriter outOriginal, outGt, outMag3D;
    std::string usedExt;

    if (!open_writers_with_fallback(outDir, frameSize, fps, outOriginal, outGt, outMag3D, usedExt)) {
        std::cout << "Failed to open video writers (MP4 and AVI both failed).\n"
                  << "This usually means your OpenCV build lacks a video backend (FFMPEG/GStreamer)\n"
                  << "or the system has no encoders available.\n";
        return -1;
    }

    std::cout << "Writing output as ." << usedExt << " in folder: " << outDir << "\n";

    // ----------------------------
    // 3D Sobel kernels
    // smooth = [1 2 1], deriv = [-1 0 +1]
    // ----------------------------
    const int smooth[3] = { 1, 2, 1 };
    const int deriv [3] = { -1, 0, 1 };

    // ----------------------------
    // Prime prev/curr/next
    // ----------------------------
    cv::Mat framePrevBgr, frameCurrBgr, frameNextBgr;
    cv::Mat prev, curr, next;

    cap >> framePrevBgr;
    cap >> frameCurrBgr;
    cap >> frameNextBgr;

    if (framePrevBgr.empty() || frameCurrBgr.empty() || frameNextBgr.empty()) {
        std::cout << "Video must have at least 3 frames for Sobel 3D.\n";
        outOriginal.release(); outGt.release(); outMag3D.release();
        return -1;
    }

    cv::cvtColor(framePrevBgr, prev, cv::COLOR_BGR2GRAY);
    cv::cvtColor(frameCurrBgr, curr, cv::COLOR_BGR2GRAY);
    cv::cvtColor(frameNextBgr, next, cv::COLOR_BGR2GRAY);

    // ----------------------------
    // Process until end-of-stream (ONCE, no looping)
    // ----------------------------
    long long frameCountWritten = 0;

    while (true) {
        cv::Mat gt(curr.size(), CV_32F, cv::Scalar(0));
        cv::Mat mag3d(curr.size(), CV_32F, cv::Scalar(0));

        // 3x3x3 convolution (skip x/y borders)
        for (int y = 1; y < curr.rows - 1; y++) {
            for (int x = 1; x < curr.cols - 1; x++) {

                float sumX = 0.0f, sumY = 0.0f, sumT = 0.0f;

                for (int dt = -1; dt <= 1; dt++) {
                    const cv::Mat* It = (dt == -1) ? &prev : (dt == 0) ? &curr : &next;

                    int wt_s = smooth[dt + 1];
                    int wt_d = deriv [dt + 1];

                    for (int dy = -1; dy <= 1; dy++) {
                        int wy_s = smooth[dy + 1];
                        int wy_d = deriv [dy + 1];

                        for (int dx = -1; dx <= 1; dx++) {
                            int wx_s = smooth[dx + 1];
                            int wx_d = deriv [dx + 1];

                            float p = static_cast<float>(It->at<uchar>(y + dy, x + dx));

                            sumX += p * static_cast<float>(wx_d * wy_s * wt_s); // d/dx, smooth y,t
                            sumY += p * static_cast<float>(wx_s * wy_d * wt_s); // d/dy, smooth x,t
                            sumT += p * static_cast<float>(wx_s * wy_s * wt_d); // d/dt, smooth x,y
                        }
                    }
                }

                gt.at<float>(y, x) = sumT;
                mag3d.at<float>(y, x) = std::sqrt(sqr(sumX) + sqr(sumY) + sqr(sumT));
            }
        }

        // Normalize to 8-bit for writing
        cv::Mat gt8, mag3d8;
        cv::normalize(cv::abs(gt), gt8,   0, 255, cv::NORM_MINMAX);
        cv::normalize(mag3d,       mag3d8, 0, 255, cv::NORM_MINMAX);

        gt8.convertTo(gt8, CV_8U);
        mag3d8.convertTo(mag3d8, CV_8U);

        cv::Mat gtBgr, mag3dBgr;
        cv::cvtColor(gt8,    gtBgr,    cv::COLOR_GRAY2BGR);
        cv::cvtColor(mag3d8, mag3dBgr, cv::COLOR_GRAY2BGR);

        // Write
        outOriginal.write(frameCurrBgr);
        outGt.write(gtBgr);
        outMag3D.write(mag3dBgr);
        frameCountWritten++;

        // Advance window
        framePrevBgr = frameCurrBgr;
        frameCurrBgr = frameNextBgr;
        prev = curr;
        curr = next;

        cap >> frameNextBgr;
        if (frameNextBgr.empty()) {
            break; // end cleanly -> MP4 finalizes
        }
        cv::cvtColor(frameNextBgr, next, cv::COLOR_BGR2GRAY);
    }

    // IMPORTANT: finalize files
    outOriginal.release();
    outGt.release();
    outMag3D.release();
    cap.release();

    std::cout << "Done. Wrote " << frameCountWritten << " frames.\n";
    return 0;
}
