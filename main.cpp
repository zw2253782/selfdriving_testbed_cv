
/*selfdriving-cv-tutorial
Selfdriving based on Computer Vision. First step: objects detection

Stop sign and traffic light.

method: opencv cascade object detection
cascade train parameter file (parameter.xml) Terminal: $ opencv_createsamples -img img.png -info obj-rects.txt -w 50 -h 50 -vec pos-samples.vec $ find negative > neg-filepaths.txt $ opencv_traincascade -data obj-classifier -vec pos-samples.vec -bg neg-filepaths.txt -precalcValBufSize 2048 -precalcIdxBufSize 2048 -numPos 200 -numNeg 2000 -nstages 20 -minhitrate 0.999 -maxfalsealarm 0.5 -w 50 -h 50 -nonsym -baseFormatSave //neg-filepaths.txt could be changed to bg.txt //new a diret named “obj-classifier” first

object detection based on the parameter

As is shown in main.cpp : cascade_object(file_name, folder_name);*/

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <math.h>
#include <time.h>
#include <map>
#include <string>

// #define WINDOWS  /* uncomment this line to use it for windows.*/
#ifdef WINDOWS
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

using namespace cv;
using namespace std;

void creatDir(string file_path);
string GetCurrentWorkingDir();
void detectStopSign(string obj_address,string model_address);
void cascade_object(string file_name,string folder_name);
void draw_locations(Mat & img, vector< Rect > & locations, const Scalar & color,string text);

int main( int argc, char** argv )
{

  string file_name = string("5.jpg");
  string folder_name = string("/home/wei/Documents/project/selfdriving-cv-tutorial/dataset/train/positive_stop/");
  //string obj_address = string("/home/wei/Downloads/cv-selfdriving/StopSignDataset/") + file_name;
  //string model_address = string("/home/wei/Downloads/cv-selfdriving/StopSignDataset/stopPrototype.png");
  //detectStopSign(obj_address, model_address);
  cascade_object(file_name, folder_name);

  return 0;
}

//get the local file path
std::string GetCurrentWorkingDir() {
  char buff[FILENAME_MAX];
  GetCurrentDir( buff, FILENAME_MAX );
  std::string current_working_dir(buff);
  return current_working_dir;
}

//creat a new directory with name file_path
void creatDir(string file_path){
	std::string dir = "mkdir -p " + GetCurrentWorkingDir() + file_path;
	const int dir_err = system(dir.c_str());
	if (-1 == dir_err)
	{
	    printf("Error creating directory!n");
	    exit(1);
	}
}

// computes mean square error between two n-d matrices.
// lower -> more similar
static double meanSquareError(const Mat &img1, const Mat &img2) {
    Mat s1;
    absdiff(img1, img2, s1);   // |img1 - img2|
    s1.convertTo(s1, CV_32F);  // cannot make a square on 8 bits
    s1 = s1.mul(s1);           // |img1 - img2|^2
    Scalar s = sum(s1);        // sum elements per channel
    double sse = s.val[0] + s.val[1] + s.val[2];  // sum channels
    double mse = sse / (double)(img1.channels() * img1.total());
    return mse;
}


