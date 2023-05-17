/* 主函数，程序入口 */

/*
This code is intended for academic use only.
You are free to use and modify the code, at your own risk.

If you use this code, or find it useful, please refer to the paper:


The comments in the code refer to the abovementioned paper.
If you need further details about the code or the algorithm, please contact me at:

lianbosong@foxmail.com

last update: 
*/

#include "stdafx.h"
#include "tools.h"
#include "CNEllipseDetector.h"
using namespace std;
using namespace cv;

/* 定义工作目录和数据集，测试图片，以及程序中执行测试数据集的方法的类型(单个图片，单个数据集等) */
string SWORKINGDIR="EllipseDataset_Lu/";//"EllipseDataset/";//"EllipseDataset_Lu/";
// Dataset#2、DatasetPrasad/Prasad Dataset、Industrial PCB Image Dataset、Traffic Sign Dataset、Calibration、Prasad_plus
string DBNAME="Prasad Dataset";//"good2";//"/PrasadImages-DatasetPrasad";//"/RandomImages-Dataset#1";//"/BRhoChange";//"Prasad s&p/0.15"
string TESTIMGNAME = "05_00022.bmp";//"079_0033.jpg";//"67.bmp";//"05_00022.bmp";//"140_0038.jpg";//"bike_0068.jpg";//
//int MethodId=1;
int MethodId = 2;

// 设置最优参数值
/* 椭圆检测参数设置(阀值-实际与理想值的差值)：椭圆评定阀值a、椭圆评定阀值b、？、弧长阀值、？、？ */
//float	fThScoreScore = 0.7f;	//0.8
//float	fMinReliability	= 0.5f;	// Const parameters to discard bad ellipses 0.4
//float	fTaoCenters = 0.05f;//0.05 	
float	fThScoreScore = 0.61f;	//0.8
float	fMinReliability	= 0.4f;	// Const parameters to discard bad ellipses 0.4
float	fTaoCenters = 0.05f;//0.05 	

int		ThLength=16;//16
float	MinOrientedRectSide=3.0f;
float	scale=1;

/* 函数声明：(每一步)消耗时间展示、？、？、？、？、设置参数集、读取参数 */
vector<double> showT(string sWorkingDir,string imagename, CNEllipseDetector cned,vector<Ellipse> ellsCned,float fThScoreScore,bool showpic=false);
vector<double> OnImage(string sWorkingDir,string imagename,float fThScoreScore,float fMinReliability,float fTaoCenters,bool showpic=false);
vector<double> OnImage_salt(string sWorkingDir,string imagename,int saltrate,float fThScoreScore,float fMinReliability,float fTaoCenters,bool showpic=false);
vector<double> database(string dirName,float fThScoreScore,float fMinReliability,float fTaoCenters);
void OnVideo();
void SetParameter(map<char, string> Parameter);
void ReadParameter(int argc, char** argv);

/* 函数实现 */
void showTime(vector<double> times){
	cout << "--------------------------------" << endl;
	cout << "Execution Time: " << endl;
	cout << "Edge Detection: \t" << times[0] << endl;
	cout << "Pre processing: \t" << times[1] << endl;
	cout << "Grouping:       \t" << times[2] << endl;
	cout << "Estimation:     \t" << times[3] << endl;
	cout << "Validation:     \t" << times[4] << endl;
	cout << "Clustering:     \t" << times[5] << endl;
	cout << "--------------------------------" << endl;
	cout << "Total:	         \t" << accumulate(times.begin(),times.begin()+6,0.0) << endl;
	cout << "F-Measure:      \t" << times[6] << endl;
	cout << "--------------------------------" << endl;
	if(times.size()==9){
		cout << "countsOfFindEllipse \t"<<times[7]<<endl;
		cout << "countsOfGetFastCenter \t"<<times[8]<<endl;
	}
}
vector<double> showT(string sWorkingDir,string imagename, CNEllipseDetector cned,vector<Ellipse> ellsCned,float fThScoreScore, bool showpic){
	vector<Ellipse> gt;
	if (DBNAME == "PCB")
	{
		LoadGT(gt, sWorkingDir + "/gt/" + imagename + ".txt", true);
	}
	else if (DBNAME == "Industrial PCB Image Dataset") 
	{
		LoadGT(gt, sWorkingDir + "/gt0/" + imagename.substr(0, imagename.find(".")) + ".txt", true);
	}
	else {
		LoadGT(gt, sWorkingDir + "/gt0/" +"gt_"+ imagename + ".txt", false);
		//prasad_plus
		//LoadGT(gt, sWorkingDir + "/gt0/" +  imagename + ".gt.txt", false);
	}
	string filename = sWorkingDir + "/images/" + imagename;
	Mat3b image = imread(filename);
	Mat3b resultImage = image.clone();

	// Draw GT ellipses
	/*for (unsigned i = 0; i < gt.size(); ++i)
	{
		Ellipse& e = gt[i];
		Scalar color(0, 0, 255);
		ellipse(resultImage, Point(cvRound(e._xc), cvRound(e._yc)), Size(cvRound(e._a), cvRound(e._b)), e._rad*180.0 / CV_PI, 0.0, 360.0, color, 3);
	}*/
	// 【为减少实验时的等待时间，第二遍或非必要下可不保存结果图片】
	cned.DrawDetectedEllipses(resultImage, ellsCned);

	Mat3b res = image.clone();
	// 评价方法1(Jia's)：同TP平均法
	float fmeasure = Evaluate(gt, ellsCned, fThScoreScore, res);
	// 评价方法2(Lu's)：异TP累加法【效果不佳，应用Matlab实现的代码单独评价】
	vector<double> tp_fn_fp = Evaluate_Lu(gt, ellsCned, fThScoreScore, res);

	if (showpic) {
		cout << "F-Measure: " << fmeasure << endl;
		/*imshow("Cned", resultImage);*/
		//cvSaveImage("result/resultImage.jpg", &IplImage(resultImage));
		imwrite("result/resultImage.jpg", resultImage);
	}
	// 更改：测试整个数据集时，将其每个测试图片的测试结果也保存到结果图片
	else {
		//【为减少实验时的等待时间，第二遍或非必要下可不保存结果图片】
		string tmpPath = sWorkingDir + "/resultDetImages/";
		_mkdir(tmpPath.data());
		imwrite(tmpPath + "det_" + imagename, resultImage);
	}

	// 为方便返回数据，将评价1-fmeasure的值放入评价2-tp_fn_fp数组后面。
	tp_fn_fp.push_back(fmeasure);
	return tp_fn_fp;
}
void OnVideo()
{
	VideoCapture cap(0);
	if(!cap.isOpened()) return;

	CNEllipseDetector cned;

	Mat1b gray;
	while(true)
	{	
		Mat3b image;
		cap >> image;
		cvtColor(image, gray, CV_BGR2GRAY);	

		vector<Ellipse> ellipses;

		//Find Ellipses		
		cned.Detect(gray, ellipses);
		cned.DrawDetectedEllipses(image,ellipses);
		imshow("Output", image);

			
		if(waitKey(10) >= 0) break;
	}
}

