/* Ellipse detection related tools */

/*
This code is intended for academic use only.
You are free to use and modify the code, at your own risk.

If you use this code, or find it useful, please refer to the paper:


The comments in the code refer to the abovementioned paper.
If you need further details about the code or the algorithm, please contact me at:

lianbosong@turingai.cc

last update:
*/

#include "tools.h"

Point2f lineCrossPoint(Point2f l1p1,Point2f l1p2,Point2f l2p1,Point2f l2p2 )
{
	/* To avoid division by zero (0, 0) as intersection point, use 'invalid point' or 'positive infinity' to represent 1.#INF = 1/tmp(=0) */
	float tmp = 0;
	float positive_infinity = 1 / tmp;
	/*int positive_infinity = 0;*/

	Point2f crossPoint;
	float k1,k2,b1,b2;
	if (l1p1.x==l1p2.x&&l2p1.x==l2p2.x){						// Both lines are parallel to x-axis
		crossPoint=Point2f(positive_infinity, positive_infinity);//invalid point
		return crossPoint;
	}
	if (l1p1.x==l1p2.x)
	{
		crossPoint.x=l1p1.x;
		k2=(l2p2.y-l2p1.y)/(l2p2.x-l2p1.x);
		b2=l2p1.y-k2*l2p1.x;
		crossPoint.y=k2*crossPoint.x+b2;
		return crossPoint;
	}
	if (l2p1.x==l2p2.x)
	{
		crossPoint.x=l2p1.x;
		k2=(l1p2.y-l1p1.y)/(l1p2.x-l1p1.x);
		b2=l1p1.y-k2*l1p1.x;
		crossPoint.y=k2*crossPoint.x+b2;
		return crossPoint;
	}

	k1=(l1p2.y-l1p1.y)/(l1p2.x-l1p1.x);
	k2=(l2p2.y-l2p1.y)/(l2p2.x-l2p1.x);
	b1=l1p1.y-k1*l1p1.x;
	b2=l2p1.y-k2*l2p1.x;
	if (k1==k2)													// Slopes are equal, lines are parallel
	{
		crossPoint=Point2f(positive_infinity, positive_infinity);//invalid point
	}
	else
	{
		crossPoint.x=(b2-b1)/(k1-k2);
		crossPoint.y=k1*crossPoint.x+b1;
	}
	return crossPoint;
}

void point2Mat(Point2f p1,Point2f p2,float mat[2][2])
{
	mat[0][0]=p1.x;
	mat[0][1]=p1.y;
	mat[1][0]=p2.x;
	mat[1][1]=p2.y;
}
// Custom function to calculate determinant using general formula, A is a 3*3 matrix
float detResult(float matR[])
{
	float result = matR[0] * matR[4] * matR[8] + matR[1] * matR[5] * matR[6]
		+ matR[2] * matR[3] * matR[7] - matR[2] * matR[4] * matR[6]
		- matR[1] * matR[3] * matR[8] - matR[0] * matR[5] * matR[7];

	return result;
}
// Custom function to calculate the distance from point R1 to the line formed by R2+R3
float getDistance(Point2f R1, Point2f R2, Point2f R3) {
	// Line equation L:R2R3
	float A = 0, B = 0, C = 0;
	A = R3.y - R2.y;	// A = y2 - y1
	B = R2.x - R3.x;	// B = x1 - x2
	C = R3.x * R2.y - R2.x * R3.y;	// C = x2 * y1 - x1 * y2
	// Calculate the distance from point R1 to line L:R2R3 using the formula
	float distance = abs(A * R1.x + B * R1.y + C) / (sqrtf(A * A + B * B));

	return distance;
}
/**
 * Important modifications for ellipse detection: Changed the method for judging concentric circles from 'three points collinear' to 'invalid point detection'
 * Original: Calculate the ratio of corresponding values (b1~b6 / a1~a6), compare the ratio value with '1',
 *	If the ratio is within the tolerance range (threshold), it is considered as concentric circles, otherwise it is not (different) circles.
 *
 * New version: Calculate the mapping points R1~R3, judge whether R1~R3 are 'collinear', if so, they are concentric circles
 */
/**
 * The format of the function is:
 * V2SP = (Point2f p3,Point2f p2,Point2f p1,Point2f p4,Point2f p5,Point2f p6)
 *	(p3, p2, p1)-(p4, p5, p6)��La:<p1, p2>~~~Lb:<p3, p4>~~~Lc:<p5, p6>��
 * Mat is the OpenCV Mat object, which is used to store image information.
 *	When storing image information in C++, we use the Mat object, which is automatically managed by OpenCV.
 * OpenCV represents an image as a Mat object, which consists of:
 *	_Mat: type, which is represented by CV_32F3C, indicating 32-bit + float + 3 channels (BGR).
 */
