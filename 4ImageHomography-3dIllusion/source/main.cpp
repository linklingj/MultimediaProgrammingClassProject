#include <opencv2/opencv.hpp>
#include "MatrixInverse.h"
#include "vec.h"
#include "mat.h"

using namespace std;

IplImage* dst = nullptr;
IplImage* src;
int W = 500;										// 결과 이미지 가로 크기
int H = 500;										// 결과 이미지 세로 크기

vec3 pos[8] = {										// 육면체를 구성하는 8개의 꼭지점 3차원 좌표
		vec3(-0.5, -0.5,  0.5),
		vec3(-0.5,  0.5,  0.5),
		vec3(0.5,  0.5,  0.5),
		vec3(0.5, -0.5,  0.5),
		vec3(-0.5, -0.5, -0.5),
		vec3(-0.5,  0.5, -0.5),
		vec3(0.5,  0.5, -0.5),
		vec3(0.5, -0.5, -0.5) };



struct rect											// 사각형 한 면
{
	int ind[4];										// 꼭지점의 인덱스
	vec3 pos[4];									// 꼭지점의 화면 방향으로의 3차원 위치
	vec3 nor;										// 법선(normal) 벡터 방향 (= 면이 향하는 방향)
};

rect setRect(int a, int b, int c, int d)			// 사각형 정보를 채워주는 함수(바로 아래 cube정의에 사용)
{
	rect r;
	r.ind[0] = a;
	r.ind[1] = b;
	r.ind[2] = c;
	r.ind[3] = d;
	return r;
}

rect cube[6] = { setRect(1, 0, 3, 2),				// 사각형 6개를 정의해 육면체를 구성
				 setRect(2, 3, 7, 6),
				 setRect(3, 0, 4, 7),
				 setRect(6, 5, 1, 2),
				 setRect(6, 7, 4, 5),
				 setRect(5, 4, 0, 1) };

vec3 epos = vec3(1.5, 1.5, 1.5);					// 카메라(시점의 3차원) 위치
mat4 ModelMat;										// 모델에 변형을 주는 변형 행렬
mat4 ViewMat;										// 카메라 시점을 맞춰주는 변형 행렬
mat4 ProjMat;										// 화면상 위치로 투영해주는 변형 행렬

void init()											// 초기화
{	
	ModelMat = mat4(1.0f);
	ViewMat = LookAt(epos, vec3(0, 0, 0), vec3(0, 1, 0));  
													// 카메라 위치(epos)에서 (0,0,0)을 바라보는 카메라 설정			
	ProjMat = Perspective(45, W / (float)H, 0.1, 100);	
													// 45도의 시야각을 가진 투영 변환 (가시거리 0.1~100)
}

void rotateModel(float rx, float ry, float rz)		// 육면체 모델에 회전을 적용하는 함수
{
	ModelMat = RotateX(rx) * RotateY(ry) * RotateZ(rz) * ModelMat;
}

vec3 convert3Dto2D(vec3 in)							// 3차원 좌표를 화면에 투영된 2차원+깊이값(z) 좌표로 변환
{
	vec4 p = ProjMat * ViewMat * ModelMat * vec4(in);
	p.x /= p.w;
	p.y /= p.w;
	p.z /= p.w;
	p.x = (p.x + 1) / 2.0f * W;
	p.y = (-p.y + 1) / 2.0f * H;
	return vec3(p.x, p.y, p.z);
}

void updatePosAndNormal(rect* r, vec3 p[])			// 육면체의 회전에 따른 각 면의 3차원 좌표 및 법선 벡터 방향 업데이트
{
	for (int i = 0; i < 4; i++)
		r->pos[i] = convert3Dto2D(p[r->ind[i]]);
	vec3 a = normalize(r->pos[0] - r->pos[1]);
	vec3 b = normalize(r->pos[2] - r->pos[1]);
	r->nor = cross(a, b);
}

// 역변환하는 함수
void applyInverseTransform(float IM[][3], IplImage* src, IplImage* dst) {
	for (float y2 = 0; y2 < dst->height; y2++)
		for (float x2 = 0; x2 < dst->width; x2++)
		{
			float w2 = 1.0f;

			float x1 = IM[0][0] * x2 + IM[0][1] * y2 + IM[0][2] * w2;
			float y1 = IM[1][0] * x2 + IM[1][1] * y2 + IM[1][2] * w2;
			float w1 = IM[2][0] * x2 + IM[2][1] * y2 + IM[2][2] * w2;
			x1 /= w1;
			y1 /= w1;

			if (x1<0 || x1>W - 1) continue;
			if (y1<0 || y1>H - 1) continue;

			CvScalar c = cvGet2D(src, y1, x1);
			cvSet2D(dst, y2, x2, c);
		}
}

