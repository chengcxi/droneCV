#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/video/tracking.hpp>
#include <bits/stdc++.h>
#include <mutex>
#include <thread>
#include <chrono>

using namespace std;
enum MotionCompensation{ORB, ECC}; // ORB homography or ECC

cv::Mat usingORB(const cv::Mat &prev, const cv::Mat &current){
    cv::Mat descriptor0, descriptor1, aligned;
    vector<cv::KeyPoint> keyPoint0, keyPoint1;

    cv::Ptr<cv::ORB> orb = cv::ORB::create();
    orb -> detectAndCompute(prev, cv::Mat(), keyPoint0, descriptor0);
    orb -> detectAndCompute(current, cv::Mat(), keyPoint1, descriptor1);

    if(descriptor0.empty() || descriptor1.empty()){return current;}

    vector<cv::DMatch> matches; // matching descriptors
    cv::BFMatcher matcher(cv::NORM_HAMMING2);
    matcher.match(descriptor0, descriptor1, matches);

    vector<cv::Point2f> points0, points1;
    for(auto &m : matches){
        points0.push_back(keyPoint0[m.queryIdx].pt);
        points1.push_back(keyPoint1[m.queryIdx].pt);
    }

    if(points0.size() < 4 || points1.size() < 4){return current;}
    cv::Mat homography = cv::findHomography(points1, points0, cv::RANSAC); // check for homography using Random sample consensus
    if(homography.empty()){return current;}

    cv::warpPerspective(current, aligned, homography, current.size());
    return aligned;
}

cv::Mat usingECC(const cv::Mat &prev, const cv::Mat &current){
    cv::Mat warpMatrix = cv::Mat::eye(2,3,CV_32F);
    cv::Mat aligned;

    int iterations = 50;
    double terminateEPS = 1e-6;
    cv::TermCriteria criteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, iterations, terminateEPS);

    try{
        cv::findTransformECC(prev, current, warpMatrix, cv::MOTION_AFFINE, criteria);
        cv::warpAffine(current, aligned, warpMatrix, current.size(), cv::INTER_LINEAR + cv::WARP_INVERSE_MAP);
        return aligned;
    }
    catch(...){
        return current;
    }
}

