
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/videoio.hpp>
#include <iostream>
#include <opencv2/highgui/highgui_c.h>
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audioclient.h>
#include <cstdio>
/////////////////////////////////////////////////////////////////////加俩/////////////////////////////////////////加俩
#include <windows.h>
#include <cstdlib>


using namespace cv;
using namespace std;

Mat img; //read image

/////////////////////////////////////////////////////////////////////////////////////加上当前窗口的坐标x,y和当前窗口的高度和宽度
int Frame_x;
int Frame_y;
int windows_width=1700;
int windows_height=1000;
int Frame_width;
int Frame_height;
int frameR=100;
int smoothening=30;



//设置系统音量
bool SetVolum(int volume)
{
    bool ret = false;
    HRESULT hr;
    IMMDeviceEnumerator* pDeviceEnumerator=0;
    IMMDevice* pDevice=0;
    IAudioEndpointVolume* pAudioEndpointVolume=0;
    IAudioClient* pAudioClient=0;

        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pDeviceEnumerator);
        hr = pDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice);
        hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pAudioEndpointVolume);
        hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);

        float fVolume;
        fVolume = volume / 100.0f;
        hr = pAudioEndpointVolume->SetMasterVolumeLevelScalar(fVolume, &GUID_NULL);

        pAudioClient->Release();
        pAudioEndpointVolume->Release();
        pDevice->Release();
        pDeviceEnumerator->Release();

        ret = true;

    return ret;
}
//获取系统音量
int GetVolume()
{
    int volumeValue = 0;
    HRESULT hr;
    IMMDeviceEnumerator* pDeviceEnumerator = 0;
    IMMDevice* pDevice = 0;
    IAudioEndpointVolume* pAudioEndpointVolume = 0;
    IAudioClient* pAudioClient = 0;

        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pDeviceEnumerator);
        hr = pDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice);
        hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pAudioEndpointVolume);
        hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);

        float fVolume;
        hr = pAudioEndpointVolume->GetMasterVolumeLevelScalar(&fVolume);


        pAudioClient->Release();
        pAudioEndpointVolume->Release();
        pDevice->Release();
        pDeviceEnumerator->Release();

        volumeValue = fVolume * 100;

    return volumeValue;
}

void getcontour(Mat imgDil,Mat img)//判断轮廓
{
    vector<vector<Point>> contour;
    vector<Vec4i> hierarchy;
    findContours(imgDil, contour, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_TC89_KCOS);

    vector<vector<Point>> conPoly(contour.size());
    vector<Rect> boundRect(contour.size());
    for (int i = 0; i < contour.size(); i++)
    {
        int area = contourArea(contour[i]);//轮廓面积用于排除黑点
        cout << area << endl;
        string objectType;//形状

        if (area > 5000)//排除小黑圆点的干扰
        {
            float peri = arcLength(contour[i], true);
            approxPolyDP(contour[i], conPoly[i], 0.02 * peri, true);//把一个连续光滑曲线折线化
            cout << conPoly[i].size() << endl;//边数
            boundRect[i] = boundingRect(conPoly[i]);

            int objCor = (int)conPoly[i].size();
            if (objCor <= 6 && objCor>=6) { objectType = "Triangle"; }
            else if (objCor <= 10 && objCor>=7) { objectType = "Pentagon"; }
            else if (objCor == 4) {
                float aspRatio = (float)boundRect[i].width / boundRect[i].height;//长宽比来区别正方形与矩形
                cout << aspRatio << endl;
                if (aspRatio > 0.95 && aspRatio < 1.05) { objectType = "Square"; }
                else { objectType = "Rectangle"; }
            }
            else { objectType = "Circle"; }
            drawContours(img, conPoly, i, Scalar(255, 0, 255), 2);
            rectangle(img, boundRect[i].tl(), boundRect[i].br(), Scalar(0.255, 0), 5);//框出图形
            putText(img, objectType, { boundRect[i].x,boundRect[i].y - 5 },1,FONT_HERSHEY_PLAIN, Scalar(0, 69, 255), 1);
        }
    }
}

