#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <time.h>
using namespace std;

#include "gdal_priv.h"
#include "cpl_conv.h"
#include "gdal_alg.h"
#include "ogrsf_frmts.h"

//��ң��Ӱ�����K��ֵ����
class CKMeans
{
public:
	CKMeans();
	~CKMeans();
	//��ں���
	string Execute(string sImageFile, string sOutputFile, int nFeatureDim, int nClusterNum, 
		int nMaxIterationNum = 10, int *pBandMap = NULL, double dError = 0.0001);

private:
	//�����ݼ�
	string OpenDataset(string sImageFile);
	//�ر����ݼ�
	void CloseDataset();
	//��ʼ����������
	string InitClusterCenter(string sOutputFile);
	//���ݾ������Ľ��о���
	void Classification(GDALDataset *pOutputDataset);
	//���ݾ����������µľ�������
	void ComputeNewClusterCenter(GDALDataset *pOutputDataset);
	//����������ĵı仯���
	double ComputeError();
	//���¾ɵľ�������
	void UpdateOldCenter();

private:
	//��ȡNoDataֵ
	double* GetNoDataValue();
	//����ÿһ�����ص�����������ĵľ���
	double GetClosestDist(GDALDataset *pDistanceDataset, int nK, double *pNoDataValue);
	//��ȡ��һ����������
	void GetNextClusterCenter(GDALDataset *pDistanceDataset, int nK, double *pNoDataValue, double dTotal);
	//�жϵ�nK�����������Ƿ���֮ǰ�ľ���������ͬ
	bool IsRepeatInClusterCenter(int nK);

private:
	//��ȡĳ��������ĳ���������ĵľ���
	double GetDistance(double *pData, double *pCluster, int nIndex);
	//��ȡ������������֮��ľ���
	double GetDistance(double *pCluster1, double *pCluster2);


private:
	GDALDataset *m_pDataset;                      //����Ӱ�����ݼ�
	int m_nRasterXSize;                           //���ݼ����
	int m_nRasterYSize;                           //���ݼ��߶�
	int m_nRasterCount;                           //���ݼ�������
	double **m_pOldCenter;                        //�ɾ�������
	double **m_pNewCenter;                        //�¾�������
	int m_nFeatureDim;                            //���о��������ά��
	int *m_pBandList;                             //���о���Ĳ����б�
	int m_nClusterNum;                            //����������
};

