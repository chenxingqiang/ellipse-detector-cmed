/* CNED�������� */

/*
This code is intended for academic use only.
You are free to use and modify the code, at your own risk.

If you use this code, or find it useful, please refer to the paper:


The comments in the code refer to the abovementioned paper.
If you need further details about the code or the algorithm, please contact me at:

lianbosong@tguringai.cc

last update: 
*/

#include "CNEllipseDetector.h"


/* ��Բ����������(��ֵ-ʵ��������ֵ�Ĳ�ֵ)���ҵ��޶�����123��ֱ����ֵⷧ�������ɸѡ��ֵ */
bool myselect1=true;
bool myselect2 = false;
bool myselect3 = false;	// TODO��select�������ǿ�����������Ϊfalse��Ϊ�˷���Լ����
float tTCNl = 0.05f;//����ֱ̫������
//float tCNC=0.3f;
float tCNC = 2.97f;	// ԭ:0.34f~0.47f; 
	// ��a:100.36f; // ��b:0.52f; // ��c1:20.26f~20.57f; // ��c2:2.97f;

// �洢ѡ�����ǰ�ĺϸ�Ļ�
Mat DPDN_temp_only_wan_arc;


CNEllipseDetector::CNEllipseDetector(void) : _times(6, 0.0), _timesHelper(6, 0.0)
{
	// Default Parameters Settings
	_szPreProcessingGaussKernelSize = Size(5, 5);
	_dPreProcessingGaussSigma = 1.0;
	_fThPosition = 1.0f;
	_fMaxCenterDistance = 100.0f * 0.05f;
	_fMaxCenterDistance2 = _fMaxCenterDistance * _fMaxCenterDistance;
	_iMinEdgeLength = 16;
	_fMinOrientedRectSide = 3.0f;
	_fDistanceToEllipseContour = 0.1f;
	_fMinScore =0.7f;
	_fMinReliability =0.5;
	_uNs = 16;

	srand(unsigned(time(NULL)));
}


CNEllipseDetector::~CNEllipseDetector(void)
{
}

void CNEllipseDetector::SetParameters(Size	szPreProcessingGaussKernelSize,
										 double	dPreProcessingGaussSigma,
										 float 	fThPosition,
										 float	fMaxCenterDistance,
										 int		iMinEdgeLength,
										 float	fMinOrientedRectSide,
										 float	fDistanceToEllipseContour,
										 float	fMinScore,
										 float	fMinReliability,
										 int     iNs
										 )
{
	_szPreProcessingGaussKernelSize = szPreProcessingGaussKernelSize;
	_dPreProcessingGaussSigma = dPreProcessingGaussSigma;
	_fThPosition = fThPosition;
	_fMaxCenterDistance = fMaxCenterDistance;
	_iMinEdgeLength = iMinEdgeLength;
	_fMinOrientedRectSide = fMinOrientedRectSide;
	_fDistanceToEllipseContour = fDistanceToEllipseContour;
	_fMinScore = fMinScore;
	_fMinReliability = fMinReliability;
	_uNs = iNs;

	_fMaxCenterDistance2 = _fMaxCenterDistance * _fMaxCenterDistance;

}

uint inline CNEllipseDetector::GenerateKey(uchar pair, ushort u, ushort v)
{
	return (pair << 30) + (u << 15) + v;
};

int CNEllipseDetector::FindMaxK(const int* v) const
{
	int max_val = 0;
	int max_idx = 0;
	for (int i = 0; i<ACC_R_SIZE; ++i)
	{
		(v[i] > max_val) ? max_val = v[i], max_idx = i : NULL;
	}

	return max_idx + 90;
};

int CNEllipseDetector::FindMaxN(const int* v) const
{
	int max_val = 0;
	int max_idx = 0;
	for (int i = 0; i<ACC_N_SIZE; ++i)
	{
		(v[i] > max_val) ? max_val = v[i], max_idx = i : NULL;
	}

	return max_idx;
};

int CNEllipseDetector::FindMaxA(const int* v) const
{
	int max_val = 0;
	int max_idx = 0;
	for (int i = 0; i<ACC_A_SIZE; ++i)
	{
		(v[i] > max_val) ? max_val = v[i], max_idx = i : NULL;
	}

	return max_idx;
};

float CNEllipseDetector::GetMedianSlope(vector<Point2f>& med, Point2f& M, vector<float>& slopes)
{//input med slopes ;output:M return  
	// med		: vector of points  
	// M		: centroid of the points in med  
	// slopes	: vector of the slopes  

	unsigned iNofPoints = med.size();
	//CV_Assert(iNofPoints >= 2);

	unsigned halfSize = iNofPoints >> 1;
	unsigned quarterSize = halfSize >> 1;

	vector<float> xx, yy;
	slopes.reserve(halfSize);
	xx.reserve(iNofPoints);
	yy.reserve(iNofPoints);

	for (unsigned i = 0; i < halfSize; ++i)
	{
		Point2f& p1 = med[i];
		Point2f& p2 = med[halfSize + i];

		xx.push_back(p1.x);
		xx.push_back(p2.x);
		yy.push_back(p1.y);
		yy.push_back(p2.y);

		float den = (p2.x - p1.x);
		float num = (p2.y - p1.y);

		if (den == 0) den = 0.00001f;

		slopes.push_back(num / den);
	}

	nth_element(slopes.begin(), slopes.begin() + quarterSize, slopes.end());
	nth_element(xx.begin(), xx.begin() + halfSize, xx.end());
	nth_element(yy.begin(), yy.begin() + halfSize, yy.end());
	M.x = xx[halfSize];
	M.y = yy[halfSize];

	return slopes[quarterSize];
};


void CNEllipseDetector::GetFastCenter(vector<Point>& e1, vector<Point>& e2, EllipseData& data)
{
	countsOfGetFastCenter++;
	data.isValid = true;

	unsigned size_1 = unsigned(e1.size());
	unsigned size_2 = unsigned(e2.size());

	unsigned hsize_1 = size_1 >> 1;
	unsigned hsize_2 = size_2 >> 1;

	Point& med1 = e1[hsize_1];
	Point& med2 = e2[hsize_2];

	Point2f M12, M34;
	float q2, q4;

	{// First to second Reference slope
		float dx_ref = float(e1[0].x - med2.x);
		float dy_ref = float(e1[0].y - med2.y);

		if (dy_ref == 0) dy_ref = 0.00001f;

		float m_ref = dy_ref / dx_ref;
		data.ra = m_ref;

		// Find points with same slope as reference
		vector<Point2f> med;
		med.reserve(hsize_2);

		unsigned minPoints = (_uNs < hsize_2) ? _uNs : hsize_2;//ƽ���Ҹ���

		vector<uint> indexes(minPoints);
		if (_uNs < hsize_2)
		{//��������������趨������ƽ�����ￄ1�7
			unsigned iSzBin = hsize_2 / unsigned(_uNs);
			unsigned iIdx = hsize_2 + (iSzBin / 2);

			for (unsigned i = 0; i<_uNs; ++i)
			{
				indexes[i] = iIdx;
				iIdx += iSzBin;
			}
		}
		else
		{
			iota(indexes.begin(), indexes.end(), hsize_2);//ת��unsigned
		}
		for (uint ii = 0; ii<minPoints; ++ii)
		{//��2��ÿ�����ҵ���Ӧƽ���ҵ�
			uint i = indexes[ii];

			float x1 = float(e2[i].x);
			float y1 = float(e2[i].y);

			uint begin = 0;
			uint end = size_1 - 1;
			//һ�����뻡1�ϵĿ�ʼ��������ƽ��������
			float xb = float(e1[begin].x);
			float yb = float(e1[begin].y);
			float res_begin = ((xb - x1) * dy_ref) - ((yb - y1) * dx_ref);
			int sign_begin = sgn(res_begin);
			if (sign_begin == 0)
			{
				//found
				med.push_back(Point2f((xb + x1)* 0.5f, (yb + y1)* 0.5f));
				continue;
			}

			float xe = float(e1[end].x);
			float ye = float(e1[end].y);
			float res_end = ((xe - x1) * dy_ref) - ((ye - y1) * dx_ref);
			int sign_end = sgn(res_end);
			if (sign_end == 0)
			{
				//found
				med.push_back(Point2f((xe + x1)* 0.5f, (ye + y1)* 0.5f));
				continue;
			}
			//�����һ�ￄ1�7
			if ((sign_begin + sign_end) != 0)
			{//������û��ƽ�е���
				continue;
			}

			//���ַ�����ƽ����
			uint j = (begin + end) >> 1;
			while (end - begin > 2)
			{
				float x2 = float(e1[j].x);
				float y2 = float(e1[j].y);
				float res = ((x2 - x1) * dy_ref) - ((y2 - y1) * dx_ref);
				int sign_res = sgn(res);

				if (sign_res == 0)
				{
					//found
					med.push_back(Point2f((x2 + x1)* 0.5f, (y2 + y1)* 0.5f));
					break;
				}

				if (sign_res + sign_begin == 0)
				{
					sign_end = sign_res;
					end = j;
				}
				else
				{
					sign_begin = sign_res;
					begin = j;
				}
				j = (begin + end) >> 1;
			}
			//���ַ�����ƽ���� end �д�
			med.push_back(Point2f((e1[j].x + x1)* 0.5f, (e1[j].y + y1)* 0.5f));
		}

		if (med.size() < 2)
		{
			data.isValid = false;
			return;
		}

		q2 = GetMedianSlope(med, M12, data.Sa);//�õ�Sa ta=q2 Ma
	}

	{// Second to first
		// Reference slope
		float dx_ref = float(med1.x - e2[0].x);
		float dy_ref = float(med1.y - e2[0].y);

		if (dy_ref == 0) dy_ref = 0.00001f;

		float m_ref = dy_ref / dx_ref;
		data.rb = m_ref;

		// Find points with same slope as reference
		vector<Point2f> med;
		med.reserve(hsize_1);

		uint minPoints = (_uNs < hsize_1) ? _uNs : hsize_1;

		vector<uint> indexes(minPoints);
		if (_uNs < hsize_1)
		{
			unsigned iSzBin = hsize_1 / unsigned(_uNs);
			unsigned iIdx = hsize_1 + (iSzBin / 2);

			for (unsigned i = 0; i<_uNs; ++i)
			{
				indexes[i] = iIdx;
				iIdx += iSzBin;
			}
		}
		else
		{
			iota(indexes.begin(), indexes.end(), hsize_1);
		}


		for (uint ii = 0; ii<minPoints; ++ii)
		{
			uint i = indexes[ii];

			float x1 = float(e1[i].x);
			float y1 = float(e1[i].y);

			uint begin = 0;
			uint end = size_2 - 1;

			float xb = float(e2[begin].x);
			float yb = float(e2[begin].y);
			float res_begin = ((xb - x1) * dy_ref) - ((yb - y1) * dx_ref);
			int sign_begin = sgn(res_begin);
			if (sign_begin == 0)
			{
				//found
				med.push_back(Point2f((xb + x1)* 0.5f, (yb + y1)* 0.5f));
				continue;
			}

			float xe = float(e2[end].x);
			float ye = float(e2[end].y);
			float res_end = ((xe - x1) * dy_ref) - ((ye - y1) * dx_ref);
			int sign_end = sgn(res_end);
			if (sign_end == 0)
			{
				//found
				med.push_back(Point2f((xe + x1)* 0.5f, (ye + y1)* 0.5f));
				continue;
			}

			if ((sign_begin + sign_end) != 0)
			{
				continue;
			}

			uint j = (begin + end) >> 1;

			while (end - begin > 2)
			{
				float x2 = float(e2[j].x);
				float y2 = float(e2[j].y);
				float res = ((x2 - x1) * dy_ref) - ((y2 - y1) * dx_ref);
				int sign_res = sgn(res);

				if (sign_res == 0)
				{
					//found
					med.push_back(Point2f((x2 + x1)* 0.5f, (y2 + y1)* 0.5f));
					break;
				}

				if (sign_res + sign_begin == 0)
				{
					sign_end = sign_res;
					end = j;
				}
				else
				{
					sign_begin = sign_res;
					begin = j;
				}
				j = (begin + end) >> 1;
			}

			med.push_back(Point2f((e2[j].x + x1)* 0.5f, (e2[j].y + y1)* 0.5f));
		}

		if (med.size() < 2)
		{
			data.isValid = false;
			return;
		}
		q4 = GetMedianSlope(med, M34, data.Sb);
	}

	if (q2 == q4)
	{
		data.isValid = false;
		return;
	}

	float invDen = 1 / (q2 - q4);
	data.Cab.x = (M34.y - q4*M34.x - M12.y + q2*M12.x) * invDen;
	data.Cab.y = (q2*M34.y - q4*M12.y + q2*q4*(M12.x - M34.x)) * invDen;
	data.ta = q2;
	data.tb = q4;
	data.Ma = M12;
	data.Mb = M34;
};