vector<double> OnImage(string filename,float fThScoreScore,float fMinReliability,float fTaoCenters,bool showpic)
{
	vector<double> times;
	CNEllipseDetector cned;
	// Read image
	Mat3b image = imread(filename);
	
	if(!image.data){
		cout<<filename<<" not exist"<<endl;
		return times;
	}
	Size sz = image.size();

	// Convert to grayscale
	Mat1b gray;
	cvtColor(image, gray, CV_BGR2GRAY);

	// Parameters Settings (Sect. 4.2)
	int		iThLength = ThLength;//过滤太短的弧
	float	fThObb = MinOrientedRectSide;
	float	fThPos = 1.0f;
	//float	fTaoCenters = 0.018f;//0.05
	int 	iNs = 16;//弦数
	float	fMaxCenterDistance = sqrt(float(sz.width*sz.width + sz.height*sz.height)) * fTaoCenters;

	//float	fThScoreScore = 0.5f;//0.8	
	//fTaoCenters = 0.05f;
	// Other constant parameters settings.
	// Gaussian filter parameters, in pre-processing
	Size	szPreProcessingGaussKernelSize	= Size(5,5);
	double	dPreProcessingGaussSigma		= 1.0;

	float	fDistanceToEllipseContour		= 0.1f;	// (Sect. 3.3.1 - Validation)
	//float	fMinReliability					= 0.4f;	// Const parameters to discard bad ellipses 0.5

	// Initialize Detector with selected parameters
	cned.SetParameters	(	szPreProcessingGaussKernelSize,	
		dPreProcessingGaussSigma,		
		fThPos,
		fMaxCenterDistance,
		iThLength,
		fThObb,
		fDistanceToEllipseContour,		
		fThScoreScore,
		fMinReliability,		
		iNs
		);
	// Detect 
	vector<Ellipse> ellsCned;
	Mat1b gray_clone=gray.clone();
	cned.Detect(gray_clone, ellsCned);
	times = cned.GetTimes();

	Mat3b resultImage = image.clone();
	cned.DrawDetectedEllipses(resultImage, ellsCned);
	imshow("Cned", resultImage);
	if(showpic){
		_mkdir("result");
		IplImage resultImage_1 = IplImage(resultImage);
		cvSaveImage("result/resultImage.jpg",&resultImage_1);
		SaveEllipses("result/result.txt", ellsCned);
	}
	double fmeasure = 0;//showT(sWorkingDir,imagename, cned,ellsCned,0.8f,showpic);
	times.push_back(fmeasure);
	times.push_back(cned.countsOfFindEllipse);
	times.push_back(cned.countsOfGetFastCenter);
	return times;
}

void SetParameter(map<char,string> Parameter){
	/**
	-N image Name
	-D DataSet Name
	-S The threshold of ellipse score
	-R The threshold of Reliability
	-C The threshold of CenterDis
	-M The method id
	-P
	*/
	map<char,string>::iterator it;
	for(it=Parameter.begin();it!=Parameter.end();++it){
		switch(it->first){
			case 'N': TESTIMGNAME=it->second;break;
			case 'D': DBNAME=it->second;break;
			case 'S': fThScoreScore=atof(it->second.c_str());break;
			case 'C': fTaoCenters=atof(it->second.c_str());break;
			case 'R': fMinReliability=atof(it->second.c_str());break;
			case 'M': MethodId=atof(it->second.c_str());break;
			case 'P': SWORKINGDIR=it->second.c_str();break;
		}
	}
}
void ReadParameter(int argc,char** argv){
    if(argc<=1||argv[1][0]!='-'){
		if(argc>1)cout<<argv[1]<<endl;
		return;
    }
    map<char,string> Parameter;
    int targc=argc;
    int cur=1,pre=1;
	string sPa;
	char cP=argv[pre][1];
    for(int i=2;i<argc;i++){
        string stmp=argv[i];
        if(0==stmp.find("-")){
            cur=i;
			cP=argv[pre][1];
			Trim(sPa);
            Parameter.insert(pair<char,string>(cP,sPa));
			pre=cur;sPa="";
        }else{
			sPa=sPa+" "+argv[i];
		}
    }
	cP=argv[pre][1];
	Trim(sPa);
    Parameter.insert(pair<char,string>(cP,sPa));
	map<char,string>::iterator it;
    for(it=Parameter.begin();it!=Parameter.end();++it)
        cout<<"key: "<<it->first <<" value: "<<it->second<<endl;
	SetParameter(Parameter);
	
}


