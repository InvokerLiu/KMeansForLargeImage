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

//对遥感影像进行K均值聚类
class CKMeans
{
public:
	CKMeans();
	~CKMeans();
	//入口函数
	string Execute(string sImageFile, string sOutputFile, int nFeatureDim, int nClusterNum, 
		int nMaxIterationNum = 10, int *pBandMap = NULL, double dError = 0.0001);

private:
	//打开数据集
	string OpenDataset(string sImageFile);
	//关闭数据集
	void CloseDataset();
	//初始化聚类中心
	string InitClusterCenter(string sOutputFile);
	//根据聚类中心进行聚类
	void Classification(GDALDataset *pOutputDataset);
	//根据聚类结果计算新的聚类中心
	void ComputeNewClusterCenter(GDALDataset *pOutputDataset);
	//计算聚类中心的变化误差
	double ComputeError();
	//更新旧的聚类中心
	void UpdateOldCenter();

private:
	//获取NoData值
	double* GetNoDataValue();
	//计算每一个像素到最近聚类中心的距离
	double GetClosestDist(GDALDataset *pDistanceDataset, int nK, double *pNoDataValue);
	//获取下一个聚类中心
	void GetNextClusterCenter(GDALDataset *pDistanceDataset, int nK, double *pNoDataValue, double dTotal);
	//判断第nK个聚类中心是否与之前的聚类中心相同
	bool IsRepeatInClusterCenter(int nK);

private:
	//获取某个像素与某个聚类中心的距离
	double GetDistance(double *pData, double *pCluster, int nIndex);
	//获取两个聚类中心之间的距离
	double GetDistance(double *pCluster1, double *pCluster2);


private:
	GDALDataset *m_pDataset;                      //输入影像数据集
	int m_nRasterXSize;                           //数据集宽度
	int m_nRasterYSize;                           //数据集高度
	int m_nRasterCount;                           //数据集波段数
	double **m_pOldCenter;                        //旧聚类中心
	double **m_pNewCenter;                        //新聚类中心
	int m_nFeatureDim;                            //进行聚类的特征维度
	int *m_pBandList;                             //进行聚类的波段列表
	int m_nClusterNum;                            //聚类的类别数
};