float value4SixPoints( V2SP )
{
/* Original: Calculate the ratio of corresponding values (b1~b6 / a1~a6), compare the ratio value with '1',
 *	If the ratio is within the tolerance range (threshold), it is considered as concentric circles, otherwise it is not (different) circles.
 */
#if 0
	float result=1;
	Mat A,B,C;
	float matB[2][2],matC[2][2];

	// Calculate the intersection points of three lines abc (p1p2 determines line a), calculate the intersection points uvw (u=<a, c>)
	Point2f v,w,u;
	v=lineCrossPoint(p1,p2,p3,p4);
	w=lineCrossPoint(p5,p6,p3,p4);
	u=lineCrossPoint(p5,p6,p1,p2);

	float tmp = 0;
	float positive_infinity = 1 / tmp;	// Positive infinity 1.#INF
	/*float positive_infinity = 0;*/
	// Check if the intersection points of the three lines are invalid
	if (v.x == positive_infinity || w.x == positive_infinity || u.x == positive_infinity) {

		// cout << "result:" << result << endl;
		// cout << "test:" << (fabs(result) > 0.47f) << endl;
		// cout << "infinity\t";

		// Return a value that makes "|x| > tCNC threshold = false", so "NaN > tCNC = false"
		return 0;
	}

	// Store u,v coordinates in two-dimensional array matB, store p1,p2 coordinates in two-dimensional array matC
	point2Mat(u,v,matB);
	point2Mat(p1,p2,matC);
	// According to the definition of two-dimensional arrays, calculate BC, then BC(uv and p1p2), then A=C*(B^-1), get matrix A[a1 b1; a2 b2]
	B=Mat(2,2,CV_32F,matB);
	C=Mat(2,2,CV_32F,matC);
	A=C*B.inv();
	// Calculate ratio: b1* b2 / a1 * a2
	/* TODO: The original code used a1*a2 / b1*b2, the theoretical ratio should be 1, but the actual result should not be wrong, why using b/a formula reduces performance? */
	result*=A.at<float>(0,0)*A.at<float>(1,0)/(A.at<float>(0,1)*A.at<float>(1,1));
	/*result *= A.at<float>(0, 1) * A.at<float>(1, 1) / (A.at<float>(0, 0) * A.at<float>(1, 0));*/

	/*cout<<"u:\t"<<u<<endl;
	cout<<"v:\t"<<v<<endl;
	cout<<"B:\t"<<B<<endl;
	cout<<"C:\t"<<C<<endl;
	cout<<"A:\t"<<A<<endl;
	cout<<"result:\t"<<result<<endl;*/

	/* Similarly calculate b3*b4 / a3*a4 */
	point2Mat(v,w,matB);
	point2Mat(p3,p4,matC);
	B=Mat(2,2,CV_32F,matB);
	C=Mat(2,2,CV_32F,matC);
	A=C*B.inv();
	result*=A.at<float>(0,0)*A.at<float>(1,0)/(A.at<float>(0,1)*A.at<float>(1,1));

	/* Similarly calculate b5*b6 / a5*a6 */
	point2Mat(w,u,matB);
	point2Mat(p5,p6,matC);
	B=Mat(2,2,CV_32F,matB);
	C=Mat(2,2,CV_32F,matC);
	A=C*B.inv();
	result*=A.at<float>(0,0)*A.at<float>(1,0)/(A.at<float>(0,1)*A.at<float>(1,1));

	// Accumulate to get the final CNC: b1*b2 * b3*b4 * b5*b6 / a1*a2 * a3*a4 * a5*a6 = 1
	return result - 1;	// Subtract 1 from CNC, then use absolute value and negative value comparison
#endif // 0

/* Method a: mapping point determinant = 1/2 * determinant = 0 ~ judge mapping points collinear ~ judge concentric circles */
#if 0
	// Calculate the mapping points R1, R2, R3 formed by the opposite sides of the hexagon formed by p1~p6 (three mapping points)
	Point2f R1, R2, R3;
	R1 = lineCrossPoint(p1, p2, p4, p5);
	R2 = lineCrossPoint(p2, p3, p5, p6);
	R3 = lineCrossPoint(p3, p4, p6, p1);

	float tmp = 0;
	float positive_infinity = 1 / tmp;	// Positive infinity 1.#INF
	/*float positive_infinity = 0;*/
	// Check if R points are invalid (parallel) - opposite sides are parallel, if R is infinite far point, then they are concentric circles
	if (R1.x == positive_infinity || R2.x == positive_infinity || R3.x == positive_infinity) {

		//Mat A = Mat(3, 3, CV_32F, matR);
		//cout << "A:" << A << endl;
		//cout << "det(A):" << result << endl;
		//cout << "test:" << (fabs(result) > 500.36f) << endl;
		//cout << "infinity\t";

		// Return a value that makes "|x| > tCNC threshold = false", so "result > tCNC = false"
		return 0;
	}

	// Calculate the determinant value det(A) formed by the three points R1, R2, R3
	float matR[] = {R1.x, R1.y, 1, R2.x, R2.y, 1, R3.x, R3.y, 1};
	// det(A) equals the area of triangle R1R2R3 multiplied by 2, if the area of triangle R1R2R3 is 0 ~ three points collinear
	float result = detResult(matR) * 0.5;

	// Return 1/2 determinant value = triangle area, if triangle area is 0, then return |result| and tCNC threshold comparison
	return result;
#endif // 0

/* Method b: mapping point ratio = -1 ~ judge mapping points collinear ~ judge concentric circles */
#if 0
	// Calculate the mapping points R1, R2, R3 formed by the opposite sides of the hexagon formed by p1~p6 (three mapping points)
	Point2f R1, R2, R3;
	R1 = lineCrossPoint(p1, p2, p4, p5);
	R2 = lineCrossPoint(p2, p3, p5, p6);
	R3 = lineCrossPoint(p3, p4, p6, p1);

	float tmp = 0;
	float positive_infinity = 1 / tmp;	// Positive infinity 1.#INF
	/*float positive_infinity = 0;*/
	// Check if R points are invalid (parallel) - opposite sides are parallel, if R is infinite far point, then they are concentric circles
	if (R1.x == positive_infinity || R2.x == positive_infinity || R3.x == positive_infinity) {

		//Mat A = Mat(3, 3, CV_32F, matR);
		//cout << "A:" << A << endl;
		//cout << "det(A):" << result << endl;
		//cout << "test:" << (fabs(result) > 500.36f) << endl;
		//cout << "infinity\t";

		// Return a value that makes "|x| > tCNC threshold = false", so "result > tCNC = false"
		return 0;
	}

	// Calculate the mapping point ratio CNL for the three mapping points R1, R2, R3, which should be -1, if collinear:
	float result = 1;
	Mat A, B, C;
	float matB[2][2], matC[1][2];

	Point2f v, w, u;
	v = lineCrossPoint(p1, p2, p3, p4);
	w = lineCrossPoint(p5, p6, p3, p4);
	u = lineCrossPoint(p5, p6, p1, p2);
	// Check if the intersection points of the three lines are invalid
	if (v.x == positive_infinity || w.x == positive_infinity || u.x == positive_infinity) {

		// cout << "result:" << result << endl;
		// cout << "test:" << (fabs(result) > 0.47f) << endl;
		// cout << "infinity\t";

		// Return a value that makes "|x| > tCNC threshold = false", so "NaN > tCNC = false"
		return 0;
	}

	// Store u,v coordinates in two-dimensional array matB, store R1 coordinates in one-dimensional array matC
	point2Mat(u, v, matB);
	matC[0][0] = R1.x;
	matC[0][1] = R1.y;
	// According to the definition of two-dimensional arrays, calculate BC, then BC(uv and R1), then A=C*(B^-1), get matrix A[a1 b1]
	B = Mat(2, 2, CV_32F, matB);
	C = Mat(1, 2, CV_32F, matC);
	A = C * B.inv();
	// Calculate ratio: b1 / a1, the original code used a1 / b1, the principle is the same, but the result is different, why?
	result *= A.at<float>(0, 0) / A.at<float>(0, 1);
	/*result *= A.at<float>(0, 1) / A.at<float>(0, 0);*/

	/* Similarly calculate b2 / a2 */
	point2Mat(u, w, matB);
	matC[0][0] = R2.x;
	matC[0][1] = R2.y;
	B = Mat(2, 2, CV_32F, matB);
	C = Mat(1, 2, CV_32F, matC);
	A = C * B.inv();
	result *= A.at<float>(0, 0) / A.at<float>(0, 1);

	/* Similarly calculate b3 / a3 */
	point2Mat(v, w, matB);
	matC[0][0] = R3.x;
	matC[0][1] = R3.y;
	B = Mat(2, 2, CV_32F, matB);
	C = Mat(1, 2, CV_32F, matC);
	A = C * B.inv();
	result *= A.at<float>(0, 0) / A.at<float>(0, 1);

	// Accumulate to get the final CNL: b1 * b2 * b3 / a1 * a2 * a3 = -1
	return result + 1;	// Add 1 to CNC, then use absolute value and negative value comparison
#endif // 0

/* Method c1+c2: point to triangle distance < Th + point to regression line distance < Th ~ judge mapping points collinear ~ judge concentric circles */
#if 1
	// Calculate the mapping points R1, R2, R3 formed by the opposite sides of the hexagon formed by p1~p6 (three mapping points)
	Point2f R1, R2, R3;
	R1 = lineCrossPoint(p1, p2, p4, p5);
	R2 = lineCrossPoint(p2, p3, p5, p6);
	R3 = lineCrossPoint(p3, p4, p6, p1);

	float tmp = 0;
	float positive_infinity = 1 / tmp;	// Positive infinity 1.#INF
	/*float positive_infinity = 0;*/
	// Check if R points are invalid (parallel) - opposite sides are parallel, if R is infinite far point, then they are concentric circles
	if (R1.x == positive_infinity || R2.x == positive_infinity || R3.x == positive_infinity) {

		//Mat A = Mat(3, 3, CV_32F, matR);
		//cout << "A:" << A << endl;
		//cout << "det(A):" << result << endl;
		//cout << "test:" << (fabs(result) > 500.36f) << endl;
		//cout << "infinity\t";

		// Return a value that makes "|x| > tCNC threshold = false", so "result > tCNC = false"
		return 0;
	}

	#if 0
		/* Method 1: Strictly calculate the distance from each point to the corresponding triangle side, take the maximum of the three corresponding distances, return and compare with threshold */
		float distanceR1 = getDistance(R1, R2, R3);
		float distanceR2 = getDistance(R2, R1, R3);
		float distanceR3 = getDistance(R3, R2, R1);
		float maxDistance = 0;
		if (distanceR1 > distanceR2) {
			maxDistance = distanceR1;
		}
		else {
			maxDistance = distanceR2;
		}
		if (maxDistance < distanceR3) {
			maxDistance = distanceR3;
		}

		//cout << "test:" << maxDistance << endl;
		return maxDistance;	// Return the maximum distance from point to line, and compare with threshold
	#endif // 0

	#if 1
		/* Method 2: Accurate least squares method to calculate the regression line, take the maximum distance to the regression line, return and compare with threshold */
		float sumRx = R1.x + R2.x + R3.x;
		float sumRxSquare = R1.x*R1.x + R2.x*R2.x + R3.x*R3.x;
		float sumRxRy = R1.x*R1.y + R2.x*R2.y + R3.x*R3.y;
		float sumRy = R1.y + R2.y + R3.y;

		// Error e^2 = sum(yi - (kxi+b))^2, take partial derivatives with respect to k and b, set = 0 (to minimize e), get the system of equations for k and b, solve the system to get the k and b values that minimize e^2, then calculate the regression line y = kx + b
		// The system can be written as matrix product form A * k_b = B, then k_b (k, b) = A.inv()*B
		float arryA[] = { sumRxSquare, sumRx, sumRx, 3 };
		Mat1f matA = Mat(2, 2, CV_32F, arryA);
		float arryB[] = { sumRxRy, sumRy };
		Mat1f matB = Mat(2, 1, CV_32F, arryB);
		Mat1f k_b = matA.inv() * matB;	// Get k and b to determine line L:y = kx + b

		// Calculate the distance from each R point to the regression line, take the maximum of the three distances
		float k = k_b.at<float>(0, 0);
		float b = k_b.at<float>(1, 0);
		float distanceR1 = abs(k * R1.x - R1.y + b) / (sqrtf(k * k + 1));
		float distanceR2 = abs(k * R2.x - R2.y + b) / (sqrtf(k * k + 1));
		float distanceR3 = abs(k * R3.x - R3.y + b) / (sqrtf(k * k + 1));
		float maxDistance = 0;
		if (distanceR1 > distanceR2) {
			maxDistance = distanceR1;
		}
		else {
			maxDistance = distanceR2;
		}
		if (maxDistance < distanceR3) {
			maxDistance = distanceR3;
		}

		//cout << "test:" << maxDistance << endl;
		return maxDistance;	// Return the maximum distance from point to regression line, and compare with threshold
	#endif // 0
#endif // 0
}
/* End of function */