vector<double> OnImage(string sWorkingDir,string imagename,float fThScoreScore,float fMinReliability,float fTaoCenters,bool showpic)
{
	CNEllipseDetector cned;

	string filename = sWorkingDir + "/images/" + imagename;
	// Read image
	Mat3b image = imread(filename);
	if(!image.data){
		cout<<filename<<" not exist"<<endl;
		return vector<double>(0);
	}
	Size sz = image.size();

	// Convert to grayscale
	Mat1b gray;
	cvtColor(image, gray, CV_BGR2GRAY);

	// Parameters Settings (Sect. 4.2)
	int		iThLength = ThLength;//过滤太短的弧
	float	fThObb = MinOrientedRectSide;
	float	fThPos = 1.0f;
	//float	fTaoCenters = 0.018f;//0.05
	int 	iNs = 16;//弦数
	float	fMaxCenterDistance = sqrt(float(sz.width*sz.width + sz.height*sz.height)) * fTaoCenters;

	//float	fThScoreScore = 0.5f;//0.8	
	//fTaoCenters = 0.05f;
	// Other constant parameters settings.
	// Gaussian filter parameters, in pre-processing
	Size	szPreProcessingGaussKernelSize	= Size(5,5);
	double	dPreProcessingGaussSigma		= 1.0;

	float	fDistanceToEllipseContour		= 0.1f;	// (Sect. 3.3.1 - Validation)
	//float	fMinReliability					= 0.4f;	// Const parameters to discard bad ellipses 0.5

	// Initialize Detector with selected parameters
	cned.SetParameters	(	szPreProcessingGaussKernelSize,	
		dPreProcessingGaussSigma,		
		fThPos,
		fMaxCenterDistance,
		iThLength,
		fThObb,
		fDistanceToEllipseContour,		
		fThScoreScore,
		fMinReliability,		
		iNs
		);
	// Detect 
	vector<Ellipse> ellsCned;
	Mat1b gray_clone=gray.clone();
	cned.Detect(gray_clone, ellsCned);
	vector<double> times = cned.GetTimes();
	if(showpic){
		_mkdir("result");
		SaveEllipses("result/result.txt", ellsCned);
	}
	// 更改：测试整个数据集时，将其每个测试图片的测试结果也保存到参数文件
	// 【为减少实验时的等待时间，第二遍或非必要下可不保存结果参数txt】
	else {
		string tmpPath = sWorkingDir + "/resultDet/";
		_mkdir(tmpPath.data());
		SaveEllipses(tmpPath+"det_"+imagename+".txt", ellsCned);
	}

	// 为方便返回数据，将评价2-tp_fn_f的值放在times数组后面。
	vector<double> fm_and_tp_fn_fp = showT(sWorkingDir,imagename, cned,ellsCned,0.8f,showpic);
	times.push_back(fm_and_tp_fn_fp[3]);
	times.push_back(cned.countsOfFindEllipse);
	times.push_back(cned.countsOfGetFastCenter);
	times.push_back(fm_and_tp_fn_fp[0]);
	times.push_back(fm_and_tp_fn_fp[1]);
	times.push_back(fm_and_tp_fn_fp[2]);
	return times;
}
vector<double> database(string sWorkingDir,float fThScoreScore,float fMinReliability,float fTaoCenters){
	vector<string> resultString;
	resultString.push_back("picName,Edge Detection,Pre processing,Grouping,Estimation,Validation,Clustering,WholeTime,F-measure,countsOfFindEllipse,countsOfGetFastCenter");
	vector<double> allTimes(9,0.0);
	string dirName=sWorkingDir+"/images/";
	string imageName;
	double countsOfFindEllipse=0;
	double countsOfGetFastCenter=0;
	vector<string> files;
	listDir(dirName,files);

	imageName=files.at(1);
	vector<double> results=OnImage(sWorkingDir,imageName,fThScoreScore,fMinReliability,fTaoCenters);
	double fmeasure = results[6];
	vector<double> times = results;	// times = results = [0~5为六步时间数据，6为F值，7~8为counts数据，9~11为tp_fn_fp]
	double wholetime=accumulate(times.begin(),times.begin()+6,0.0);
	
	fmeasure=0.0;	// 评价1
	double tps = 0;	// 评价2
	double fns = 0;
	double fps = 0;
	for (unsigned int i = 0; i < files.size(); i++)
	{
		stringstream resultsitem;
		imageName = files.at(i);
		results = OnImage(sWorkingDir, imageName, fThScoreScore, fMinReliability, fTaoCenters);
		fmeasure += results[6];	// 评价1
		tps += results[9];	// 评价2
		fns += results[10];
		fps += results[11];
		countsOfFindEllipse += results[7];
		countsOfGetFastCenter += results[8];
		times = results;
		wholetime = accumulate(times.begin(), times.begin() + 6, 0.0);
		for (unsigned int j = 0; j < allTimes.size(); j++) {
			allTimes[j] += times[j];
		}
		resultsitem << imageName << "," << times[0] << "," << times[1] << "," << times[2] << "," << times[3] << "," << times[4] << "," << times[5] << "," << wholetime << "," << times[6] << "," << times[7] << "," << times[8];
		resultString.push_back(resultsitem.str());
	}
	for (unsigned int i=0;i<allTimes.size();i++){
		times[i]=allTimes[i]/(1.0*files.size());
	}
	wholetime=accumulate(times.begin(),times.begin()+6,0.0);
	
	// 评价1
	fmeasure = fmeasure / (1.0 * files.size());
	// 评价2
	double Precision;
	double Recall;
	double resultFM;
	if (tps == 0){
		Precision = 0;
		Recall = 0;
		resultFM = 0;
	} else {
		Precision = tps / (tps + fps);
		Recall = tps / (tps + fns);
		resultFM = 2 * Precision * Recall / (Precision + Recall);
	}

	cout << "--------------------------------" << endl;
	cout << "DataSet:        \t"<<sWorkingDir<<endl;
	cout << "Execution Time" << endl;
	cout << "Edge Detection: \t" << times[0] << endl;
	cout << "Pre processing: \t" << times[1] << endl;
	cout << "Grouping:       \t" << times[2] << endl;
	cout << "Estimation:     \t" << times[3] << endl;
	cout << "Validation:     \t" << times[4] << endl;
	cout << "Clustering:     \t" << times[5] << endl;
	cout << "--------------------------------" << endl;
	cout << "【仅参考时间数据】Total:	         \t" << accumulate(times.begin(),times.begin()+6,0.0) << endl;
	// 评价1
	cout << "【弃用评价1，单独评价】1-Jia's F-Measure: " << fmeasure << endl;
	// 评价2【效果不佳，应用Matlab实现的代码单独评价】
	cout << "【弃用评价2，单独评价】2-Lu's Evaluate(pre, re, fm): " << Precision << "\t" << Recall << "\t" << resultFM << endl;
	cout << "--------------------------------" << endl;
	cout << "countsOfFindEllipse \t"<<times[7]<<endl;
	cout << "countsOfGetFastCenter \t"<<times[8]<<endl;
	
	stringstream resultsitem;
	resultsitem<<"average"<<","<<times[0]<<","<<times[1]<<","<<times[2]<<","<<times[3]<<","<<times[4]<<","<<times[5]<<","<<wholetime<<","<<times[6]<<","<<times[7]<<","<<times[8];
	resultString.push_back(resultsitem.str());
	// 更改：测试整个数据集时，将dataset.cvs文件保存到数据集目录下
	// 【为减少实验时的等待时间，第二遍或非必要下可不保存每张图的详细检测内容】
	writeFile(sWorkingDir+"/dataset.csv",resultString);
	return times;
}