void detectStopSign(string obj_address, string model_address) {
	// set threshold of detection a stop sign
	int THRESHOLD = 7100;
    Mat targetImage = imread(obj_address, IMREAD_COLOR);;

    // resize the image
    int width = 500;
    int height = width * targetImage.rows / targetImage.cols;
    resize(targetImage, targetImage, Size(width, height));
    // read prototype image
    Mat prototypeImg = imread(model_address, IMREAD_COLOR);

    int minMSE = INT_MAX;
    int location[4] = {0, 0, 0, 0};
    // start time
    int t0 = clock();
    //Mat tmpImg;
    Mat tmpImg = prototypeImg.clone();
    Mat window;
    for (int wsize = tmpImg.cols; wsize > 15; wsize /= 1.3) {
        if (tmpImg.rows < 15 || tmpImg.cols < 15)
        	break;
        if (tmpImg.rows > 900 || tmpImg.cols > 900) {
        	resize(tmpImg, tmpImg, Size(wsize, wsize));
        	continue;
        }
        cout << "Image pyramid width: " << wsize << " height: " << wsize << endl;
        for (int y = 0; y < targetImage.rows; y += 5) {
            for (int x = 0; x < targetImage.cols; x += 5) {
            	if (x + tmpImg.cols > targetImage.cols || y + tmpImg.cols > targetImage.rows)
            	    continue;
                Rect R(x, y, tmpImg.cols, tmpImg.cols); // create a rectangle
                window = targetImage(R); // crop the region of interest using above rectangle
                if (window.rows != tmpImg.rows || window.cols != tmpImg.cols)
                    continue;
                double tempSim = meanSquareError(tmpImg, window);
                if (tempSim < minMSE) {
                    minMSE = tempSim;
                    location[0] = x;
                    location[1] = y;
                    location[2] = tmpImg.rows;
                    location[3] = tmpImg.cols;
                }
            }
        }
        resize(tmpImg, tmpImg, Size(wsize, wsize));
    }

    // end time
    int t1 = clock();

    cout << "Execution time: " << (t1 - t0)/double(CLOCKS_PER_SEC)*1000 << " ms" << endl;
    cout << "Minimum MSE: " << minMSE << endl;
    if (minMSE < THRESHOLD) {
    	int buff1 = 50;
    	int x = location[0];
    	int y = location[1];
    	int w = location[2];
    	int h = location[3];
    	// draw the rectangle
    	rectangle(targetImage, Point(x-buff1/2,y-buff1/2), Point(x+w+buff1/2,y+h+buff1/2), Scalar(0,255,0), 2);
    	cout << "Stop sign found!" << endl;
    } else {
    	cout << "Stop sign not found!" << endl;
    }

    // show the image
    imshow("image", targetImage);
    waitKey(50000);
}

//use cascade to detect object
#define STOP_SIGN_CASCADE_NAME GetCurrentWorkingDir() + "/parameter/stopsign_classifier2.xml"
#define TRAFFIC_LIGHT_CASCADE_NAME GetCurrentWorkingDir() + "/parameter/trafficlight_classifier2.xml"

void cascade_object(string file_name, string folder_name) {
    cout << "OpenCV Version: " << CV_VERSION << endl;
	CascadeClassifier cars, traffic_light, stop_sign, pedestrian;
	vector<Rect> cars_found, traffic_light_found, stop_sign_found, pedestrian_found;

	string obj_address = folder_name + file_name;
    Mat targetImage;
    if (!obj_address.empty()) {
    	targetImage = imread(obj_address);
    	cout << "Target Image: " << obj_address << endl;
    }

    traffic_light.load(TRAFFIC_LIGHT_CASCADE_NAME);
    stop_sign.load(STOP_SIGN_CASCADE_NAME);

    double t = (double) getTickCount(); // start time

	traffic_light.detectMultiScale(targetImage, traffic_light_found, 1.1, 2, 0 | CASCADE_SCALE_IMAGE, Size(10, 10));
    stop_sign.detectMultiScale(targetImage, stop_sign_found, 1.1, 2, 0 | CASCADE_SCALE_IMAGE, Size(10, 10));

    t = (double) getTickCount() - t; // time duration
    cout << "Time taken : " << (t*1000./cv::getTickFrequency()) << " s" << endl;

    // draw_locations(targetImage, cars_found, Scalar(0, 255, 0), "Car");
    draw_locations(targetImage, traffic_light_found, Scalar(0, 255, 255), "Traffic Light");
    draw_locations(targetImage, stop_sign_found, Scalar(0, 0, 255), "Stop Sign");

	// imshow(WINDOW_NAME, targetImage);
	// waitKey(3000);
    creatDir("/object_detection_output");
    string output_address = GetCurrentWorkingDir() + "/object_detection_output/output" + file_name;
    imwrite(output_address, targetImage);
}

//rectangle
void draw_locations(Mat & img, vector< Rect > &locations, const Scalar & color, string text) {
	if (!locations.empty()) {
        for(size_t i = 0 ; i < locations.size(); ++i){
            rectangle(img, locations[i], color, 3);
            putText(img, text, Point(locations[i].x+1, locations[i].y+8), FONT_HERSHEY_DUPLEX, 0.3, color, 1);
        }
	}
}