#define	DISCARD_TCN2
void CNEllipseDetector::DetectEdges13(Mat1b& DP, VVP& points_1, VVP& points_3)
{
	// Vector of connected edge points
	VVP contours;
	int countedges=0;
	// Labeling 8-connected edge points, discarding edge too small
	Labeling(DP, contours, _iMinEdgeLength);//����һ�����ĵ�ŵ�һ�����򼯺��ￄ1�7
	int iContoursSize = int(contours.size());

	// For each edge
	for (int i = 0; i < iContoursSize; ++i)
	{
		VP& edgeSegment = contours[i];
#ifndef DISCARD_CONSTRAINT_OBOX

		// Selection strategy - Step 1 - See Sect [3.1.2] of the paper
		// Constraint on axes aspect ratio
		RotatedRect oriented = minAreaRect(edgeSegment);
		float o_min = min(oriented.size.width, oriented.size.height);

		if (o_min < _fMinOrientedRectSide)
		{
			countedges++;
			continue;
		}

		// ������ߵ����˵����������㹻��Ļ������������¼������13�����ڵĻ���
		/*for (int kl = 0; kl < edgeSegment.size(); kl++) {	// ������������ÿһ����
			DPDN_temp_only_wan_arc.at<uchar>(edgeSegment[kl].y, edgeSegment[kl].x) = 255;
		}*/

#endif
		// Order edge points of the same arc
		sort(edgeSegment.begin(), edgeSegment.end(), SortTopLeft2BottomRight);//�������ӵ�
		int iEdgeSegmentSize = unsigned(edgeSegment.size());

		// Get extrema of the arc
		Point& left = edgeSegment[0];
		Point& right = edgeSegment[iEdgeSegmentSize - 1];
#ifndef DISCARD_TCN
#ifndef DISCARD_TCN2
		int flag=0;
		for(int j=0;j<iEdgeSegmentSize;j++){
			Point& mid = edgeSegment[j];
			float data[] = {static_cast<float>(left.x), static_cast<float>(left.y), 1.0f, static_cast<float>(mid.x), static_cast<float>(mid.y), 1.0f, static_cast<float>(right.x), static_cast<float>(right.y), 1.0f};
			Mat threePoints(Size(3, 3), CV_32FC1, data);
			double ans = determinant(threePoints);

			float dx=1.0f*(left.x-right.x);
			float dy=1.0f*(left.y-right.y);
			float edgelength2=dx*dx+dy*dy;
			//double TCNl=ans/edgelength2;
			double TCNl=ans/(2*sqrt(edgelength2));
			if (abs(TCNl)>tTCNl){
				flag=1;
				break;
			}
		}
		if(0==flag){
			countedges++;
			continue;
		}
#endif
#ifndef DISCARD_TCN1
		Point& mid = edgeSegment[iEdgeSegmentSize/2];
		float data[] = {static_cast<float>(left.x), static_cast<float>(left.y), 1.0f, static_cast<float>(mid.x), static_cast<float>(mid.y), 1.0f, static_cast<float>(right.x), static_cast<float>(right.y), 1.0f};
		Mat threePoints(Size(3, 3), CV_32FC1, data);
		double ans = determinant(threePoints);

		float dx=1.0f*(left.x-right.x);
		float dy=1.0f*(left.y-right.y);
		float edgelength2=dx*dx+dy*dy;
		double TCNl=ans/edgelength2;
		//double TCNl=ans/(2*sqrt(edgelength2));
		if (abs(TCNl)<tTCNl){
			countedges++;
			continue;
		}
#endif
#endif
		// Find convexity - See Sect [3.1.3] of the paper
		int iCountTop = 0;
		int xx = left.x;
		for (int k = 1; k < iEdgeSegmentSize; ++k)
		{
			if (edgeSegment[k].x == xx) continue;

			iCountTop += (edgeSegment[k].y - left.y);
			xx = edgeSegment[k].x;
		}

		int width = abs(right.x - left.x) + 1;
		int height = abs(right.y - left.y) + 1;
		int iCountBottom = (width * height) - iEdgeSegmentSize - iCountTop;

		if (iCountBottom > iCountTop)
		{	//1
			points_1.push_back(edgeSegment);
		}
		else if (iCountBottom < iCountTop)
		{	//3
			points_3.push_back(edgeSegment);
		}
	}
};


void CNEllipseDetector::DetectEdges24(Mat1b& DN, VVP& points_2, VVP& points_4 )
{
	// Vector of connected edge points
	VVP contours;
	int countedges=0;
	/// Labeling 8-connected edge points, discarding edge too small
	Labeling(DN, contours, _iMinEdgeLength);

	int iContoursSize = unsigned(contours.size());

	// For each edge
	for (int i = 0; i < iContoursSize; ++i)
	{
		VP& edgeSegment = contours[i];

#ifndef DISCARD_CONSTRAINT_OBOX

		// Selection strategy - Step 1 - See Sect [3.1.2] of the paper
		// Constraint on axes aspect ratio
		RotatedRect oriented = minAreaRect(edgeSegment);
		float o_min = min(oriented.size.width, oriented.size.height);

		if (o_min < _fMinOrientedRectSide)
		{
			countedges++;
			continue;
		}

		// ������ߵ����˵����������㹻��Ļ������������¼������24�����ڵĻ���
		/*for (int kl = 0; kl < edgeSegment.size(); kl++) {	// ������������ÿһ����
			DPDN_temp_only_wan_arc.at<uchar>(edgeSegment[kl].y, edgeSegment[kl].x) = 255;
		}*/

#endif
		// Order edge points of the same arc
		sort(edgeSegment.begin(), edgeSegment.end(), SortBottomLeft2TopRight);
		int iEdgeSegmentSize = unsigned(edgeSegment.size());

		// Get extrema of the arc
		Point& left = edgeSegment[0];
		Point& right = edgeSegment[iEdgeSegmentSize - 1];
#ifndef DISCARD_TCN
#ifndef DISCARD_TCN2
		int flag=0;
		for(int j=0;j<iEdgeSegmentSize;j++){
			Point& mid = edgeSegment[j];
			float data[] = {static_cast<float>(left.x), static_cast<float>(left.y), 1.0f, static_cast<float>(mid.x), static_cast<float>(mid.y), 1.0f, static_cast<float>(right.x), static_cast<float>(right.y), 1.0f};
			Mat threePoints(Size(3, 3), CV_32FC1, data);
			double ans = determinant(threePoints);

			float dx=1.0f*(left.x-right.x);
			float dy=1.0f*(left.y-right.y);
			float edgelength2=dx*dx+dy*dy;
			//double TCNl=ans/edgelength2;
			double TCNl=ans/(2*sqrt(edgelength2));
			if (abs(TCNl)>tTCNl){
				flag=1;
				break;
			}
		}
		if(0==flag){
			countedges++;
			continue;
		}else{
		}
#endif
#ifndef DISCARD_TCN1
		Point& mid = edgeSegment[iEdgeSegmentSize/2];
		float data[] = {static_cast<float>(left.x), static_cast<float>(left.y), 1.0f, static_cast<float>(mid.x), static_cast<float>(mid.y), 1.0f, static_cast<float>(right.x), static_cast<float>(right.y), 1.0f};
		Mat threePoints(Size(3, 3), CV_32FC1, data);
		double ans = determinant(threePoints);

		float dx=1.0f*(left.x-right.x);
		float dy=1.0f*(left.y-right.y);
		float edgelength2=dx*dx+dy*dy;
		double TCNl=ans/edgelength2;
		//double TCNl=ans/(2*sqrt(edgelength2));
		if (abs(TCNl)<tTCNl){
			countedges++;
			continue;
		}
#endif
#endif
		// Find convexity - See Sect [3.1.3] of the paper
		//�жϰ�͹��
		int iCountBottom = 0;
		int xx = left.x;
		for (int k = 1; k < iEdgeSegmentSize; ++k)
		{
			if (edgeSegment[k].x == xx) continue;

			iCountBottom += (left.y - edgeSegment[k].y);
			xx = edgeSegment[k].x;
		}

		int width = abs(right.x - left.x) + 1;
		int height = abs(right.y - left.y) + 1;
		int iCountTop = (width *height) - iEdgeSegmentSize - iCountBottom;

		if (iCountBottom > iCountTop)
		{
			//2
			points_2.push_back(edgeSegment);
		}
		else if (iCountBottom < iCountTop)
		{
			//4
			points_4.push_back(edgeSegment);
		}
	}
};

