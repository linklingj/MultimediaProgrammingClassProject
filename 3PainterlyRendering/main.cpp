#include<iostream>
#include<opencv2/opencv.hpp>
#include<random>

using namespace std;

enum style{CIRCLE, STROKE};

const int brushCount = 5;									//브러쉬 종류 수
const int brushSize[brushCount] = { 30, 15, 7, 4, 2 };		//브러쉬 크기
const int gridSize[] = { 35, 20, 10, 6, 4 };				//격자의 크기
const int threshold = 40;									//점을 찍기 위한 최소 픽셀차
const int minStrokeLength = 4;								//stroke 최소 길이
const int maxStrokeLength = 20;								//stroke 최대 길이
const float smooth = 0.9f;									//부드러운 stroke 회전을 위한 계수, 낮을수록 부드러움 (0~1)

class Brush;
class LineBrush;
void Paint(IplImage* src, IplImage* dst, style style);
void PaintLayer(IplImage* dst, IplImage* ref, int index, style style);
inline float GetDiff(CvScalar f, CvScalar g) { return sqrt((f.val[0] - g.val[0]) * (f.val[0] - g.val[0]) + (f.val[1] - g.val[1]) * (f.val[1] - g.val[1]) + (f.val[2] - g.val[2]) * (f.val[2] - g.val[2])); }
inline float GetDiff2(CvScalar f, CvScalar g) { return (f.val[0] - g.val[0] + f.val[1] - g.val[1] + f.val[2] - g.val[2]) / 3.0f; }
void ShuffleBrushes(Brush** strokes, int cnt);
void MakeSplineStroke(LineBrush* brush, IplImage* ref, IplImage* dst);

//붓질을 저장하는 클래스
class Brush {
public:
	CvPoint pos;	//(초기)위치
	int size;		//붓 크기
	CvScalar color;	//색
	Brush(CvPoint pos, int size, CvScalar color) {
		this->pos = pos;
		this->size = size;
		this->color = color;
	}
	//점을 찍는 메서드
	virtual void Draw(IplImage *dst) {
		cvCircle(dst, pos, size, color, -1);
	}
};

//선형 붓질을 저장하는 클래스
class LineBrush : public Brush {
public:
	int len;							//점의 개수
	CvPoint points[maxStrokeLength];	//spline 점
	LineBrush(CvPoint pos, int size, CvScalar color) : Brush(pos, size, color), len(1) {}
	void setPoint(CvPoint point) {
		if (len >= maxStrokeLength) return;
		points[len] = point;
		len++;
	}
	//선을 그리는 메서드
	void Draw(IplImage* dst) override {
		if (len == 1) Brush::Draw(dst);
		CvPoint prev = pos;
		for (int i = 1; i < len; i++) {
			cvLine(dst, prev, points[i], color, size);
			prev = points[i];
		}
	}
};

int main() {
	char path[1000] = "C:\\Temp\\lena.jpg";	//기본 경로
	int mode = 1;							//모드(0:원형, 1:선)

	cout << "============================================" << endl;
	cout << "Deparment of Software, Sejong University" << endl;
	cout << "Multimedia Programing Homework #4" << endl;
	cout << "Paintery Rendering" << endl;
	cout << "============================================" << endl;
	cout << "Input File Path: ";
	cin >> path;
	cout << "Select Drawing Mode (0=circle, 1=stroke):";
	cin >> mode;


	IplImage* src = cvLoadImage(path);
	if (src == nullptr) {
		cout << "Error: cannot open " << path << endl;
		return 0;
	}
	IplImage* dst = cvCreateImage(cvGetSize(src), 8, 3);

	//칠하기
	Paint(src, dst, (style)mode);

	cvShowImage("src", src);
	cvShowImage("canvas", dst);
	cvWaitKey();

	cvSaveImage("c:\\Temp\\test.png", dst);

	return 0;
}

//색칠 함수
//각각의 붓의 크기에 따라 큰 붓부터 작은 붓까지 layer를 칠한다
//붓의 크기에 비례하게 gaussian blur 한 이미지를 레퍼런스로 칠한다
void Paint(IplImage* src, IplImage* dst, style style) {
	cvSet(dst, cvScalar(255, 255, 255));
	for (int i = 0; i < brushCount; i++) {
		IplImage *ref = cvCreateImage(cvGetSize(src), 8, 3);
		int kernal = gridSize[i];
		if (kernal % 2 == 0) kernal++;
		cvSmooth(src, ref, CV_GAUSSIAN, kernal);
		cvShowImage("canvas", dst);
		cvWaitKey(1);
		PaintLayer(dst, ref, i, style);
		cvReleaseImage(&ref);
	}
}

