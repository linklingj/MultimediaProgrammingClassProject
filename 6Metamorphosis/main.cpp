#include <opencv2/opencv.hpp>

IplImage* src1;
IplImage* src2;
IplImage* tmp1;
IplImage* tmp2;
IplImage* mid1;
IplImage* mid2;
IplImage* dst;
int numLine1 = 0;
int numLine2 = 0;
CvPoint P1[100], Q1[100], P2[100], Q2[100];

CvPoint getMorphPoint(int x1, int y1,
	CvPoint P1, CvPoint Q1, CvPoint P2, CvPoint Q2,
	float* pW)
{
	// u:		 P1Q1 길이비율
	// v:	     실제 떨어져있는 픽셀거리

	float PQx = (Q1.x - P1.x);
	float PQy = (Q1.y - P1.y);
	float PXx = (x1 - P1.x);
	float PXy = (y1 - P1.y);

	float lenPQ = sqrt(PQx * PQx + PQy * PQy);
	float dot1 = PQx * PXx + PQy * PXy;

	float perPQx = -PQy;
	float perPQy = PQx;
	float dot2 = perPQx * PXx + perPQy * PXy;

	float u = dot1 / (lenPQ * lenPQ);
	float v = dot2 / lenPQ;

	float PQ2x = Q2.x - P2.x;
	float PQ2y = Q2.y - P2.y;
	float perPQ2x = -PQ2y;
	float perPQ2y = PQ2x;
	float lenPQ2 = sqrt(PQ2x * PQ2x + PQ2y * PQ2y);

	float x2 = P2.x + u * PQ2x + v * perPQ2x / lenPQ2;
	float y2 = P2.y + u * PQ2y + v * perPQ2y / lenPQ2;

	// w = (len) / (0.1 + dist)

	float dist = abs(v);
	if (u > 1) dist = sqrt((Q2.x - x2) * (Q2.x - x2) + (Q2.y - y2) * (Q2.y - y2));
	if (u < 0) dist = sqrt((P2.x - x2) * (P2.x - x2) + (P2.y - y2) * (P2.y - y2));
	*pW = pow((lenPQ2 / (0.1 + dist * dist)), 0.5);
	return cvPoint(x2, y2);
}

void doMorphing2(IplImage* src, IplImage* dst,
	CvPoint P1[], CvPoint Q1[], CvPoint P2[], CvPoint Q2[], int n)
{
	cvSet(dst, cvScalar(0, 0, 0));
	for (int y2 = 0; y2 < dst->height; y2++)
		for (int x2 = 0; x2 < dst->width; x2++)
		{
			CvPoint X1[100];
			float w[100];
			for (int i = 0; i < n; i++)
				X1[i] = getMorphPoint(x2, y2, P2[i], Q2[i], P1[i], Q1[i],
					&w[i]);

			float x1 = 0;
			float y1 = 0;
			float sumW = 0;
			for (int i = 0; i < n; i++)
			{
				x1 += w[i] * X1[i].x;
				y1 += w[i] * X1[i].y;
				sumW += w[i];
			}
			x1 /= sumW;
			y1 /= sumW;


			if (x1<0 || x1>src->width - 1) continue;
			if (y1<0 || y1>src->height - 1) continue;

			CvScalar c = cvGet2D(src, y1, x1);
			cvSet2D(dst, y2, x2, c);
		}
}


/*
void doMorphing(IplImage * src, IplImage * dst,
	CvPoint P1, CvPoint Q1, CvPoint P2, CvPoint Q2)
{
	cvSet(dst, cvScalar(0, 0, 0));
	for(int y2 = 0; y2<dst->height; y2++)
		for (int x2 = 0; x2 < dst->width; x2++)
		{
			CvPoint X1 = getMorphPoint(x2, y2, P2, Q2, P1, Q1);
			int x1 = X1.x;
			int y1 = X1.y;

			if (x1<0 || x1>src->width - 1) continue;
			if (y1<0 || y1>src->height - 1) continue;

			CvScalar c = cvGet2D(src, y1, x1);
			cvSet2D(dst, y2, x2, c);
		}
}
*/



