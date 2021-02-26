#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <iostream>

using namespace cv;
using namespace std;

void setup_windows(void);
Mat gray_image(Mat img_gray, Mat img_roi);
Mat threshold_image(Mat img_gray, Mat img_roi);
void gesture(int count, char (&a)[40]);

int main(int argc, const char** argv)
{
    setup_windows();

    return 0;
}

void setup_windows(void)
{
    VideoCapture cam(0);
    if (!cam.isOpened()) {
        cout << "ERROR not opened " << endl;
        return;
    }
    Mat img;
    Mat img_threshold;
    Mat img_gray;
    Mat img_roi;
    namedWindow("Original_image", WINDOW_AUTOSIZE);
    namedWindow("Gray_image", WINDOW_AUTOSIZE);
    namedWindow("Thresholded_image", WINDOW_AUTOSIZE);
    namedWindow("ROI", WINDOW_AUTOSIZE);

    char a[40];
    int count = 0;
    while (1) {
        bool b = cam.read(img);
        if (!b) {
            cout << "ERROR : cannot read" << endl;
            return;
        }
        Rect roi(0, 0, img.cols, img.rows);
        img_roi = img(roi);
        
        img_gray = gray_image(img_gray, img_roi);
        img_threshold = threshold_image(img_gray, img_threshold);
        
        vector<vector<Point> >contours;
        vector<Vec4i>hierarchy;
        findContours(img_threshold, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point());
        if (contours.size() > 0) {
            int indexOfBiggestContour = -1;
            int sizeOfBiggestContour = 0;

            for (int i = 0; i < contours.size(); i++) {
                if (contours[i].size() > sizeOfBiggestContour) {
                    sizeOfBiggestContour = contours[i].size();
                    indexOfBiggestContour = i;
                }
            }
            vector<vector<int> >hull(contours.size());
            vector<vector<Point> >hullPoint(contours.size());
            vector<vector<Vec4i> >defects(contours.size());
            vector<vector<Point> >defectPoint(contours.size());
            vector<vector<Point> >contours_poly(contours.size());
            Point2f rect_point[4];
            vector<Rect> boundRect(contours.size());
            for (int i = 0; i < contours.size(); i++) {
                if (contourArea(contours[i]) > 5000) {
                    convexHull(contours[i], hull[i], true);
                    convexityDefects(contours[i], hull[i], defects[i]);
                    if (indexOfBiggestContour == i) {
                        for (int k = 0; k < hull[i].size(); k++) {
                            int ind = hull[i][k];
                            hullPoint[i].push_back(contours[i][ind]);
                        }
                        count = 0;

                        for (int k = 0; k < defects[i].size(); k++) {
                            if (defects[i][k][3] > 13 * 256) {
                                /*   int p_start=defects[i][k][0];   */
                                int p_end = defects[i][k][1];
                                int p_far = defects[i][k][2];
                                defectPoint[i].push_back(contours[i][p_far]);
                                circle(img_roi, contours[i][p_end], 3, Scalar(0, 255, 0), 2);
                                count++;
                            }

                        }

                        gesture(count, a);

                        putText(img, a, Point(70, 70), FONT_HERSHEY_SIMPLEX, 3, Scalar(255, 0, 0), 2, 8, false);

                    }
                }

            }
            imshow("Original_image", img);
            imshow("Gray_image", img_gray);
            imshow("Thresholded_image", img_threshold);
            imshow("ROI", img_roi);
            if (waitKey(30) == 27) {
                return ;
            }

        }

    }
}

Mat gray_image(Mat img_gray, Mat img_roi)
{
    cvtColor(img_roi, img_gray, COLOR_BGR2GRAY);
    GaussianBlur(img_gray, img_gray, Size(19, 19), 0.0, 0);
    return img_gray;
}

Mat threshold_image(Mat img_gray, Mat img_threshold)
{
    threshold(img_gray, img_threshold, 0, 255, THRESH_BINARY_INV + THRESH_OTSU);
    return img_threshold;
}

void gesture(int count, char (&a)[40])
{
    if (count == 1)
        strcpy_s(a, "one");
    else if (count == 2)
        strcpy_s(a, "two");
    else if (count == 3)
        strcpy_s(a, "three");
    else if (count == 4)
        strcpy_s(a, "four");
    else if (count == 5)
        strcpy_s(a, "five");
    else
        strcpy_s(a, "nothing");
}