//레이어를 칠하는 함수
//그리드 내에서 칠할 위치를 찾고 붓질을 저장한후 랜덤하게 섞고 칠한다
void PaintLayer(IplImage* dst, IplImage* ref, int index, style style) {
	int total = (ref->height / gridSize[index] + 1) * (ref->width / gridSize[index] + 1);	//예상 붓질 횟수
	Brush** strokes = new Brush*[total];	//붓질을 저장할 배열을 동적할당한다

	//모든 grid 칸에 대해 각 칸에서 원본과 가장 차이가 큰 위치를 찾는다
	int cnt = 0;
	for (int y = 0; y < ref->height; y+= gridSize[index]) {
		for (int x = 0; x < ref->width; x+= gridSize[index]) {
			float max_diff = 0;
			int py = 0, px = 0;
			for (int v = 0; v < gridSize[index]; v++) {
				for (int u = 0; u < gridSize[index]; u++) {
					int ny = y + v;
					int nx = x + u;
					if (ny > ref->height - 1 || nx > ref->width - 1) continue;
					CvScalar f = cvGet2D(ref, ny, nx);
					CvScalar g = cvGet2D(dst, ny, nx);
					float diff = GetDiff(f, g);
					if (diff > max_diff) {
						max_diff = diff;
						py = ny;
						px = nx;
					}
				}
			}
			if (max_diff < threshold) continue;	//threshold보다 차이가 크거나 같은 경우에만 진행
			
			//차이가 최대가 되는 위치에서 해당 색으로 붓질을 추가, 선형 모드일 경우 선을 찾는 함수를 호출한다
			CvScalar c = cvGet2D(ref, py, px);
			if (style == CIRCLE)
				strokes[cnt++] = new Brush(cvPoint(px,py), brushSize[index], c);
			if (style == STROKE) {
				LineBrush *b = new LineBrush(cvPoint(px, py), brushSize[index], c);
				MakeSplineStroke(b, ref, dst);
				strokes[cnt++] = dynamic_cast<Brush*>(b);
			}
		}
	}

	ShuffleBrushes(strokes, cnt);

	//각각의 붓질을 실제로 그리기
	for (int i = 0; i < cnt; i++)
		strokes[i]->Draw(dst);

	for (int i = 0; i < cnt; i++)
		delete strokes[i];
	delete[] strokes;
}

//붓질의 순서를 랜덤으로 섞는 함수
void ShuffleBrushes(Brush** strokes, int cnt) {
	random_device rd;
	mt19937 gen(rd());

	for (int i = cnt - 1; i > 0; --i) {
		uniform_int_distribution<> dis(0, i);
		int j = dis(gen);
		swap(strokes[i], strokes[j]);
	}
}

//붓질의 경로를(spline) 찾는 함수
void MakeSplineStroke(LineBrush *brush, IplImage *ref, IplImage *dst) {
	int x = brush->pos.x;
	int y = brush->pos.y;
	int lineLength = brush->size;	//한 점에서 다음 점까지의 길이, 일단은 브러쉬 사이즈로 설정한다
	float lastDx = 0, lastDy = 0;
	
	for (int i = 1; i <= maxStrokeLength; i++) {
		if (x < 1 || x > ref->width - 2 || y < 1 || y > ref->height - 2) break;
		CvScalar f = cvGet2D(ref, y, x);
		//레퍼런스와 캔버스와의 차이가 붓 색과의 차이보다 적을 경우 중단한다
		if (i > minStrokeLength && abs(GetDiff2(f, cvGet2D(dst,y,x))) < abs(GetDiff2(f, brush->color))) break;

		//gradient의 방향을 구한다. x방향 벡터는 x+1과 x-1의 차로, y방향 벡터는 y+1과 y-1의 차로 정의한다
		float gx = GetDiff2(cvGet2D(ref, y, x + 1), cvGet2D(ref, y, x - 1));
		float gy = GetDiff2(cvGet2D(ref, y + 1, x), cvGet2D(ref, y - 1, x));
		if (sqrt(gx * gx + gy * gy) < 0.01f) break;

		//법선 벡터의 방향을 구한다
		float dx = -gy;
		float dy = gx;

		//전 붓질의 방향과 반대로 가는 경우 벡터를 180도 돌린다
		if (lastDx * dx + lastDy * dy < 0) {
			dx *= -1;
			dy *= -1;
		}

		//normalize하여 벡터의 크기를 일정하게 한다
		float ddx = dx/ sqrt(dx * dx + dy * dy);
		float ddy = dy/ sqrt(dx * dx + dy * dy);

		//급하게 꺾이지 않도록 보간한다
		ddx = smooth * ddx + (1 - smooth) * lastDx;
		ddy = smooth * ddy + (1 - smooth) * lastDy;

		//벡터 방향으로 이동하여 경로를 저장한다
		x = x + ddx * lineLength;
		y = y + ddy * lineLength;
		lastDx = ddx;
		lastDy = ddy;
		brush->setPoint(cvPoint(x,y));
	}
}