// Most important function for detecting ellipses. See Sect[3.2.3] of the paper
void CNEllipseDetector::FindEllipses(	Point2f& center,
										VP& edge_i,VP& edge_j,VP& edge_k,
										EllipseData& data_ij,EllipseData& data_ik,
										vector<Ellipse>& ellipses)
{//�������λ�������Բ������������õ�����Բ������׼ȷ���ￄ1�7
	countsOfFindEllipse++;
	// Find ellipse parameters

	// 0-initialize accumulators
	memset(accN, 0, sizeof(int)*ACC_N_SIZE);
	memset(accR, 0, sizeof(int)*ACC_R_SIZE);
	memset(accA, 0, sizeof(int)*ACC_A_SIZE);

	Tac(3); //estimation

	// Get size of the 4 vectors of slopes (2 pairs of arcs)
	int sz_ij1 = int(data_ij.Sa.size());
	int sz_ij2 = int(data_ij.Sb.size());
	int sz_ik1 = int(data_ik.Sa.size());
	int sz_ik2 = int(data_ik.Sb.size());

	// Get the size of the 3 arcs
	size_t sz_ei = edge_i.size();
	size_t sz_ej = edge_j.size();
	size_t sz_ek = edge_k.size();

	// Center of the estimated ellipse
	float a0 = center.x;
	float b0 = center.y;


	// Estimation of remaining parameters
	// Uses 4 combinations of parameters. See Table 1 and Sect [3.2.3] of the paper.
	//ij1 and ik
	{
		float q1 = data_ij.ra;
		float q3 = data_ik.ra;
		float q5 = data_ik.rb;

		for (int ij1 = 0; ij1 < sz_ij1; ++ij1)
		{
			float q2 = data_ij.Sa[ij1];//�б�Ҫ������

			float q1xq2 = q1*q2;
			// ij1 and ik1
			for (int ik1 = 0; ik1 < sz_ik1; ++ik1)
			{
				float q4 = data_ik.Sa[ik1];//�б�Ҫ������

				float q3xq4 = q3*q4;

				// See Eq. [13-18] in the paper

				float a = (q1xq2 - q3xq4);//gama
				float b = (q3xq4 + 1)*(q1 + q2) - (q1xq2 + 1)*(q3 + q4);//beta
				float Kp = (-b + sqrt(b*b + 4 * a*a)) / (2 * a);//K+
				float zplus = ((q1 - Kp)*(q2 - Kp)) / ((1 + q1*Kp)*(1 + q2*Kp));
				// �ж���Ч��  zplus ��K�������Թ�ϵ
				if (zplus >= 0.0f) continue;

				float Np = sqrt(-zplus);//N+
				float rho = atan(Kp);//rho tmp
				int rhoDeg;
				if (Np > 1.f)
				{
					Np = 1.f / Np;
					rhoDeg = cvRound((rho * 180 / CV_PI) + 180) % 180; // [0,180)					
				}
				else
				{
					rhoDeg = cvRound((rho * 180 / CV_PI) + 90) % 180; // [0,180)//rho �Ƕȱ�ʾ����һ��
				}

				int iNp = cvRound(Np * 100); // [0, 100]ȡ��

				if (0 <= iNp	&& iNp < ACC_N_SIZE &&
					0 <= rhoDeg	&& rhoDeg < ACC_R_SIZE
					)
				{//Ϊʲô��������ֻ����һ��������  ��Ϊ zplus ��K�������Թ�ϵ
					++accN[iNp];	// Increment N accumulator
					++accR[rhoDeg];	// Increment R accumulator
				}
			}
			// ij1 and ik2
			for (int ik2 = 0; ik2 < sz_ik2; ++ik2)
			{
				float q4 = data_ik.Sb[ik2];

				float q5xq4 = q5*q4;

				// See Eq. [13-18] in the paper

				float a = (q1xq2 - q5xq4);
				float b = (q5xq4 + 1)*(q1 + q2) - (q1xq2 + 1)*(q5 + q4);
				float Kp = (-b + sqrt(b*b + 4 * a*a)) / (2 * a);
				float zplus = ((q1 - Kp)*(q2 - Kp)) / ((1 + q1*Kp)*(1 + q2*Kp));

				if (zplus >= 0.0f)
				{
					continue;
				}

				float Np = sqrt(-zplus);
				float rho = atan(Kp);
				int rhoDeg;
				if (Np > 1.f)
				{
					Np = 1.f / Np;
					rhoDeg = cvRound((rho * 180 / CV_PI) + 180) % 180; // [0,180)					
				}
				else
				{
					rhoDeg = cvRound((rho * 180 / CV_PI) + 90) % 180; // [0,180)
				}

				int iNp = cvRound(Np * 100); // [0, 100]

				if (0 <= iNp	&& iNp < ACC_N_SIZE &&
					0 <= rhoDeg	&& rhoDeg < ACC_R_SIZE
					)
				{
					++accN[iNp];		// Increment N accumulator
					++accR[rhoDeg];		// Increment R accumulator
				}
			}

		}
	}

	//ij2 and ik
	{
		float q1 = data_ij.rb;
		float q3 = data_ik.rb;
		float q5 = data_ik.ra;

		for (int ij2 = 0; ij2 < sz_ij2; ++ij2)
		{
			float q2 = data_ij.Sb[ij2];

			float q1xq2 = q1*q2;
			//ij2 and ik2
			for (int ik2 = 0; ik2 < sz_ik2; ++ik2)
			{
				float q4 = data_ik.Sb[ik2];

				float q3xq4 = q3*q4;

				// See Eq. [13-18] in the paper

				float a = (q1xq2 - q3xq4);
				float b = (q3xq4 + 1)*(q1 + q2) - (q1xq2 + 1)*(q3 + q4);
				float Kp = (-b + sqrt(b*b + 4 * a*a)) / (2 * a);
				float zplus = ((q1 - Kp)*(q2 - Kp)) / ((1 + q1*Kp)*(1 + q2*Kp));

				if (zplus >= 0.0f)
				{
					continue;
				}

				float Np = sqrt(-zplus);
				float rho = atan(Kp);
				int rhoDeg;
				if (Np > 1.f)
				{
					Np = 1.f / Np;
					rhoDeg = cvRound((rho * 180 / CV_PI) + 180) % 180; // [0,180)
				}
				else
				{
					rhoDeg = cvRound((rho * 180 / CV_PI) + 90) % 180; // [0,180)
				}

				int iNp = cvRound(Np * 100); // [0, 100]

				if (0 <= iNp	&& iNp < ACC_N_SIZE &&
					0 <= rhoDeg	&& rhoDeg < ACC_R_SIZE
					)
				{
					++accN[iNp];		// Increment N accumulator
					++accR[rhoDeg];		// Increment R accumulator
				}
			}

			//ij2 and ik1
			for (int ik1 = 0; ik1 < sz_ik1; ++ik1)
			{
				float q4 = data_ik.Sa[ik1];

				float q5xq4 = q5*q4;

				// See Eq. [13-18] in the paper

				float a = (q1xq2 - q5xq4);
				float b = (q5xq4 + 1)*(q1 + q2) - (q1xq2 + 1)*(q5 + q4);
				float Kp = (-b + sqrt(b*b + 4 * a*a)) / (2 * a);
				float zplus = ((q1 - Kp)*(q2 - Kp)) / ((1 + q1*Kp)*(1 + q2*Kp));

				if (zplus >= 0.0f)
				{
					continue;
				}

				float Np = sqrt(-zplus);
				float rho = atan(Kp);
				int rhoDeg;
				if (Np > 1.f)
				{
					Np = 1.f / Np;
					rhoDeg = cvRound((rho * 180 / CV_PI) + 180) % 180; // [0,180)
				}
				else
				{
					rhoDeg = cvRound((rho * 180 / CV_PI) + 90) % 180; // [0,180)
				}

				int iNp = cvRound(Np * 100); // [0, 100]

				if (0 <= iNp	&& iNp < ACC_N_SIZE &&
					0 <= rhoDeg	&& rhoDeg < ACC_R_SIZE
					)
				{
					++accN[iNp];		// Increment N accumulator
					++accR[rhoDeg];		// Increment R accumulator
				}
			}

		}
	}

	// Find peak in N and K accumulator
	int iN = FindMaxN(accN);
	int iK = FindMaxK(accR);

	// Recover real values
	float fK = float(iK);
	float Np = float(iN) * 0.01f;
	float rho = fK * float(CV_PI) / 180.f;	//deg 2 rad
	float Kp = tan(rho);

	// Estimate A. See Eq. [19 - 22] in Sect [3.2.3] of the paper  
	// ���λ��ϵĵ㶼��������A
	//�����Ż�   
	for (ushort l = 0; l < sz_ei; ++l)
	{
		Point& pp = edge_i[l];
		float sk = 1.f / sqrt(Kp*Kp + 1.f);//cos rho
		float x0 = ((pp.x - a0) * sk) + (((pp.y - b0)*Kp) * sk);//���Ż�
		float y0 = -(((pp.x - a0) * Kp) * sk) + ((pp.y - b0) * sk);//���Ż�
		float Ax = sqrt((x0*x0*Np*Np + y0*y0) / ((Np*Np)*(1.f + Kp*Kp)));
		int A = cvRound(abs(Ax / cos(rho)));//�����Ż�
		if ((0 <= A) && (A < ACC_A_SIZE))
		{
			++accA[A];
		}
	}

	for (ushort l = 0; l < sz_ej; ++l)
	{
		Point& pp = edge_j[l];
		float sk = 1.f / sqrt(Kp*Kp + 1.f);
		float x0 = ((pp.x - a0) * sk) + (((pp.y - b0)*Kp) * sk);
		float y0 = -(((pp.x - a0) * Kp) * sk) + ((pp.y - b0) * sk);
		float Ax = sqrt((x0*x0*Np*Np + y0*y0) / ((Np*Np)*(1.f + Kp*Kp)));
		int A = cvRound(abs(Ax / cos(rho)));
		if ((0 <= A) && (A < ACC_A_SIZE))
		{
			++accA[A];
		}
	}

	for (ushort l = 0; l < sz_ek; ++l)
	{
		Point& pp = edge_k[l];
		float sk = 1.f / sqrt(Kp*Kp + 1.f);
		float x0 = ((pp.x - a0) * sk) + (((pp.y - b0)*Kp) * sk);
		float y0 = -(((pp.x - a0) * Kp) * sk) + ((pp.y - b0) * sk);
		float Ax = sqrt((x0*x0*Np*Np + y0*y0) / ((Np*Np)*(1.f + Kp*Kp)));
		int A = cvRound(abs(Ax / cos(rho)));
		if ((0 <= A) && (A < ACC_A_SIZE))
		{
			++accA[A];
		}
	}

	// Find peak in A accumulator
	int A = FindMaxA(accA);
	float fA = float(A);

	// Find B value. See Eq [23] in the paper
	float fB = abs(fA * Np);

	// Got all ellipse parameters!
	Ellipse ell(a0, b0, fA, fB, fmod(rho + float(CV_PI)*2.f, float(CV_PI)));

	Toc(3); //estimation
	Tac(4); //validation

	// Get the score. See Sect [3.3.1] in the paper

	// Find the number of edge pixel lying on the ellipse
	float _cos = cos(-ell._rad);
	float _sin = sin(-ell._rad);

	float invA2 = 1.f / (ell._a * ell._a);
	float invB2 = 1.f / (ell._b * ell._b);

	float invNofPoints = 1.f / float(sz_ei + sz_ej + sz_ek);
	int counter_on_perimeter = 0;

	for (ushort l = 0; l < sz_ei; ++l)
	{
		float tx = float(edge_i[l].x) - ell._xc;
		float ty = float(edge_i[l].y) - ell._yc;
		float rx = (tx*_cos - ty*_sin);
		float ry = (tx*_sin + ty*_cos);

		float h = (rx*rx)*invA2 + (ry*ry)*invB2;
		if (abs(h - 1.f) < _fDistanceToEllipseContour)
		{
			++counter_on_perimeter;
		}
	}

	for (ushort l = 0; l < sz_ej; ++l)
	{
		float tx = float(edge_j[l].x) - ell._xc;
		float ty = float(edge_j[l].y) - ell._yc;
		float rx = (tx*_cos - ty*_sin);
		float ry = (tx*_sin + ty*_cos);

		float h = (rx*rx)*invA2 + (ry*ry)*invB2;
		if (abs(h - 1.f) < _fDistanceToEllipseContour)
		{
			++counter_on_perimeter;
		}
	}

	for (ushort l = 0; l < sz_ek; ++l)
	{
		float tx = float(edge_k[l].x) - ell._xc;
		float ty = float(edge_k[l].y) - ell._yc;
		float rx = (tx*_cos - ty*_sin);
		float ry = (tx*_sin + ty*_cos);

		float h = (rx*rx)*invA2 + (ry*ry)*invB2;
		if (abs(h - 1.f) < _fDistanceToEllipseContour)
		{
			++counter_on_perimeter;
		}
	}

	//no points found on the ellipse
	if (counter_on_perimeter <= 0)
	{
		Toc(4); //validation
		return;
	}


	// Compute score
	float score = float(counter_on_perimeter) * invNofPoints;
	if (score < _fMinScore)
	{
		Toc(4); //validation
		return;
	}

	// Compute reliability	
	// this metric is not described in the paper, mostly due to space limitations.
	// The main idea is that for a given ellipse (TD) even if the score is high, the arcs 
	// can cover only a small amount of the contour of the estimated ellipse. 
	// A low reliability indicate that the arcs form an elliptic shape by chance, but do not underlie
	// an actual ellipse. The value is normalized between 0 and 1. 
	// The default value is 0.4.

	// It is somehow similar to the "Angular Circumreference Ratio" saliency criteria 
	// as in the paper: 
	// D. K. Prasad, M. K. Leung, S.-Y. Cho, Edge curvature and convexity
	// based ellipse detection method, Pattern Recognition 45 (2012) 3204-3221.

	float di, dj, dk;
	{
		Point2f p1(float(edge_i[0].x), float(edge_i[0].y));
		Point2f p2(float(edge_i[sz_ei - 1].x), float(edge_i[sz_ei - 1].y));
		p1.x -= ell._xc;
		p1.y -= ell._yc;
		p2.x -= ell._xc;
		p2.y -= ell._yc;
		Point2f r1((p1.x*_cos - p1.y*_sin), (p1.x*_sin + p1.y*_cos));
		Point2f r2((p2.x*_cos - p2.y*_sin), (p2.x*_sin + p2.y*_cos));
		di = abs(r2.x - r1.x) + abs(r2.y - r1.y);
	}
	{
		Point2f p1(float(edge_j[0].x), float(edge_j[0].y));
		Point2f p2(float(edge_j[sz_ej - 1].x), float(edge_j[sz_ej - 1].y));
		p1.x -= ell._xc;
		p1.y -= ell._yc;
		p2.x -= ell._xc;
		p2.y -= ell._yc;
		Point2f r1((p1.x*_cos - p1.y*_sin), (p1.x*_sin + p1.y*_cos));
		Point2f r2((p2.x*_cos - p2.y*_sin), (p2.x*_sin + p2.y*_cos));
		dj = abs(r2.x - r1.x) + abs(r2.y - r1.y);
	}
	{
		Point2f p1(float(edge_k[0].x), float(edge_k[0].y));
		Point2f p2(float(edge_k[sz_ek - 1].x), float(edge_k[sz_ek - 1].y));
		p1.x -= ell._xc;
		p1.y -= ell._yc;
		p2.x -= ell._xc;
		p2.y -= ell._yc;
		Point2f r1((p1.x*_cos - p1.y*_sin), (p1.x*_sin + p1.y*_cos));
		Point2f r2((p2.x*_cos - p2.y*_sin), (p2.x*_sin + p2.y*_cos));
		dk = abs(r2.x - r1.x) + abs(r2.y - r1.y);
	}

	// This allows to get rid of thick edges
	float rel = min(1.f, ((di + dj + dk) / (3 * (ell._a + ell._b))));

	if (rel < _fMinReliability)
	{
		Toc(4); //validation
		return;
	}

	// Assign the new score!
	ell._score = (score + rel) * 0.5f;//need to change

	// The tentative detection has been confirmed. Save it!
	ellipses.push_back(ell);

	Toc(4); // Validation
};