void MultiImage_OneWin(const std::string& MultiShow_WinName, const vector<Mat>& SrcImg_V, CvSize SubPlot, CvSize ImgMax_Size)
{
	//Reference : http://blog.csdn.net/yangyangyang20092010/article/details/21740373

	//************* Usage *************//
	//vector<Mat> imgs(4);
	//imgs[0] = imread("F:\\SA2014.jpg");
	//imgs[1] = imread("F:\\SA2014.jpg");
	//imgs[2] = imread("F:\\SA2014.jpg");
	//imgs[3] = imread("F:\\SA2014.jpg");
	//MultiImage_OneWin("T", imgs, cvSize(2, 2), cvSize(400, 280));

	//Window's image
	Mat Disp_Img;
	//Width of source image
	CvSize Img_OrigSize = cvSize(SrcImg_V[0].cols, SrcImg_V[0].rows);
	//******************** Set the width for displayed image ********************//
	//Width vs height ratio of source image
	float WH_Ratio_Orig = Img_OrigSize.width/(float)Img_OrigSize.height;
	CvSize ImgDisp_Size = cvSize(100, 100);
	if(Img_OrigSize.width > ImgMax_Size.width)
		ImgDisp_Size = cvSize(ImgMax_Size.width, (int)(ImgMax_Size.width/WH_Ratio_Orig));
	else if(Img_OrigSize.height > ImgMax_Size.height)
		ImgDisp_Size = cvSize((int)(ImgMax_Size.height*WH_Ratio_Orig), ImgMax_Size.height);
	else
		ImgDisp_Size = cvSize(Img_OrigSize.width, Img_OrigSize.height);
	//******************** Check Image numbers with Subplot layout ********************//
	int Img_Num = (int)SrcImg_V.size();
	if(Img_Num > SubPlot.width * SubPlot.height)
	{
		cout<<"Your SubPlot Setting is too small !"<<endl;
		exit(0);
	}
	//******************** Blank setting ********************//
	CvSize DispBlank_Edge = cvSize(80, 60);
	CvSize DispBlank_Gap  = cvSize(15, 15);
	//******************** Size for Window ********************//
	Disp_Img.create(Size(ImgDisp_Size.width*SubPlot.width + DispBlank_Edge.width + (SubPlot.width - 1)*DispBlank_Gap.width,
		ImgDisp_Size.height*SubPlot.height + DispBlank_Edge.height + (SubPlot.height - 1)*DispBlank_Gap.height), CV_8UC3);
	Disp_Img.setTo(0);//Background
	//Left top position for each image
	int EdgeBlank_X = (Disp_Img.cols - (ImgDisp_Size.width*SubPlot.width + (SubPlot.width - 1)*DispBlank_Gap.width))/2;
	int EdgeBlank_Y = (Disp_Img.rows - (ImgDisp_Size.height*SubPlot.height + (SubPlot.height - 1)*DispBlank_Gap.height))/2;
	CvPoint LT_BasePos = cvPoint(EdgeBlank_X, EdgeBlank_Y);
	CvPoint LT_Pos = LT_BasePos;

	//Display all images
	for (int i=0; i < Img_Num; i++)
	{
		//Obtain the left top position
		if ((i%SubPlot.width == 0) && (LT_Pos.x != LT_BasePos.x))
		{
			LT_Pos.x = LT_BasePos.x;
			LT_Pos.y += (DispBlank_Gap.height + ImgDisp_Size.height);
		}
		//Writting each to Window's Image
		Mat imgROI = Disp_Img(Rect(LT_Pos.x, LT_Pos.y, ImgDisp_Size.width, ImgDisp_Size.height));
		resize(SrcImg_V[i], imgROI, Size(ImgDisp_Size.width, ImgDisp_Size.height));

		LT_Pos.x += (DispBlank_Gap.width + ImgDisp_Size.width);
	}

	//Get the screen size of computer  #include<windows.h> #include <WinUser.h>
	int Scree_W = 1366;//GetSystemMetrics(SM_CXSCREEN);
	int Scree_H = 768;//GetSystemMetrics(SM_CYSCREEN);
	//cout<<Scree_W<<"\t"<<Scree_H<<endl;
	namedWindow(MultiShow_WinName, WINDOW_NORMAL);
	moveWindow(MultiShow_WinName,(Scree_W - Disp_Img.cols)/2 ,(Scree_H - Disp_Img.rows)/2);//Centralize the window
	imshow(MultiShow_WinName, Disp_Img);
	// waitKey(0);  // Commented out to avoid waiting for user input
	destroyWindow(MultiShow_WinName);
}