int main() {
    main_loop:
    //init variables

    Mat imgThresholded; //used to contour
    int iLastX = -1;
    int iLastY = -1;


    VideoCapture cap(0);
    cap.read(img);

    Mat imgLines = Mat::zeros(img.size(), CV_8UC3); //used to draw lines
    Mat imgShape = Mat::zeros(img.size(), CV_8UC3); //separate mat only contains drawn shape
    Mat imgshapetempt= Mat::zeros(img.size(), CV_8UC3);
    CascadeClassifier rhandCascade, lhandCascade, fistCascade;
    rhandCascade.load("../haarcascade/rpalm.xml");
    lhandCascade.load("../haarcascade/lpalm.xml");
    fistCascade.load("../haarcascade/closed_frontal_palm.xml");

    if (rhandCascade.empty()) { cout << "XML file not loaded" << endl; }

    int curx, cury, lastx, lasty;
    lastx = 1;
    lasty = -1;

    bool  isFist;
    while (true) {

        cap.read(img);
        flip(img, img, 1);

        Mat imgRect = Mat::zeros(img.size(), CV_8UC3); //used to draw rectangles(not save it on frame)
        //create gray image
        Mat gray;
        cvtColor(img, gray, COLOR_BGR2GRAY);
        vector<Rect> rhand, lhand, fist;

        //fist sometimes detect not fist objects
        fistCascade.detectMultiScale(gray, fist, 1.1, 2, CASCADE_SCALE_IMAGE, Size(30, 30));
        lhandCascade.detectMultiScale(gray, lhand, 1.1, 2, CASCADE_SCALE_IMAGE, Size(30, 30));
        rhandCascade.detectMultiScale(gray, rhand, 1.1, 2, CASCADE_SCALE_IMAGE, Size(30, 30));

        cv::rectangle(img,Point (frameR,frameR),
                      Point (4*frameR,3*frameR),(255,0,255),2);//////////////////////////////////////////////////////设置移动区域
    //rhand 移动鼠标
        if (!rhand.empty()) {
            isFist= false;
            for (int i = 0; i < rhand.size(); i++) {
                rectangle(imgRect, rhand[i].tl(), rhand[i].br(), Scalar(255, 0, 0), 2);
                curx = (rhand[i].br().x + rhand[i].tl().x) / 2;
                cury = (rhand[i].br().y + rhand[i].tl().y) / 2;

                if (lastx >= 0 && lasty >= 0 && curx >= 0 && cury >= 0
                    && abs(curx-lastx)<50 && abs(cury-lasty)<50 //ignore point too far
                    && (abs(curx-lastx)>1 || abs(cury-lasty)>1)) { //ignore point too close
                    line(imgLines, Point(lastx, lasty), Point(curx, cury), Scalar(255, 0, 255), 2);
                }
                POINT p;
                int mouse_x=lastx+(curx-lastx)/smoothening;
                int mouse_y=lasty+(cury-lasty)/smoothening;

                if(mouse_x<=4.5*frameR&&mouse_x>=frameR/2&&mouse_y>=frameR/2&&mouse_y<=3.5*frameR){
                    SetCursorPos((mouse_x-frameR>0)?(mouse_x-frameR)*(windows_width/300):0,
                                 mouse_y-frameR>0?((mouse_y-frameR)*(windows_height/200)):0);
                    cv::circle(imgRect,Point(mouse_x, mouse_y) , 15, (255,255,255), cv::FILLED);

                }
                cout<<"Frame_x: "<<(Frame_x)<<"   Frame_y: "<<(Frame_y)<<"  mouse_x: "<<(mouse_x)<<"  mouse_y: "<<(mouse_y)
                    <<"frameR"<<(frameR)<<"  "<<
                    ((abs(mouse_x-frameR))*(windows_width/300))<<"  "<<((abs(mouse_y-frameR))*(windows_height/200))<<" "<<endl;
                if(abs(lastx-curx)>2){
                    lastx = curx;
                }
                if(abs(lasty-cury)>2){
                    lasty = cury;
                }


            }
        }
//            lhand 设置音量
        else if (!lhand.empty()) {
            isFist= false;
            for (int i = 0; i < lhand.size(); i++) {
                rectangle(imgRect, lhand[i].tl(), lhand[i].br(), Scalar(0, 0, 255), 2);
                curx = (lhand[i].br().x + lhand[i].tl().x) / 2;
                cury = (lhand[i].br().y + lhand[i].tl().y) / 2;
                if (lastx >= 0 && lasty >=  0 && curx >= 0 && cury >= 0
                    && abs(curx - lastx) <100 && abs(cury - lasty) < 100 //ignore point too far
                    && (abs(curx - lastx) > 10 || abs(cury - lasty) > 10)) { //ignore point too close
                    line(imgLines, Point(lastx, lasty), Point(curx, cury), Scalar(0, 0, 255), 1);
                }
                CoInitialize(0);

                int volumeValue = GetVolume();
                cout <<"current volume "<<volumeValue<<"set volume "<<(int)((double)cury/windows_height*100)<<"cury:"<<cury<<endl;
                SetVolum((int)((double)cury/windows_height*100));

                CoUninitialize();
                if (abs(lastx - curx) > 3) {
                    lastx = curx;
                }
                if (abs(lasty - cury) > 3) {
                    lasty = cury;
                }
            }
        }
        //拳头单击或双击
        else if (!fist.empty()) {
            bool isDouble= false;

            for (int i = 0; i < fist.size(); i++) {
                rectangle(imgRect, fist[i].tl(), fist[i].br(), Scalar(0, 255, 0), 2);
                curx = (fist[i].br().x + fist[i].tl().x) / 2;
                cury = (fist[i].br().y + fist[i].tl().y) / 2;
                if (isFist&&(abs(cury - lasty) >10||abs(curx - lastx)>10)){ //ignore point too far
                    isDouble= true;
                }

                if(isDouble){
                    mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);
                    Sleep(100);
                    mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);
                    mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);
                    Sleep(100);
                    mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);
                    cout<<"Double click     curx:"<<curx<<"cury:"<<cury<<"lastx:"<<lastx<<"lasty"<<lasty<<endl;
                }else{
                    mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);
                    Sleep(100);
                    mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);
                    cout<<"single click     curx:"<<curx<<"cury:"<<cury<<"lastx:"<<lastx<<"lasty"<<lasty<<endl;
                }

                if(abs(lastx-curx)>4){
                    lastx = curx;
                }
                if(abs(lasty-cury)>4){
                    lasty = cury;
                }
                Sleep(100);
                isFist= true;
            }
        }
        else {
            lastx =  -1;
            lasty = -1;
        }
        img = img + imgRect;
        img = img + imgLines;
        imgShape = imgShape + imgLines;
        imgshapetempt= imgshapetempt + imgLines;

        int keyboard = waitKey(30);
        if (keyboard == 'c') {
            //  destroyWindow("Contour");
            destroyWindow("Frame");
            goto main_loop;
        }
        if(keyboard == 'w' ){

            //shape_detect-----------------------------------------------------
            Mat imgGray=Mat::zeros(img.size(), CV_8UC3);
            Mat imgBlur=Mat::zeros(img.size(), CV_8UC3);
            Mat imgCanny=Mat::zeros(img.size(), CV_8UC3);
            Mat imgDila=Mat::zeros(img.size(), CV_8UC3);
            Mat imgErode=Mat::zeros(img.size(), CV_8UC3);
            Mat kernel = getStructuringElement(MORPH_RECT, Size(50, 50));
            //preprocessing
            cvtColor(imgShape, imgGray,COLOR_BGR2GRAY, 0);
            GaussianBlur(imgGray, imgBlur, Size(65, 65), 1, 1);
            Canny(imgBlur, imgCanny, 40, 120);
            dilate(imgCanny, imgDila, kernel);
            vector<vector<Point>> contour;
            vector<Vec4i> hierarchy;
            findContours(imgDila, contour, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
            vector<vector<Point>> conPoly(contour.size());
            vector<vector<Point>> conPoly2(contour.size());
            vector<Rect> boundRect(contour.size());
            for (int i = 0; i < contour.size(); i++) {
                int area = contourArea(contour[i]);
                cout << area << endl;
                string objectType;

                if (area > 5000)
                {
                    float peri = arcLength(contour[i], true);
                    approxPolyDP(contour[i], conPoly[i], 0.02 * peri, true);
                    cout << conPoly[i].size() << endl;
                    boundRect[i] = boundingRect(conPoly[i]);

                    int objCor = (int) conPoly[i].size();
                    if (objCor == 6) {
                        objectType = "Triangle";
                        approxPolyDP(contour[i], conPoly2[i], 0.13 * peri, true);

                        drawContours(imgshapetempt, conPoly2, i, Scalar(255, 0, 255), 2);


                    } else if (objCor == 5) {
                        objectType = "Pentagon";
                        drawContours(imgshapetempt, conPoly, i, Scalar(255, 0, 255), 2);

                    } else if (objCor == 4) {
                        float aspRatio = (float) boundRect[i].width / boundRect[i].height;//长宽比来区别正方形与矩形
                        cout << aspRatio << endl;

                        objectType = "Quadrilateral";
                        drawContours(imgshapetempt, conPoly, i, Scalar(255, 0, 255), 2);


                    } else {
                        objectType = "Circle";
                        drawContours(imgshapetempt, conPoly, i, Scalar(255, 0, 255), 2);

                    }
                    putText(imgshapetempt, objectType, {boundRect[i].x, boundRect[i].y - 5}, 1, FONT_HERSHEY_PLAIN, Scalar(0, 69, 255),
                            1);
                }
            }
            imshow("Image dila", imgDila);
            imshow("Image tempt", imgshapetempt);
//-----------------------------------------------------------------------------
        }
if(keyboard == 'p'){
    string writePath = "tempt_img/";
    VideoCapture capture(0);//specify the camera device to open
    string name;
    namedWindow("hello", CV_WINDOW_AUTOSIZE); //window name and make the image appear the size of the original image
    int i = 0;
    while (1) {
        Mat frame;
        capture >> frame;
        if (32 == waitKey(20)) {            //space to photo
            name = writePath + to_string(i) + ".jpg"; //photo name
            imwrite(name, frame); // store photo
            cout << name << endl;
            i++;
        }
        imshow("hello", frame); //display the picture in the window

    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////改了一下可调节页面大小/////////////////////////////////////////////////
        namedWindow("Frame", CV_WINDOW_NORMAL);//CV_WINDOW_NORMAL就是0
        imshow("Frame", img);
        Rect  rect=cv::getWindowImageRect("Frame");
//        cout<<"x: "<<rect.x<<"   y: "<<rect.y<<"   width: "<<rect.width<<"   height: "<<rect.height<<endl;
        Frame_x=rect.x;
        Frame_y=rect.y;
        Frame_width=rect.width;
        Frame_height=rect.height;



        //imshow("Contour",imgThresholded);
        if (keyboard == 'q' || keyboard == 27) { break; }


    }

}