// Get the coordinates of the center, given the intersection of the estimated lines. See Fig. [8] in Sect [3.2.3] in the paper.
Point2f CNEllipseDetector::GetCenterCoordinates(EllipseData& data_ij, EllipseData& data_ik)
{
	float xx[7];
	float yy[7];

	xx[0] = data_ij.Cab.x;
	xx[1] = data_ik.Cab.x;
	yy[0] = data_ij.Cab.y;
	yy[1] = data_ik.Cab.y;

	{
		//1-1
		float q2 = data_ij.ta;
		float q4 = data_ik.ta;
		Point2f& M12 = data_ij.Ma;
		Point2f& M34 = data_ik.Ma;

		float invDen = 1 / (q2 - q4);
		xx[2] = (M34.y - q4*M34.x - M12.y + q2*M12.x) * invDen;
		yy[2] = (q2*M34.y - q4*M12.y + q2*q4*(M12.x - M34.x)) * invDen;
	}

	{
		//1-2
		float q2 = data_ij.ta;
		float q4 = data_ik.tb;
		Point2f& M12 = data_ij.Ma;
		Point2f& M34 = data_ik.Mb;

		float invDen = 1 / (q2 - q4);
		xx[3] = (M34.y - q4*M34.x - M12.y + q2*M12.x) * invDen;
		yy[3] = (q2*M34.y - q4*M12.y + q2*q4*(M12.x - M34.x)) * invDen;
	}

	{
		//2-2
		float q2 = data_ij.tb;
		float q4 = data_ik.tb;
		Point2f& M12 = data_ij.Mb;
		Point2f& M34 = data_ik.Mb;

		float invDen = 1 / (q2 - q4);
		xx[4] = (M34.y - q4*M34.x - M12.y + q2*M12.x) * invDen;
		yy[4] = (q2*M34.y - q4*M12.y + q2*q4*(M12.x - M34.x)) * invDen;
	}

	{
		//2-1
		float q2 = data_ij.tb;
		float q4 = data_ik.ta;
		Point2f& M12 = data_ij.Mb;
		Point2f& M34 = data_ik.Ma;

		float invDen = 1 / (q2 - q4);
		xx[5] = (M34.y - q4*M34.x - M12.y + q2*M12.x) * invDen;
		yy[5] = (q2*M34.y - q4*M12.y + q2*q4*(M12.x - M34.x)) * invDen;
	}

	xx[6] = (xx[0] + xx[1]) * 0.5f;
	yy[6] = (yy[0] + yy[1]) * 0.5f;


	// Median
	nth_element(xx, xx + 3, xx + 7);
	nth_element(yy, yy + 3, yy + 7);
	float xc = xx[3];
	float yc = yy[3];

	return Point2f(xc, yc);
};


/* ����Բ�����ɸф1�7-������������ϣￄ1�7 */
//123456 124 80 48 246
//#define T124 pil,pim,pif,pjl,pjm,pjf // ��һ�еĶ�����ԭ���ġ����Ƽ�ʹ�ú궨�塷
#define T124 pjf,pjm,pjl,pif,pim,pil
#define T231 pil,pim,pif,pjf,pjm,pjl
#define T342 pif,pim,pil,pjf,pjm,pjl
#define T413 pif,pim,pil,pjl,pjm,pjf	
/* TODO��Ϊʲô������Ҫ��˳��(��������ӳ����й��߲���Ҫ˳�ￄ1�7)��ΪʲôV2SP������Ϊ(p3, p2, p1)-(p4, p5, p6)�� */

// VPΪ�㼯�ϣ�������һ���������ϵ�����аￄ1�7<����ҵķ��ￄ1�7>���У�VVPΪ�㼯�ϵļ��ϣ�������һ�����ļ��ϡ�
// unordered_map����HashMap����hashֵ�ж�Ԫ���Ƿ���ȣ��ڲ�Ԫ�����������HashMapǰ��֧��string���Ӷ�����keyԪ�ء�

/* �ֱ���124�����еĻ�����(pi, pj, pk)�и�ȡһ������Ϻ���м���Լ����ͬԲ�жϣ�������ɸѡ�����ļ����ѡ��ϵļ����С� */
// Verify triplets of arcs with convexity: i=1, j=2, k=4
void CNEllipseDetector::Triplets124(VVP& pi,
									   VVP& pj,
									   VVP& pk,
									   unordered_map<uint, EllipseData>& data,
									   vector<Ellipse>& ellipses
									   )
{
	/* �ֱ��ȡ��������ÿ�������ϵĴ�Є1�7 */
	// get arcs length(size)
	ushort sz_i = ushort(pi.size());
	ushort sz_j = ushort(pj.size());
	ushort sz_k = ushort(pk.size());

	/* 1-���ȱ����м����޵Ļ�����(1����-pi) */
	// For each edge i
	for (ushort i = 0; i < sz_i; ++i)
	{
		/* ȡ����ĳ����edge_i(arc_i) */
		VP& edge_i = pi[i];
		ushort sz_ei = ushort(edge_i.size());

		/* ��arc_i��ȡ���㣺��˵ￄ1�7+���е�+�Ҷ˵�(��x��С < ��x��ￄ1�7) */
		Point& pif = edge_i[0];
		Point& pim = edge_i[sz_ei/2];
		Point& pil = edge_i[sz_ei - 1];

		/* ����arc_i�ϵ����е�Ԫ�����򣬸�ֵ������rev_i�У�
		TODO������ΪʲôҪ��edge_i������rev_i������Բ�ģ� */
		// 1,2 -> reverse 1, swap
		VP rev_i(edge_i.size());
		reverse_copy(edge_i.begin(), edge_i.end(), rev_i.begin());

		/* 2-��α��������޵Ļ����ￄ1�7(2����-pj) */
		// For each edge j
		for (ushort j = 0; j < sz_j; ++j)
		{
			/* ȡ����ĳ����edge_j(arc_j) */
			VP& edge_j = pj[j];
			ushort sz_ej = ushort(edge_j.size());

			/* ��arc_j��ȡ���㣺��˵ￄ1�7+���е�+�Ҷ˵�(��x��С < ��x��ￄ1�7) */
			Point& pjf = edge_j[0];
			Point& pjm = edge_j[sz_ej/2];
			Point& pjl = edge_j[sz_ej - 1];

#ifndef DISCARD_CONSTRAINT_POSITION
			/* ����Լ����֤-1��2�����л��ϵ��x���� < 1�����л��ϵ��x���ꣻ */
			/* ��2.x > 1.x����������Լ��������������������һ�������жϡ�(1.x+1����΄1�7) */
			// CONSTRAINTS on position
			if (pjl.x > pif.x + _fThPosition) //is right	
			{
				//discard
				continue;
			}
#endif
/* ����������Ҫ�Ķ���value4SixPoints�����������������ж�ͬԲ�ķ�������Ч���Ƿ��������� */
#ifndef DISCARD_CONSTRAINT_CNC
			/* ͬԲ�ж�-0����arci-1���޺ͻ�arcj-2�����ϵķֱ�����(������)����������ֵCNC�����㷧ֵ��Χ��������ͬԲ�� */
			/* �� |T124=12������ֵ - 1| > thCNC(=0.3)���������㷧ֵ��Χ����������ͬԲ������������һ�������жϡ�(0.3����Χ���ϸ�˵T124��CNCֵӦ��Ϊ1) */
			/* ����myselect������Ӧ���ǿ���'�˴�����ʱĳ���������ж�ͬԲ�����Ƿ���Ч'�Ŀ��� */
			// �����޶�����  2se se1//pil,pim,pif,pjf,pjm,pjl��pjf,pjm,pjl,pif,pim,pil
			if(myselect1&&fabs(value4SixPoints(T124))>tCNC)
			{
				continue;
			}
#endif
			/* ��������������Լ���������������������ѡ���������ij����ֵ��12����key������HasnTable�Ĵ洢key���Ա���������ѡ��ϼ����Сￄ1�7 */
			uint key_ij = GenerateKey(PAIR_12, i, j);

			/* 3-��������һ�����޵Ļ�����(4����-pk) */
			//for each edge k
			for (ushort k = 0; k < sz_k; ++k)
			{
				/* ȡ����ĳ����edge_k(arc_k) */
				VP& edge_k = pk[k];
				ushort sz_ek = ushort(edge_k.size());

				/* ��arc_k��ȡ���㣺��˵ￄ1�7+���е�+�Ҷ˵�(��x��С < ��x��ￄ1�7) */
				Point& pkf = edge_k[0];
				Point& pkm = edge_k[sz_ek/2];
				Point& pkl = edge_k[sz_ek - 1];

#ifndef DISCARD_CONSTRAINT_POSITION
				/* ����Լ����֤-2��4�����л��ϵ��y���� > 1�����л��ϵ��y���ꣻ */
				/* ��4.y < 1.y����������Լ��������������������һ�������жϡ�(1.y-1����΄1�7) */
				//CONSTRAINTS on position
				if (pkl.y < pil.y - _fThPosition)
				{
					//discard
					continue;
				}
#endif

#ifndef DISCARD_CONSTRAINT_CNC
				/* ͬԲ�ж�-0����arci-1���޺ͻ�arck-4�����ϵķֱ�����(������)����������ֵCNC�����㷧ֵ��Χ��������ͬԲ�� */
				/* �� |T124=14������ֵ - 1| > thCNC(=0.3)���������㷧ֵ��Χ����������ͬԲ������������һ�������жϡ�(0.3����Χ���ϸ�˵T124��CNCֵӦ��Ϊ1) */
				// �����޶�����2		pif,pim,pil,pkf,pkm,pkl
				if(myselect2&&fabs(value4SixPoints(pif, pim, pil, pkf, pkm, pkl))>tCNC)
				{
					continue;
				}
				/* ͬԲ�ж�-0����arcj-2���޺ͻ�arck-4�����ϵķֱ�����(������)����������ֵCNC�����㷧ֵ��Χ��������ͬԲ�� */
				/* �� |T124=24������ֵ - 1| > thCNC(=0.3)���������㷧ֵ��Χ����������ͬԲ������������һ�������жϡ�(0.3����Χ���ϸ�˵T124��CNCֵӦ��Ϊ1) */
				// �����޶�����3		pjf,pjm,pjl,pkf,pkm,pkl
				if(myselect3&&fabs(value4SixPoints(pjf, pjm, pjl, pkf, pkm, pkl))>tCNC)
				{
					continue;
				}
#endif
				/* ��������������Լ���������������������ѡ���������ij����ֵ��12����key������HasnTable�Ĵ洢key���Ա���������ѡ��ϼ����Сￄ1�7 */
				uint key_ik = GenerateKey(PAIR_14, i, k);


				/* data������Map����(ɸѡ��ĺ�ѡ�����)������keyֵ��GenerateKey()�������ɣ�valueΪEllipseData���͵Ķ��� */
				/* ���ݻ�arci-1�ͻ�arcj-2�ҵ�Բ��O12-data_ij���Լ����ݻ�arci-1�ͻ�arck-4�ҵ�Բ��O14-data_ik */
				// Find centers
				EllipseData data_ij, data_ik;

				/* �ж�key_ij��Ӧ��EllipseDataԪ��data_ij�Ƿ����ڼ����У� */
				// If the data for the pair i-j have not been computed yet
				if (data.count(key_ij) == 0)
				{
					/* �����ڼ����У�������data_ij��������ￄ1�7(Բ��O12��)�������뼯���� */
					
					//1,2 -> reverse 1, swap

					// Compute data!
					GetFastCenter(edge_j, rev_i, data_ij);
					// Insert computed data in the hash table
					data.insert(pair<uint, EllipseData>(key_ij, data_ij));
				}
				else
				{
					/* �����ڼ����У������keyֵȡ����Ӧ��data_ijԪ��(Բ��O12����������ڲ��뼯��֮ǰ�Ѿ��������) */
					
					// Otherwise, just lookup the data in the hash table
					data_ij = data.at(key_ij);
				}

				/* �ж�key_ik��Ӧ��EllipseDataԪ��data_ik�Ƿ����ڼ����У� */
				// If the data for the pair i-k have not been computed yet
				if (data.count(key_ik) == 0)
				{
					/* �����ڼ����У�������data_ik��������ￄ1�7(Բ��O14��)�������뼯���� */

					//1,4 -> ok

					// Compute data!
					GetFastCenter(edge_i, edge_k, data_ik);
					// Insert computed data in the hash table
					data.insert(pair<uint, EllipseData>(key_ik, data_ik));
				}
				else
				{
					/* �����ڼ����У������keyֵȡ����Ӧ��data_ikԪ��(Բ��O14����������ڲ��뼯��֮ǰ�Ѿ��������) */
					
					// Otherwise, just lookup the data in the hash table
					data_ik = data.at(key_ik);
				}
				
				/* �豣֤data_ij��data_ik��Ϊ��Ч���� */
				// INVALID CENTERS
				if (!data_ij.isValid || !data_ik.isValid)
				{
					continue;
				}

#ifndef DISCARD_CONSTRAINT_CENTER
				/* ������Բ��λ��Լ����O12��O14����������ϸ�˵��Ϊarci��arcjͬԲ��arci��arckͬԲ����O12��O14Ӧ���غϣ�Ŀǰֻ���������Χ�ڼ��ɡ� */
				// Selection strategy - Step 3. See Sect [3.2.2] in the paper
				// The computed centers are not close enough
				if (ed2(data_ij.Cab, data_ik.Cab) > _fMaxCenterDistance2)
				{
					//discard
					continue;
				}
#endif
				/* ����������Լ����ÿ�����ļ���Լ��+ͬԲ�жϣ���Բ��O12��O14��λ��Լ�����������㣬��ɸ��������������Բ������ͳ�����Ȳ����� */
				// If all constraints of the selection strategy have been satisfied, 
				// we can start estimating the ellipse parameters

				// Find ellipse parameters

				// Get the coordinates of the center (xc, yc)
				Point2f center = GetCenterCoordinates(data_ij, data_ik);

				// Find remaining paramters (A,B,rho)
				FindEllipses(center, edge_i, edge_j, edge_k, data_ij, data_ik, ellipses);
			}
		}
	}
};