void PyrDown(string picName)
{
	Mat img1=imread(picName);
	Mat img2;
	Size sz;
	//pyrDown(img1,img2,sz,BORDER_DEFAULT);
	pyrUp(img1,img2,sz,BORDER_DEFAULT);
	pyrUp(img2,img2,sz,BORDER_DEFAULT);
	namedWindow("WindowOrg");
	namedWindow("WindowNew");
	imshow("WindowOrg",img1);
	imshow("WindowNew",img2);

	// waitKey(10000);  // Commented out to avoid waiting for user input
}
Mat matResize(Mat src,double scale){
	Mat img2;
	bool showtimeandpic=false;
	if(!showtimeandpic){
		Size dsize = Size(int(src.cols*scale),int(src.rows*scale));
		img2 = Mat(dsize,CV_32S);
		resize(src, img2,dsize,INTER_CUBIC);
	}
	else{
		clock_t start_time=clock();
		{
			Size dsize = Size(int(src.cols*scale),int(src.rows*scale));
			img2 = Mat(dsize,CV_32S);
			resize(src, img2,dsize,INTER_CUBIC);
		}
		clock_t end_time=clock();
		cout<< "Running time is: "<<static_cast<double>(end_time-start_time)/CLOCKS_PER_SEC*1000<<"ms"<<endl;//�������ʱ��

		//CV_INTER_NN - ����ڲ�ֵ,
		//CV_INTER_LINEAR -  ˫���Բ�ֵ (ȱʡʹ��)
		//CV_INTER_AREA -  ʹ�����ع�ϵ�ز�������ͼ����Сʱ�򣬸÷���
		//���Ա��Ⲩ�Ƴ��֡���ͼ��Ŵ�ʱ��������  CV_INTER_NN  ����..
		//CV_INTER_CUBIC -  ������ֵ.
		namedWindow("WindowOrg",WINDOW_AUTOSIZE);
		namedWindow("WindowNew",WINDOW_AUTOSIZE);
		imshow("WindowOrg",src);
		imshow("WindowNew",img2);

		waitKey(1000);
	}
	return img2;
}

