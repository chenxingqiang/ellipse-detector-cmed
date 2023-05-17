/* 像素点操作方法的工具类 */

/*
This code is intended for academic use only.
You are free to use and modify the code, at your own risk.

If you use this code, or find it useful, please refer to the paper:


The comments in the code refer to the abovementioned paper.
If you need further details about the code or the algorithm, please contact me at:

lianbosong@foxmail.com

last update: 
*/

#include "tools.h"

Point2f lineCrossPoint(Point2f l1p1,Point2f l1p2,Point2f l2p1,Point2f l2p2 )
{
	/* 为避免刚好(0, 0)点为交点的情况，'无效点'由'正无穷小数'表示：1.#INF = 1/tmp(=0) */
	float tmp = 0;
	float positive_infinity = 1 / tmp;
	/*int positive_infinity = 0;*/

	Point2f crossPoint;
	float k1,k2,b1,b2;
	if (l1p1.x==l1p2.x&&l2p1.x==l2p2.x){						// 垂直x轴产生平行
		crossPoint=Point2f(positive_infinity, positive_infinity);//无效点
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
	if (k1==k2)													// 斜率相等产生平行
	{
		crossPoint=Point2f(positive_infinity, positive_infinity);//无效点
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
// 自定义函数：通过行列式的计算方式求A的行列式，代替内置的det函数，A为3*3的矩阵。
float detResult(float matR[])
{
	float result = matR[0] * matR[4] * matR[8] + matR[1] * matR[5] * matR[6]
		+ matR[2] * matR[3] * matR[7] - matR[2] * matR[4] * matR[6]
		- matR[1] * matR[3] * matR[8] - matR[0] * matR[5] * matR[7];

	return result;
}
// 自定义函数：求点R1到点R2+R3所定的直线的距离。
float getDistance(Point2f R1, Point2f R2, Point2f R3) {
	// 求直线方程L:R2R3
	float A = 0, B = 0, C = 0;
	A = R3.y - R2.y;	// A = y2 - y1
	B = R2.x - R3.x;	// B = x1 - x2
	C = R3.x * R2.y - R2.x * R3.y;	// C = x2 * y1 - x1 * y2
	// 代入求点R1到直线L:R2R3距离的公式中
	float distance = abs(A * R1.x + B * R1.y + C) / (sqrtf(A * A + B * B));

	return distance;
}
/**
 *这里是我需要改动的：更换两弧上六点判断同圆的方法为三点共线，看效率是否有提升！
 * 原版：计算出这六点的特征值(b1~b6 / a1~a6)并返回，绝对值后再与'1'比较，
 *	若差距在误差范围(阀值)内，则视为满足条件即视为两弧(六点)共圆。
 *
 * 新版：算出六点的映射点R1~R3，判断R1~R3是否'近似'共线，则视为满足同圆条件。
 */
/**
 *参数解释：
 * V2SP = (Point2f p3,Point2f p2,Point2f p1,Point2f p4,Point2f p5,Point2f p6)
 *	(p3, p2, p1)-(p4, p5, p6)，La:<p1, p2>~~~Lb:<p3, p4>~~~Lc:<p5, p6>。
 * Mat：是OpenCV表达二维图片的基础，在存储像素点信息时使用C++接口-Mat可以自动管理内存，
 *	由矩阵头(矩阵大小+存储地址等信息)和指针(指向像素值矩阵)组成。
 * OpenCV中表示图片一般用Mat的子类_Mat类型，并指定通道位数+数据类型+通道，
 *	如'CV_32F3C'表示32位+float类型+三通道，一般使用BGR三通道即可，存储值为像素灰度值。
 */
float value4SixPoints( V2SP )
{
/* 原方法：六点特征数=1~判断同圆 */
#if 0
	float result=1;
	Mat A,B,C;
	float matB[2][2],matC[2][2];
	
	// 三直线abc(两点确定一条直线如p1p2确定直线a)两两交于uvw三点( u=<a, c> )
	Point2f v,w,u;
	v=lineCrossPoint(p1,p2,p3,p4);
	w=lineCrossPoint(p5,p6,p3,p4);
	u=lineCrossPoint(p5,p6,p1,p2);

	float tmp = 0;
	float positive_infinity = 1 / tmp;	// 正无穷小数1.#INF
	/*float positive_infinity = 0;*/
	// 判断三直线交点是否为无效点
	if (v.x == positive_infinity || w.x == positive_infinity || u.x == positive_infinity) {

		// cout << "result:" << result << endl;
		// cout << "test:" << (fabs(result) > 0.47f) << endl;
		// cout << "infinity\t";

		// 返回一个"|x| > tCNC阀值 = false"的数即可，符合"NaN > tCNC = false"。
		return 0;
	}

	// 将u,v坐标放入二维数组matB的两行，p1,p2坐标放入二维数组matC的两行
	point2Mat(u,v,matB);
	point2Mat(p1,p2,matC);
	// 根据二维数组定义矩阵BC；根据BC(uv和p1p2)，求A=C*(B^-1)；赋值A[a1 b1; a2 b2]
	B=Mat(2,2,CV_32F,matB);
	C=Mat(2,2,CV_32F,matC);
	A=C*B.inv();
	// 求特征比：b1* b2 / a1 * a2
	/* TODO：这里用的是a1*a2 / b1*b2，因严格中比值为1所以二者结果应该差别不大；为什么这里使用b/a的形式会降低性能？ */
	result*=A.at<float>(0,0)*A.at<float>(1,0)/(A.at<float>(0,1)*A.at<float>(1,1));
	/*result *= A.at<float>(0, 1) * A.at<float>(1, 1) / (A.at<float>(0, 0) * A.at<float>(1, 0));*/

	/*cout<<"u:\t"<<u<<endl;
	cout<<"v:\t"<<v<<endl;
	cout<<"B:\t"<<B<<endl;
	cout<<"C:\t"<<C<<endl;
	cout<<"A:\t"<<A<<endl;
	cout<<"result:\t"<<result<<endl;*/

	/* 同理求出 b3*b4 / a3*a4 */
	point2Mat(v,w,matB);
	point2Mat(p3,p4,matC);
	B=Mat(2,2,CV_32F,matB);
	C=Mat(2,2,CV_32F,matC);
	A=C*B.inv();
	result*=A.at<float>(0,0)*A.at<float>(1,0)/(A.at<float>(0,1)*A.at<float>(1,1));

	/* 同理求出 b5*b6 / a5*a6 */
	point2Mat(w,u,matB);
	point2Mat(p5,p6,matC);
	B=Mat(2,2,CV_32F,matB);
	C=Mat(2,2,CV_32F,matC);
	A=C*B.inv();
	result*=A.at<float>(0,0)*A.at<float>(1,0)/(A.at<float>(0,1)*A.at<float>(1,1));
	
	// 累计求积得到特征数CNC：b1*b2 * b3*b4 * b5*b6 / a1*a2 * a3*a4 * a5*a6 = 1
	return result - 1;	// 特征数CNC先'-1'操作，再返回用绝对值和阀值比较
#endif // 0

/* 改a：三映射点面积=1/2*行列式=0~判断映射点共线~判断同圆 */
#if 0
	// 六点p1~p6组成的六边形，三对对边延长线交于R1, R2, R3三点(特征映射点)
	Point2f R1, R2, R3;
	R1 = lineCrossPoint(p1, p2, p4, p5);
	R2 = lineCrossPoint(p2, p3, p5, p6);
	R3 = lineCrossPoint(p3, p4, p6, p1);
	
	float tmp = 0;
	float positive_infinity = 1 / tmp;	// 正无穷小数1.#INF
	/*float positive_infinity = 0;*/
	// 判断R点是否为无效点(无穷)-对边平行，若R为无穷远点则暂将其判为同圆；
	if (R1.x == positive_infinity || R2.x == positive_infinity || R3.x == positive_infinity) {

		//Mat A = Mat(3, 3, CV_32F, matR);
		//cout << "A:" << A << endl;
		//cout << "det(A):" << result << endl;
		//cout << "test:" << (fabs(result) > 500.36f) << endl;
		//cout << "infinity\t";

		// 返回一个"|x| > tCNC阀值 = false"的数即可，符合"result > tCNC = false"。
		return 0;
	}
	
	// 计算R1, R2, R3三点构成的行列式的值det(A)
	float matR[] = {R1.x, R1.y, 1, R2.x, R2.y, 1, R3.x, R3.y, 1};
	// det(A)是三角形R1R2R3面积的2倍，而三角形R1R2R3面积近似为0 ~ 三点近似共线
	float result = detResult(matR) * 0.5;
	
	// 计算出1/2行列式的值=三点面积近似为0，再返回用|result|和tCNC阀值比较
	return result;
#endif // 0

/* 改b：三映射点特征数=-1~判断映射点共线~判断同圆 */
#if 0
	// 六点p1~p6组成的六边形，三对对边延长线交于R1, R2, R3三点(特征映射点)
	Point2f R1, R2, R3;
	R1 = lineCrossPoint(p1, p2, p4, p5);
	R2 = lineCrossPoint(p2, p3, p5, p6);
	R3 = lineCrossPoint(p3, p4, p6, p1);

	float tmp = 0;
	float positive_infinity = 1 / tmp;	// 正无穷小数1.#INF
	/*float positive_infinity = 0;*/
	// 判断R点是否为无效点(无穷)-对边平行，若R为无穷远点则暂将其判为同圆；
	if (R1.x == positive_infinity || R2.x == positive_infinity || R3.x == positive_infinity) {

		//Mat A = Mat(3, 3, CV_32F, matR);
		//cout << "A:" << A << endl;
		//cout << "det(A):" << result << endl;
		//cout << "test:" << (fabs(result) > 500.36f) << endl;
		//cout << "infinity\t";

		// 返回一个"|x| > tCNC阀值 = false"的数即可，符合"result > tCNC = false"。
		return 0;
	}

	// 计算三映射点R1, R2, R3的特征数CNL近似为-1，则三点共线；
	float result = 1;
	Mat A, B, C;
	float matB[2][2], matC[1][2];

	Point2f v, w, u;
	v = lineCrossPoint(p1, p2, p3, p4);
	w = lineCrossPoint(p5, p6, p3, p4);
	u = lineCrossPoint(p5, p6, p1, p2);
	// 判断三直线交点是否为无效点
	if (v.x == positive_infinity || w.x == positive_infinity || u.x == positive_infinity) {

		// cout << "result:" << result << endl;
		// cout << "test:" << (fabs(result) > 0.47f) << endl;
		// cout << "infinity\t";

		// 返回一个"|x| > tCNC阀值 = false"的数即可，符合"NaN > tCNC = false"。
		return 0;
	}

	// 将u,v坐标放入二维数组matB的两行；R1坐标放入二维数组matC的一行
	point2Mat(u, v, matB);
	matC[0][0] = R1.x;
	matC[0][1] = R1.y;
	// 根据二维数组定义矩阵BC；根据BC(uv和R1)，求A=C*(B^-1)；赋值A[a1 b1]
	B = Mat(2, 2, CV_32F, matB);
	C = Mat(1, 2, CV_32F, matC);
	A = C * B.inv();
	// 求特征比：b1 / a1；这里用的是a1 / b1，原因同上述第一个方法中六点特征比
	result *= A.at<float>(0, 0) / A.at<float>(0, 1);
	/*result *= A.at<float>(0, 1) / A.at<float>(0, 0);*/

	/* 同理求出 b2 / a2 */
	point2Mat(u, w, matB);
	matC[0][0] = R2.x;
	matC[0][1] = R2.y;
	B = Mat(2, 2, CV_32F, matB);
	C = Mat(1, 2, CV_32F, matC);
	A = C * B.inv();
	result *= A.at<float>(0, 0) / A.at<float>(0, 1);

	/* 同理求出 b3 / a3 */
	point2Mat(v, w, matB);
	matC[0][0] = R3.x;
	matC[0][1] = R3.y;
	B = Mat(2, 2, CV_32F, matB);
	C = Mat(1, 2, CV_32F, matC);
	A = C * B.inv();
	result *= A.at<float>(0, 0) / A.at<float>(0, 1);

	// 累计求积得到特征数CNL：b1 * b2 * b3 / a1 * a2 * a3 = -1
	return result + 1;	// 特征数CNC先'+1'操作，再返回用绝对值和阀值比较
#endif // 0

/* 改c1+c2：点到另两点线最大距离<Th + 点到回归线最大距离<Th~判断映射点共线~判断同圆 */
#if 1
	// 六点p1~p6组成的六边形，三对对边延长线交于R1, R2, R3三点(特征映射点)
	Point2f R1, R2, R3;
	R1 = lineCrossPoint(p1, p2, p4, p5);
	R2 = lineCrossPoint(p2, p3, p5, p6);
	R3 = lineCrossPoint(p3, p4, p6, p1);

	float tmp = 0;
	float positive_infinity = 1 / tmp;	// 正无穷小数1.#INF
	/*float positive_infinity = 0;*/
	// 判断R点是否为无效点(无穷)-对边平行，若R为无穷远点则暂将其判为同圆；
	if (R1.x == positive_infinity || R2.x == positive_infinity || R3.x == positive_infinity) {

		//Mat A = Mat(3, 3, CV_32F, matR);
		//cout << "A:" << A << endl;
		//cout << "det(A):" << result << endl;
		//cout << "test:" << (fabs(result) > 500.36f) << endl;
		//cout << "infinity\t";

		// 返回一个"|x| > tCNC阀值 = false"的数即可，符合"result > tCNC = false"。
		return 0;
	}

	#if 0
		/* 方法1较严格：求点到另两点所定直线的距离，取三次对应距离的最大，返回和阀值比较；*/
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
		return maxDistance;	// 返回点到直线的最大距离，和阀值比较
	#endif // 0

	#if 1
		/* 方法2较准确：最小二乘法求三点的回归线，取其到回归线距离的最大，返回和阀值比较；*/
		float sumRx = R1.x + R2.x + R3.x;
		float sumRxSquare = R1.x*R1.x + R2.x*R2.x + R3.x*R3.x;
		float sumRxRy = R1.x*R1.y + R2.x*R2.y + R3.x*R3.y;
		float sumRy = R1.y + R2.y + R3.y;

		// 残差e^2=求和(yi - (kxi+b))^2；分别对kb求偏导且令导数=0(可使误差e最小)，可得关于kb的方程组，求解方程组的可得使e^2最小的kb值，即使得e^2最小的y(确定回归线)；
		// 将方程组化为矩阵乘积形式A * k_b = B，即k_b(含k, b) = A.inv()*B；
		float arryA[] = { sumRxSquare, sumRx, sumRx, 3 };
		Mat1f matA = Mat(2, 2, CV_32F, arryA);
		float arryB[] = { sumRxRy, sumRy };
		Mat1f matB = Mat(2, 1, CV_32F, arryB);
		Mat1f k_b = matA.inv() * matB;	// 解出kb确定直线L:y = kx + b

		// 求三个R点到回归线的三个距离中的最大距离
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
		return maxDistance;	// 返回点到回归线的最大距离，和阀值比较
	#endif // 0
#endif // 0
}
/* 改动结束 */

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
	cvNamedWindow(MultiShow_WinName.c_str(), CV_WINDOW_NORMAL);
	cvMoveWindow(MultiShow_WinName.c_str(),(Scree_W - Disp_Img.cols)/2 ,(Scree_H - Disp_Img.rows)/2);//Centralize the window  
	IplImage tmp_Disp_Img= IplImage(Disp_Img);
	cvShowImage(MultiShow_WinName.c_str(), &tmp_Disp_Img);  
	cvWaitKey(0);  
	cvDestroyWindow(MultiShow_WinName.c_str());  
}  


void PyrDown(string picName)
{
	Mat img1=imread(picName);
	Mat img2;
	Size sz;
	//金字塔向下或者向上采样操作 ，基本不改变图像长宽比率
	//pyrDown(img1,img2,sz,BORDER_DEFAULT);
	pyrUp(img1,img2,sz,BORDER_DEFAULT);
	pyrUp(img2,img2,sz,BORDER_DEFAULT);
	namedWindow("WindowOrg");
	namedWindow("WindowNew");
	imshow("WindowOrg",img1);
	imshow("WindowNew",img2);

	waitKey(10000);
}
Mat matResize(Mat src,double scale){
	Mat img2;
	bool showtimeandpic=false;
	if(!showtimeandpic){
		Size dsize = Size(int(src.cols*scale),int(src.rows*scale));
		img2 = Mat(dsize,CV_32S);
		resize(src, img2,dsize,CV_INTER_CUBIC);
	}
	else{
		clock_t start_time=clock();
		{
			Size dsize = Size(int(src.cols*scale),int(src.rows*scale));
			img2 = Mat(dsize,CV_32S);
			resize(src, img2,dsize,CV_INTER_CUBIC);
		}
		clock_t end_time=clock();
		cout<< "Running time is: "<<static_cast<double>(end_time-start_time)/CLOCKS_PER_SEC*1000<<"ms"<<endl;//输出运行时间

		//CV_INTER_NN - 最近邻差值,
		//CV_INTER_LINEAR -  双线性差值 (缺省使用) 
		//CV_INTER_AREA -  使用象素关系重采样。当图像缩小时候，该方法
		//可以避免波纹出现。当图像放大时，类似于  CV_INTER_NN  方法.. 
		//CV_INTER_CUBIC -  立方差值. 
		namedWindow("WindowOrg",CV_WINDOW_AUTOSIZE);
		namedWindow("WindowNew",CV_WINDOW_AUTOSIZE);
		imshow("WindowOrg",src);
		imshow("WindowNew",img2);

		waitKey(1000);
	}
	return img2;
}

//绘制选出的弧段
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
			//参数为：承载的图像、圆心、半径、颜色、粗细、线型  
			//circle(picture,center,radius,color); 
			prev_point=Edge.at(iPoint);
			current_point=Edge.at(iPoint+1);
			//Mat to IplImage to cvArr
			IplImage ipl_img = picture;
			cvLine(&ipl_img, prev_point, current_point, color, 1, CV_AA);
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
		cout<<"读写文件失败"<<endl;
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
		cout<<"读写文件失败"<<endl;
		return -1;
	}
	while(getline(in,line))
	{
		data.push_back(line);     //读取文件每一行数据，并放到“容器”里面
	}
	in.close();
	/******遍历data里面的内容******/
	for(unsigned int i=0;i<data.size();i++)
	{
		cout<<data.at(i)<<endl;
	}
	return 0;
	/******遍历data里面的内容******/
}
int readFileByChar(string fileName_split){

	string line="";
	vector<string> data;
	vector<string> data_split;
	ifstream in_split(fileName_split);
	if(!in_split)
	{
		cout<<"读写文件失败"<<endl;
		return -1;
	}
	while(getline(in_split,line))
	{
		data_split.push_back(line);     //读取文件每一行数据，并放到“容器”里面
	}
	in_split.close();
	/******读文件******/
	/******提取split.txt文件里面的数据******/

	/******遍历data_split里面的内容(数据分离)******/
	for(unsigned int i=0;i<data_split.size();i++)
	{
		cout<<"--------------------"<<endl;
		for(unsigned int j=0;j<getStr(data_split.at(i)).size();j++)
		{
			cout<<getStr(data_split.at(i)).at(j)<<endl;
		}
	}
	/******遍历data_split里面的内容(数据分离)******/
	return 0;
}
void Trim(string &str)
{
	int s=str.find_first_not_of(" \t\n");
	int e=str.find_last_not_of(" \t\n");
	str=str.substr(s,e-s+1);
}
/******分离特定格式的数据******/
//C++中没有Split()这个方法，需要自定义函数分离数据，而C#和Java中有这个方法
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
/******分离特定格式的数据******/

