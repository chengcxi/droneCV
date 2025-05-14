#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/video/tracking.hpp>
#include <opencv2/imgproc.hpp> // For cvtColor, etc.
#include <opencv2/features2d.hpp> // For ORB
#include <opencv2/calib3d.hpp>   // For findHomography

#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <iostream>
#include <string> // For std::string

// --- Global variables for thread communication ---
std::mutex g_frame_mutex;
cv::Mat g_prev_gray_frame_main;
cv::Mat g_current_gray_frame_main;
cv::Mat g_current_bgr_frame_main;
cv::Mat g_flow_visualization_output;
cv::Mat g_foreground_mask_output;
cv::Mat g_aligned_gray_output; // To show the aligned image
bool g_new_data_available = false;
bool g_running = true;
// --- End global variables ---

// Constants for display layout
const int QUAD_WIDTH = 640; // Adjust as needed
const int QUAD_HEIGHT = 360; // Adjust as needed
const cv::Size QUAD_SIZE(QUAD_WIDTH, QUAD_HEIGHT);
const int CANVAS_WIDTH = QUAD_WIDTH * 2;
const int CANVAS_HEIGHT = QUAD_HEIGHT * 2;


cv::Mat usingORB(const cv::Mat &prev, const cv::Mat &current) {
    cv::Mat descriptor0, descriptor1, aligned;
    std::vector<cv::KeyPoint> keyPoint0, keyPoint1;

    cv::Ptr<cv::ORB> orb = cv::ORB::create();
    orb->detectAndCompute(prev, cv::Mat(), keyPoint0, descriptor0);
    orb->detectAndCompute(current, cv::Mat(), keyPoint1, descriptor1);

    if (descriptor0.empty() || descriptor1.empty() || descriptor0.type() != CV_8U || descriptor1.type() != CV_8U) {
        std::cerr << "ORB: Empty or invalid descriptors." << std::endl;
        return current.clone();
    }
    if (keyPoint0.empty() || keyPoint1.empty()){
         std::cerr << "ORB: No keypoints found." << std::endl;
        return current.clone();
    }

    std::vector<cv::DMatch> matches;
    cv::BFMatcher matcher(cv::NORM_HAMMING2);
    matcher.match(descriptor0, descriptor1, matches);

    std::vector<cv::Point2f> points0, points1;
    for (const auto &m : matches) {
        points0.push_back(keyPoint0[m.queryIdx].pt);
        points1.push_back(keyPoint1[m.trainIdx].pt);
    }

    if (points0.size() < 4 || points1.size() < 4) {
        std::cerr << "ORB: Not enough matches for homography (" << points0.size() << ")" << std::endl;
        return current.clone();
    }

    cv::Mat homography = cv::findHomography(points1, points0, cv::RANSAC);
    if (homography.empty()) {
        std::cerr << "ORB: Homography could not be computed." << std::endl;
        return current.clone();
    }

    cv::warpPerspective(current, aligned, homography, current.size());
    return aligned;
}

cv::Mat usingECC(const cv::Mat &prev, const cv::Mat &current) {
    cv::Mat warpMatrix = cv::Mat::eye(2, 3, CV_32F);
    cv::Mat aligned;

    int iterations = 50;
    double terminateEPS = 1e-6;
    cv::TermCriteria criteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, iterations, terminateEPS);

    try {
        cv::Mat prev8u, current8u;
        if (prev.depth() != CV_8U) prev.convertTo(prev8u, CV_8U); else prev8u = prev;
        if (current.depth() != CV_8U) current.convertTo(current8u, CV_8U); else current8u = current;

        if(prev8u.empty() || current8u.empty()){
            std::cerr << "ECC: Input image is empty after conversion." << std::endl;
            return current.clone();
        }
        if(prev8u.channels() != 1 || current8u.channels() != 1){
            std::cerr << "ECC: Input images must be single channel." << std::endl;
            return current.clone();
        }

        cv::findTransformECC(prev8u, current8u, warpMatrix, cv::MOTION_AFFINE, criteria, cv::Mat(), 1); // Added last 2 default params
        cv::warpAffine(current, aligned, warpMatrix, current.size(), cv::INTER_LINEAR + cv::WARP_INVERSE_MAP);
        return aligned;
    } catch (const cv::Exception& e) {
        std::cerr << "ECC Error: " << e.what() << std::endl;
        return current.clone();
    } catch (...) {
        std::cerr << "ECC: Unknown error during transform." << std::endl;
        return current.clone();
    }
}