int main_OnePic()
{
	string sWorkingDir = SWORKINGDIR+DBNAME;
	string imagename = TESTIMGNAME;

	vector<double> results=OnImage(sWorkingDir,imagename,fThScoreScore,fMinReliability,fTaoCenters,true);
	if(!results.empty()){
		showTime(results);//显示运行信息
		waitKey(0);
		cvDestroyWindow("Cned");
	}
	
	return 0;
}
int main_OneDB()
{
	string sWorkingDir = SWORKINGDIR+DBNAME;
	string imagename = TESTIMGNAME;
	cout<<DBNAME<<"\t"<<fThScoreScore<<" "<<fMinReliability<<" "<<fTaoCenters<<endl;
	database(sWorkingDir,fThScoreScore,fMinReliability,fTaoCenters);
	return 0;
}
int main_allDB(int argc, char** argv)
{
	string sWorkingDirPath=SWORKINGDIR;
	string sWorkingDirName[3] = {"PrasadImages-DatasetPrasad","RandomImages-Dataset#1","Dataset#2"};
	float afThScoreScore[6]={0.3f,0.4f,0.5f,0.6f,0.7f,0.8f};//0-5
	float afMinReliability[6]={0.3f,0.4f,0.5f,0.6f,0.7f,0.8f};//0-5
	float afTaoCenters[6]={0.01f,0.02f,0.03f,0.04f,0.05f,0.6f};//0-5
	int iDir=0;
	int iScore=0;
	int iReliability=0;
	int iTaoCenter=0;
	string sWorkingDir;

	float	fThScoreScore = 0.5f;	//0.8
	float	fMinReliability	= 0.50f;	// Const parameters to discard bad ellipses 0.4
	float	fTaoCenters = 0.05f;//0.05 
	string oldname="dataset.csv";
	string newname="";

	vector<double> times;
	char oneTimeName[100];
	
	char cnewDir[100];
	sprintf(cnewDir,"normal/");
	string newDir=cnewDir;
	_mkdir(cnewDir);
	vector<string> resultString;
	resultString.push_back("iDir,fThScoreScore,fMinReliability,fTaoCenters,Edge Detection,Pre processing,Grouping,Estimation,Validation,Clustering,WholeTime,F-measure,countsOfFindEllipse,countsOfGetFastCenter");

	for(iDir=0;iDir<3;iDir++){
		sWorkingDir=sWorkingDirPath+sWorkingDirName[iDir];
		for(iScore=0;iScore<6;iScore++){
			fThScoreScore = afThScoreScore[iScore];
			for(iReliability=0;iReliability<6;iReliability++){
				system("cls");
				fMinReliability = afMinReliability[iReliability];
				for(iTaoCenter=0;iTaoCenter<6;iTaoCenter++){
					fTaoCenters = afTaoCenters[iTaoCenter];
					times=database(sWorkingDir,fThScoreScore,fMinReliability,fTaoCenters);
					double wholetime=accumulate(times.begin(),times.begin()+6,0.0);
					sprintf(oneTimeName,"our_%s_%4.03f_%4.03f_%4.03f.csv",sWorkingDirName[iDir].c_str(),fThScoreScore,fMinReliability,fTaoCenters);
					newname=newDir+oneTimeName;
					rename( oldname.c_str(),newname.c_str());
					cout<<newname<<endl;
					stringstream resultsitem;

					resultsitem<<iDir<<","<<fThScoreScore<<","<<fMinReliability<<","<<fTaoCenters<<","
						<<times[0]<<","<<times[1]<<","<<times[2]<<","<<times[3]<<","<<times[4]<<","<<times[5]<<","
						<<wholetime<<","<<times[6]<<","<<times[7]<<","<<times[8];
					resultString.push_back(resultsitem.str());
				}
			}
		}
	}
	
	writeFile(newDir+"our_allaverage.csv",resultString);
	return 0;
}
//salt 
vector<double> OnImage_salt(string sWorkingDir,string imagename,int saltrate,float fThScoreScore,float fMinReliability,float fTaoCenters,bool showpic)
{
	CNEllipseDetector cned;

	string filename = sWorkingDir + "/images/" + imagename;
	// Read image
	Mat3b image = imread(filename);
	Size sz = image.size();
	int n=sz.width*sz.height;
	
	// Convert to grayscale
	//medianBlur(image.clone(),image,3);

	// gray
	Mat1b gray;
	cvtColor(image, gray, CV_BGR2GRAY);
	//salt 添加
	salt(gray, n*saltrate/100);
	medianBlur(gray,gray,3);
	// Parameters Settings (Sect. 4.2)
	int		iThLength = 16;//过滤太短的弧
	float	fThObb = 3.0f;
	float	fThPos = 1.0f;
	//float	fTaoCenters = 0.018f;//0.05
	int 	iNs = 16;//弦数
	float	fMaxCenterDistance = sqrt(float(sz.width*sz.width + sz.height*sz.height)) * fTaoCenters;

	//float	fThScoreScore = 0.5f;//0.8	
	//fTaoCenters = 0.05f;
	// Other constant parameters settings.
	// Gaussian filter parameters, in pre-processing
	Size	szPreProcessingGaussKernelSize	= Size(5,5);
	double	dPreProcessingGaussSigma		= 1.0;

	float	fDistanceToEllipseContour		= 0.1f;	// (Sect. 3.3.1 - Validation)
	//float	fMinReliability					= 0.4f;	// Const parameters to discard bad ellipses 0.5

	// Initialize Detector with selected parameters
	cned.SetParameters	(	szPreProcessingGaussKernelSize,	
		dPreProcessingGaussSigma,		
		fThPos,
		fMaxCenterDistance,
		iThLength,
		fThObb,
		fDistanceToEllipseContour,		
		fThScoreScore,
		fMinReliability,		
		iNs
		);
	// Detect
	vector<Ellipse> ellsCned;
	Mat1b gray_clone=gray.clone();
	cned.Detect(gray_clone, ellsCned);
	vector<double> times = cned.GetTimes();
	// 为方便返回数据，将评价2-tp_fn_f的值放在times数组后面。
	vector<double> fm_and_tp_fn_f = showT(sWorkingDir, imagename, cned, ellsCned, 0.8f, showpic);
	times.push_back(fm_and_tp_fn_f[3]);
	times.push_back(cned.countsOfFindEllipse);
	times.push_back(cned.countsOfGetFastCenter);
	times.push_back(fm_and_tp_fn_f[0]);
	times.push_back(fm_and_tp_fn_f[1]);
	times.push_back(fm_and_tp_fn_f[2]);
	/*double fmeasure = showT(sWorkingDir,imagename, cned,ellsCned,0.8f,showpic);
	times.push_back(fmeasure);
	times.push_back(cned.countsOfFindEllipse);
	times.push_back(cned.countsOfGetFastCenter);*/
	return times;
}
vector<double> database_salt(string sWorkingDir,int saltrate,float fThScoreScore,float fMinReliability,float fTaoCenters){
	vector<string> resultString;
	resultString.push_back("picName,Edge Detection,Pre processing,Grouping,Estimation,Validation,Clustering,WholeTime,F-measure,countsOfFindEllipse,countsOfGetFastCenter");
	vector<double> allTimes(9,0.0);
	string dirName=sWorkingDir+"/images/";
	string imageName;
	double countsOfFindEllipse=0;
	double countsOfGetFastCenter=0;
	vector<string> files;
	listDir(dirName,files);

	imageName=files.at(1);
	vector<double> results=OnImage(sWorkingDir,imageName,fThScoreScore,fMinReliability,fTaoCenters);
	double fmeasure = results[6];
	vector<double> times = results;
	double wholetime=accumulate(times.begin(),times.begin()+6,0.0);
	fmeasure=0.0;
	for(unsigned int i=0;i<files.size();i++)
	{
		stringstream resultsitem;
		imageName=files.at(i);
		results=OnImage_salt(sWorkingDir,imageName,saltrate,fThScoreScore,fMinReliability,fTaoCenters);
		fmeasure += results[6];
		countsOfFindEllipse+=results[7];
		countsOfGetFastCenter+=results[8];
		times = results;
		wholetime=accumulate(times.begin(),times.begin()+6,0.0);

		for (unsigned int j=0;j<allTimes.size();j++){
			allTimes[j]+=times[j];
		}
		resultsitem<<imageName<<","<<times[0]<<","<<times[1]<<","<<times[2]<<","<<times[3]<<","<<times[4]<<","<<times[5]<<","<<wholetime<<","<<times[6]<<","<<times[7]<<","<<times[8];
		resultString.push_back(resultsitem.str());
		//if((i+1)%10==0){cout<<endl;}//10个数据一个回车
	}
	for (unsigned int i=0;i<allTimes.size();i++){
		times[i]=allTimes[i]/(1.0*files.size());
	}
	wholetime=accumulate(times.begin(),times.begin()+6,0.0);
	fmeasure=fmeasure/(1.0*files.size());
	cout << "--------------------------------" << endl;
	cout << "DataSet:        \t"<<sWorkingDir<<endl;
	cout << "Execution Time" << endl;
	cout << "Edge Detection: \t" << times[0] << endl;
	cout << "Pre processing: \t" << times[1] << endl;
	cout << "Grouping:       \t" << times[2] << endl;
	cout << "Estimation:     \t" << times[3] << endl;
	cout << "Validation:     \t" << times[4] << endl;
	cout << "Clustering:     \t" << times[5] << endl;
	cout << "--------------------------------" << endl;
	cout << "Total:	         \t" << accumulate(times.begin(),times.begin()+6,0.0) << endl;
	cout << "F-Measure: " << fmeasure << endl;
	cout << "--------------------------------" << endl;
	cout << "countsOfFindEllipse \t"<<times[7]<<endl;
	cout << "countsOfGetFastCenter \t"<<times[8]<<endl;
	stringstream resultsitem;
	resultsitem<<"average"<<","<<times[0]<<","<<times[1]<<","<<times[2]<<","<<times[3]<<","<<times[4]<<","<<times[5]<<","<<wholetime<<","<<times[6]<<","<<times[7]<<","<<times[8];
	resultString.push_back(resultsitem.str());
	writeFile("dataset.csv",resultString);
	return times;
}
int main_salt(int argc, char** argv)
{//salt
	string sWorkingDirPath=SWORKINGDIR;
	string sWorkingDirName[3] = {"PrasadImages-DatasetPrasad","RandomImages-Dataset#1","Dataset#2"};
	//string sWorkingDirName[3] = {"good2"};
	float afThScoreScore[6]={0.3f,0.4f,0.5f,0.6f,0.7f,0.8f};//0-5
	float afMinReliability[6]={0.3f,0.4f,0.5f,0.6f,0.7f,0.8f};//0-5
	float afTaoCenters[6]={0.01f,0.02f,0.03f,0.04f,0.05f,0.6f};//0-5
	int saltrates[6]={3,6,9,12,15,18};//0-5
	int iDir=0;
	int iScore=0;
	int iReliability=0;
	int iTaoCenter=0;
	int isaltrate=0;
	string sWorkingDir;
	sWorkingDir=sWorkingDirPath+sWorkingDirName[iDir];


	float	fThScoreScore = 0.5f;	//0.8
	float	fMinReliability	= 0.50f;	// Const parameters to discard bad ellipses 0.4
	float	fTaoCenters = 0.05f;//0.05 
	string oldname="dataset.csv";
	string newname="";

	vector<double> times;
	char oneTimeName[100];
	char partDataName[50];
	for(isaltrate=0;isaltrate<6;isaltrate++){
		int saltrate=saltrates[isaltrate];
		char cnewDir[20];
		sprintf(cnewDir,"%02d/",saltrate);
		string newDir=cnewDir;
		_mkdir(cnewDir);
		vector<string> resultString;
		resultString.push_back("iDir,fThScoreScore,fMinReliability,fTaoCenters,Edge Detection,Pre processing,Grouping,Estimation,Validation,Clustering,Totaltimes,F-measure,countsOfFindEllipse,countsOfGetFastCenter");
		for(iDir=0;iDir<3;iDir++){
			sWorkingDir=sWorkingDirPath+sWorkingDirName[iDir];
			for(iScore=0;iScore<6;iScore++){
				fThScoreScore = afThScoreScore[iScore];
				for(iReliability=0;iReliability<6;iReliability++){
					fMinReliability = afMinReliability[iReliability]; 
					for(iTaoCenter=0;iTaoCenter<6;iTaoCenter++){
						fTaoCenters = afTaoCenters[iTaoCenter];
						times=database_salt(sWorkingDir,saltrate,fThScoreScore,fMinReliability,fTaoCenters);
						double wholetime=accumulate(times.begin(),times.begin()+6,0.0);
						sprintf(oneTimeName,"our_%s_%02d_%4.03f_%4.03f_%4.03f.csv",sWorkingDirName[iDir].c_str(),saltrate,fThScoreScore,fMinReliability,fTaoCenters);
						newname=newDir+oneTimeName;
						rename( oldname.c_str(),newname.c_str());
						cout<<newname<<endl;
						stringstream resultsitem;
						resultsitem<<iDir<<","<<fThScoreScore<<","<<fMinReliability<<","<<fTaoCenters<<","
							<<times[0]<<","<<times[1]<<","<<times[2]<<","<<times[3]<<","<<times[4]<<","<<times[5]<<","
							<<wholetime<<","<<times[6]<<","<<times[7]<<","<<times[8];
						resultString.push_back(resultsitem.str());
					}
				}
			}
		}
		sprintf(partDataName,"%02d/our_%02d_allaverage.csv",saltrate,saltrate);
		writeFile(partDataName,resultString);
	}
	return 0;
}