int main(){
    MotionCompensation method;
    cout << "Choose motion compensation method (0 for ORB, 1 for ECC): " << endl;
    int methodInput;
    cin >> methodInput;
    method = static_cast<MotionCompensation>(methodInput);

    cv::Mat frame, currentGray, prevGray, flow, foregroundMask, displayFrame;
    mutex mtx;
    atomic<bool> running = true;
    atomic<bool> frameReady = false;

    cv::namedWindow("Test", cv::WINDOW_NORMAL);
    cv::resizeWindow("Test", 1024, 768);
    cv::VideoCapture cap("/home/cheng/Downloads/DroneVision/test/test.mp4");

    cv::Ptr<cv::DISOpticalFlow> opticalFlow = cv::DISOpticalFlow::create();
    opticalFlow->setFinestScale(1);
    //opticalFlow->setGradientDescentIterations(100);
    //opticalFlow->setPatchSize(8) # default is usually fine (8x8);
    opticalFlow->setPatchStride(5);
    opticalFlow->setUseMeanNormalization(true);
    opticalFlow->setUseSpatialPropagation(true);
    opticalFlow->setVariationalRefinementAlpha(1000);
    opticalFlow->setVariationalRefinementDelta(100);
    opticalFlow->setVariationalRefinementGamma(5);
    opticalFlow->setVariationalRefinementIterations(0); // increases accuracy at cost of performance

    // cv::Ptr<cv::VariationalRefinement> refineFlow = cv::VariationalRefinement::create();
    // refineFlow->setAlpha(100); // Smoothness # default: 20
    // refineFlow->setDelta(100); // # default: 5
    // // refineFlow->setFixedPointIterations(5); // # default: 5
    // // refineFlow->setGamma(); // # default: 10
    // refineFlow->setOmega(1.2); // # default: 1.6
    // refineFlow->setSorIterations(10); // # default: 5

    // Post-processors
    cv::Ptr<cv::BackgroundSubtractorKNN> knn = cv::createBackgroundSubtractorKNN(10, 750, false);
    // cv::Ptr<cv::GFTTDetector> detector = cv::GFTTDetector::create();
    // vector<cv::KeyPoint> keypoints;

    cv::Mat threadPrevGray, threadCurrentGray, threadFrame, threadMotionMask, threadFlow, threadDisplayFrame;
    atomic<bool> processedFrame = false;
    thread DISThread([&](){
        while(running){
            if(!frameReady){
                this_thread::sleep_for(chrono::milliseconds(1));
                continue;
            }
            {
                
                lock_guard<mutex> lock(mtx);            
                threadPrevGray = prevGray.clone();
                threadCurrentGray = currentGray.clone();
                threadFrame = frame.clone();
                frameReady = false;
            }
            if(threadPrevGray.empty() || threadCurrentGray.empty()){continue;}
            cv::Mat aligned;
            if(method == ORB){aligned = usingORB(threadPrevGray, threadCurrentGray);}
            else{aligned = usingECC(threadPrevGray, threadCurrentGray);}
            opticalFlow -> calc(threadPrevGray, aligned, threadFlow);
            threadDisplayFrame = threadFrame.clone();
            int flowCoverage = 25;
            for(int y = 0; y < threadDisplayFrame.rows; y+=flowCoverage){
                for(int x = 0; x < threadDisplayFrame.cols; x+=flowCoverage){
                    cv::Point2f flowAtPoint = threadFlow.at<cv::Point2f>(y, x);
                    cv::line(threadDisplayFrame, cv::Point(x,y), cv::Point(cvRound(x+flowAtPoint.x), cvRound(y+flowAtPoint.y)), cv::Scalar_(0,0,0),2);
                    cv::circle(threadDisplayFrame, cv::Point(x,y), 1, cv::Scalar(255, 0, 0), -1);
                }
            }
            knn -> apply(aligned, threadMotionMask); // apply k-th nearest neighbor
            processedFrame = true;
        }
    });
    
    int frames = 0;
    while(cap.read(frame)){
        if(frame.empty()){break;}
        cv::cvtColor(frame, currentGray, cv::COLOR_BGR2GRAY); // apply gray bg
        // // detector -> detect(frame, keypoints); // apply good features to track
        // for(cv::KeyPoint kp : keypoints){
        //     cv::circle(frame, kp.pt, 3, cv::Scalar(0,255,0), 1);
        // }
        {
            lock_guard<mutex> lock(mtx);
            prevGray = currentGray.clone();
            frameReady = true;
            // if(frames % 14 == 0){prevFrame = frame.clone();}
        }
        if(processedFrame){
            cv::imshow("Test", threadMotionMask);
            processedFrame = false;
        }
        // cv::imshow("Test", frame); // change to outFrame if needed
        if(cv::waitKey(1)>=0){break;}
        // frames++;
    }
    running = false;
    DISThread.join();

    // Frame capture (wip)
    // cv::Mat frame;cv::waitKeyEx(1);
    // //--- INITIALIZE VIDEOCAPTURE
    // cv::VideoCapture cap;
    // // open the default camera using default API
    // // cap.open(0);
    // // OR advance usage: select any API backend
    // int deviceID = 0;             // 0 = open default camera
    // int apiID = cv::CAP_ANY;      // 0 = autodetect default API
    // // open selected camera using selected API
    // cap.open(deviceID, apiID);
    // // check if we succeeded
    // if (!cap.isOpened()) {
    //     cerr << "ERROR! Unable to open camera\n";
    //     return -1;
    // }
 
    // //--- GRAB AND WRITE LOOP
    // cout << "Start grabbing" << endl
    //     << "Press any key to terminate" << endl;
    // for (;;)
    // {
    //     // wait for a new frame from camera and store it into 'frame'
    //     cap.read(frame);
    //     // check if we succeeded
    //     if (frame.empty()) {
    //         cerr << "ERROR! blank frame grabbed\n";
    //         break;
    //     }
    //     // show live and wait for a key with timeout long enough to show images
    //     imshow("Live", frame);
    //     if (cv::waitKey(5) >= 0)
    //         break;
    // }
    // // the camera will be deinitialized automatically in VideoCapture destructor
    return 0;
}