//����ѡ���Ļ���
void showEdge(vector<vector<Point>> points_,Mat& picture)
{
	srand( (unsigned)time( NULL ));
	int radius=1;
	Point center;

	int sEdge=points_.size();
	Point prev_point;
	Point current_point;
	for (int iEdge=0;iEdge<sEdge;iEdge++){
		int r=rand()%256;
		int g=rand()%256;
		int b=rand()%256;
		Scalar color=Scalar(b,g,r);
		vector<Point> Edge=points_.at(iEdge);
		int sPoints=Edge.size();
		for(int iPoint=0;iPoint<sPoints-1;iPoint++){
			center=Edge.at(iPoint);
			//circle(picture,center,radius,color);
			prev_point=Edge.at(iPoint);
			current_point=Edge.at(iPoint+1);
					// Use modern OpenCV line function
		line(picture, prev_point, current_point, color, 1, LINE_AA);
		}
	}
}
// file operation
int writeFile(string fileName_cpp,vector<string> vsContent){
	string line="";
	vector<string> data;
	vector<string> data_split;
	ofstream out(fileName_cpp);
	if(!out)
	{
		cout<<"Failed to write file"<<endl;
		return -1;
	}
	for(vector<string>::iterator i=vsContent.begin();i<vsContent.end();i++){
		out<<*i<<endl;
	}
	out.close();
	return 1;
}