vector<double> detect4s(string sWorkingDir, string imageName){
	vector<double>  Counts;
	string wholeImageName=sWorkingDir+"/images/"+imageName;

	bool showpic=false;

	//show edges
	Mat1b gray;
	cvtColor(imread(wholeImageName), gray, CV_BGR2GRAY);
	CNEllipseDetector cned;
	int		iThLength = ThLength;//过滤太短的弧
	float	fThPos = 1.0f;
	float	fThObb = MinOrientedRectSide;

	float	fMaxCenterDistance = 10;
	int 	iNs = 16;//弦数

	Size	szPreProcessingGaussKernelSize	= Size(5,5);
	double	dPreProcessingGaussSigma		= 1.0;

	float	fDistanceToEllipseContour		= 0.1f;	// (Sect. 3.3.1 - Validation)

	// Initialize Detector with selected parameters
	cned.SetParameters	(	szPreProcessingGaussKernelSize,	
		dPreProcessingGaussSigma,		
		fThPos,
		fMaxCenterDistance,
		iThLength,
		fThObb,
		fDistanceToEllipseContour,		
		fThScoreScore,
		fMinReliability,		
		iNs
		);

	Mat1b gray_clone=gray.clone();
	int EdgeNumber=cned.showEdgeInPic(gray_clone,showpic);
	Counts.push_back(EdgeNumber);

	vector<double> results;
	// 用于防止缓存影响速度
	myselect1=false;myselect2=false;myselect3=false;
	results=OnImage(sWorkingDir,imageName,fThScoreScore,fMinReliability,fTaoCenters);
	double fmeasure = results[6];
	double wholetime=accumulate(results.begin(),results.begin()+6,0.0);
	// 4个
	myselect1=true;myselect2=false;myselect3=false;
	results=OnImage(sWorkingDir,imageName,fThScoreScore,fMinReliability,fTaoCenters);
	fmeasure = results[6];
	wholetime=accumulate(results.begin(),results.begin()+6,0.0);
	Counts.push_back(results[7]);
	Counts.push_back(results[8]);
	Counts.push_back(wholetime);
	Counts.push_back(fmeasure);

	myselect1=true;myselect2=true;myselect3=false;
	results=OnImage(sWorkingDir,imageName,fThScoreScore,fMinReliability,fTaoCenters);
	fmeasure = results[6];
	wholetime=accumulate(results.begin(),results.begin()+6,0.0);
	Counts.push_back(results[7]);
	Counts.push_back(results[8]);
	Counts.push_back(wholetime);
	Counts.push_back(fmeasure);

	myselect1=true;myselect2=true;myselect3=true;
	results=OnImage(sWorkingDir,imageName,fThScoreScore,fMinReliability,fTaoCenters);
	fmeasure = results[6];
	wholetime=accumulate(results.begin(),results.begin()+6,0.0);
	Counts.push_back(results[7]);
	Counts.push_back(results[8]);
	Counts.push_back(wholetime);
	Counts.push_back(fmeasure);

	myselect1=false;myselect2=false;myselect3=false;
	results=OnImage(sWorkingDir,imageName,fThScoreScore,fMinReliability,fTaoCenters);
	fmeasure = results[6];
	wholetime=accumulate(results.begin(),results.begin()+6,0.0);
	Counts.push_back(results[7]);
	Counts.push_back(results[8]);
	Counts.push_back(wholetime);
	Counts.push_back(fmeasure);

	return Counts;
}
vector<double> database_4s(string sWorkingDir){
	vector<string> resultString;
	resultString.push_back("ImageName,EdgeNumber,cOFE1,cOGFC1,select100Time,F-m1,cOFE2,cOGFC2,select110Time,F-m2,cOFE3,cOGFC3,select111Time,F-m3,cOFE,cOGFC,select000Time,F-m");

	string dirName=sWorkingDir+"/images/";
	vector<string> files;
	listDir(dirName,files);
	string imageName;

	imageName=files.at(1);
	vector<double> Counts=detect4s(sWorkingDir,imageName);
	vector<double> AllCounts(17,0.0);
	for(unsigned int i=0;i<files.size();i++)
	{
		imageName=files.at(i);
		Counts=detect4s(sWorkingDir,imageName);
		stringstream resultsitem;
		resultsitem<<imageName<<","<<Counts[0]
			<<","<<Counts[1]<<","<<Counts[2]<<","<<Counts[3]<<","<<Counts[4]
			<<","<<Counts[5]<<","<<Counts[6]<<","<<Counts[7]<<","<<Counts[8]
			<<","<<Counts[9]<<","<<Counts[10]<<","<<Counts[11]<<","<<Counts[12]
			<<","<<Counts[13]<<","<<Counts[14]<<","<<Counts[15]<<","<<Counts[16];
		resultString.push_back(resultsitem.str());
		
		for (unsigned int j=0;j<AllCounts.size();j++){
			AllCounts[j]+=Counts[j];
		}
	}
	for (unsigned int j=0;j<AllCounts.size();j++){
		Counts[j]=AllCounts[j]/(files.size());
	}
	cout << "--------------------------------" << endl;
	cout << "--------------------------------" << endl;
	cout << "DataSet:        \t"<<sWorkingDir<<endl;
	cout<<"Edge Number:\t"<<Counts[0]<<endl;
	cout<<"tTCNl:\t"<<tTCNl<<endl;
	cout<<"tCNC:\t"<<tCNC<<endl;
	cout << "--------------------------------" << endl;
	cout<<"countsOfFindEllipse:\t"<<Counts[1]<<endl;
	cout<<"countsOfGetFastCenter\t"<<Counts[2]<<endl<<endl;
	cout<<"exec_time1:\t\t"<<Counts[3]<<endl;
	cout<<"F-M 1:\t"<<Counts[4]<<endl;
	cout << "--------------------------------" << endl;
	cout<<"countsOfFindEllipse:\t"<<Counts[5]<<endl;
	cout<<"countsOfGetFastCenter\t"<<Counts[6]<<endl<<endl;
	cout<<"exec_time2:\t\t"<<Counts[7]<<endl;
	cout<<"F-M 2:\t"<<Counts[8]<<endl;
	cout << "--------------------------------" << endl;
	cout<<"countsOfFindEllipse:\t"<<Counts[9]<<endl;
	cout<<"countsOfGetFastCenter\t"<<Counts[10]<<endl<<endl;
	cout<<"exec_time3:\t\t"<<Counts[11]<<endl;
	cout<<"F-M 3:\t"<<Counts[12]<<endl;
	cout << "--------------------------------" << endl;
	cout<<"countsOfFindEllipse:\t"<<Counts[13]<<endl;
	cout<<"countsOfGetFastCenter\t"<<Counts[14]<<endl<<endl;
	cout<<"exec_time:\t\t"<<Counts[15]<<endl;
	cout<<"F-M :\t"<<Counts[16]<<endl;
	cout << "--------------------------------" << endl;
	cout<<"rate:\t"<<Counts[3]/Counts[15]<<"\t"<<Counts[7]/Counts[15]
	<<"\t"<<Counts[11]/Counts[15]<<endl;
	stringstream resultsitem;
	resultsitem<<"average"<<","<<Counts[0]
			<<","<<Counts[1]<<","<<Counts[2]<<","<<Counts[3]<<","<<Counts[4]
			<<","<<Counts[5]<<","<<Counts[6]<<","<<Counts[7]<<","<<Counts[8]
			<<","<<Counts[9]<<","<<Counts[10]<<","<<Counts[11]<<","<<Counts[12]
			<<","<<Counts[13]<<","<<Counts[14]<<","<<Counts[15]<<","<<Counts[16];
	resultString.push_back(resultsitem.str());
	writeFile("dataset.csv",resultString);
	return Counts;
}
int main_TCN()
{// 遍历tTCNl
	string sWorkingDirPath=SWORKINGDIR;
	string sWorkingDirName[3] = {"PrasadImages-DatasetPrasad","RandomImages-Dataset#1","good2"};
	
	//float tTCN[6]={0,1.0f/64,2.0f/64,4.0f/64,6.0f/64,8.0f/64};
	float tTCN[6]={0,1,2,3,4,5};
	vector<double>  Counts;

	int iDir=0;
	string sWorkingDir;
	sWorkingDir=sWorkingDirPath+sWorkingDirName[iDir];

	float	fThScoreScore = 0.5f;	//0.8
	float	fMinReliability	= 0.50f;	// Const parameters to discard bad ellipses 0.4
	float	fTaoCenters = 0.05f;//0.05 
	string oldname="dataset.csv";
	string newname="";

	vector<double> times;
	
	char oneTimeName[100];
	vector<string> resultString;
	resultString.push_back("iDir,tTCNl,EdgeNumber,select100Time,cOFE1,cOGFC1,F-m1,select110Time,cOFE2,cOGFC2,F-m2,select111Time,cOFE3,cOGFC3,F-m3,select000Time,cOFE,cOGFC,F-m");
	MinOrientedRectSide=0;
	for(iDir=0;iDir<3;iDir++){
		for(int itTCN=0;itTCN<6;itTCN++){
			tTCNl=tTCN[itTCN];
			MinOrientedRectSide=tTCNl;
			cout<<tTCNl<<endl;
			sWorkingDir=sWorkingDirPath+sWorkingDirName[iDir];
			Counts=database_4s(sWorkingDir);
			sprintf(oneTimeName,"EdgeCompare_%s_%4.03f.csv",sWorkingDirName[iDir].c_str(),tTCNl);
			newname=oneTimeName;
			rename( oldname.c_str(),newname.c_str());
			stringstream resultsitem;
			resultsitem<<iDir<<","<<tTCNl<<","<<Counts[0]
				<<","<<Counts[1]<<","<<Counts[2]<<","<<Counts[3]<<","<<Counts[4]
				<<","<<Counts[5]<<","<<Counts[6]<<","<<Counts[7]<<","<<Counts[8]
				<<","<<Counts[9]<<","<<Counts[10]<<","<<Counts[11]<<","<<Counts[12]
				<<","<<Counts[13]<<","<<Counts[14]<<","<<Counts[15]<<","<<Counts[16];
			resultString.push_back(resultsitem.str());
		}
		
	}
	
	writeFile("our_allEdgeCompareaverage.csv",resultString);
	return 0;
}
int main_CNC(){
	string sWorkingDirPath=SWORKINGDIR;
	string sWorkingDirName[3] = {"PrasadImages-DatasetPrasad","RandomImages-Dataset#1","Dataset#2"};
	
	float tCNCs[21]={0,0.1f,0.2f,0.3f,0.4f,0.5f,0.6f,1,2,3,4,5,10,20,30,40,50,100,200,500,1000};
	vector<double>  Counts;

	int iDir=0;
	string sWorkingDir;
	sWorkingDir=sWorkingDirPath+sWorkingDirName[iDir];
	string oldname="dataset.csv";
	string newname="";

	vector<double> times;
	
	char oneTimeName[100];
	vector<string> resultString;
	resultString.push_back("iDir,tTCNl,EdgeNumber,cOFE1,cOGFC1,select100Time,F-m1,cOFE2,cOGFC2,select110Time,F-m2,cOFE3,cOGFC3,select111Time,F-m3,cOFE,cOGFC,select000Time,F-m");
	MinOrientedRectSide=0;
	for(iDir=0;iDir<3;iDir++){
		for(int itCNCs=0;itCNCs<21;itCNCs++){
			tCNC=tCNCs[itCNCs];
			cout<<tCNC<<endl;
			sWorkingDir=sWorkingDirPath+sWorkingDirName[iDir];
			Counts=database_4s(sWorkingDir);
			sprintf(oneTimeName,"EdgeCompare_CNC_%s_%4.03f.csv",sWorkingDirName[iDir].c_str(),tCNC);
			newname=oneTimeName;
			rename( oldname.c_str(),newname.c_str());
			stringstream resultsitem;
			resultsitem<<iDir<<","<<tCNC<<","<<Counts[0]
				<<","<<Counts[1]<<","<<Counts[2]<<","<<Counts[3]<<","<<Counts[4]
				<<","<<Counts[5]<<","<<Counts[6]<<","<<Counts[7]<<","<<Counts[8]
				<<","<<Counts[9]<<","<<Counts[10]<<","<<Counts[11]<<","<<Counts[12]
				<<","<<Counts[13]<<","<<Counts[14]<<","<<Counts[15]<<","<<Counts[16];
			resultString.push_back(resultsitem.str());
		}
		
	}
	
	writeFile("our_allEdgeCompareCNCaverage.csv",resultString);
	return 0;
}