void DISThreadFunction(MotionCompensation method) {
    cv::Mat flow;
    cv::Ptr<cv::DISOpticalFlow> opticalFlow = cv::DISOpticalFlow::create(cv::DISOpticalFlow::PRESET_MEDIUM);
    cv::Ptr<cv::BackgroundSubtractorKNN> knn = cv::createBackgroundSubtractorKNN(50, 400, false);

    int thread_frames_processed = 0;

    while (g_running) {
        cv::Mat local_prev_gray, local_current_gray, local_current_bgr;
        bool data_to_process = false;

        {
            std::lock_guard<std::mutex> lock(g_frame_mutex);
            if (g_new_data_available && !g_prev_gray_frame_main.empty() && !g_current_gray_frame_main.empty()) {
                local_prev_gray = g_prev_gray_frame_main.clone();
                local_current_gray = g_current_gray_frame_main.clone();
                local_current_bgr = g_current_bgr_frame_main.clone();
                g_new_data_available = false;
                data_to_process = true;
            }
        }

        if (data_to_process) {
            cv::Mat aligned_current_gray;
            if (method == ORB) {
                aligned_current_gray = usingORB(local_prev_gray, local_current_gray);
            } else {
                aligned_current_gray = usingECC(local_prev_gray, local_current_gray);
            }

            // Share the aligned image
            if (!aligned_current_gray.empty()) {
                std::lock_guard<std::mutex> lock(g_frame_mutex);
                g_aligned_gray_output = aligned_current_gray.clone();
            } else {
                std::lock_guard<std::mutex> lock(g_frame_mutex);
                g_aligned_gray_output.release(); // Clear if alignment failed
            }


            if (aligned_current_gray.empty() || local_prev_gray.empty()){
                std::cerr << "Alignment failed or prev_gray empty, skipping flow." << std::endl;
            } else {
                cv::Mat flow_prev = local_prev_gray.clone();
                cv::Mat flow_curr = aligned_current_gray.clone();
                if (flow_prev.channels() != 1) cv::cvtColor(flow_prev, flow_prev, cv::COLOR_BGR2GRAY);
                if (flow_curr.channels() != 1) cv::cvtColor(flow_curr, flow_curr, cv::COLOR_BGR2GRAY);
                if (flow_prev.type() != CV_8UC1) flow_prev.convertTo(flow_prev, CV_8UC1);
                if (flow_curr.type() != CV_8UC1) flow_curr.convertTo(flow_curr, CV_8UC1);

                if (!flow_prev.empty() && !flow_curr.empty() && flow_prev.type() == flow_curr.type()) {
                    opticalFlow->calc(flow_prev, flow_curr, flow);

                    cv::Mat temp_flow_vis = local_current_bgr.clone();
                    int flowCoverage = 30; // Increased for better visualization in smaller quad
                    for (int y = 0; y < temp_flow_vis.rows; y += flowCoverage) {
                        for (int x = 0; x < temp_flow_vis.cols; x += flowCoverage) {
                            if (y < flow.rows && x < flow.cols) {
                                const cv::Point2f& flowAtPoint = flow.at<cv::Point2f>(y, x);
                                cv::line(temp_flow_vis, cv::Point(x, y),
                                         cv::Point(cvRound(x + flowAtPoint.x), cvRound(y + flowAtPoint.y)),
                                         cv::Scalar(0, 255, 0), 1);
                                cv::circle(temp_flow_vis, cv::Point(x, y), 1, cv::Scalar(0, 0, 255), -1);
                            }
                        }
                    }
                    {
                        std::lock_guard<std::mutex> lock(g_frame_mutex);
                        g_flow_visualization_output = temp_flow_vis;
                    }
                } else {
                    std::cerr << "Skipping optical flow due to invalid inputs." << std::endl;
                }
            }

            cv::Mat temp_fg_mask;
            if (!aligned_current_gray.empty()){
                 knn->apply(aligned_current_gray, temp_fg_mask);
                {
                    std::lock_guard<std::mutex> lock(g_frame_mutex);
                    g_foreground_mask_output = temp_fg_mask;
                }
            }
            thread_frames_processed++;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    std::cout << "DIS Thread finished. Processed " << thread_frames_processed << " frames." << std::endl;
}

void addText(cv::Mat& image, const std::string& text, cv::Point origin) {
    cv::putText(image, text, origin, cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 0), 1, cv::LINE_AA);
}

int main() {
    MotionCompensation method_choice;
    std::cout << "Choose motion compensation method (0 for ORB, 1 for ECC): ";
    int methodInput;
    std::cin >> methodInput;
    method_choice = (methodInput == 1) ? ECC : ORB;
    std::cout << "Using " << (method_choice == ORB ? "ORB" : "ECC") << "." << std::endl;

    cv::VideoCapture cap("/home/cheng/Downloads/DroneVision/test/test.mp4");
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open video file." << std::endl;
        return -1;
    }

    cv::namedWindow("Combined Output", cv::WINDOW_NORMAL);
    cv::resizeWindow("Combined Output", CANVAS_WIDTH, CANVAS_HEIGHT);

    std::thread processingThread(DISThreadFunction, method_choice);

    cv::Mat frame_bgr, current_gray_for_main;
    int main_frames_read = 0;

    while (true) {
        cap.read(frame_bgr);
        if (frame_bgr.empty()) {
            std::cout << "End of video or error reading frame." << std::endl;
            break;
        }
        main_frames_read++;

        cv::cvtColor(frame_bgr, current_gray_for_main, cv::COLOR_BGR2GRAY);

        {
            std::lock_guard<std::mutex> lock(g_frame_mutex);
            g_current_gray_frame_main = current_gray_for_main.clone();
            g_current_bgr_frame_main = frame_bgr.clone();
            g_new_data_available = true;
        }

        // Create canvas for combined output
        cv::Mat canvas = cv::Mat::zeros(CANVAS_HEIGHT, CANVAS_WIDTH, CV_8UC3);

        // 1. Input Video (Top-Left)
        cv::Mat display_input = frame_bgr.clone();
        cv::resize(display_input, display_input, QUAD_SIZE);
        addText(display_input, "Input", cv::Point(10, 20));
        display_input.copyTo(canvas(cv::Rect(0, 0, QUAD_WIDTH, QUAD_HEIGHT)));

        // Retrieve processed frames for display
        cv::Mat local_flow_vis, local_fg_mask, local_aligned_gray;
        {
            std::lock_guard<std::mutex> lock(g_frame_mutex);
            if (!g_flow_visualization_output.empty()) local_flow_vis = g_flow_visualization_output.clone();
            if (!g_foreground_mask_output.empty()) local_fg_mask = g_foreground_mask_output.clone();
            if (!g_aligned_gray_output.empty()) local_aligned_gray = g_aligned_gray_output.clone();
        }

        // 2. Optical Flow (Top-Right)
        cv::Mat display_flow = cv::Mat::zeros(QUAD_HEIGHT, QUAD_WIDTH, CV_8UC3); // Placeholder
        if (!local_flow_vis.empty()) {
            cv::resize(local_flow_vis, display_flow, QUAD_SIZE);
        }
        addText(display_flow, "Optical Flow", cv::Point(10, 20));
        display_flow.copyTo(canvas(cv::Rect(QUAD_WIDTH, 0, QUAD_WIDTH, QUAD_HEIGHT)));

        // 3. Foreground Mask (Bottom-Left)
        cv::Mat display_mask = cv::Mat::zeros(QUAD_HEIGHT, QUAD_WIDTH, CV_8UC3); // Placeholder
        if (!local_fg_mask.empty()) {
            cv::Mat temp_mask_bgr;
            if (local_fg_mask.channels() == 1) {
                cv::cvtColor(local_fg_mask, temp_mask_bgr, cv::COLOR_GRAY2BGR);
            } else {
                temp_mask_bgr = local_fg_mask; // Should already be BGR if not 1 channel
            }
            cv::resize(temp_mask_bgr, display_mask, QUAD_SIZE);
        }
        addText(display_mask, "Foreground Mask (KNN)", cv::Point(10, 20));
        display_mask.copyTo(canvas(cv::Rect(0, QUAD_HEIGHT, QUAD_WIDTH, QUAD_HEIGHT)));


        // 4. Aligned Image (Bottom-Right)
        cv::Mat display_aligned = cv::Mat::zeros(QUAD_HEIGHT, QUAD_WIDTH, CV_8UC3); // Placeholder
        if(!local_aligned_gray.empty()){
            cv::Mat temp_aligned_bgr;
             if (local_aligned_gray.channels() == 1) {
                cv::cvtColor(local_aligned_gray, temp_aligned_bgr, cv::COLOR_GRAY2BGR);
            } else {
                temp_aligned_bgr = local_aligned_gray;
            }
            cv::resize(temp_aligned_bgr, display_aligned, QUAD_SIZE);
        }
        addText(display_aligned, "Aligned (Compensated)", cv::Point(10, 20));
        display_aligned.copyTo(canvas(cv::Rect(QUAD_WIDTH, QUAD_HEIGHT, QUAD_WIDTH, QUAD_HEIGHT)));


        cv::imshow("Combined Output", canvas);
        
        {
            std::lock_guard<std::mutex> lock(g_frame_mutex);
            g_prev_gray_frame_main = current_gray_for_main.clone();
        }

        if (cv::waitKey(10) >= 0) {
            std::cout << "User pressed key, exiting." << std::endl;
            break;
        }
    }

    g_running = false;
    if (processingThread.joinable()) {
        processingThread.join();
    }
    std::cout << "Main thread finished. Read " << main_frames_read << " frames." << std::endl;

    cap.release();
    cv::destroyAllWindows();
    return 0;
}