int readFile(string fileName_cpp){
	string line="";
	vector<string> data;
	ifstream in(fileName_cpp);
	if(!in)
	{
		cout<<"Failed to read file"<<endl;
		return -1;
	}
	while(getline(in,line))
	{
		data.push_back(line);    
	}
	in.close();
	for(unsigned int i=0;i<data.size();i++)
	{
		cout<<data.at(i)<<endl;
	}
	return 0;
}
int readFileByChar(string fileName_split){

	string line="";
	vector<string> data;
	vector<string> data_split;
	ifstream in_split(fileName_split);
	if(!in_split)
	{
		cout<<"Failed to read file"<<endl;
		return -1;
	}
	while(getline(in_split,line))
	{
		data_split.push_back(line);     //Read each line of the file and store it in the data_split vector
	}
	in_split.close();
	/******Read file******/
	/******Read split.txt file and process the data******/

	/******Process data_split vector data (data analysis)******/
	for(unsigned int i=0;i<data_split.size();i++)
	{
		cout<<"--------------------"<<endl;
		for(unsigned int j=0;j<getStr(data_split.at(i)).size();j++)
		{
			cout<<getStr(data_split.at(i)).at(j)<<endl;
		}
	}
	/******Process data_split vector data (data analysis)******/
	return 0;
}
void Trim(string &str)
{
	int s=str.find_first_not_of(" \t\n");
	int e=str.find_last_not_of(" \t\n");
	str=str.substr(s,e-s+1);
}
/******Process specific format data******/
//C++ doesn't have Split() function, so we need to define a custom function to process data, similar to C# and Java
vector<string> getStr(string str)
{
	int j=0;
	string a[100];
	vector<string> v_a;
	//Split()
	for(unsigned int i=0;i<str.size();i++)
	{
		if((str[i]!=',')&&str[i]!='\0')
		{
			a[j]+=str[i];
		}
		else j++;
	}

	for(int k=0;k<j+1;k++)
	{
		v_a.push_back(a[k]);
	}
	return v_a;
}
/******Process specific format data******/

/**
* path: directory
* files: vector to store file names
* r: whether to recursively search subdirectories
*/
void listDir(string real_dir,vector<string>& files,bool r){
	DIR *pDir;
	struct dirent *ent;
	string childpath;
	string absolutepath;
	pDir = opendir(real_dir.c_str());
	while ((ent = readdir(pDir)) != NULL){
		if (ent->d_type & DT_DIR){
			if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0){
				continue;
			}
			if(r){ //If we need to recursively search subdirectories
				childpath=real_dir+ent->d_name;
				listDir(childpath,files);
			}
		}
		else{
			absolutepath= real_dir+ent->d_name;
			files.push_back(ent->d_name);//File name
		}
	}
	sort(files.begin(),files.end());//Sort
}
void SaveEllipses(const string& fileName, const vector<Ellipse>& ellipses){
	unsigned n = ellipses.size();
	vector<string> resultString;
	stringstream resultsitem;
	// Save number of ellipses
	resultsitem << n;
	resultString.push_back(resultsitem.str());
	// Save ellipses data (ellipse data does not include _score field)
	for (unsigned i = 0; i<n; ++i)
	{
		const Ellipse& e = ellipses[i];
		resultsitem.str("");
		resultsitem << e._xc << "\t" << e._yc << "\t" 
			<< e._a << "\t" << e._b << "\t" 
			<< e._rad/* << "\t" << e._score*/;
		resultString.push_back(resultsitem.str());
	}
	writeFile(fileName,resultString);
	// When outputting data to console, there is no need to output ellipse information
	/*for (int i=0;i<resultString.size();i++){
		cout<<resultString[i]<<endl;
	}*/
}

// 14pr
// Should be checked
void SaveEllipses(const string& workingDir, const string& imgName, const vector<Ellipse>& ellipses /*, const vector<double>& times*/)
{
	string path(workingDir + "/" + imgName + ".txt");
	ofstream out(path, ofstream::out | ofstream::trunc);
	if (!out.good())
	{
		cout << "Error saving: " << path << endl;
		return;
	}
	unsigned n = ellipses.size();

	// Save number of ellipses
	out << n << "\n";
	// Save ellipses
	for (unsigned i = 0; i<n; ++i)
	{
		const Ellipse& e = ellipses[i];
		out << e._xc << "\t" << e._yc << "\t" << e._a << "\t" << e._b << "\t" << e._rad << "\t" << e._score << "\n";
	}
	out.close();
}

/* �����������Բ���е��������rad��ֵΪ[0~180]��[-PI/2~PI/2]��
��תΪ[0~PI]�Ļ�����ֵ�����ϲ����Test����rad�򣬶���һ�·�����Fֵ�� */
void LoadGT(vector<Ellipse>& gt, const string& sGtFileName, bool bIsAngleInRadians)
{
	ifstream in(sGtFileName);
	if (!in.good())
	{
		cout << "Error opening: " << sGtFileName << endl;
		return;
	}

	unsigned n;
	in >> n;

	gt.clear();
	gt.reserve(n);

	while (in.good() && n--)
	{
		Ellipse e;
		in >> e._xc >> e._yc >> e._a >> e._b >> e._rad;

		if (!bIsAngleInRadians)
		{
			// convert to radians
			e._rad = float(e._rad * CV_PI / 180.0);
		}

		if (e._a < e._b)
		{
			float temp = e._a;
			e._a = e._b;
			e._b = temp;

			e._rad = e._rad + float(0.5*CV_PI);
		}

		e._rad = fmod(float(e._rad + 2.f*CV_PI), float(CV_PI));
		e._score = 1.f;
		gt.push_back(e);
	}
	in.close();
}

