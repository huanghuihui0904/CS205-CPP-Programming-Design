
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

using namespace cv;
using namespace std;




int main() {
    string path = "../q4_test.png";
    Mat img = imread(path);
    Mat Gray, Blur, imgCanny, Dila;
    Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));

    cvtColor(img, Gray, COLOR_BGR2GRAY, 0);
    GaussianBlur(Gray, Blur, Size(65, 65), 1, 1);
    Canny(Blur, imgCanny, 40, 120);
    dilate(imgCanny, Dila, kernel);

    vector<vector<Point>> contour;
    vector<Vec4i> hierarchy;
    findContours(Dila, contour, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_TC89_KCOS);

    vector<vector<Point>> conPoly(contour.size());
    vector<Rect> boundRect(contour.size());
    for (int i = 0; i < contour.size(); i++) {
        int area = contourArea(contour[i]);

        string objectType;

        if (area > 1200)
        {
            float peri = arcLength(contour[i], true);
            approxPolyDP(contour[i], conPoly[i], 0.03 * peri, true);

            boundRect[i] = boundingRect(conPoly[i]);

            int objCor = (int) conPoly[i].size();
            if (objCor == 3) {
                objectType = "Triangle";
                drawContours(img, conPoly, i, Scalar(255, 0, 255), 2);
                rectangle(img, boundRect[i].tl(), boundRect[i].br(), Scalar(0.255, 0), 5);
                putText(img, objectType, {boundRect[i].x, boundRect[i].y - 5}, 1, FONT_HERSHEY_PLAIN,
                        Scalar(0, 69, 255), 1);
            } else if (objCor == 4) {
                float aspRatio = (float) boundRect[i].width / boundRect[i].height;
                cout << aspRatio << endl;
                if (aspRatio > 0.95 && aspRatio < 1.05) {
                    objectType = "Square";
                    drawContours(img, conPoly, i, Scalar(255, 0, 255), 2);
                    rectangle(img, boundRect[i].tl(), boundRect[i].br(), Scalar(0.255, 0), 5);
                    putText(img, objectType, {boundRect[i].x, boundRect[i].y - 5}, 1, FONT_HERSHEY_PLAIN,
                            Scalar(0, 69, 255), 1);
                } else {
                    objectType = "Rectangle";
                    drawContours(img, conPoly, i, Scalar(255, 0, 255), 2);
                    rectangle(img, boundRect[i].tl(), boundRect[i].br(), Scalar(0.255, 0), 5);
                    putText(img, objectType, {boundRect[i].x, boundRect[i].y - 5}, 1, FONT_HERSHEY_PLAIN,
                            Scalar(0, 69, 255), 1);
                }
            } else if (objCor > 4) {
                objectType = "Circle";
                drawContours(img, conPoly, i, Scalar(255, 0, 255), 2);
                rectangle(img, boundRect[i].tl(), boundRect[i].br(), Scalar(0.255, 0), 5);
                putText(img, objectType, {boundRect[i].x, boundRect[i].y - 5}, 1, FONT_HERSHEY_PLAIN,
                        Scalar(0, 69, 255), 1);
            }
        }
    }



    imshow("Img", img);

    waitKey(0);
}