//
//!과제 부분!
//
//점 4개를 인자로 받아 Homography를 통해 변형된 이미지를 채우는 함수
void drawSurface(vec3 pos[4]) {
	//이미지의 4꼭지점 좌표 (원본 평면)
	float x[4] = { 0,W,W,0 };
	float y[4] = { 0,0,H,H };

	//8*8 행렬 M과 그 역행렬 IM
	float M[8][8];
	float IM[8][8];

	//x` = x / w2, y` = y / w2
	//방정식을 통해 M 행렬을 구한다
	for (int i = 0; i < 8; i++) {
		float xc = x[i / 2];		//원본 평면의 x좌표
		float yc = y[i / 2];		//원본 평면의 y좌표
		float xp = pos[i / 2].x;	//대상 평면의 대응점 x`
		float yp = pos[i / 2].y;	//대상 평면의 대응점 y`

		//짝수 행은 x` 관련 홀수 행은 y` 관련
		M[i][0] = (i % 2) ? 0 : xc;
		M[i][1] = (i % 2) ? 0 : yc;
		M[i][2] = (i % 2) ? 0 : 1;
		M[i][3] = (i % 2) ? xc : 0;
		M[i][4] = (i % 2) ? yc : 0;
		M[i][5] = (i % 2) ? 1 : 0;
		M[i][6] = (i % 2) ? -yp * xc : -xp * xc;
		M[i][7] = (i % 2) ? -yp * yc : -xp * yc;
	}

	// b = [x`1, y`1, x`2, y`2, x`3, y`3, x`4, y`4] T
	// M * h` = b
	// h` = IM * b
	float b[8] = { pos[0].x, pos[0].y, pos[1].x, pos[1].y, pos[2].x, pos[2].y, pos[3].x, pos[3].y};
	float h[8];

	//M의 역행렬 IM을 구한다
	if (!InverseMatrixGJ8(M, IM)) {
		std::cout << "Error" << std::endl; return;
	}

	// h 벡터 계산
	for (int i = 0; i < 8; i++) {
		h[i] = 0;
		for (int j = 0; j < 8; j++)
			h[i] += IM[i][j] * b[j];
	}

	// h9 = 1을 넣어 3*3 행렬 H 완성
	float H[3][3] = {
	{h[0], h[1], h[2]},
	{h[3], h[4], h[5]},
	{h[6], h[7], 1}
	};

	float IH[3][3];
	InverseMatrixGJ3(H, IH);

	//이미지에 역변환 적용
	applyInverseTransform(IH, src, dst);
}

void drawImage()									// 그림을 그린다 (각 면의 테두리를 직선으로 그림)
{
	cvSet(dst, cvScalar(0, 0, 0));
	for (int i = 0; i < 6; i++)
	{
		updatePosAndNormal(&cube[i], pos);
		if (cube[i].nor.z < 0) continue;			// 보이지 않는 사각형을 제외, 보이는 사각형만 그린다	

		drawSurface(cube[i].pos);		//면에 이미지 채우기

		for (int j = 0; j < 4; j++)
		{
			vec3 p1 = cube[i].pos[j];
			vec3 p2 = cube[i].pos[(j + 1) % 4];
			cvLine(dst, cvPoint(p1.x, p1.y), cvPoint(p2.x, p2.y), cvScalar(255,255,255), 3);
		}
	}
	cvShowImage("3D view", dst);
}

void myMouse(int event, int x, int y, int flags, void*)
{
	static CvPoint prev = cvPoint(0, 0);
	if (event == CV_EVENT_LBUTTONDOWN)
		prev = cvPoint(x, y);
	if (event == CV_EVENT_MOUSEMOVE && (flags & CV_EVENT_FLAG_LBUTTON) == CV_EVENT_FLAG_LBUTTON)
	{
		int dx = x - prev.x;
		int dy = y - prev.y;
		rotateModel(dy, dx, -dy);					// 마우스 조작에 따라 모델을 회전함
		drawImage();
		prev = cvPoint(x, y);
	}
}

int main()
{
	char path[1000] = "C:\\Temp\\lena.jpg";

	//src 이미지 경로 입력
	cout << "============================================" << endl;
	cout << "Deparment of Software, Sejong University" << endl;
	cout << "Multimedia Programing Homework #5" << endl;
	cout << "Image Warping - Homography Practice" << endl;
	cout << "============================================" << endl;
	cout << "Input File Path: ";
	cin >> path;

	src = cvLoadImage(path);
	if (src == nullptr) {
		cout << "Error: cannot open " << path << endl;
		return 0;
	}

	dst = cvCreateImage(cvSize(W, H), 8, 3);
	init();

	while (true)
	{
		rotateModel(0, 1, 0);
		drawImage();
		cvSetMouseCallback("3D view", myMouse);
		int key = cvWaitKey(1);
		if (key == ' ') key = cvWaitKey();
	}

	return 0;
}