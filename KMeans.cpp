#include "KMeans.h"
#include "Utils.h"


CKMeans::CKMeans()
{
	GDALAllRegister();
	OGRRegisterAll();
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
	CPLSetConfigOption("SHAPE_ENCODING", "");
	m_pDataset = NULL;
	m_pOldCenter = NULL;
	m_pNewCenter = NULL;
	m_pBandList = NULL;

	m_nFeatureDim = 0;
	m_nRasterXSize = 0;
	m_nRasterYSize = 0;
	m_nRasterCount = 0;
	m_nClusterNum = 0;
}

CKMeans::~CKMeans()
{
	CloseDataset();
}

string CKMeans::OpenDataset(string sImageFile)
{
	m_pDataset = (GDALDataset*)GDALOpen(sImageFile.c_str(), GA_ReadOnly);
	if (m_pDataset == NULL)
		return "Open File " + sImageFile + " Failed!";

	m_nRasterXSize = m_pDataset->GetRasterXSize();
	m_nRasterYSize = m_pDataset->GetRasterYSize();
	m_nRasterCount = m_pDataset->GetRasterCount();

	return "";
}

void CKMeans::CloseDataset()
{
	if(m_pDataset)
	{
		GDALClose(m_pDataset);
		m_pDataset = NULL;
	}
	if(m_pOldCenter)
	{
		for (int i = 0; i < m_nClusterNum; i++)
			delete[]m_pOldCenter[i];
		delete[]m_pOldCenter;
		m_pOldCenter = NULL;
	}
	if(m_pNewCenter)
	{
		for (int i = 0; i < m_nClusterNum; i++)
			delete[]m_pNewCenter[i];
		delete[]m_pNewCenter;
		m_pNewCenter = NULL;
	}
	if(m_pBandList)
	{
		delete[]m_pBandList;
		m_pBandList = NULL;
	}
}

string CKMeans::Execute(string sImageFile, string sOutputFile, int nFeatureDim,
	int nClusterNum, int nMaxIterationNum, int *pBandMap, double dError)
{
	cout << "Cluster Begin!" << endl;
	string sResult = "";
	sResult = OpenDataset(sImageFile);
	if (sResult != "")
		return sResult;

	m_nFeatureDim = nFeatureDim;
	m_nClusterNum = nClusterNum;
	m_pBandList = pBandMap;
	if (m_pBandList == NULL && m_nFeatureDim != m_nRasterCount)
	{
		CloseDataset();
		return "FeatureDim Is Not Match Band Map!";
	}

	//初始化聚类中心
	sResult = InitClusterCenter(sOutputFile);
	if (sResult != "")
	{
		CloseDataset();
		return sResult;
	}

	//创建输出数据集
	GDALDriver *pDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
	GDALDataset *pOutputDataset = pDriver->Create(sOutputFile.c_str(), m_nRasterXSize, m_nRasterYSize, 1, GDT_Byte, NULL);
	if(pOutputDataset == NULL)
	{
		CloseDataset();
		return "Create File " + sOutputFile + " Failed!";
	}
	pOutputDataset->GetRasterBand(1)->SetNoDataValue(255);
	double *pGeoTrans = new double[6];
	m_pDataset->GetGeoTransform(pGeoTrans);
	pOutputDataset->SetGeoTransform(pGeoTrans);
	delete[]pGeoTrans;
	pOutputDataset->SetProjection(m_pDataset->GetProjectionRef());
	//GDALClose(pOutputDataset);

	int nIter = 1;
	double dTempError = 0;
	while(nIter <= nMaxIterationNum)
	{
		cout << "In the Iteration " << nIter << ":" << endl;
		//pOutputDataset = (GDALDataset*)GDALOpen(sOutputFile.c_str(), GA_Update);
		Classification(pOutputDataset);
		//GDALClose(pOutputDataset);
		//pOutputDataset = (GDALDataset*)GDALOpen(sOutputFile.c_str(), GA_ReadOnly);
		ComputeNewClusterCenter(pOutputDataset);
		//GDALClose(pOutputDataset);
		dTempError = ComputeError();
		cout << "Iteration " << nIter << " Finished, The Error is " << dTempError << endl;
		if (dTempError <= dError)
			break;
		UpdateOldCenter();
		nIter++;
	}
	GDALClose(pOutputDataset);
	CloseDataset();
	cout << "Cluster Finished!" << endl;
	return "";
}