/**
* path:目录
* files：用于保存文件名的vector
* r：是否需要遍历子目录
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
			if(r){ //如果需要遍历子目录
				childpath=real_dir+ent->d_name;
				listDir(childpath,files);
			}
		}
		else{
			absolutepath= real_dir+ent->d_name;
			files.push_back(ent->d_name);//文件名
		}
	}
	sort(files.begin(),files.end());//排序
}
void SaveEllipses(const string& fileName, const vector<Ellipse>& ellipses){
	unsigned n = ellipses.size();
	vector<string> resultString;
	stringstream resultsitem;
	// Save number of ellipses
	resultsitem << n;
	resultString.push_back(resultsitem.str());
	// Save ellipses：暂不保存椭圆评分_score参数
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
	// 测试数据集时不需要控制台输出椭圆集信息
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

/* 不管输入的椭圆集中第五个参数rad的值为[0~180]或[-PI/2~PI/2]，
都转为[0~PI]的弧度制值，符合测出的Test集的rad域，二者一致方便算F值。 */
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
	//绘制每个目标椭圆(线粗为-1代表填充椭圆)
	for (unsigned i = 0; i<sz_gt; ++i)
	{
		const Ellipse& e = ellGT[i];

		Mat1b tmp(img.rows, img.cols, uchar(0));
		ellipse(tmp, Point((int)e._xc, (int)e._yc), Size((int)e._a, (int)e._b), e._rad * 180.0 / CV_PI, 0.0, 360.0, Scalar(255), -1);
		gts[i] = tmp;

		/*imshow("TestGT", tmp);
		imwrite("result/TestGT.jpg", tmp);*/
	}
	//绘制检测的椭圆(线粗为-1代表填充椭圆)
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
			//重叠区域占真个区域的比例 与比上并大于阈值 为255
			overlap(r, c) = TestOverlap(gts[r], tests[c], threshold_overlap) ? uchar(255) : uchar(0);
		}
	}

	int counter = 0;

	vector<bool> vec_gt(sz_gt, false);
	//矩阵每行有一个就代表找到
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
// 【效果不佳】另一种评价方法(Lu's)：将tp“全部按行统计”改为“分别按行、按列统计”，并在后面的步骤将每个图的tp，fn和fp累加后，再计算Pre+Re+F值。
vector<double> Evaluate_Lu(const vector<Ellipse>& ellGT, const vector<Ellipse>& ellTest, const float th_score, const Mat3b& img)
{
	float threshold_overlap = 0.8f;
	//float threshold = 0.95f;

	unsigned sz_gt = ellGT.size();
	unsigned size_test = ellTest.size();

	unsigned sz_test = unsigned(min(1000, int(size_test)));

	vector<Mat1b> gts(sz_gt);
	vector<Mat1b> tests(sz_test);
	//绘制每个目标椭圆(线粗为-1代表填充椭圆)
	for (unsigned i = 0; i < sz_gt; ++i)
	{
		const Ellipse& e = ellGT[i];

		Mat1b tmp(img.rows, img.cols, uchar(0));
		ellipse(tmp, Point((int)e._xc, (int)e._yc), Size((int)e._a, (int)e._b), e._rad * 180.0 / CV_PI, 0.0, 360.0, Scalar(255), -1);
		gts[i] = tmp;

		/*imshow("TestGT", tmp);
		imwrite("result/TestGT.jpg", tmp);*/
	}
	//绘制检测的椭圆(线粗为-1代表填充椭圆)
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
			//重叠区域占真个区域的比例 与比上并大于阈值 为255
			overlap(r, c) = TestOverlap(gts[r], tests[c], threshold_overlap) ? uchar(255) : uchar(0);
		}
	}

	int counter = 0;

	vector<bool> vec_gt(sz_gt, false);
	//矩阵每行有一个就代表找到
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
	//矩阵每列有一个就代表找到
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

/* 代码改动：
1、line-20：更换lineCrossPoint()方法中无效点的表示，由'正无穷小数1.#INF'代替'0'，并在value4SixPoints( V2SP )方法中进行无效点的判定。
2、line-110：改变特征比的形式，由a/b改为标准b/a有明显性能下降，暂未发现原因，回滚。
2、line-661、672：SaveEllipses()输出控制台时，暂不保存椭圆评分_score参数，且测试数据集时不需要控制台输出椭圆集信息；
辅助3、line-73、82：添加自定义计算行列式函数；line-82:添加自定义求点到线距离函数。
重点4、line-110：更改value4SixPoints()方法，换六点同圆'特征数=1'为'三点共线'：
	改a、利用'1/2*det(映射点坐标) = 三点面积 = 0'~映射点共线~判断同圆；
	改b、利用'映射点特征数 = -1'~映射点共线~判断同圆；
	改c1-严格、利用映射点'一点到另两点确定的直线的(最大的)距离 < Th'~映射点共线~判断同圆；
	改c2-准确、利用映射点'三点到其确定的回归线的(最大的)距离 < Th'~映射点共线~判断同圆；
*/