// Should be checked
bool LoadTest(vector<Ellipse>& ellipses, const string& sTestFileName, vector<double>& times, bool bIsAngleInRadians)
{
	ifstream in(sTestFileName);
	if (!in.good())
	{
		cout << "Error opening: " << sTestFileName << endl;
		return false;
	}

	times.resize(6);
	in >> times[0] >> times[1] >> times[2] >> times[3] >> times[4] >> times[5];

	unsigned n;
	in >> n;

	ellipses.clear();

	if (n == 0) return true;

	ellipses.reserve(n);

	while (in.good() && n--)
	{
		Ellipse e;
		in >> e._xc >> e._yc >> e._a >> e._b >> e._rad >> e._score;

		if (!bIsAngleInRadians)
		{
			e._rad = e._rad * float(CV_PI / 180.0);
		}

		e._rad = fmod(float(e._rad + 2.0*CV_PI), float(CV_PI));

		if ((e._a > 0) && (e._b > 0) && (e._rad >= 0))
		{
			ellipses.push_back(e);
		}
	}
	in.close();

	// Sort ellipses by decreasing score
	sort(ellipses.begin(), ellipses.end());

	return true;
}
// Should be checked !!!!!
//  TestOverlap
float Evaluate(const vector<Ellipse>& ellGT, const vector<Ellipse>& ellTest, const float th_score, const Mat3b& img)
{
	float threshold_overlap = 0.8f;
	//float threshold = 0.95f;

	unsigned sz_gt = ellGT.size();
	unsigned size_test = ellTest.size();

	unsigned sz_test = unsigned(min(1000, int(size_test)));

	vector<Mat1b> gts(sz_gt);
	vector<Mat1b> tests(sz_test);
	//����ÿ��Ŀ����Բ(�ߴ�Ϊ-1���������Բ)
	for (unsigned i = 0; i<sz_gt; ++i)
	{
		const Ellipse& e = ellGT[i];

		Mat1b tmp(img.rows, img.cols, uchar(0));
		ellipse(tmp, Point((int)e._xc, (int)e._yc), Size((int)e._a, (int)e._b), e._rad * 180.0 / CV_PI, 0.0, 360.0, Scalar(255), -1);
		gts[i] = tmp;

		/*imshow("TestGT", tmp);
		imwrite("result/TestGT.jpg", tmp);*/
	}
	//���Ƽ�����Բ(�ߴ�Ϊ-1���������Բ)
	for (unsigned i = 0; i<sz_test; ++i)
	{
		const Ellipse& e = ellTest[i];

		Mat1b tmp(img.rows, img.cols, uchar(0));
		ellipse(tmp, Point((int)e._xc, (int)e._yc), Size((int)e._a, (int)e._b), e._rad * 180.0 / CV_PI, 0.0, 360.0, Scalar(255), -1);
		tests[i] = tmp;

		/*imshow("TestTest", tmp);
		imwrite("result/TestTest.jpg", tmp);*/
	}

	Mat1b overlap(sz_gt, sz_test, uchar(0));
	for (int r = 0; r < overlap.rows; ++r)
	{
		for (int c = 0; c < overlap.cols; ++c)
		{
			//�ص�����ռ�������ı��� ����ϲ�������ֵ Ϊ255
			overlap(r, c) = TestOverlap(gts[r], tests[c], threshold_overlap) ? uchar(255) : uchar(0);
		}
	}

	int counter = 0;

	vector<bool> vec_gt(sz_gt, false);
	//����ÿ����һ���ʹ����ҵ�
	for (unsigned int i = 0; i < sz_test; ++i)
	{
		//const Ellipse& e = ellTest[i];
		for (unsigned int j = 0; j < sz_gt; ++j)
		{
			if (vec_gt[j]) { continue; }

			bool bTest = overlap(j, i) != 0;

			if (bTest)
			{
				vec_gt[j] = true;
				break;
			}
		}
	}

	int tp = Count(vec_gt);
	int fn = int(sz_gt) - tp;
	int fp = size_test - tp; // !!!!

	float pr(0.f);
	float re(0.f);
	float fmeasure(0.f);

	if (tp == 0)
	{
		if (fp == 0)
		{
			pr = 1.f;
			re = 0.f;
			fmeasure = (2.f * pr * re) / (pr + re);
		}
		else
		{
			pr = 0.f;
			re = 0.f;
			fmeasure = 0.f;
		}
	}
	else
	{
		pr = float(tp) / float(tp + fp);
		re = float(tp) / float(tp + fn);
		fmeasure = (2.f * pr * re) / (pr + re);
	}

	return fmeasure;
}
// ��Ч�����ѡ���һ�����۷���(Lu's)����tp��ȫ������ͳ�ơ���Ϊ���ֱ��С�����ͳ�ơ������ں���Ĳ��轫ÿ��ͼ��tp��fn��fp�ۼӺ��ټ���Pre+Re+Fֵ��
vector<double> Evaluate_Lu(const vector<Ellipse>& ellGT, const vector<Ellipse>& ellTest, const float th_score, const Mat3b& img)
{
	float threshold_overlap = 0.8f;
	//float threshold = 0.95f;

	unsigned sz_gt = ellGT.size();
	unsigned size_test = ellTest.size();

	unsigned sz_test = unsigned(min(1000, int(size_test)));

	vector<Mat1b> gts(sz_gt);
	vector<Mat1b> tests(sz_test);
	//����ÿ��Ŀ����Բ(�ߴ�Ϊ-1���������Բ)
	for (unsigned i = 0; i < sz_gt; ++i)
	{
		const Ellipse& e = ellGT[i];

		Mat1b tmp(img.rows, img.cols, uchar(0));
		ellipse(tmp, Point((int)e._xc, (int)e._yc), Size((int)e._a, (int)e._b), e._rad * 180.0 / CV_PI, 0.0, 360.0, Scalar(255), -1);
		gts[i] = tmp;

		/*imshow("TestGT", tmp);
		imwrite("result/TestGT.jpg", tmp);*/
	}
	//���Ƽ�����Բ(�ߴ�Ϊ-1���������Բ)
	for (unsigned i = 0; i < sz_test; ++i)
	{
		const Ellipse& e = ellTest[i];

		Mat1b tmp(img.rows, img.cols, uchar(0));
		ellipse(tmp, Point((int)e._xc, (int)e._yc), Size((int)e._a, (int)e._b), e._rad * 180.0 / CV_PI, 0.0, 360.0, Scalar(255), -1);
		tests[i] = tmp;

		/*imshow("TestTest", tmp);
		imwrite("result/TestTest.jpg", tmp);*/
	}

	Mat1b overlap(sz_gt, sz_test, uchar(0));
	for (int r = 0; r < overlap.rows; ++r)
	{
		for (int c = 0; c < overlap.cols; ++c)
		{
			//�ص�����ռ�������ı��� ����ϲ�������ֵ Ϊ255
			overlap(r, c) = TestOverlap(gts[r], tests[c], threshold_overlap) ? uchar(255) : uchar(0);
		}
	}

	int counter = 0;

	vector<bool> vec_gt(sz_gt, false);
	//����ÿ����һ���ʹ����ҵ�
	for (unsigned int i = 0; i < sz_test; ++i)
	{
		//const Ellipse& e = ellTest[i];
		for (unsigned int j = 0; j < sz_gt; ++j)
		{
			if (vec_gt[j]) { continue; }

			bool bTest = overlap(j, i) != 0;

			if (bTest)
			{
				vec_gt[j] = true;
				break;
			}
		}
	}
	vector<bool> vec_test(sz_test, false);
	//����ÿ����һ���ʹ����ҵ�
	for (unsigned int j = 0; j < sz_gt; ++j)
	{
		//const Ellipse& e = ellTest[i];
		for (unsigned int i = 0; i < sz_test; ++i)
		{
			if (vec_test[i]) { continue; }

			bool bTest = overlap(j, i) != 0;

			if (bTest)
			{
				vec_test[i] = true;
				break;
			}
		}
	}

	int tp = Count(vec_gt);
	int fn = int(sz_gt) - tp;
	int fp = size_test - Count(vec_test); // !!!!

	vector<double> tp_fn_fp;
	tp_fn_fp.push_back(tp);
	tp_fn_fp.push_back(fn);
	tp_fn_fp.push_back(fp);
	return tp_fn_fp;
}