//初始化聚类中心
string CKMeans::InitClusterCenter(string sOutputFile)
{
	//创建聚类中心
	cout << "Init Cluster Center Begin!" << endl;
	if(m_pOldCenter)
	{
		for (int i = 0; i < m_nClusterNum; i++)
			delete[]m_pOldCenter[i];
		delete[]m_pOldCenter;
		m_pOldCenter = NULL;
	}
	if (m_pNewCenter)
	{
		for (int i = 0; i < m_nClusterNum; i++)
			delete[]m_pNewCenter[i];
		delete[]m_pNewCenter;
		m_pNewCenter = NULL;
	}
	m_pOldCenter = new double*[m_nClusterNum];
	m_pNewCenter = new double*[m_nClusterNum];
	for(int i = 0; i < m_nClusterNum; i++)
	{
		m_pOldCenter[i] = new double[m_nFeatureDim];
		m_pNewCenter[i] = new double[m_nFeatureDim];
		for(int j = 0; j < m_nFeatureDim; j++)
		{
			m_pOldCenter[i][j] = 0;
			m_pNewCenter[i][j] = 0;
		}
	}

	//获取NoDataValue
	double *pNoDataValue = GetNoDataValue();
	int nRandomX = 0, nRandomY = 0;
	srand((int)time(0));
	double *pTempData = new double[m_nFeatureDim];
	bool bIsNoData = false;
	//随机选取第一个聚类中心，不能为NoData
	cout << "Select The First Cluster Center Randomly!" << endl;
	while(true)
	{
		nRandomX = rand() % m_nRasterXSize;
		nRandomY = rand() % m_nRasterYSize;
		bIsNoData = false;
		m_pDataset->RasterIO(GF_Read, nRandomX, nRandomY, 1, 1, pTempData, 1, 1, 
			GDT_Float64, m_nFeatureDim, m_pBandList, 0, 0, 0);
		for(int i = 0; i < m_nFeatureDim; i++)
		{
			if(pTempData[i] == pNoDataValue[i])
			{
				bIsNoData = true;
				break;
			}
		}
		if (bIsNoData == false)
		{
			for (int i = 0; i < m_nFeatureDim; i++)
				m_pOldCenter[0][i] = pTempData[i];
			break;
		}
	}
	delete[]pTempData;

	//创建一个中间文件，用于存储每一个数据离最近一个聚类中心的距离
	string sDirectory = GetDirectory(sOutputFile);
	string sDistanceFile = sDirectory + "\\Distance.tif";
	GDALDriver *pDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
	GDALDataset *pDistanceDataset = pDriver->Create(sDistanceFile.c_str(), m_nRasterXSize, m_nRasterYSize, 1, GDT_Float32, NULL);
	if (pDistanceDataset == NULL)
		return "Create File " + sDistanceFile + "Failed!";
	pDistanceDataset->GetRasterBand(1)->SetNoDataValue(pNoDataValue[0]);
	double dTotal = 0.0;
	for(int i = 1; i < m_nClusterNum; i++)
	{
		cout << "For The " << i + 1 << "-th Cluster Center:" << endl;
		dTotal = GetClosestDist(pDistanceDataset, i, pNoDataValue);
		//防止新产生的聚类中心与之前的聚类中心相同
		while(true)
		{
			dTotal = dTotal * (rand() % 10000 / 10000.0);
			GetNextClusterCenter(pDistanceDataset, i, pNoDataValue, dTotal);
			if (IsRepeatInClusterCenter(i) == false)
				break;
		}
	}
	delete[]pNoDataValue;
	GDALClose(pDistanceDataset);
	DeleteTif(sDistanceFile);
	cout << "Init Cluster Center Finished!" << endl;
	return "";
}

void CKMeans::Classification(GDALDataset *pOutputDataset)
{
	cout << "Classification Begin!" << endl;
	double *pData = new double[m_nRasterXSize * m_nFeatureDim];
	unsigned char *pOutputData = new unsigned char[m_nRasterXSize];
	double *pNoDataValue = GetNoDataValue();

	bool bIsNoData = false;
	double dMinDis = DBL_MAX, dTempDis = 0.0;
	int nMinDisIndex = 0;
	int nOldPercent = 0, nNewPercent = 0;
	cout << "0%";
	for(int y = 0; y < m_nRasterYSize; y++)
	{
		m_pDataset->RasterIO(GF_Read, 0, y, m_nRasterXSize, 1, pData, m_nRasterXSize, 1,
			GDT_Float64, m_nFeatureDim, m_pBandList, 0, 0, 0);
		for(int x = 0; x < m_nRasterXSize; x++)
		{
			bIsNoData = false;
			for(int i = 0; i < m_nFeatureDim; i++)
			{
				if(pData[i * m_nRasterXSize + x] == pNoDataValue[i])
				{
					bIsNoData = true;
					break;
				}
			}
			if(bIsNoData)
			{
				pOutputData[x] = (unsigned char)255;
			}
			else
			{
				dMinDis = DBL_MAX;
				for(int i = 0; i < m_nClusterNum; i++)
				{
					dTempDis = GetDistance(pData, m_pOldCenter[i], x);
					if(dTempDis < dMinDis)
					{
						dMinDis = dTempDis;
						nMinDisIndex = i + 1;
					}
				}
				pOutputData[x] = (unsigned char)nMinDisIndex;
			}
		}
		pOutputDataset->RasterIO(GF_Write, 0, y, m_nRasterXSize, 1, pOutputData, m_nRasterXSize, 1,
			GDT_Byte, 1, NULL, 0, 0, 0);
		nNewPercent = y * 100 / m_nRasterYSize;
		if(nNewPercent != nOldPercent)
		{
			nOldPercent = nNewPercent;
			cout << "\r" << nNewPercent << "%";
		}
	}
	cout << endl;
	delete[]pData;
	delete[]pOutputData;
	delete[]pNoDataValue;
	cout << "Classification Finished!" << endl;
}

