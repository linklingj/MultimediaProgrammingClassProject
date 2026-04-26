#include<opencv2/opencv.hpp>

inline float diff2(CvScalar f, CvScalar g) {
	return sqrt((f.val[0] - g.val[0]) * (f.val[0] - g.val[0]) + (f.val[1] - g.val[1]) * (f.val[1] - g.val[1]) + (f.val[2] - g.val[2]) * (f.val[2] - g.val[2]));
}

float getError(IplImage* src, int sx, int sy, IplImage *dst, int tx, int ty, int K, int **mask) {
	float err = 0;
	int count = 0;
	for (int v = -K; v <= K; v++) {
		for (int u = -K; u <= K; u++) {
			int ssx = sx + u;
			int ssy = sy + v;
			int ttx = tx + u;
			int tty = ty + v;
			if (ttx < 0 || ttx > dst->width - 1) continue;
			if (tty < 0 || tty > dst->height - 1) continue;
			if (mask[tty][ttx] == 0) continue;
			CvScalar f = cvGet2D(src, ssy, ssx);
			CvScalar g = cvGet2D(dst, tty, ttx);
			err += diff2(f, g);
			count++;
		}
	}
	return err/count;
}

CvScalar findBestMatchColor(IplImage* src, IplImage* dst, int tx, int ty, int** mask) {
	const int K = 2;
	float min_err = FLT_MAX;
	int min_sx = -1;
	int min_sy = -1;
	for (int sy = K; sy < src->height-K; sy++) {
		for (int sx = K; sx < src->width-K; sx++) {
			float err = getError(src,sx,sy,dst,tx,ty, K, mask);
			if (min_err > err) {
				min_err = err;
				min_sx = sx;
				min_sy = sy;
			}
		}
	}

	return cvGet2D(src, min_sy, min_sx);
}

int main() {
	IplImage* src = cvLoadImage("c:\\Temp\\161.bmp");
	CvSize size = cvSize(src->width * 2, src->height * 2);
	IplImage* dst = cvCreateImage(size, 8, 3);
	cvSet(dst, cvScalar(0, 0, 0));

	int** mask = new int*[size.height];
	for (int y = 0; y < size.height; y++)
		mask[y] = new int[size.width];
	for (int y = 0; y < size.height; y++)
		for (int x = 0; x < size.width; x++)
			mask[y][x] = 0;

	
	for (int y = 0; y < src->height; y++) {
		for (int x = 0; x < src->width; x++) {
			CvScalar f = cvGet2D(src, y, x);
			cvSet2D(dst, y, x, f);
			mask[y][x] = 1;
		}
	}
	
	for (int x = src->width; x < dst->width; x++) {
		for (int y = 0; y < src->height; y++) {
			CvScalar c = findBestMatchColor(src, dst, x, y, mask);
			cvSet2D(dst, y, x, c);
			mask[y][x] = 1;
			cvShowImage("dst", dst);
			cvWaitKey(1);
		}
	}
	for (int y = src->height; y < dst->height; y++) {
		for (int x = 0; x < dst->width; x++) {
			CvScalar c = findBestMatchColor(src, dst, x, y, mask);
			cvSet2D(dst, y, x, c);
			mask[y][x] = 1;
			cvShowImage("dst", dst);
			cvWaitKey(1);
		}
	}

	cvShowImage("src", src);
	cvShowImage("dst", dst);
	cvSaveImage("c:\\Temp\\6-1.png", src);
	cvSaveImage("c:\\Temp\\6-2.png",dst);
	cvWaitKey();

	for(int i = 0; i < size.height; i++)
		delete[] mask[i];
	delete[] mask;

	return 0;
}