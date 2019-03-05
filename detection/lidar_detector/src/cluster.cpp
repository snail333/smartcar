/*
 * @Description: 
 * @Author: sunm
 * @Github: https://github.com/sunmiaozju
 * @LastEditors: sunm
 * @Date: 2019-03-05 20:38:52
 * @LastEditTime: 2019-03-05 22:44:38
 */
#include "cluster.h"

using namespace cv;

namespace LidarDetector {

std::vector<cv::Scalar> color_table;

Cluster::Cluster()
{
    generateColors(color_table, 255, 100);
}
Cluster::~Cluster() {}
void Cluster::setCloud(const pcl::PointCloud<pcl::PointXYZ>::Ptr in_cloud,
    const std::vector<int>& cluster_indices, const double& cluster_id)
{
    float min_x = std::numeric_limits<float>::max();
    float max_x = -std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float max_y = -std::numeric_limits<float>::max();
    float min_z = std::numeric_limits<float>::max();
    float max_z = -std::numeric_limits<float>::max();

    for (size_t i = 0; i < cluster_indices.size(); i++) {
        pcl::PointXYZRGB p;
        p.x = in_cloud->points[i].x;
        p.y = in_cloud->points[i].y;
        p.z = in_cloud->points[i].z;
    }
}

/**
 * @description: opencv2 的功能函数 产生随机颜色数组 
 */
void generateColors(std::vector<cv::Scalar>& colors, size_t count, size_t factor)
{
    if (count < 1)
        return;

    colors.resize(count);

    if (count == 1) {
        colors[0] = Scalar(0, 0, 255); // red
        return;
    }
    if (count == 2) {
        colors[0] = Scalar(0, 0, 255); // red
        colors[1] = Scalar(0, 255, 0); // green
        return;
    }

    // Generate a set of colors in RGB space. A size of the set is severel times (=factor) larger then
    // the needed count of colors.
    Mat bgr(1, (int)(count * factor), CV_8UC3);
    randu(bgr, 0, 256);

    // Convert the colors set to Lab space.
    // Distances between colors in this space correspond a human perception.
    Mat lab;
    cvtColor(bgr, lab, cv::COLOR_BGR2Lab);

    // Subsample colors from the generated set so that
    // to maximize the minimum distances between each other.
    // Douglas-Peucker algorithm is used for this.
    Mat lab_subset;
    downsamplePoints(lab, lab_subset, count);

    // Convert subsampled colors back to RGB
    Mat bgr_subset;
    cvtColor(lab_subset, bgr_subset, cv::COLOR_BGR2Lab);

    CV_Assert(bgr_subset.total() == count);
    for (size_t i = 0; i < count; i++) {
        Point3_<uchar> c = bgr_subset.at<Point3_<uchar>>((int)i);
        colors[i] = Scalar(c.x, c.y, c.z);
    }
}

static void downsamplePoints(const cv::Mat& src, cv::Mat& dst, size_t count)
{
    CV_Assert(count >= 2);
    CV_Assert(src.cols == 1 || src.rows == 1);
    CV_Assert(src.total() >= count);
    CV_Assert(src.type() == CV_8UC3);

    dst.create(1, (int)count, CV_8UC3);
    //TODO: optimize by exploiting symmetry in the distance matrix
    Mat dists((int)src.total(), (int)src.total(), CV_32FC1, Scalar(0));
    if (dists.empty())
        std::cerr << "Such big matrix cann't be created." << std::endl;

    for (int i = 0; i < dists.rows; i++) {
        for (int j = i; j < dists.cols; j++) {
            float dist = (float)norm(src.at<Point3_<uchar>>(i) - src.at<Point3_<uchar>>(j));
            dists.at<float>(j, i) = dists.at<float>(i, j) = dist;
        }
    }

    double maxVal;
    Point maxLoc;
    minMaxLoc(dists, 0, &maxVal, 0, &maxLoc);

    dst.at<Point3_<uchar>>(0) = src.at<Point3_<uchar>>(maxLoc.x);
    dst.at<Point3_<uchar>>(1) = src.at<Point3_<uchar>>(maxLoc.y);

    Mat activedDists(0, dists.cols, dists.type());
    Mat candidatePointsMask(1, dists.cols, CV_8UC1, Scalar(255));
    activedDists.push_back(dists.row(maxLoc.y));
    candidatePointsMask.at<uchar>(0, maxLoc.y) = 0;

    for (size_t i = 2; i < count; i++) {
        activedDists.push_back(dists.row(maxLoc.x));
        candidatePointsMask.at<uchar>(0, maxLoc.x) = 0;

        Mat minDists;
        reduce(activedDists, minDists, 0, CV_REDUCE_MIN);
        minMaxLoc(minDists, 0, &maxVal, 0, &maxLoc, candidatePointsMask);
        dst.at<Point3_<uchar>>((int)i) = src.at<Point3_<uchar>>(maxLoc.x);
    }
}
}