void CKMeans::ComputeNewClusterCenter(GDALDataset *pOutputDataset)
{
	cout << "Compute New Cluster Center Begin!" << endl;
	int *pCount = new int[m_nClusterNum];
	for(int i = 0; i < m_nClusterNum; i++)
	{
		for(int j = 0; j < m_nFeatureDim; j++)
		{
			m_pNewCenter[i][j] = 0;
		}
		pCount[i] = 0;
	}
	
	double *pData = new double[m_nRasterXSize * m_nFeatureDim];
	int *pLabelData = new int[m_nRasterXSize];
	int nOldPercent = 0, nNewPercent = 0;
	cout << "0%";
	for(int y = 0; y < m_nRasterYSize; y++)
	{
		m_pDataset->RasterIO(GF_Read, 0, y, m_nRasterXSize, 1, pData, m_nRasterXSize, 1,
			GDT_Float64, m_nFeatureDim, m_pBandList, 0, 0, 0);
		pOutputDataset->RasterIO(GF_Read, 0, y, m_nRasterXSize, 1, pLabelData, m_nRasterXSize, 1,
			GDT_Int32, 1, NULL, 0, 0, 0);
		for(int x = 0; x < m_nRasterXSize; x++)
		{
			if(pLabelData[x] != 255)
			{
				pCount[pLabelData[x] - 1]++;
				for(int i = 0; i < m_nFeatureDim; i++)
				{
					m_pNewCenter[pLabelData[x] - 1][i] += pData[i * m_nRasterXSize + x];
				}
			}
		}
		nNewPercent = y * 100 / m_nRasterYSize;
		if(nNewPercent != nOldPercent)
		{
			nOldPercent = nNewPercent;
			cout << "\r" << nOldPercent << "%";
		}
	}

	for(int i = 0; i < m_nClusterNum; i++)
	{
		for(int j = 0; j < m_nFeatureDim; j++)
		{
			m_pNewCenter[i][j] /= pCount[i];
		}
	}
	cout << endl;
	delete[]pCount;
	delete[]pData;
	delete[]pLabelData;
	cout << "Compute New Cluster Center Finished!" << endl;
}

double CKMeans::ComputeError()
{
	double dTotal = 0.0;
	for(int i = 0; i < m_nClusterNum; i++)
	{
		dTotal += GetDistance(m_pOldCenter[i], m_pNewCenter[i]);
	}
	return dTotal / m_nClusterNum;
}

void CKMeans::UpdateOldCenter()
{
	for(int i = 0; i < m_nClusterNum; i++)
	{
		for (int j = 0; j < m_nFeatureDim; j++)
			m_pOldCenter[i][j] = m_pNewCenter[i][j];
	}
}

double* CKMeans::GetNoDataValue()
{
	double *pNoDataValue = new double[m_nFeatureDim];
	if(m_pBandList)
	{
		for(int i = 0; i < m_nFeatureDim; i++)
		{
			pNoDataValue[i] = m_pDataset->GetRasterBand(m_pBandList[i])->GetNoDataValue();
		}
	}
	else
	{
		for(int i = 0; i < m_nFeatureDim; i++)
		{
			pNoDataValue[i] = m_pDataset->GetRasterBand(i + 1)->GetNoDataValue();
		}
	}
	return pNoDataValue;
}