void CNEllipseDetector::Triplets231(VVP& pi,
									   VVP& pj,
									   VVP& pk,
									   unordered_map<uint, EllipseData>& data,
									   vector<Ellipse>& ellipses
									   )
{
	ushort sz_i = ushort(pi.size());
	ushort sz_j = ushort(pj.size());
	ushort sz_k = ushort(pk.size());

	// For each edge i
	for (ushort i = 0; i < sz_i; ++i)
	{
		VP& edge_i = pi[i];
		ushort sz_ei = ushort(edge_i.size());

		Point& pif = edge_i[0];
		Point& pim = edge_i[sz_ei/2];
		Point& pil = edge_i[sz_ei - 1];

		VP rev_i(edge_i.size());
		reverse_copy(edge_i.begin(), edge_i.end(), rev_i.begin());

		// For each edge j
		for (ushort j = 0; j < sz_j; ++j)
		{
			VP& edge_j = pj[j];
			ushort sz_ej = ushort(edge_j.size());

			Point& pjf = edge_j[0];
			Point& pjm = edge_j[sz_ej/2];
			Point& pjl = edge_j[sz_ej - 1];			

#ifndef DISCARD_CONSTRAINT_POSITION
			// CONSTRAINTS on position
			if (pjf.y < pif.y - _fThPosition)
			{
				//discard
				continue;
			}
#endif

#ifndef DISCARD_CONSTRAINT_CNC
			//����Լ��
			// �����޶����� 2es se3 //pif,pim,pil,pjf,pjm,pjl pil,pim,pif,pjf,pjm,pjl
			if(myselect1&&fabs(value4SixPoints(T231))>tCNC)
			{
				continue;
			}
#endif

			VP rev_j(edge_j.size());
			reverse_copy(edge_j.begin(), edge_j.end(), rev_j.begin());

			uint key_ij = GenerateKey(PAIR_23, i, j);

			// For each edge k
			for (ushort k = 0; k < sz_k; ++k)
			{
				VP& edge_k = pk[k];
				ushort sz_ek = ushort(edge_k.size());

				Point& pkf = edge_k[0];
				Point& pkm = edge_k[sz_ek/2];
				Point& pkl = edge_k[sz_ek - 1];

#ifndef DISCARD_CONSTRAINT_POSITION
				// CONSTRAINTS on position
				if (pkf.x < pil.x - _fThPosition)
				{
					//discard
					continue;
				}
#endif

#ifndef DISCARD_CONSTRAINT_CNC
				// �����޶�����2		pif,pim,pil,pkf,pkm,pkl
				if(myselect2&&fabs(value4SixPoints(pif, pim, pil, pkf, pkm, pkl))>tCNC)
				{
					continue;
				}
				// �����޶�����3		pjf,pjm,pjl,pkf,pkm,pkl
				if(myselect3&&fabs(value4SixPoints(pjf, pjm, pjl, pkf, pkm, pkl))>tCNC)
				{
					continue;
				}
#endif
				uint key_ik = GenerateKey(PAIR_12, k, i);

				// Find centers

				EllipseData data_ij, data_ik;

				if (data.count(key_ij) == 0)
				{
					// 2,3 -> reverse 2,3

					GetFastCenter(rev_i, rev_j, data_ij);
					data.insert(pair<uint, EllipseData>(key_ij, data_ij));
				}
				else
				{
					data_ij = data.at(key_ij);
				}

				if (data.count(key_ik) == 0)
				{
					// 2,1 -> reverse 1
					VP rev_k(edge_k.size());
					reverse_copy(edge_k.begin(), edge_k.end(), rev_k.begin());

					GetFastCenter(edge_i, rev_k, data_ik);
					data.insert(pair<uint, EllipseData>(key_ik, data_ik));
				}
				else
				{
					data_ik = data.at(key_ik);
				}

				// INVALID CENTERS
				if (!data_ij.isValid || !data_ik.isValid)
				{
					continue;
				}

#ifndef DISCARD_CONSTRAINT_CENTER
				// CONSTRAINT ON CENTERS
				if (ed2(data_ij.Cab, data_ik.Cab) > _fMaxCenterDistance2)
				{
					//discard
					continue;
				}
#endif
				// Find ellipse parameters
				Point2f center = GetCenterCoordinates(data_ij, data_ik);

				FindEllipses(center, edge_i, edge_j, edge_k, data_ij, data_ik, ellipses);

			}
		}
	}
};


void CNEllipseDetector::Triplets342(VVP& pi,
									   VVP& pj,
									   VVP& pk,
									   unordered_map<uint, EllipseData>& data,
									   vector<Ellipse>& ellipses
									   )
{
	ushort sz_i = ushort(pi.size());
	ushort sz_j = ushort(pj.size());
	ushort sz_k = ushort(pk.size());

	// For each edge i
	for (ushort i = 0; i < sz_i; ++i)
	{
		VP& edge_i = pi[i];
		ushort sz_ei = ushort(edge_i.size());

		Point& pif = edge_i[0];
		Point& pim = edge_i[sz_ei/2];
		Point& pil = edge_i[sz_ei - 1];

		VP rev_i(edge_i.size());
		reverse_copy(edge_i.begin(), edge_i.end(), rev_i.begin());

		// For each edge j
		for (ushort j = 0; j < sz_j; ++j)
		{
			VP& edge_j = pj[j];
			ushort sz_ej = ushort(edge_j.size());

			Point& pjf = edge_j[0];
			Point& pjm = edge_j[sz_ej/2];
			Point& pjl = edge_j[sz_ej - 1];

#ifndef DISCARD_CONSTRAINT_POSITION
			//CONSTRAINTS on position
			if (pjf.x < pil.x - _fThPosition) 		//is left
			{
				//discard
				continue;
			}
#endif

#ifndef DISCARD_CONSTRAINT_CNC
			// �����޶����� 3se se4 // pil,pim,pif,pjf,pjm,pjl pif,pim,pil,pjf,pjm,pjl
			if(myselect1&&fabs(value4SixPoints(T342))>tCNC)
			{
				continue;
			}
#endif

			VP rev_j(edge_j.size());
			reverse_copy(edge_j.begin(), edge_j.end(), rev_j.begin());

			uint key_ij = GenerateKey(PAIR_34, i, j);

			// For each edge k
			for (ushort k = 0; k < sz_k; ++k)
			{
				VP& edge_k = pk[k];
				ushort sz_ek = ushort(edge_k.size());

				Point& pkf = edge_k[0];
				Point& pkm = edge_k[sz_ek/2];
				Point& pkl = edge_k[sz_ek - 1];

#ifndef DISCARD_CONSTRAINT_POSITION
				//CONSTRAINTS on position
				if (pkf.y > pif.y + _fThPosition)
				{
					//discard
					continue;
				}
#endif

#ifndef DISCARD_CONSTRAINT_CNC
				// �����޶�����2		pif,pim,pil,pkf,pkm,pkl
				if(myselect2&&fabs(value4SixPoints(pif, pim, pil, pkf, pkm, pkl))>tCNC)
				{
					continue;
				}
				// �����޶�����3		pjf,pjm,pjl,pkf,pkm,pkl
				if(myselect3&&fabs(value4SixPoints(pjf, pjm, pjl, pkf, pkm, pkl))>tCNC)
				{
					continue;
				}
#endif
				uint key_ik = GenerateKey(PAIR_23, k, i);

				// Find centers

				EllipseData data_ij, data_ik;

				if (data.count(key_ij) == 0)
				{
					//3,4 -> reverse 4

					GetFastCenter(edge_i, rev_j, data_ij);
					data.insert(pair<uint, EllipseData>(key_ij, data_ij));
				}
				else
				{
					data_ij = data.at(key_ij);
				}

				if (data.count(key_ik) == 0)
				{
					//3,2 -> reverse 3,2

					VP rev_k(edge_k.size());
					reverse_copy(edge_k.begin(), edge_k.end(), rev_k.begin());

					GetFastCenter(rev_i, rev_k, data_ik);

					data.insert(pair<uint, EllipseData>(key_ik, data_ik));
				}
				else
				{
					data_ik = data.at(key_ik);
				}


				// INVALID CENTERS
				if (!data_ij.isValid || !data_ik.isValid)
				{
					continue;
				}

#ifndef DISCARD_CONSTRAINT_CENTER
				// CONSTRAINT ON CENTERS
				if (ed2(data_ij.Cab, data_ik.Cab) > _fMaxCenterDistance2)
				{
					//discard
					continue;
				}
#endif
				// Find ellipse parameters
				Point2f center = GetCenterCoordinates(data_ij, data_ik);
				FindEllipses(center, edge_i, edge_j, edge_k, data_ij, data_ik, ellipses);
			}
		}

	}
};



void CNEllipseDetector::Triplets413(VVP& pi,
									   VVP& pj,
									   VVP& pk,
									   unordered_map<uint, EllipseData>& data,
									   vector<Ellipse>& ellipses
									   )
{
	ushort sz_i = ushort(pi.size());
	ushort sz_j = ushort(pj.size());
	ushort sz_k = ushort(pk.size());

	// For each edge i
	for (ushort i = 0; i < sz_i; ++i)
	{
		VP& edge_i = pi[i];
		ushort sz_ei = ushort(edge_i.size());

		Point& pif = edge_i[0];
		Point& pim = edge_i[sz_ei/2];
		Point& pil = edge_i[sz_ei - 1];

		VP rev_i(edge_i.size());
		reverse_copy(edge_i.begin(), edge_i.end(), rev_i.begin());

		// For each edge j
		for (ushort j = 0; j < sz_j; ++j)
		{
			VP& edge_j = pj[j];
			ushort sz_ej = ushort(edge_j.size());

			Point& pjf = edge_j[0];
			Point& pjm = edge_j[sz_ej/2];
			Point& pjl = edge_j[sz_ej - 1];

#ifndef DISCARD_CONSTRAINT_POSITION
			//CONSTRAINTS on position
			if (pjl.y > pil.y + _fThPosition)  		//is below
			{
				//discard
				continue;
			}
#endif

#ifndef DISCARD_CONSTRAINT_CNC
			// �����޶����� 4se es1//pif,pim,pil,pjf,pjm,pjl pil,pim,pif,pjl,pjm,pjf pif,pim,pil,pjl,pjm,pjf
			if(myselect1&&fabs(value4SixPoints(T413))>tCNC)
			{
				continue;
			}
#endif

			uint key_ij = GenerateKey(PAIR_14, j, i);

			// For each edge k
			for (ushort k = 0; k < sz_k; ++k)
			{
				VP& edge_k = pk[k];
				ushort sz_ek = ushort(edge_k.size());

				Point& pkf = edge_k[0];
				Point& pkm = edge_k[sz_ek/2];
				Point& pkl = edge_k[sz_ek - 1];

#ifndef DISCARD_CONSTRAINT_POSITION
				//CONSTRAINTS on position
				if (pkl.x > pif.x + _fThPosition)
				{
					//discard
					continue;
				}
#endif

#ifndef DISCARD_CONSTRAINT_CNC
				// �����޶�����2		pif,pim,pil,pkf,pkm,pkl
				if(myselect2&&fabs(value4SixPoints(pif, pim, pil, pkf, pkm, pkl))>tCNC)
				{
					continue;
				}
				// �����޶�����3		pjf,pjm,pjl,pkf,pkm,pkl
				if(myselect3&&fabs(value4SixPoints(pjf, pjm, pjl, pkf, pkm, pkl))>tCNC)
				{
					continue;
				}
#endif
				uint key_ik = GenerateKey(PAIR_34, k, i);

				// Find centers

				EllipseData data_ij, data_ik;

				if (data.count(key_ij) == 0)
				{
					// 4,1 -> OK
					GetFastCenter(edge_i, edge_j, data_ij);
					data.insert(pair<uint, EllipseData>(key_ij, data_ij));
				}
				else
				{
					data_ij = data.at(key_ij);
				}

				if (data.count(key_ik) == 0)
				{
					// 4,3 -> reverse 4
					GetFastCenter(rev_i, edge_k, data_ik);
					data.insert(pair<uint, EllipseData>(key_ik, data_ik));
				}
				else
				{
					data_ik = data.at(key_ik);
				}

				// INVALID CENTERS
				if (!data_ij.isValid || !data_ik.isValid)
				{
					continue;
				}

#ifndef DISCARD_CONSTRAINT_CENTER
				// CONSTRAIN ON CENTERS
				if (ed2(data_ij.Cab, data_ik.Cab) > _fMaxCenterDistance2)
				{
					//discard
					continue;
				}
#endif
				// Find ellipse parameters
				Point2f center = GetCenterCoordinates(data_ij, data_ik);

				FindEllipses(center, edge_i, edge_j, edge_k, data_ij, data_ik, ellipses);

			}
		}
	}
};
/* ����Բ�����ɸѡ���ￄ1�7 */


