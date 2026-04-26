#include<iostream>
#include<opencv2/opencv.hpp>
#include<time.h>
#include<string>
#include<random>

using namespace std;

//함수 선언
void CutImage(IplImage* src, IplImage* img[3], CvSize size);				//src 이미지를 삼등분 하여 img 배열에 저장
void CombineRGB(IplImage* dst, IplImage* rgb[3], CvSize size, pair<int,int> offset[3]);//rgb에 해당하는 세 개의 이미지와 각각의 offset이 주어졌을때 컬러 이미지로 만들어주는 함수
pair<int,int> Match(IplImage* origin, IplImage* move, CvSize size);			//origin과 move 이미지가 주어질때 move 이미지를 origin에 맞춰 정렬되기 위한 x,y값을 pair로 반환
//void overlay(IplImage* s1, IplImage* s2, IplImage* img, int u, int v);	//[디버깅용] 이미지가 맞춰질때 시각화
double getSSD(IplImage* s1, IplImage* s2, int u, int v);					//두 이미지가 주어질때 점들의 차이의 평균을 반환 
int min(int a, int b) { return (a < b) ? a : b; }

//정합 관련 상수 설정
const int area_size = 80;	//정렬 서치할 범위 (-area_size~area_size)
const int area_jump = 10;	//초기 서치할 간격 (area_jump만큼 건너뛰며 서치)
const int ssd_jump = 10;	//ssd 샘플링 간격
const int frame_size = 80;	//ssd 계산에서 제외할 테두리 크기


int main() {
	//경로 입력 받기
	char path[1000] = "C:\\Temp\\pg1.jpg";//기본 경로
	cout << "Test CV" << endl;
	cout << "Input File Name: ";
	cin >> path;

	//원본 이미지 로드 및 출력
	IplImage* src = cvLoadImage(path);
	if (src == nullptr) {
		cout << "Error: cannot open " << path << endl;
		return 0;
	}
	cvShowImage("src", src);

	clock_t start = clock();

	//3개의 이미지로 나누기
	CvSize size = cvSize(src->width, src->height / 3);
	IplImage* img[3] = { cvCreateImage(size, 8, 3), cvCreateImage(size, 8, 3), cvCreateImage(size, 8, 3) };
	CutImage(src, img, size);

	//3개의 이미지를 정렬
	pair<int,int> g = Match(img[0], img[1], size);
	pair<int, int> r = Match(img[0], img[2], size);
	pair<int, int> offset[3] = {{0,0},{g.first,g.second},{r.first,r.second}};
	
	//이미지 합치기
	IplImage* dst = cvCreateImage(size, 8, 3);
	CombineRGB(dst, img, size, offset);

	//결과 출력
	cvShowImage("dst", dst);
	clock_t end = clock();
	cout << "Your Image Is Ready!" << endl;
	cout << "Time: " << (double)(end - start) / CLOCKS_PER_SEC << " second" << endl;

	cvWaitKey();

	return 0;
}

//세로로 3등분 하여 각 채널 이미지에 저장
void CutImage(IplImage* src, IplImage* img[3], CvSize size) {
	for (int i = 0; i < 3; i++) {
		for (int y = 0; y < size.height; y++) {
			for (int x = 0; x < size.width; x++) {
				cvSet2D(img[i], y, x, cvGet2D(src, y + i * size.height, x));
			}
		}
	}
}

//채널 오프셋을 고려하여 rgb를 합성하여 컬러 이미지 생성
void CombineRGB(IplImage* dst, IplImage* rgb[3], CvSize size, pair<int,int> offset[3]) {
	for (int y = 0; y < size.height; y++) {
		for (int x = 0; x < size.width; x++) {
			CvScalar g = cvScalar(0, 0, 0);
			for (int k = 0; k < 3; k++) {
				int nx = x + offset[k].first;
				int ny = y + offset[k].second;
				if (nx < 0 || nx > dst->width - 1) continue;
				if (ny < 0 || ny > dst->height - 1) continue;
				CvScalar tmp = cvGet2D(rgb[k], ny, nx);
				g.val[k] = (tmp.val[0] + tmp.val[1] + tmp.val[2]) / 3;	//각 채널의 밝기를 색 값에 넣는다
			}
			cvSet2D(dst, y, x, g);
		}
	}
}

//두 이미지 사이의 최적 오프셋을 ssd를 통해 탐색
pair<int, int> Match(IplImage* s1, IplImage* s2, CvSize size) {
	float min_avg = FLT_MAX;
	int min_v, min_u;
	//우선 area_jump만큼 건너뛰어가며 탐색한다. 최소 ssd를 저장한다
	for (int v = -area_size; v <= area_size; v += area_jump) {
		for (int u = -area_size; u <= area_size; u += area_jump) {
			double diff = getSSD(s1, s2, u, v);
			if (min_avg > diff) {
				min_avg = diff;
				min_v = v, min_u = u;
			}
		}
	}
	//최소 ssd 지점에서 정밀 탐색을 한다. area_jump 크기의 사각형을 1픽셀씩 건너뛰며 탐색한다
	for (int v = min_v - area_jump / 2 + 1; v < min_v + area_jump / 2; v++) {
		for (int u = min_u - area_jump / 2 + 1; u < min_u + area_jump / 2; u++) {
			double diff = getSSD(s1, s2, u, v);
			if (min_avg > diff) {
				min_avg = diff;
				min_v = v, min_u = u;
			}
		}
	}
	cout << "offset: " << min_u << ',' << min_v << endl;
	return { min_u, min_v };
}

//ssd(sum of squared differences) 를 통한 유사도 계산
double getSSD(IplImage* s1, IplImage* s2, int u, int v) {
	double out = 0;
	int count = 0;
	//시간 효율을 위해 ssd_jump 만큼 건너뛰어가며 계산한다
	for (int y = frame_size; y < s1->height - frame_size; y+=ssd_jump) {
		for (int x = frame_size; x < s1->width - frame_size; x+=ssd_jump) {
			int x2 = x + u;
			int y2 = y + v;
			if (x2 < 0 || x2 > s2->width - 1) continue;	//움직인 이미지의 범위를 벗어나는 경우 무시
			if (y2 < 0 || y2 > s2->height - 1) continue;
			CvScalar f1 = cvGet2D(s1, y, x);
			CvScalar f2 = cvGet2D(s2, y2, x2);
			long long tmp = 0;
			for (int k = 0; k < 3; k++) {
				tmp += (long long)((f1.val[k] - f2.val[k]) * (f1.val[k] - f2.val[k]));	//픽셀값의 차이의 제곱만큼 더하기
			}
			out += tmp;
			count++;
		}
	}
	out /= count;	//평균화
	return out;
}


void overlay(IplImage* s1, IplImage* s2, IplImage* dst, int u, int v)
{
	cvCopy(s1, dst);
	for (int y = 0; y < s1->height; y++) {
		for (int x = 0; x < s1->width; x++)
		{
			int x2 = x + u;
			int y2 = y + v;
			if (x2<0 || x2>s2->width - 1) continue;
			if (y2<0 || y2>s2->height - 1) continue;

			CvScalar f1 = cvGet2D(s1, y, x);
			CvScalar f2 = cvGet2D(s2, y2, x2);
			CvScalar g;
			for (int k = 0; k < 3; k++)
				g.val[k] = (f1.val[k] + f2.val[k]) / 2;
			cvSet2D(dst, y, x, g);
		}
	}
}
