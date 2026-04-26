#include<opencv2/opencv.hpp>
#include<iostream>
using namespace std;

void myFastestMeanFilter(IplImage* src, IplImage* dst, int k);

int main() {
	char file_path[1000] = "c:\\Temp\\lebron2.jpg";
	int k = 50;

	//파일 경로 입력
	cout << "Fastest Mean Filtering!" << endl;
	cout << "Input File Dir: ";
	cin >> file_path;

	IplImage* src = cvLoadImage(file_path);
	if (src == nullptr) {
		cout << "Error: cannot open " << file_path << endl;
		return 0;
	}
	IplImage* dst = cvCreateImage(cvGetSize(src), 8, 3);

	//k 값 입력
	cout << "Input Kernal Size: ";
	cin >> k;
	if (k < 0 || k > src->height || k > src->width) {
		cout << "Error: Invalid k size!" << endl;
		return 0;
	}

	//빠른 평균필터 적용
	myFastestMeanFilter(src, dst, k);

	//결과 출력
	cvShowImage("src", src);
	cvShowImage("dst", dst);
	cvSaveImage("c:\\temp\\tmp1.png", dst);
	cvWaitKey();

	return 0;
}

void myFastestMeanFilter(IplImage* src, IplImage* dst, int k) {
	//배열 동적 할당
	CvScalar **sat = new CvScalar *[src->height];
	for (int i = 0; i < src->height; i++) 
		sat[i] = new CvScalar[src->width];

	//SAT(summed area table) 구하기
	for (int y = 0; y < src->height; y++) {
		for (int x = 0; x < src->width; x++) {
			CvScalar g = cvGet2D(src, y, x);
			for (int i = 0; i < 3; i++) {
				double a = (y) ? sat[y - 1][x].val[i] : 0;
				double b = (x) ? sat[y][x - 1].val[i] : 0;
				double c = (y * x) ? sat[y - 1][x - 1].val[i] : 0;
				sat[y][x].val[i] = a + b - c + g.val[i];	//sat[y][x] = sat[y-1][x] + sat[y][x-1] - sat[y-1][x-1] + g
			}
		}
	}

	//Mean Filtering
	for (int y = 0; y < src->height; y++) {
		for (int x = 0; x < src->width; x++) {
			CvScalar f = cvScalar(0, 0, 0);
			int corner_u = max(0, y - k - 1), corner_l = max(0, x - k - 1), corner_r = min(src->width - 1, x + k), corner_d = min(src->height - 1, y + k);
			int count = (corner_d - corner_u) * (corner_r - corner_l);
			for (int i = 0; i < 3; i++) {
				double a = sat[corner_u][corner_l].val[i];
				double b = sat[corner_u][corner_r].val[i];
				double c = sat[corner_d][corner_l].val[i];
				double d = sat[corner_d][corner_r].val[i];
				f.val[i] = (d - b - c + a) / count; //avg = (LR-UR-LL+UL)/w*h
			}
			cvSet2D(dst, y, x, f);
		}
	}

	//동적 배열 메모리 해제
	for (int i = 0; i < src->height; i++)
		delete[] sat[i];
	delete[] sat;
}