/* 程序开始 */
int main(int argc, char** argv)
{
#if 0
	// 【效果不佳】用我们的评价方法，计算Arc代码实验结果的F-M值：根据测试圆和标准圆的五参数，算重叠程度=>F值。

	// 设置数据集的根路径和数据集名称
	string rootDir = "./../../../../DatasetTestResult/";
	string dbName = "Dataset#2";// "Dataset#2"/"DatasetPrasad"/"PCB"/"Traffic"
	// 遍历数据集中的测试结果result文件夹，得到检测椭圆集的五参数文件
	string testDir = rootDir + dbName + "/result/";
	vector<string> testFiles;
	listDir(testDir, testFiles);

	// 遍历test文件集，得到每个test参数文件对应的测试椭圆集ellTest，再和标准椭圆集ellGT比较，根据重叠区域比例=>F值。
	float FMeasure = 0.0f;
	string currImgName = "";
	for (unsigned int i = 0; i < testFiles.size(); i++)
	{
		// 根据第i个测试图片的参数文件(已是弧度制)，获取其测试椭圆集ellTest
		currImgName = testFiles.at(i);
		vector<Ellipse> ellTest;
		LoadGT(ellTest, testDir + currImgName, true);

		// 依第i个测试图对应的标准参数文件(只PCB为弧度制)，获取标准椭圆集ellGT
		vector<Ellipse> ellGT;
		if (dbName == "PCB" || dbName == "Industrial PCB Image Dataset")
		{
			LoadGT(ellGT, rootDir+dbName+"/gt/"+currImgName, true);
		}
		else {
			LoadGT(ellGT, rootDir+dbName+"/gt/gt_"+currImgName, false);
		}

		// 根据测试椭圆集ellTest和标准椭圆集ellGT对应的重叠区域比例，算F值
		currImgName = currImgName.substr(0, currImgName.length()-4);
		string filename = rootDir + dbName + "/images/" + currImgName;
		Mat3b image = imread(filename);
		Mat3b res = image.clone();
		// 将每个(第i个)测试图片的F值累计
		float tmpFM = Evaluate(ellGT, ellTest, fThScoreScore, res);
		cout << currImgName << " 's\tFM=" << tmpFM << endl;
		FMeasure = FMeasure + tmpFM;
	}

	// 算出最终F值：取平均数
	FMeasure = FMeasure / (1.0 * testFiles.size());
	cout << "result:\t" << FMeasure << endl;
#endif // 0

#if 1
	//tCNC=0.2f;
	fThScoreScore = 0.6f;	//0.8
	fMinReliability	= 0.4f;	// Const parameters to discard bad ellipses 0.4
	fTaoCenters = 0.04f;//0.05 	
	ThLength=16;//16
	MinOrientedRectSide=3.0f;
	if(argc==2){
		string filename= argv[1];
		if(NULL!=strstr(filename.c_str(),".jpg")||NULL!=strstr(filename.c_str(),".bmp")
			||NULL!=strstr(filename.c_str(),".JPG")||NULL!=strstr(filename.c_str(),".png")){	
			cout<<filename<<endl;
			vector<double> results=OnImage(filename,fThScoreScore,fMinReliability, fTaoCenters, true);
			if(!results.empty()){
				showTime(results);//显示运行信息
				waitKey(0);	cvDestroyWindow("Cned");
				MethodId=0;
				system("pause");
			}
		}
	}
	ReadParameter(argc,argv);
	//cout << MethodId << endl;
	switch(MethodId){
	case 1:main_OnePic();//单张图片
		break;
	case 2:main_OneDB();//单个数据库
		break;
	case 3:main_allDB(argc, argv);//论文中三个数据库，不同参数的实验
		break;
	case 4:main_salt(argc, argv);
		break;
	case 5:main_TCN();
		break;
	case 6:main_CNC();
		break;
	case 7:OnVideo();
		break;
	case 8:{
		main_allDB(argc, argv);

		tCNC=0.2f;ThLength=16;tTCNl=3;
		fThScoreScore = 0.7f;fMinReliability = 0.5f;fTaoCenters = 0.05f;//0.05 	
		MinOrientedRectSide=3.0f;
		main_salt(argc, argv);

		tCNC=0.2f;ThLength=16;tTCNl=3;
		fThScoreScore = 0.7f;fMinReliability = 0.5f;fTaoCenters = 0.05f;//0.05 	
		MinOrientedRectSide=3.0f;
		main_TCN();

		tCNC=0.2f;ThLength=16;tTCNl=3;
		fThScoreScore = 0.7f;fMinReliability = 0.5f;fTaoCenters = 0.05f;//0.05 	
		MinOrientedRectSide=3.0f;
		main_CNC();
		}break;
	default: break;
	}
	system("pause");
	//main_normalDB(argc, argv);//论文中三个数据库，不同参数的实验
	//OnVideo();//摄像头
	return 0;
#endif // 1
}

/* 代码改动：
1、line-28：更改测试对象为整个数据集而不是单个图片，MethodId=2。
2、line-33~38：调整最优化参数，fThScoreScore = 0.61f；fMinReliability = 0.4f；fTaoCenters = 0.05f；
3、line-74：若测试数据集为PCB，则showT()中调用loadGT()时的参数需要有对应改变；
4、line-101：3.X版本的OpenCV的cvSaveImage()函数和&IplImage(Mat xxx)有冲突，改成imwrite("", Mat xxx)；
5、line-105、387：测试整个数据集时，将其每个测试图片的测试结果也保存到结果图片和参数文件，并将dataset.cvs文件保存到数据集目录下；
6、line-954(main第一行)：main方法中把全局tCNC赋值注释，因为会重复赋值；
*/