bool TestOverlap(const Mat1b& gt, const Mat1b& test, float th)
{
	float fAND = float(countNonZero(gt & test));
	float fOR = float(countNonZero(gt | test));
	float fsim = fAND / fOR;

	return (fsim >= th);
}

int Count(const vector<bool> v)
{
	int counter = 0;
	for (unsigned i = 0; i<v.size(); ++i)
	{
		if (v[i]) { ++counter; }
	}
	return counter;
}

void salt(cv::Mat& image, int n){
	for(int k=0; k<n; k++){
		int i = rand()%image.cols;
		int j = rand()%image.rows;

		if(image.channels() == 1){
			image.at<uchar>(j,i) = 255;
		}else{
			image.at<cv::Vec3b>(j,i)[0] = 255;
			image.at<cv::Vec3b>(j,i)[1] = 255;
			image.at<cv::Vec3b>(j,i)[2] = 255;
		}
	}
}

/* ����Ķ���
1��line-20������lineCrossPoint()��������Ч��ı�ʾ����'������С��1.#INF'����'0'������value4SixPoints( V2SP )�����н�����Ч����ж���
2��line-110���ı������ȵ���ʽ����a/b��Ϊ��׼b/a�����������½�����δ����ԭ�򣬻ع���
2��line-661��672��SaveEllipses()�������̨ʱ���ݲ�������Բ����_score�������Ҳ������ݼ�ʱ����Ҫ����̨�����Բ����Ϣ��
����3��line-73��82�������Զ����������ʽ������line-82:�����Զ�����㵽�߾��뺯����
�ص�4��line-110������value4SixPoints()������������ͬԲ'������=1'Ϊ'���㹲��'��
	��a������'1/2*det(ӳ�������) = ������� = 0'~ӳ��㹲��~�ж�ͬԲ��
	��b������'ӳ��������� = -1'~ӳ��㹲��~�ж�ͬԲ��
	��c1-�ϸ�����ӳ���'һ�㵽������ȷ����ֱ�ߵ�(����)���� < Th'~ӳ��㹲��~�ж�ͬԲ��
	��c2-׼ȷ������ӳ���'���㵽��ȷ���Ļع��ߵ�(����)���� < Th'~ӳ��㹲��~�ж�ͬԲ��
*/