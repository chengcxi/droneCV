#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/video/tracking.hpp>
#include <bits/stdc++.h>
#include <atomic>

using namespace std;

enum MotionCompensation{ORB_HOMOGRAPHY, ECC};

cv::Mat usingORB(const cv::Mat &prev, const cv::Mat &current){
    cv::Mat descriptor0, descriptor1, aligned;
    vector<cv::KeyPoint> keyPoint0, keyPoint1;

    cv::Ptr<cv::ORB> orb = cv::ORB::create();
    orb -> detectAndCompute(prev, cv::Mat(), keyPoint0, descriptor0);
    orb -> detectAndCompute(current, cv::Mat(), keyPoint1, descriptor1);

    if(descriptor0.empty() || descriptor1.empty()){
        return current.clone();
    }

    vector<cv::DMatch> matches;
    cv::BFMatcher matcher(cv::NORM_HAMMING2);
    matcher.match(descriptor0, descriptor1, matches);

    vector<cv::Point2f> points0, points1;
    for(auto &m : matches){
        points0.push_back(keyPoint0[m.queryIdx].pt);
        points1.push_back(keyPoint1[m.trainIdx].pt);
    }

    if(points0.size() < 4 || points1.size() < 4){
        return current.clone();
    }
    cv::Mat homography = cv::findHomography(points1, points0, cv::RANSAC);
    if(homography.empty()){
        return current.clone();
    }

    cv::warpPerspective(current, aligned, homography, prev.size());
    return aligned;
}

cv::Mat usingECC(const cv::Mat &prev, const cv::Mat &current){
    cv::Mat warpMatrix = cv::Mat::eye(2,3,CV_32F);
    cv::Mat aligned;

    int iterations = 50;
    double terminateEPS = 1e-6;
    cv::TermCriteria criteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, iterations, terminateEPS);

    try{
        cv::Mat prevGray, currentGray;
        if (prev.channels() > 1) cv::cvtColor(prev, prevGray, cv::COLOR_BGR2GRAY);
        else prevGray = prev;
        if (current.channels() > 1) cv::cvtColor(current, currentGray, cv::COLOR_BGR2GRAY);
        else currentGray = current;
        
        cv::findTransformECC(prevGray, currentGray, warpMatrix, cv::MOTION_AFFINE, criteria);
        cv::warpAffine(current, aligned, warpMatrix, current.size(), cv::INTER_LINEAR + cv::WARP_INVERSE_MAP);
        return aligned;
    }
    catch(const cv::Exception& e){
        return current.clone();
    }
    catch(...){
        return current.clone();
    }
}

struct SharedData {
    mutex mtx;
    condition_variable cv_new_frame;
    condition_variable cv_result_ready;

    cv::Mat prevFrame_worker;
    cv::Mat currentFrame_worker;
    bool new_data_available = false;

    cv::Mat resultMask_worker;
    cv::Mat resultFrameWithFlow_worker;
    bool processing_done = false;
    bool has_new_result = false;

    atomic<bool> stop_requested{false};
};


void processingThreadFunc(SharedData& data, MotionCompensation method) {
    cv::Ptr<cv::DISOpticalFlow> opticalFlow = cv::DISOpticalFlow::create();
    opticalFlow->setFinestScale(1);
    opticalFlow->setPatchStride(5);
    opticalFlow->setUseMeanNormalization(true);
    opticalFlow->setUseSpatialPropagation(true);
    opticalFlow->setVariationalRefinementAlpha(1000);
    opticalFlow->setVariationalRefinementDelta(100);
    opticalFlow->setVariationalRefinementGamma(5);
    opticalFlow->setVariationalRefinementIterations(0);

    cv::Ptr<cv::BackgroundSubtractorKNN> knn = cv::createBackgroundSubtractorKNN(10, 750, false);

    cv::Mat localPrevFrame, localCurrentFrame;

    while (!data.stop_requested) {
        {
            unique_lock<mutex> lock(data.mtx);
            data.cv_new_frame.wait(lock, [&data] { return data.new_data_available || data.stop_requested; });

            if (data.stop_requested) break;

            localPrevFrame = data.prevFrame_worker.clone();
            localCurrentFrame = data.currentFrame_worker.clone();
            data.new_data_available = false;
            data.processing_done = false;
            data.has_new_result = false;
        }

        if (localPrevFrame.empty() || localCurrentFrame.empty()) {
            continue;
        }

        cv::Mat aligned, flow, motionMask, frameWithFlow;
        frameWithFlow = localCurrentFrame.clone(); // For drawing flow vectors

        if (method == ORB_HOMOGRAPHY) {
            aligned = usingORB(localPrevFrame, localCurrentFrame);
        } else {
            aligned = usingECC(localPrevFrame, localCurrentFrame);
        }

        if (aligned.empty() || aligned.size() != localPrevFrame.size()) {
             aligned = localCurrentFrame.clone();
        }


        cv::Mat alignedGray, localPrevGray;
        if (aligned.channels() > 1) cv::cvtColor(aligned, alignedGray, cv::COLOR_BGR2GRAY);
        else alignedGray = aligned;
        if (localPrevFrame.channels() > 1) cv::cvtColor(localPrevFrame, localPrevGray, cv::COLOR_BGR2GRAY);
        else localPrevGray = localPrevFrame;

        if (alignedGray.empty() || localPrevGray.empty()) {
             knn->apply(localCurrentFrame, motionMask);
        } else {
            opticalFlow->calc(localPrevGray, alignedGray, flow);
            knn->apply(alignedGray, motionMask);
        }


        if (!flow.empty()) {
            int flowCoverage = 25;
            cv::Mat displayFlowFrame;
            if (alignedGray.channels() == 1) cv::cvtColor(alignedGray, displayFlowFrame, cv::COLOR_GRAY2BGR);
            else displayFlowFrame = alignedGray.clone();


            for (int y = 0; y < displayFlowFrame.rows; y += flowCoverage) {
                for (int x = 0; x < displayFlowFrame.cols; x += flowCoverage) {
                    if (y >= flow.rows || x >= flow.cols) continue;
                    cv::Point2f flowAtPoint = flow.at<cv::Point2f>(y, x);
                    cv::line(displayFlowFrame, cv::Point(x, y), cv::Point(cvRound(x + flowAtPoint.x), cvRound(y + flowAtPoint.y)), cv::Scalar(0, 255, 0), 1);
                    cv::circle(displayFlowFrame, cv::Point(x, y), 1, cv::Scalar(0, 0, 255), -1);
                }
            }
            frameWithFlow = displayFlowFrame;
        }


        {
            lock_guard<mutex> lock(data.mtx);
            data.resultMask_worker = motionMask.clone();
            data.resultFrameWithFlow_worker = frameWithFlow.clone();
            data.processing_done = true;
            data.has_new_result = true;
            data.cv_result_ready.notify_one(); // notify main thread
        }
    }
}