void CNEllipseDetector::RemoveShortEdges(Mat1b& edges, Mat1b& clean)
{
	VVP contours;

	// Labeling and contraints on length
	Labeling(edges, contours, _iMinEdgeLength);

	int iContoursSize = contours.size();
	for (int i = 0; i < iContoursSize; ++i)
	{
		VP& edge = contours[i];
		unsigned szEdge = edge.size();

		// Constraint on axes aspect ratio
		RotatedRect oriented = minAreaRect(edge);
		if (oriented.size.width < _fMinOrientedRectSide ||
			oriented.size.height < _fMinOrientedRectSide ||
			oriented.size.width > oriented.size.height * _fMaxRectAxesRatio ||
			oriented.size.height > oriented.size.width * _fMaxRectAxesRatio)
		{
			continue;
		}

		for (unsigned j = 0; j < szEdge; ++j)
		{
			clean(edge[j]) = (uchar)255;
		}
	}
}



void CNEllipseDetector::PrePeocessing(Mat1b& I,
										 Mat1b& DP,
										 Mat1b& DN
										 )
{

	Tic(0); //edge detection
	// Mid smooth
	//medianBlur(I,I,3);
	// Smooth image
	GaussianBlur(I, I, _szPreProcessingGaussKernelSize, _dPreProcessingGaussSigma);

	// Temp variables
	Mat1b E;				//edge mask
	Mat1s DX, DY;			//sobel derivatives

	// Detect edges
	Canny3(I, E, DX, DY, 3, false);

	Toc(0); //edge detection

	Tac(1); //preprocessing

	// For each edge points, compute the edge direction
	for (int i = 0; i<_szImg.height; ++i)
	{
		short* _dx = DX.ptr<short>(i);
		short* _dy = DY.ptr<short>(i);
		uchar* _e = E.ptr<uchar>(i);
		uchar* _dp = DP.ptr<uchar>(i);
		uchar* _dn = DN.ptr<uchar>(i);

		for (int j = 0; j<_szImg.width; ++j)
		{
			if (!((_e[j] <= 0) || (_dx[j] == 0) || (_dy[j] == 0)))
			{
				// Angle of the tangent
				float phi = -(float(_dx[j]) / float(_dy[j]));

				// Along positive or negative diagonal
				if (phi > 0)	_dp[j] = (uchar)255;
				else if (phi < 0)	_dn[j] = (uchar)255;
			}
		}
	}
	Toc(1); //preprocessing
};


void CNEllipseDetector::DetectAfterPreProcessing(vector<Ellipse>& ellipses, Mat1b& E, Mat1f& PHI)
{
	// Set the image size
	_szImg = E.size();

	// Initialize temporary data structures
	Mat1b DP = Mat1b::zeros(_szImg);		// arcs along positive diagonal
	Mat1b DN = Mat1b::zeros(_szImg);		// arcs along negative diagonal

	// For each edge points, compute the edge direction
	for (int i = 0; i<_szImg.height; ++i)
	{
		float* _phi = PHI.ptr<float>(i);
		uchar* _e = E.ptr<uchar>(i);
		uchar* _dp = DP.ptr<uchar>(i);
		uchar* _dn = DN.ptr<uchar>(i);

		for (int j = 0; j<_szImg.width; ++j)
		{
			if ((_e[j] > 0) && (_phi[j] != 0))
			{
				// Angle

				// along positive or negative diagonal
				if (_phi[j] > 0)	_dp[j] = (uchar)255;
				else if (_phi[j] < 0)	_dn[j] = (uchar)255;
			}
		}
	}

	// Initialize accumulator dimensions
	ACC_N_SIZE = 101;
	ACC_R_SIZE = 180;
	ACC_A_SIZE = max(_szImg.height, _szImg.width);

	// Allocate accumulators
	accN = new int[ACC_N_SIZE];
	accR = new int[ACC_R_SIZE];
	accA = new int[ACC_A_SIZE];

	// Other temporary 
	VVP points_1, points_2, points_3, points_4;		//vector of points, one for each convexity class
	unordered_map<uint, EllipseData> centers;		//hash map for reusing already computed EllipseData

	// Detect edges and find convexities
	DetectEdges13(DP, points_1, points_3);
	DetectEdges24(DN, points_2, points_4);

	// Find triplets
	Triplets124(points_1, points_2, points_4, centers, ellipses);
	Triplets231(points_2, points_3, points_1, centers, ellipses);
	Triplets342(points_3, points_4, points_2, centers, ellipses);
	Triplets413(points_4, points_1, points_3, centers, ellipses);

	// Sort detected ellipses with respect to score
	sort(ellipses.begin(), ellipses.end());

	//free accumulator memory
	delete[] accN;
	delete[] accR;
	delete[] accA;

	//cluster detections
	//ClusterEllipses(ellipses);
};


void CNEllipseDetector::Detect(Mat1b& I, vector<Ellipse>& ellipses)
{
	countsOfFindEllipse=0;
	countsOfGetFastCenter=0;
	/*ʹ�÷�װ�ĺ���Tic()��-Toc()β��¼ÿһ��������ʱ��(ms)��ʱ������_times�У�
	    [��Ե��ￄ1�7, Ԥ����, ����, Ԥ��, У��, ����]��*/
	Tic(1); //prepare data structure

	// Set the image size
	_szImg = I.size();

	// Initialize temporary data structures
	Mat1b DP = Mat1b::zeros(_szImg);		// arcs along positive diagonal
	Mat1b DN = Mat1b::zeros(_szImg);		// arcs along negative diagonal

	// Initialize accumulator dimensions
	ACC_N_SIZE = 101;
	ACC_R_SIZE = 180;
	ACC_A_SIZE = max(_szImg.height, _szImg.width);

	// Allocate accumulators
	accN = new int[ACC_N_SIZE];
	accR = new int[ACC_R_SIZE];
	accA = new int[ACC_A_SIZE];

	// Other temporary 
	VVP points_1, points_2, points_3, points_4;		//vector of points, one for each convexity class
	unordered_map<uint, EllipseData> centers;		//hash map for reusing already computed EllipseData

	Toc(1); //prepare data structure

	// Preprocessing
	// From input image I, find edge point with coarse convexity along positive (DP) or negative (DN) diagonal
	PrePeocessing(I, DP, DN);

	Tac(1); //preprocessing
	// Detect edges and find convexities

	/* �����ͼƄ1�713+24�����ںϸ�Ļ�����ʼ������1�713����+��¼24���ޣ�����д��ͼƬ */
	/*DPDN_temp_only_wan_arc = DP.clone();
	int rows = DP.rows;  //ͼƬ�Ŀ�
	int cols = DP.cols;  //ͼƬ�ĸ�
	for (int i = 0; i < rows; i++)
		for (int j = 0; j < cols; j++)
		{
			DPDN_temp_only_wan_arc.at<uchar>(i, j) = 0;
		}*/

	DetectEdges13(DP, points_1, points_3);
	DetectEdges24(DN, points_2, points_4);

	/*imwrite("./result/�ϸ�Ļￄ1�7.jpg", DPDN_temp_only_wan_arc);*/
	/* �����ͼƄ1�713+24�����ںϸ�Ļ������ￄ1�7*/

	Toc(1); //preprocessing


	//// DEBUG
	//Mat3b out(I.rows, I.cols, Vec3b(0,0,0));
	//for(unsigned i=0; i<points_1.size(); ++i)
	//{
	//	//Vec3b color(rand()%255, 128+rand()%127, 128+rand()%127);
	//	Vec3b color(255,0,0);
	//	for(unsigned j=0; j<points_1[i].size(); ++j)
	//		out(points_1[i][j]) = color;
	//}

	//for(unsigned i=0; i<points_2.size(); ++i)
	//{
	//	//Vec3b color(rand()%255, 128+rand()%127, 128+rand()%127);
	//	Vec3b color(0,255,0);
	//	for(unsigned j=0; j<points_2[i].size(); ++j)
	//		out(points_2[i][j]) = color;
	//}
	//for(unsigned i=0; i<points_3.size(); ++i)
	//{
	//	//Vec3b color(rand()%255, 128+rand()%127, 128+rand()%127);
	//	Vec3b color(0,0,255);
	//	for(unsigned j=0; j<points_3[i].size(); ++j)
	//		out(points_3[i][j]) = color;
	//}

	//for(unsigned i=0; i<points_4.size(); ++i)
	//{
	//	//Vec3b color(rand()%255, 128+rand()%127, 128+rand()%127);
	//	Vec3b color(255,0,255);
	//	for(unsigned j=0; j<points_4[i].size(); ++j)
	//		out(points_4[i][j]) = color;
	//}


	// time estimation, validation  inside

	Tic(2); //grouping
	//find triplets
	Triplets124(points_1, points_2, points_4, centers, ellipses);
	Triplets231(points_2, points_3, points_1, centers, ellipses);
	Triplets342(points_3, points_4, points_2, centers, ellipses);
	Triplets413(points_4, points_1, points_3, centers, ellipses);
	Toc(2); //grouping	
	// time estimation, validation inside
	_times[2] -= (_times[3] + _times[4]);//����׼ȷ

	Tac(4); //validation
	// Sort detected ellipses with respect to score
	sort(ellipses.begin(), ellipses.end());//���շ������� ������ͬ Խ�ӽ�ԲԽС
	Toc(4); //validation


	// Free accumulator memory
	delete[] accN;
	delete[] accR;
	delete[] accA;

	Tic(5);
	// Cluster detections
	ClusterEllipses(ellipses);
	Toc(5);

};