void myMouse1(int event, int x, int y, int flags, void*) {
	if (event == CV_EVENT_LBUTTONDOWN)
		P1[numLine1] = cvPoint(x, y);
	if (event == CV_EVENT_LBUTTONUP)
	{
		Q1[numLine1] = cvPoint(x, y);
		numLine1++;

		cvCopy(src1, tmp1);
		for (int i = 0; i < numLine1; i++)
			cvLine(tmp1, P1[i], Q1[i], cvScalar(0, 0, 255), 3);

		cvShowImage("src1", tmp1);
	}
}
void myMouse2(int event, int x, int y, int flags, void*) {
	if (event == CV_EVENT_LBUTTONDOWN)
		P2[numLine2] = cvPoint(x, y);
	if (event == CV_EVENT_LBUTTONUP)
	{
		Q2[numLine2] = cvPoint(x, y);
		numLine2++;

		//		doMorphing(src, dst, P1, Q1, P2, Q2);

		cvCopy(src2, tmp2);
		for (int i = 0; i < numLine2; i++)
			cvLine(tmp2, P2[i], Q2[i], cvScalar(255, 0, 0), 3);
		cvShowImage("src2", tmp2);
	}

}

int main()
{
	src1 = cvLoadImage("c:\\Temp\\1.jpg");
	tmp1 = cvCreateImage(cvGetSize(src1), 8, 3);
	mid1 = cvCreateImage(cvGetSize(src1), 8, 3);

	src2 = cvLoadImage("c:\\Temp\\3.jpg");
	tmp2 = cvCreateImage(cvGetSize(src2), 8, 3);
	mid2 = cvCreateImage(cvGetSize(src2), 8, 3);

	dst = cvCreateImage(cvGetSize(src1), 8, 3);


	cvShowImage("src1", src1);
	cvShowImage("src2", src2);
	cvSetMouseCallback("src1", myMouse1);
	cvSetMouseCallback("src2", myMouse2);
	while (true) {
		int key = cvWaitKey();
		if (key == ' ' && numLine1 == numLine2) {
			break;
		}
	}

	for (float t = 0.0f; t <= 1.0f; t += 0.05f) {
		CvPoint P[100], Q[100];
		int num = numLine1;

		for (int i = 0; i < num; i++) {
			P[i].x = (1 - t) * P1[i].x + t * P2[i].x;
			P[i].y = (1 - t) * P1[i].y + t * P2[i].y;
			Q[i].x = (1 - t) * Q1[i].x + t * Q2[i].x;
			Q[i].y = (1 - t) * Q1[i].y + t * Q2[i].y;
		}

		doMorphing2(src1, mid1, P1, Q1, P, Q, num);
		doMorphing2(src2, mid2, P2, Q2, P, Q, num);

		for (int y = 0; y < dst->height; y++) {
			for (int x = 0; x < dst->width; x++) {
				if (x > mid1->width - 1) continue;
				if (x > mid2->width - 1) continue;
				if (y > mid1->height - 1) continue;
				if (y > mid2->height - 1) continue;
				CvScalar f1 = cvGet2D(mid1, y, x);
				CvScalar f2 = cvGet2D(mid2, y, x);
				CvScalar g;
				g.val[0] = (1 - t) * f1.val[0] + t * f2.val[0];
				g.val[1] = (1 - t) * f1.val[1] + t * f2.val[1];
				g.val[2] = (1 - t) * f1.val[2] + t * f2.val[2];
				cvSet2D(dst, y, x, g);
			}
		}
		cvShowImage("dst", dst);
		printf("time = %f\n", t);
		cvWaitKey(10);
	}
	cvWaitKey();


	/*if (numLine1 == numLine2) {
		doMorphing2(src, dst, P1, Q1, P2, Q2, numLine1);
		cvShowImage("dst", dst);
		cvWaitKey();
	}*/
	return 0;
}