int main(){
    MotionCompensation method_choice;
    cout << "Choose motion compensation method (0 for ORB, 1 for ECC):" << endl;
    int choice;
    cin >> choice;
    if (choice == 0) method_choice = ORB_HOMOGRAPHY;
    else if (choice == 1) method_choice = ECC;
    else {
        cout << "Invalid choice, defaulting to ORB." << endl;
        method_choice = ORB_HOMOGRAPHY;
    }

    SharedData shared_data;
    thread worker_thread(processingThreadFunc, ref(shared_data), method_choice);

    cv::Mat frame, prevFrame, grayFrame;
    cv::Mat displayMask, displayFrameWithFlow;

    cv::namedWindow("Motion Mask", cv::WINDOW_NORMAL);
    cv::namedWindow("Frame with Flow", cv::WINDOW_NORMAL);
    cv::resizeWindow("Motion Mask", 1024, 768);
    cv::resizeWindow("Frame with Flow", 1024, 768);

    // cv::VideoCapture cap("/home/cheng/Downloads/DroneVision/test/test.mp4");
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        cerr << "Error: Could not open video source." << endl;
        return -1;
    }

    int frame_skip = 2; // process every nth frame to allow worker to keep up
    int frame_count = 0;

    while(true){
        cap.read(frame);
        if(frame.empty()){
            cout << "End of video or cannot read frame." << endl;
            break;
        }

        // main thread preprocessing (minimal)
        // worker thread will handle grayscale conversion if needed
        // cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);

        if (!prevFrame.empty() && (frame_count % frame_skip == 0)) {
            unique_lock<mutex> lock(shared_data.mtx);
            if (!shared_data.new_data_available) { // only push if worker is ready for new data
                shared_data.prevFrame_worker = prevFrame.clone();
                shared_data.currentFrame_worker = frame.clone(); // send original color frame
                shared_data.new_data_available = true;
                shared_data.has_new_result = false; // new data invalidates old result flag
                lock.unlock(); // unlock before notifying
                shared_data.cv_new_frame.notify_one(); // notify main loop
            } else {
                // lock.unlock(); // release and wait
            }
        }
        
        prevFrame = frame.clone(); // update prevFrame for the next iteration
        frame_count++;

        // display results from worker thread (non-blocking check)
        {
            lock_guard<mutex> lock(shared_data.mtx);
            if (shared_data.has_new_result) {
                if (!shared_data.resultMask_worker.empty()) {
                    displayMask = shared_data.resultMask_worker.clone();
                }
                if (!shared_data.resultFrameWithFlow_worker.empty()) {
                    displayFrameWithFlow = shared_data.resultFrameWithFlow_worker.clone();
                }
                // shared_data.has_new_result = false; // consume the result flag
            }
        }

        if (!displayMask.empty()) {
            cv::imshow("Motion Mask", displayMask);
        } else {
            cv::imshow("Motion Mask", frame);
        }

        if (!displayFrameWithFlow.empty()) {
            cv::imshow("Frame with Flow", displayFrameWithFlow);
        } else {
            cv::imshow("Frame with Flow", frame);
        }


        if(cv::waitKey(10) >= 0){
            break;
        }
    }

    cout << "Stopping thread..." << endl;
    {
        lock_guard<mutex> lock(shared_data.mtx);
        shared_data.stop_requested = true;
    }
    shared_data.cv_new_frame.notify_all(); // wake up waiting threads
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
    cout << "Thread joined." << endl;

    cap.release();
    cv::destroyAllWindows();
    return 0;
}