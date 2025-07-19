#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/video/tracking.hpp>
#include <bits/stdc++.h>
#include <chrono>
#include <opencv2/optflow.hpp>

using namespace std;

int main(){
    cv::Ptr<cv::optflow::DualTVL1OpticalFlow> opticalFlow = cv::optflow::DualTVL1OpticalFlow::create();
    cv::Mat frame, prevFrame, flow;
    cv::namedWindow("Test", cv::WINDOW_NORMAL);
    cv::resizeWindow("Test", 1024, 768);
    cv::VideoCapture cap("/home/cheng/Downloads/DroneVision/test/test.mp4");
    // opticalFlow->setFinestScale(1);
    // //opticalFlow->setGradientDescentIterations(100);
    // //opticalFlow->setPatchSize() # default is usually fine (8x8);
    // opticalFlow->setPatchStride(5);
    // opticalFlow->setUseMeanNormalization(true);
    // opticalFlow->setUseSpatialPropagation(false);
    // opticalFlow->setVariationalRefinementAlpha(1000);
    // opticalFlow->setVariationalRefinementDelta(100);
    // opticalFlow->setVariationalRefinementGamma(1);
    // opticalFlow->setVariationalRefinementIterations(10); // increases accuracy at cost of performance

    cv::Ptr<cv::BackgroundSubtractorKNN> knn = cv::createBackgroundSubtractorKNN(10, 750, false);

    int cnt = 0;
    while(cap.read(frame)){
        if(frame.empty()){break;}
        cv::cvtColor(frame, frame, cv::COLOR_BGR2GRAY);
        cv::medianBlur(frame, frame, 5);
        cnt++;
        if(!prevFrame.empty() && cnt % 5 == 0){
            opticalFlow -> calc(prevFrame, frame, flow);
            int flowCoverage = 10;
            for(int y = 0; y < frame.rows; y+=flowCoverage){
                for(int x = 0; x < frame.cols; x+=flowCoverage){
                    cv::Point2f flowAtPoint = flow.at<cv::Point2f>(y, x);
                    cv::line(frame, cv::Point(x,y), cv::Point(cvRound(x+flowAtPoint.x), cvRound(y+flowAtPoint.y)), cv::Scalar_(0,255,0),1);
                    cv::circle(frame, cv::Point(x,y), 1, cv::Scalar(255, 0, 0), -1);
                }
            }
        }
        knn -> apply(frame, prevFrame); // apply k-th nearest neighbor
        if(cnt % 14 == 0){prevFrame = frame.clone();}
        cv::imshow("Test", frame);
        if(cv::waitKey(1)>=0){break;}
    }
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