//计算每一个像素离最近一个聚类中心的距离
//pDistanceDataset 保存最近距离的数据集
//nK 已经初始化成功的聚类中心的个数
//pNoDataValue 非正常值的取值
double CKMeans::GetClosestDist(GDALDataset *pDistanceDataset, int nK, double *pNoDataValue)
{
	cout << "Calculate Closest Distance Begin!" << endl;
	double dTotal = 0.0;
	double *pData = new double[m_nRasterXSize * m_nFeatureDim];
	float *pDistanceData = new float[m_nRasterXSize];

	bool bIsNoData = false;
	double dMinDistance = DBL_MAX;
	double dTempDistance = 0;
	int nOldPercent = 0, nNewPercent = 0;
	cout << "0%";
	for(int y = 0; y < m_nRasterYSize; y++)
	{
		m_pDataset->RasterIO(GF_Read, 0, y, m_nRasterXSize, 1, pData, m_nRasterXSize, 1,
			GDT_Float64, m_nFeatureDim, m_pBandList, 0, 0, 0);
		for(int x = 0; x < m_nRasterXSize; x++)
		{
			bIsNoData = false;
			for(int i = 0; i < m_nFeatureDim; i++)
			{
				if(pData[i * m_nRasterXSize + x] == pNoDataValue[i])
				{
					bIsNoData = true;
					break;
				}
			}
			if(bIsNoData)
			{
				pDistanceData[x] = (float)pNoDataValue[0];
			}
			else
			{
				dMinDistance = DBL_MAX;
				//计算一个像素到最近聚类中心的距离
				for(int i = 0; i < nK; i++)
				{
					dTempDistance = GetDistance(pData, m_pOldCenter[i], x);
					if (dTempDistance < dMinDistance)
						dMinDistance = dTempDistance;
				}
				dTotal += dMinDistance;
				pDistanceData[x] = (float)dMinDistance;
			}
		}
		pDistanceDataset->RasterIO(GF_Write, 0, y, m_nRasterXSize, 1, pDistanceData, m_nRasterXSize, 1,
			GDT_Float32, 1, NULL, 0, 0, 0);
		nNewPercent = y * 100 / m_nRasterYSize;
		if(nNewPercent != nOldPercent)
		{
			nOldPercent = nNewPercent;
			cout << "\r" << nOldPercent << "%";
		}
	}
	cout << endl;
	delete[]pData;
	delete[]pDistanceData;
	cout << "Calculate Closest Distance Finished!" << endl;
	return dTotal;
}

void CKMeans::GetNextClusterCenter(GDALDataset *pDistanceDataset, int nK, double *pNoDataValue, double dTotal)
{
	cout << "Get Cluster Center Begin!" << endl;
	double *pDistanceData = new double[m_nRasterXSize];
	int nX = 0, nY = 0;
	bool bIsBreak = false;
	int nOldPercent = 0, nNewPercent = 0;
	cout << "0%";
	for(int y = 0; y < m_nRasterYSize; y++)
	{
		pDistanceDataset->RasterIO(GF_Read, 0, y, m_nRasterXSize, 1, pDistanceData, m_nRasterXSize, 1,
			GDT_Float64, 1, NULL, 0, 0, 0);
		for(int x = 0; x < m_nRasterXSize; x++)
		{
			if(pDistanceData[x] != pNoDataValue[0])
			{
				dTotal -= pDistanceData[x];
				if(dTotal <= 0)
				{
					nX = x;
					nY = y;
					bIsBreak = true;
					break;
				}
			}
		}
		if (bIsBreak)
			break;
		nNewPercent = y * 100 / m_nRasterYSize;
		if(nNewPercent != nOldPercent)
		{
			nOldPercent = nNewPercent;
			cout << "\r" << nOldPercent << "%";
		}
	}
	cout << endl;
	delete[]pDistanceData;
	double *pData = new double[m_nFeatureDim];
	m_pDataset->RasterIO(GF_Read, nX, nY, 1, 1, pData, 1, 1,
		GDT_Float64, m_nFeatureDim, m_pBandList, 0, 0, 0);
	for(int i = 0; i < m_nFeatureDim; i++)
	{
		m_pOldCenter[nK][i] = pData[i];
	}
	delete[]pData;
	cout << "Get Cluster Center Finished!" << endl;
}

bool CKMeans::IsRepeatInClusterCenter(int nK)
{
	double dDis = 0.0;
	for(int i = 0; i < nK - 1; i++)
	{
		dDis = GetDistance(m_pOldCenter[nK], m_pOldCenter[i]);
		if (dDis == 0.0)
			return true;
	}
	return false;
}

//计算一个像素与一个聚类中心之间的距离
double CKMeans::GetDistance(double *pData, double *pCluster, int nIndex)
{
	double dTotal = 0.0;
	for(int i = 0; i < m_nFeatureDim; i++)
	{
		dTotal += pow(pData[i * m_nRasterXSize + nIndex] - pCluster[i], 2);
	}
	return sqrt(dTotal);
}

double CKMeans::GetDistance(double *pCluster1, double *pCluster2)
{
	double dTotal = 0.0;
	for (int i = 0; i < m_nFeatureDim; i++)
	{
		dTotal += pow(pCluster1[i] - pCluster2[i], 2);
	}
	return sqrt(dTotal);
}