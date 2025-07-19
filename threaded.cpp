#include <opencv2/opencv.hpp>
#include <opencv2/video/tracking.hpp>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

using namespace std;

enum MotionCompensation { ORB_HOMOGRAPHY, ECC };

cv::Mat usingORB(const cv::Mat &prev, const cv::Mat &curr)
{
    cv::Ptr<cv::ORB> orb = cv::ORB::create();

    vector<cv::KeyPoint> kp1, kp2;
    cv::Mat desc1,  desc2;
    orb->detectAndCompute(prev,  cv::Mat(), kp1, desc1);
    orb->detectAndCompute(curr,  cv::Mat(), kp2, desc2);
    if (desc1.empty() || desc2.empty()) return curr;

    vector<cv::DMatch> matches;
    cv::BFMatcher(cv::NORM_HAMMING2).match(desc1, desc2, matches);

    vector<cv::Point2f> p1, p2;
    for (auto &m: matches) {
        p1.push_back(kp1[m.queryIdx].pt);
        p2.push_back(kp2[m.trainIdx ].pt);
    }
    if (p1.size() < 4) return curr;

    cv::Mat H = cv::findHomography(p2, p1, cv::RANSAC);
    if (H.empty()) return curr;

    cv::Mat aligned;
    cv::warpPerspective(curr, aligned, H, curr.size());
    return aligned;
}

cv::Mat usingECC(const cv::Mat &prev, const cv::Mat &curr)
{
    cv::Mat warp = cv::Mat::eye(2,3,CV_32F), aligned;
    cv::TermCriteria term(cv::TermCriteria::COUNT+cv::TermCriteria::EPS,
                          50, 1e-6);
    try {
        cv::findTransformECC(prev, curr, warp, cv::MOTION_AFFINE, term);
        cv::warpAffine(curr, aligned, warp, curr.size(),
                       cv::INTER_LINEAR | cv::WARP_INVERSE_MAP);
        return aligned;
    } catch(...) { return curr; }
}

/* --------------------- worker job ---------------------------------- */
struct JobInput  { cv::Mat prev, curr; };
struct JobOutput { cv::Mat vis, mask; };

void workerThread(MotionCompensation method,
                  cv::Ptr<cv::DISOpticalFlow>  dis,
                  cv::Ptr<cv::BackgroundSubtractorKNN> knn,
                  queue<JobInput>& qIn,   mutex& mIn,  condition_variable& cvIn,
                  queue<JobOutput>&qOut,  mutex& mOut, condition_variable& cvOut,
                  atomic<bool>& stop)
{
    while (!stop)
    {
        /* -------- wait for a new job -------- */
        unique_lock<mutex> lk(mIn);
        cvIn.wait(lk, [&]{ return stop || !qIn.empty(); });
        if (stop) break;
        JobInput job = move(qIn.front()); qIn.pop();
        lk.unlock();

        /* -------- process -------- */
        cv::Mat aligned = (method==ORB_HOMOGRAPHY) ?
                           usingORB(job.prev, job.curr)
                         : usingECC (job.prev, job.curr);

        cv::Mat flow;  dis->calc(job.prev, aligned, flow);

        /* visualise flow */
        cv::Mat vis;
        cv::cvtColor(job.curr, vis, cv::COLOR_GRAY2BGR);
        int step = 25;
        for (int y=0; y<vis.rows; y+=step)
            for (int x=0; x<vis.cols; x+=step)
            {
                cv::Point2f v = flow.at<cv::Point2f>(y,x);
                cv::line  (vis,{x,y},{cvRound(x+v.x),cvRound(y+v.y)}, {0,255,0},2);
                cv::circle(vis,{x,y},1,{0,0,255},-1);
            }

        cv::Mat mask; knn->apply(aligned, mask);

        /* -------- push result -------- */
        {
            lock_guard<mutex> lg(mOut);
            qOut.push({vis,mask});
        }
        cvOut.notify_one();
    }
}

/* ---------------------- main ------------------------------------ */
int main()
{
    MotionCompensation method;
    cout << "Select compensation (0=ORB 1=ECC): ";  int m; cin>>m;
    method = static_cast<MotionCompensation>(m);

    cv::Ptr<cv::DISOpticalFlow> dis = cv::DISOpticalFlow::create();
    dis->setFinestScale(1);
    dis->setPatchStride(5);
    dis->setUseMeanNormalization(true);
    dis->setUseSpatialPropagation(true);

    cv::Ptr<cv::BackgroundSubtractorKNN> knn =
            cv::createBackgroundSubtractorKNN(10,750,false);

    /* queues + sync primitives */
    queue<JobInput>  qIn;   mutex mIn;  condition_variable cvIn;
    queue<JobOutput> qOut;  mutex mOut; condition_variable cvOut;
    atomic<bool> stop(false);

    thread worker(workerThread, method, dis, knn,
                  ref(qIn), ref(mIn), ref(cvIn),
                  ref(qOut),ref(mOut), ref(cvOut),
                  ref(stop));

    cv::VideoCapture cap("/home/cheng/Downloads/DroneVision/test/test.mp4");
    if(!cap.isOpened()){cerr<<"Cannot open video\n"; return -1;}

    cv::namedWindow("flow",  cv::WINDOW_NORMAL);
    cv::namedWindow("mask",  cv::WINDOW_NORMAL);

    cv::Mat prevGray, frame, gray;
    int frameID = 0;

    while(cap.read(frame))
    {
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        /* -------- schedule work every nth frames -------- */
        if (!prevGray.empty() && frameID % 2 == 0)           // adjust rate
        {
            {
                lock_guard<mutex> lg(mIn);
                qIn.push({prevGray.clone(), gray.clone()});
            }
            cvIn.notify_one();
        }
        prevGray = gray.clone();
        frameID++;

        /* -------- fetch finished job if available ------ */
        {
            unique_lock<mutex> lk(mOut);
            if (!qOut.empty())
            {
                JobOutput r = move(qOut.front()); qOut.pop();
                lk.unlock();
                cv::imshow("flow", r.vis);
                cv::imshow("mask", r.mask);
            }
        }
        if (cv::waitKey(1)>=0) break;
    }

    /* ---------- shutdown ---------- */
    stop = true;  cvIn.notify_one();
    worker.join();
    return 0;
}