// Ellipse clustering procedure. See Sect [3.3.2] in the paper.
void CNEllipseDetector::ClusterEllipses(vector<Ellipse>& ellipses)
{
	float th_Da = 0.1f;
	float th_Db = 0.1f;
	float th_Dr = 0.1f;

	float th_Dc_ratio = 0.1f;
	float th_Dr_circle = 0.9f;

	int iNumOfEllipses = int(ellipses.size());
	if (iNumOfEllipses == 0) return;

	// The first ellipse is assigned to a cluster
	vector<Ellipse> clusters;
	clusters.push_back(ellipses[0]);

	bool bFoundCluster = false;

	for (int i = 1; i<iNumOfEllipses; ++i)
	{
		Ellipse& e1 = ellipses[i];

		int sz_clusters = int(clusters.size());

		float ba_e1 = e1._b / e1._a;
		float Decc1 = e1._b / e1._a;

		bool bFoundCluster = false;
		for (int j = 0; j<sz_clusters; ++j)
		{
			Ellipse& e2 = clusters[j];

			float ba_e2 = e2._b / e2._a;
			float th_Dc = min(e1._b, e2._b) * th_Dc_ratio;
			th_Dc *= th_Dc;

			// Centers
			float Dc = ((e1._xc - e2._xc)*(e1._xc - e2._xc) + (e1._yc - e2._yc)*(e1._yc - e2._yc));
			if (Dc > th_Dc)
			{
				//not same cluster
				continue;
			}

			// a
			float Da = abs(e1._a - e2._a) / max(e1._a, e2._a);
			if (Da > th_Da)
			{
				//not same cluster
				continue;
			}

			// b
			float Db = abs(e1._b - e2._b) / min(e1._b, e2._b);
			if (Db > th_Db)
			{
				//not same cluster
				continue;
			}

			// angle
			float Dr = GetMinAnglePI(e1._rad, e2._rad) / float(CV_PI);
			if ((Dr > th_Dr) && (ba_e1 < th_Dr_circle) && (ba_e2 < th_Dr_circle))
			{
				//not same cluster
				continue;
			}

			// Same cluster as e2
			bFoundCluster = true;//
			// Discard, no need to create a new cluster
			break;
		}

		if (!bFoundCluster)
		{
			// Create a new cluster			
			clusters.push_back(e1);
		}
	}

	clusters.swap(ellipses);
};



//Draw at most iTopN detected ellipses.
void CNEllipseDetector::DrawDetectedEllipses(Mat3b& output, vector<Ellipse>& ellipses, int iTopN, int thickness)
{
	int sz_ell = int(ellipses.size());
	int n = (iTopN == 0) ? sz_ell : min(iTopN, sz_ell);
	for (int i = 0; i < n; ++i)
	{
		Ellipse& e = ellipses[n - i - 1];
		int g = cvRound(e._score * 255.f);
		Scalar color(0, g, 0);
		ellipse(output, Point(cvRound(e._xc), cvRound(e._yc)), Size(cvRound(e._a), cvRound(e._b)), e._rad*180.0 / CV_PI, 0.0, 360.0, color, thickness);
	}
}

// add 2015��12��24��
void CNEllipseDetector:: showEdgeInPic(Mat1b& I){
	_szImg = I.size();
	// initialize temporary data structures
	Mat1b DP	= Mat1b::zeros(_szImg);		// arcs along positive diagonal
	Mat1b DN	= Mat1b::zeros(_szImg);		// arcs along negative diagonal

	//other temporary 
	vector<vector<Point>> points_1, points_2, points_3, points_4;		//vector of points, one for each convexity class

	//Preprocessing
	//From input image I, find edge point with coarse convexity along positive (DP) or negative (DN) diagonal


	PrePeocessing(I, DP, DN);

	// show DP
	Mat1b DTMP	= Mat1b::ones(_szImg)*255;	
	Mat1b tmp=DTMP-DP;
	Mat1b tmp2=DTMP-DP;
	/*cvShowImage("DP",&(IplImage)tmp );
	cvSaveImage("DP.jpg",&IplImage(tmp));
	tmp=DTMP-DN;
	cvShowImage("DN",&(IplImage)tmp );
	cvSaveImage("DN.jpg",&IplImage(tmp));*/
	tmp = DP+DN;
	tmp2 = DTMP-tmp;

//	IplImage ttt = (IplImage)tmp2;
//	IplImage *tt2 = &ttt;
//	cvShowImage("DNP", tt2);
	// IplImage conversions removed for compatibility
	// IplImage tmp3 = (IplImage)tmp2;
	// IplImage tmp4 = IplImage(tmp2);
	// cvShowImage("DNP", &tmp3);
	// cvSaveImage("DPN.jpg",&tmp4);
	imshow("DNP", tmp2);
	imwrite("DPN.jpg", tmp2);
//  
	//end show DpDn
	float t_fMinOrientedRectSide = _fMinOrientedRectSide;
	_fMinOrientedRectSide=4;
	//detect edges and find convexities// 4������
	DetectEdges13(DP, points_1, points_3);
	DetectEdges24(DN, points_2, points_4);
	_fMinOrientedRectSide=t_fMinOrientedRectSide;
	//��ʾѡ���ı�
	Mat picture(_szImg, CV_8UC3,Scalar(255,255,255));
	// IplImage *image= cvCreateImage(_szImg,IPL_DEPTH_8U,3);
	Mat image(_szImg, CV_8UC3);
	// Mat picture(_szImg,CV_8UC3);
	/*for(int ih=0;ih<_szImg.height;ih++){
	for(int iw=0;iw<_szImg.width;iw++){
	picture.at<Vec3b>(ih,iw)[0]=I.at<uchar>(ih,iw);
	picture.at<Vec3b>(ih,iw)[1]=I.at<uchar>(ih,iw);
	picture.at<Vec3b>(ih,iw)[2]=I.at<uchar>(ih,iw);

	}
	}*/
	vector<Mat> imgs(4);
	Mat picture1=picture.clone();
	Mat picture2=picture.clone();
	Mat picture3=picture.clone();
	Mat picture4=picture.clone();
	showEdge(points_1,picture1);
	showEdge(points_2,picture2);
	showEdge(points_3,picture3);
	showEdge(points_4,picture4);
	/*
	imshow("��1����", picture1);
	imshow("��2����", picture2);
	imshow("��3����", picture3);
	imshow("��4����", picture4);*/
	
	Mat picture5=picture.clone();
	showEdge(points_1,picture5);
	showEdge(points_2,picture5);
	showEdge(points_3,picture5);
	showEdge(points_4,picture5);

	imgs[0] = picture2;
	imgs[1] = picture1;
	imgs[2] = picture3;
	imgs[3] = picture4; 
	// IplImage picture5_1 = (IplImage)picture5;
	// IplImage picture5_2 = IplImage(picture5);
	// cvNamedWindow("all arcs", CV_WINDOW_NORMAL);  
	// cvShowImage("all arcs", &picture5_1);
	// cvSaveImage("arcs.jpg", &picture5_2);
	namedWindow("all arcs", WINDOW_NORMAL);
	imshow("all arcs", picture5);
	imwrite("arcs.jpg", picture5);

	//imshow("all arcs", picture5);

	MultiImage_OneWin("�����еĻ�", imgs, cvSize(2, 2), cvSize(400,800)); 
	//cvWaitKey(0);  
	//cvDestroyWindow("all arcs");
}
void CNEllipseDetector:: showAllEdgeInPic(Mat1b& I){
	_szImg = I.size();
	// initialize temporary data structures
	Mat1b DP	= Mat1b::zeros(_szImg);		// arcs along positive diagonal
	Mat1b DN	= Mat1b::zeros(_szImg);		// arcs along negative diagonal

	//other temporary 
	vector<vector<Point>> points_1, points_2, points_3, points_4;		//vector of points, one for each convexity class

	//Preprocessing
	//From input image I, find edge point with coarse convexity along positive (DP) or negative (DN) diagonal

	//this->_fMaxRectAxesRatio=0;
	//this->_iMinEdgeLength=0;
	//this->_uNs=0;

	PrePeocessing(I, DP, DN);
	namedWindow("aaas", WINDOW_NORMAL);
	Mat1b tmp	= Mat1b::zeros(_szImg);
	
	// IplImage DP_1 = (IplImage)DP;
	// cvShowImage("aaas", &DP_1);
	imshow("aaas", DP);
//	cvShowImage("aaas",&(IplImage)DP );
	//detect edges and find convexities// 4������
	//DetectEdges13(DP, points_1, points_3);
	//DetectEdges24(DN, points_2, points_4);
}

int CNEllipseDetector:: showEdgeInPic(Mat1b& I,bool showedge){
	_szImg = I.size();
	// initialize temporary data structures
	Mat1b DP	= Mat1b::zeros(_szImg);		// arcs along positive diagonal
	Mat1b DN	= Mat1b::zeros(_szImg);		// arcs along negative diagonal

	//other temporary 
	vector<vector<Point>> points_1, points_2, points_3, points_4;		//vector of points, one for each convexity class

	//Preprocessing
	//From input image I, find edge point with coarse convexity along positive (DP) or negative (DN) diagonal


	PrePeocessing(I, DP, DN);
	//detect edges and find convexities// 4������
	DetectEdges13(DP, points_1, points_3);
	DetectEdges24(DN, points_2, points_4);
	int EdgesNumber=points_1.size()+points_2.size()+points_3.size()+points_4.size();
	//��ʾѡ���ı�
	return EdgesNumber;
	//cvWaitKey(0);  
	//cvDestroyWindow("all arcs");
}

/* ����Ķ��ￄ1�7
1��line-27��72�����Ż�������tCNC = 0.47f��score = 0.61��_fMinScore = 0.61��
2��line-1147��124+231+342+413�����еķ�ֵ�ж�ʱ��'-1'��������value4SixPoints��������resultֵʱ���У�
3����DISCARD_TCN��DISCARD_TCN1�ĺ궨��ȥ������ΪDISCARD_TCN2�궨�塣
*/