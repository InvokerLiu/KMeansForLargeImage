#include "Utils.h"

string GetImageUnions(string sImageFile1, string sImageFile2, string sOutputFile1, string sOutputFile2, string sRangeFile, GDALDataType type)
{
	GDALDataset *pDataset1 = (GDALDataset*)GDALOpen(sImageFile1.c_str(), GA_ReadOnly);
	if(pDataset1 == NULL)
	{
		return "打开影像" + sImageFile1 + "失败！";
	}
	GDALDataset *pDataset2 = (GDALDataset*)GDALOpen(sImageFile2.c_str(), GA_ReadOnly);
	if(pDataset2 == NULL)
	{
		return "打开影像" + sImageFile2 + "失败！";
	}
	if(type != GDT_Byte && type != GDT_Int32 && type != GDT_Float32)
	{
		return "不支持的影像格式！";
	}
	string ref = pDataset1->GetProjectionRef();
	int nRasterXSize1 = pDataset1->GetRasterXSize();
	int nRasterYSize1 = pDataset1->GetRasterYSize();
	int nRasterCount1 = pDataset1->GetRasterCount();
	int nRasterXSize2 = pDataset2->GetRasterXSize();
	int nRasterYSize2 = pDataset2->GetRasterYSize();
	int nRasterCount2 = pDataset2->GetRasterCount();

	double *pGeoTransform1 = new double[6];
	pDataset1->GetGeoTransform(pGeoTransform1);
	double *pGeoTransform2 = new double[6];
	pDataset2->GetGeoTransform(pGeoTransform2);
	double dLeft = 0, dRight = 0, dTop = 0, dBottom = 0;

	double *pOutputGeoTransform1 = new double[6];
	double *pOutputGeoTransform2 = new double[6];
	pOutputGeoTransform1[1] = pGeoTransform1[1];
	pOutputGeoTransform1[2] = pGeoTransform1[2];
	pOutputGeoTransform1[4] = pGeoTransform1[4];
	pOutputGeoTransform1[5] = pGeoTransform1[5];

	pOutputGeoTransform2[1] = pGeoTransform2[1];
	pOutputGeoTransform2[2] = pGeoTransform2[2];
	pOutputGeoTransform2[4] = pGeoTransform2[4];
	pOutputGeoTransform2[5] = pGeoTransform2[5];

	int nDeltaXSize1 = 0, nDeltaYSize1 = 0;
	int nDeltaXSize2 = 0, nDeltaYSize2 = 0;
	

	dLeft = pGeoTransform1[0];
	dTop = pGeoTransform1[3];

	dRight = pGeoTransform1[0] + pGeoTransform1[1] * nRasterXSize1;
	dBottom = pGeoTransform1[3] + pGeoTransform1[5] * nRasterYSize1;

	if (pGeoTransform2[0] > dLeft)
	{
		dLeft = pGeoTransform2[0];
		nDeltaXSize1 = (int)((dLeft - pGeoTransform1[0]) / pGeoTransform1[1] + 0.5);
	}
	else
	{
		nDeltaXSize2 = (int)((dLeft - pGeoTransform2[0]) / pGeoTransform2[1] + 0.5);
	}
	if (pGeoTransform2[3] < dTop)
	{
		dTop = pGeoTransform2[3];
		nDeltaYSize1 = (int)((pGeoTransform1[3] - dTop) / -pGeoTransform1[5] + 0.5);
	}
	else
	{
		nDeltaYSize2 = (int)((pGeoTransform2[3] - dTop) / -pGeoTransform2[5] + 0.5);
	}

	if (pGeoTransform2[0] + pGeoTransform2[1] * nRasterXSize2 < dRight)
		dRight = pGeoTransform2[0] + pGeoTransform2[1] * nRasterXSize2;
	if (pGeoTransform2[3] + pGeoTransform2[5] * nRasterYSize2 > dBottom)
		dBottom = pGeoTransform2[3] + pGeoTransform2[5] * nRasterYSize2;

	pOutputGeoTransform1[0] = pOutputGeoTransform2[0] = dLeft;
	pOutputGeoTransform1[3] = pOutputGeoTransform2[3] = dTop;

	double dXRange = dRight - dLeft, dYRange = dTop - dBottom;
	int nOutputXSize1 = (int)(dXRange / pGeoTransform1[1] + 0.5);
	int nOutputYSize1 = (int)(dYRange / -pGeoTransform1[5] + 0.5);

	if (nOutputXSize1 + nDeltaXSize1 > nRasterXSize1)
		nOutputXSize1 = nRasterXSize1 - nDeltaXSize1;
	if (nOutputYSize1 + nDeltaYSize1 > nRasterYSize1)
		nOutputYSize1 = nRasterYSize1 - nDeltaYSize1;

	int nOutputXSize2 = (int)(dXRange / pGeoTransform2[1] + 0.5);
	int nOutputYSize2 = (int)(dYRange / -pGeoTransform2[5] + 0.5);
	if (nOutputXSize2 + nDeltaXSize2 > nRasterXSize2)
		nOutputXSize2 = nRasterXSize2 - nDeltaXSize2;
	if (nOutputYSize2 + nDeltaYSize2 > nRasterYSize2)
		nOutputYSize2 = nRasterYSize2 - nDeltaYSize2;

	int nPatchSize1 = 500;
	int nPatchSize2 = (int)(nPatchSize1 * pGeoTransform1[1] / pGeoTransform2[1] + 0.5);

	delete[]pGeoTransform1;
	delete[]pGeoTransform2;

	int nRows1 = nOutputYSize1 / nPatchSize1;
	if (nRows1 * nPatchSize1 != nOutputYSize1)
		nRows1++;
	int nCols1 = nOutputXSize1 / nPatchSize1;
	if (nCols1 * nPatchSize1 != nOutputXSize1)
		nCols1++;

	int nRows2 = nOutputYSize2 / nPatchSize2;
	if (nRows2 * nPatchSize2 != nOutputYSize2)
		nRows2++;
	int nCols2 = nOutputXSize2 / nPatchSize2;
	if (nCols2 * nPatchSize2 != nOutputXSize2)
		nCols2++;

	if(nRows1 != nRows2 || nCols1 != nCols2)
	{
		nPatchSize2 += 1;
		int nRows2 = nOutputYSize2 / nPatchSize2;
		if (nRows2 * nPatchSize2 != nOutputYSize2)
			nRows2++;
		int nCols2 = nOutputXSize2 / nPatchSize2;
		if (nCols2 * nPatchSize2 != nOutputXSize2)
			nCols2++;
		if(nRows1 != nRows2 || nCols1 != nCols2)
		{
			nPatchSize2 -= 2;
			int nRows2 = nOutputYSize2 / nPatchSize2;
			if (nRows2 * nPatchSize2 != nOutputYSize2)
				nRows2++;
			int nCols2 = nOutputXSize2 / nPatchSize2;
			if (nCols2 * nPatchSize2 != nOutputXSize2)
				nCols2++;
			if (nRows1 != nRows2 || nCols1 != nCols2)
				return "无法对齐两景影像！";
		}
	}
	
	GDALDriver *pDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
	GDALDataset *pOutputDataset1 = pDriver->Create(sOutputFile1.c_str(), nOutputXSize1, nOutputYSize1, nRasterCount1, type, NULL);
	GDALDataset *pOutputDataset2 = pDriver->Create(sOutputFile2.c_str(), nOutputXSize2, nOutputYSize2, nRasterCount2, type, NULL);
	
	pOutputDataset1->SetGeoTransform(pOutputGeoTransform1);
	pOutputDataset2->SetGeoTransform(pOutputGeoTransform2);
	pOutputDataset1->SetProjection(pDataset1->GetProjectionRef());
	pOutputDataset2->SetProjection(pDataset2->GetProjectionRef());

	GDALDataset *pRangeDataset = NULL;
	if (sRangeFile != "")
	{
		pRangeDataset = pDriver->Create(sRangeFile.c_str(), nOutputXSize1, nOutputYSize1, 1, GDT_Byte, NULL);
		pRangeDataset->SetProjection(pDataset1->GetProjectionRef());
		pRangeDataset->SetGeoTransform(pGeoTransform1);
	}

	int nXSize1 = 0, nXSize2 = 0, nYSize1 = 0, nYSize2 = 0;
	int nBeginXSize1 = 0, nBeginXSize2 = 0, nBeginYSize1 = 0, nBeginYSize2 = 0;

	int y1 = 0, y2 = 0, x1 = 0, x2 = 0;
	bool bZero1 = false, bZero2 = false;

	unsigned char *pImageCharData1 = NULL, *pImageCharData2 = NULL;
	int *pImageIntData1 = NULL, *pImageIntData2 = NULL;
	float *pImageFloatData1 = NULL, *pImageFloatData2 = NULL;
	unsigned char *pRangeData = NULL;
	int nOldPercent = 0, nNewPercent = 0;
	for(int r = 0; r < nRows1; r++)
	{
		for(int c = 0; c < nCols1; c++)
		{
			nBeginXSize1 = nDeltaXSize1 + c * nPatchSize1;
			nBeginYSize1 = nDeltaYSize1 + r * nPatchSize1;
			nBeginXSize2 = nDeltaXSize2 + c * nPatchSize2;
			nBeginYSize2 = nDeltaYSize2 + r * nPatchSize2;

			nXSize1 = nPatchSize1;
			if (nOutputXSize1 - nBeginXSize1 + nDeltaXSize1 < nPatchSize1)
				nXSize1 = nOutputXSize1 - nBeginXSize1 + nDeltaXSize1;
			nYSize1 = nPatchSize1;
			if (nOutputYSize1 - nBeginYSize1 + nDeltaYSize1 < nPatchSize1)
				nYSize1 = nOutputYSize1 - nBeginYSize1 + nDeltaYSize1;

			nXSize2 = nPatchSize2;
			if (nOutputXSize2 - nBeginXSize2 + nDeltaXSize2 < nPatchSize2)
				nXSize2 = nOutputXSize2 - nBeginXSize2 + nDeltaXSize2;
			nYSize2 = nPatchSize2;
			if (nOutputYSize2 - nBeginYSize2 + nDeltaYSize2 < nPatchSize2)
				nYSize2 = nOutputYSize2 - nBeginYSize2 + nDeltaYSize2;

			if (pRangeDataset != NULL)
			{
				pRangeData = new unsigned char[nXSize1 * nYSize1];
				memset(pRangeData, 255, sizeof(unsigned char) * nXSize1 * nYSize1);
			}

			if(type == GDT_Byte)
			{
				pImageCharData1 = new unsigned char[nXSize1 * nYSize1 * nRasterCount1];
				pImageCharData2 = new unsigned char[nXSize2 * nYSize2 * nRasterCount2];
				pDataset1->RasterIO(GF_Read, nBeginXSize1, nBeginYSize1, nXSize1, nYSize1, pImageCharData1,
					nXSize1, nYSize1, type, nRasterCount1, NULL, 0, 0, 0);
				pDataset2->RasterIO(GF_Read, nBeginXSize2, nBeginYSize2, nXSize2, nYSize2, pImageCharData2,
					nXSize2, nYSize2, type, nRasterCount2, NULL, 0, 0, 0);
				for (int y = 0; y < nYSize1; y++)
				{
					for (int x = 0; x < nXSize1; x++)
					{
						x1 = x;
						y1 = y;
						x2 = (int)(x * nXSize2 * 1.0 / nXSize1 + 0.5);
						if (x2 >= nXSize2)
							x2 = nXSize2 - 1;
						y2 = (int)(y * nYSize2 * 1.0 / nYSize2 + 0.5);
						if (y2 >= nYSize2)
							y2 = nYSize2 - 1;
						bZero1 = true;
						bZero2 = true;
						for (int n = 0; n < nRasterCount1; n++)
						{
							if (pImageCharData1[n * nXSize1 * nYSize1 + y1 * nXSize1 + x1] != 0)
							{
								bZero1 = false;
								break;
							}
						}
						for(int n = 0; n < nRasterCount2; n++)
						{
							if(pImageCharData2[n * nXSize2 * nYSize2 + y2 * nXSize2 + x2] != 0)
							{
								bZero2 = false;
								break;
							}
						}
						if(bZero1 && bZero2)
						{
							for (int n = 0; n < nRasterCount1; n++)
							{
								pImageCharData1[n * nXSize1 * nYSize1 + y1 * nXSize1 + x1] = 0;
							}
							for (int n = 0; n < nRasterCount2; n++)
							{
								pImageCharData2[n * nXSize2 * nYSize2 + y2 * nXSize2 + x2] = 0;
							}
							pRangeData[y1 * nXSize1 + x1] = 0;
						}
					}
				}
				pOutputDataset1->RasterIO(GF_Write, nBeginXSize1 - nDeltaXSize1, nBeginYSize1 - nDeltaYSize1,
					nXSize1, nYSize1, pImageCharData1, nXSize1, nYSize1, type, nRasterCount1, NULL, 0, 0, 0);
				pOutputDataset2->RasterIO(GF_Write, nBeginXSize2 - nDeltaXSize2, nBeginYSize2 - nDeltaYSize2,
					nXSize2, nYSize2, pImageCharData2, nXSize2, nYSize2, type, nRasterCount2, NULL, 0, 0, 0);
				delete[]pImageCharData1;
				delete[]pImageCharData2;
			}
			else if(type == GDT_Int32)
			{
				pImageIntData1 = new int[nXSize1 * nYSize1 * nRasterCount1];
				pImageIntData2 = new int[nXSize2 * nYSize2 * nRasterCount2];
				pDataset1->RasterIO(GF_Read, nBeginXSize1, nBeginYSize1, nXSize1, nYSize1, pImageIntData1,
					nXSize1, nYSize1, type, nRasterCount1, NULL, 0, 0, 0);
				pDataset2->RasterIO(GF_Read, nBeginXSize2, nBeginYSize2, nXSize2, nYSize2, pImageIntData2,
					nXSize2, nYSize2, type, nRasterCount2, NULL, 0, 0, 0);
				for (int y = 0; y < nYSize1; y++)
				{
					for (int x = 0; x < nXSize1; x++)
					{
						x1 = x;
						y1 = y;
						x2 = (int)(x * nXSize2 * 1.0 / nXSize1 + 0.5);
						if (x2 >= nXSize2)
							x2 = nXSize2 - 1;
						y2 = (int)(y * nYSize2 * 1.0 / nYSize2 + 0.5);
						if (y2 >= nYSize2)
							y2 = nYSize2 - 1;
						bZero1 = true;
						bZero2 = true;
						for (int n = 0; n < nRasterCount1; n++)
						{
							if (pImageIntData1[n * nXSize1 * nYSize1 + y1 * nXSize1 + x1] != 0)
							{
								bZero1 = false;
								break;
							}
						}
						for (int n = 0; n < nRasterCount2; n++)
						{
							if (pImageIntData2[n * nXSize2 * nYSize2 + y2 * nXSize2 + x2] != 0)
							{
								bZero2 = false;
								break;
							}
						}
						if (bZero1 && bZero2)
						{
							for (int n = 0; n < nRasterCount1; n++)
							{
								pImageIntData1[n * nXSize1 * nYSize1 + y1 * nXSize1 + x1] = 0;
							}
							for (int n = 0; n < nRasterCount2; n++)
							{
								pImageIntData2[n * nXSize2 * nYSize2 + y2 * nXSize2 + x2] = 0;
							}
							pRangeData[y1 * nXSize1 + x1] = 0;
						}
					}
				}
				pOutputDataset1->RasterIO(GF_Write, nBeginXSize1 - nDeltaXSize1, nBeginYSize1 - nDeltaYSize1,
					nXSize1, nYSize1, pImageIntData1, nXSize1, nYSize1, type, nRasterCount1, NULL, 0, 0, 0);
				pOutputDataset2->RasterIO(GF_Write, nBeginXSize2 - nDeltaXSize2, nBeginYSize2 - nDeltaYSize2,
					nXSize2, nYSize2, pImageIntData2, nXSize2, nYSize2, type, nRasterCount2, NULL, 0, 0, 0);
				delete[]pImageIntData1;
				delete[]pImageIntData2;
			}
			else
			{
				pImageFloatData1 = new float[nXSize1 * nYSize1 * nRasterCount1];
				pImageFloatData2 = new float[nXSize2 * nYSize2 * nRasterCount2];
				pDataset1->RasterIO(GF_Read, nBeginXSize1, nBeginYSize1, nXSize1, nYSize1, pImageFloatData1,
					nXSize1, nYSize1, type, nRasterCount1, NULL, 0, 0, 0);
				pDataset2->RasterIO(GF_Read, nBeginXSize2, nBeginYSize2, nXSize2, nYSize2, pImageFloatData2,
					nXSize2, nYSize2, type, nRasterCount2, NULL, 0, 0, 0);
				for (int y = 0; y < nYSize1; y++)
				{
					for (int x = 0; x < nXSize1; x++)
					{
						x1 = x;
						y1 = y;
						x2 = (int)(x * nXSize2 * 1.0 / nXSize1 + 0.5);
						if (x2 >= nXSize2)
							x2 = nXSize2 - 1;
						y2 = (int)(y * nYSize2 * 1.0 / nYSize2 + 0.5);
						if (y2 >= nYSize2)
							y2 = nYSize2 - 1;
						bZero1 = true;
						bZero2 = true;
						for (int n = 0; n < nRasterCount1; n++)
						{
							if (pImageFloatData1[n * nXSize1 * nYSize1 + y1 * nXSize1 + x1] != 0)
							{
								bZero1 = false;
								break;
							}
						}
						for (int n = 0; n < nRasterCount2; n++)
						{
							if (pImageFloatData2[n * nXSize2 * nYSize2 + y2 * nXSize2 + x2] != 0)
							{
								bZero2 = false;
								break;
							}
						}
						if (bZero1 && bZero2)
						{
							for (int n = 0; n < nRasterCount1; n++)
							{
								pImageFloatData1[n * nXSize1 * nYSize1 + y1 * nXSize1 + x1] = 0;
							}
							for (int n = 0; n < nRasterCount2; n++)
							{
								pImageFloatData2[n * nXSize2 * nYSize2 + y2 * nXSize2 + x2] = 0;
							}
							pRangeData[y1 * nXSize1 + x1] = 0;
						}
					}
				}
				pOutputDataset1->RasterIO(GF_Write, nBeginXSize1 - nDeltaXSize1, nBeginYSize1 - nDeltaYSize1,
					nXSize1, nYSize1, pImageFloatData1, nXSize1, nYSize1, type, nRasterCount1, NULL, 0, 0, 0);
				pOutputDataset2->RasterIO(GF_Write, nBeginXSize2 - nDeltaXSize2, nBeginYSize2 - nDeltaYSize2,
					nXSize2, nYSize2, pImageFloatData2, nXSize2, nYSize2, type, nRasterCount2, NULL, 0, 0, 0);
				delete[]pImageFloatData1;
				delete[]pImageFloatData2;
			}
			
			if(pRangeDataset != NULL)
			{
				pRangeDataset->RasterIO(GF_Write, nBeginXSize1 - nDeltaXSize1, nBeginYSize1 - nDeltaYSize1, nXSize1,
					nYSize1, pRangeData, nXSize1, nYSize1, GDT_Byte, 1, NULL, 0, 0, 0);
				delete[]pRangeData;
			}
			nNewPercent = (r * nCols1 + c) * 100 / nRows1 / nCols1;
			if(nNewPercent != nOldPercent)
			{
				cout << nOldPercent << "%" << endl;
				nOldPercent = nNewPercent;
			}
		}
	}
	GDALClose(pDataset1);
	GDALClose(pDataset2);
	delete pOutputDataset1;
	delete pOutputDataset2;
	if (pRangeDataset != NULL)
		delete pRangeDataset;
	return "";
}

string GetImageUnions(string sImageFile1, string sImageFile2, string sRangeFile)
{
	GDALDataset *pDataset1 = (GDALDataset*)GDALOpen(sImageFile1.c_str(), GA_ReadOnly);
	if (pDataset1 == NULL)
	{
		return "打开影像" + sImageFile1 + "失败！";
	}
	GDALDataset *pDataset2 = (GDALDataset*)GDALOpen(sImageFile2.c_str(), GA_ReadOnly);
	if (pDataset2 == NULL)
	{
		return "打开影像" + sImageFile2 + "失败！";
	}
	int nRasterXSize1 = pDataset1->GetRasterXSize();
	int nRasterYSize1 = pDataset1->GetRasterYSize();
	int nRasterCount1 = pDataset1->GetRasterCount();
	int nRasterXSize2 = pDataset2->GetRasterXSize();
	int nRasterYSize2 = pDataset2->GetRasterYSize();
	int nRasterCount2 = pDataset2->GetRasterCount();

	double *pGeoTransform1 = new double[6];
	pDataset1->GetGeoTransform(pGeoTransform1);
	double *pGeoTransform2 = new double[6];
	pDataset2->GetGeoTransform(pGeoTransform2);
	double dLeft = 0, dRight = 0, dTop = 0, dBottom = 0;

	double *pOutputGeoTransform1 = new double[6];
	double *pOutputGeoTransform2 = new double[6];
	pOutputGeoTransform1[1] = pGeoTransform1[1];
	pOutputGeoTransform1[2] = pGeoTransform1[2];
	pOutputGeoTransform1[4] = pGeoTransform1[4];
	pOutputGeoTransform1[5] = pGeoTransform1[5];

	pOutputGeoTransform2[1] = pGeoTransform2[1];
	pOutputGeoTransform2[2] = pGeoTransform2[2];
	pOutputGeoTransform2[4] = pGeoTransform2[4];
	pOutputGeoTransform2[5] = pGeoTransform2[5];

	int nDeltaXSize1 = 0, nDeltaYSize1 = 0;
	int nDeltaXSize2 = 0, nDeltaYSize2 = 0;


	dLeft = pGeoTransform1[0];
	dTop = pGeoTransform1[3];

	dRight = pGeoTransform1[0] + pGeoTransform1[1] * nRasterXSize1;
	dBottom = pGeoTransform1[3] + pGeoTransform1[5] * nRasterYSize1;

	if (pGeoTransform2[0] > dLeft)
	{
		dLeft = pGeoTransform2[0];
		nDeltaXSize1 = (int)((dLeft - pGeoTransform1[0]) / pGeoTransform1[1] + 0.5);
	}
	else
	{
		nDeltaXSize2 = (int)((dLeft - pGeoTransform2[0]) / pGeoTransform2[1] + 0.5);
	}
	if (pGeoTransform2[3] < dTop)
	{
		dTop = pGeoTransform2[3];
		nDeltaYSize1 = (int)((pGeoTransform1[3] - dTop) / -pGeoTransform1[5] + 0.5);
	}
	else
	{
		nDeltaYSize2 = (int)((pGeoTransform2[3] - dTop) / -pGeoTransform2[5] + 0.5);
	}

	if (pGeoTransform2[0] + pGeoTransform2[1] * nRasterXSize2 < dRight)
		dRight = pGeoTransform2[0] + pGeoTransform2[1] * nRasterXSize2;
	if (pGeoTransform2[3] + pGeoTransform2[5] * nRasterYSize2 > dBottom)
		dBottom = pGeoTransform2[3] + pGeoTransform2[5] * nRasterYSize2;

	pOutputGeoTransform1[0] = pOutputGeoTransform2[0] = dLeft;
	pOutputGeoTransform1[3] = pOutputGeoTransform2[3] = dTop;

	double dXRange = dRight - dLeft, dYRange = dTop - dBottom;
	int nOutputXSize1 = (int)(dXRange / pGeoTransform1[1] + 0.5);
	int nOutputYSize1 = (int)(dYRange / -pGeoTransform1[5] + 0.5);

	if (nOutputXSize1 + nDeltaXSize1 > nRasterXSize1)
		nOutputXSize1 = nRasterXSize1 - nDeltaXSize1;
	if (nOutputYSize1 + nDeltaYSize1 > nRasterYSize1)
		nOutputYSize1 = nRasterYSize1 - nDeltaYSize1;

	int nOutputXSize2 = (int)(dXRange / pGeoTransform2[1] + 0.5);
	int nOutputYSize2 = (int)(dYRange / -pGeoTransform2[5] + 0.5);
	if (nOutputXSize2 + nDeltaXSize2 > nRasterXSize2)
		nOutputXSize2 = nRasterXSize2 - nDeltaXSize2;
	if (nOutputYSize2 + nDeltaYSize2 > nRasterYSize2)
		nOutputYSize2 = nRasterYSize2 - nDeltaYSize2;

	int nPatchSize1 = 500;
	int nPatchSize2 = (int)(nPatchSize1 * pGeoTransform1[1] / pGeoTransform2[1] + 0.5);

	delete[]pGeoTransform1;
	delete[]pGeoTransform2;

	int nRows1 = nOutputYSize1 / nPatchSize1;
	if (nRows1 * nPatchSize1 != nOutputYSize1)
		nRows1++;
	int nCols1 = nOutputXSize1 / nPatchSize1;
	if (nCols1 * nPatchSize1 != nOutputXSize1)
		nCols1++;

	int nRows2 = nOutputYSize2 / nPatchSize2;
	if (nRows2 * nPatchSize2 != nOutputYSize2)
		nRows2++;
	int nCols2 = nOutputXSize2 / nPatchSize2;
	if (nCols2 * nPatchSize2 != nOutputXSize2)
		nCols2++;

	if (nRows1 != nRows2 || nCols1 != nCols2)
	{
		nPatchSize2 += 1;
		int nRows2 = nOutputYSize2 / nPatchSize2;
		if (nRows2 * nPatchSize2 != nOutputYSize2)
			nRows2++;
		int nCols2 = nOutputXSize2 / nPatchSize2;
		if (nCols2 * nPatchSize2 != nOutputXSize2)
			nCols2++;
		if (nRows1 != nRows2 || nCols1 != nCols2)
		{
			nPatchSize2 -= 2;
			int nRows2 = nOutputYSize2 / nPatchSize2;
			if (nRows2 * nPatchSize2 != nOutputYSize2)
				nRows2++;
			int nCols2 = nOutputXSize2 / nPatchSize2;
			if (nCols2 * nPatchSize2 != nOutputXSize2)
				nCols2++;
			if (nRows1 != nRows2 || nCols1 != nCols2)
				return "无法对齐两景影像！";
		}
	}

	GDALDriver *pDriver = GetGDALDriverManager()->GetDriverByName("GTiff");

	GDALDataset *pRangeDataset = NULL;
	pRangeDataset = pDriver->Create(sRangeFile.c_str(), nOutputXSize1, nOutputYSize1, 1, GDT_Byte, NULL);
	pRangeDataset->SetProjection(pDataset1->GetProjectionRef());
	pRangeDataset->SetGeoTransform(pOutputGeoTransform1);

	delete[]pOutputGeoTransform1;
	delete[]pOutputGeoTransform2;

	int nXSize1 = 0, nXSize2 = 0, nYSize1 = 0, nYSize2 = 0;
	int nBeginXSize1 = 0, nBeginXSize2 = 0, nBeginYSize1 = 0, nBeginYSize2 = 0;

	int y1 = 0, y2 = 0, x1 = 0, x2 = 0;
	bool bZero1 = false, bZero2 = false;

	float *pImageFloatData1 = NULL, *pImageFloatData2 = NULL;
	unsigned char *pRangeData = NULL;
	int nOldPercent = 0, nNewPercent = 0;
	for (int r = 0; r < nRows1; r++)
	{
		for (int c = 0; c < nCols1; c++)
		{
			nBeginXSize1 = nDeltaXSize1 + c * nPatchSize1;
			nBeginYSize1 = nDeltaYSize1 + r * nPatchSize1;
			nBeginXSize2 = nDeltaXSize2 + c * nPatchSize2;
			nBeginYSize2 = nDeltaYSize2 + r * nPatchSize2;

			nXSize1 = nPatchSize1;
			if (nOutputXSize1 - nBeginXSize1 + nDeltaXSize1 < nPatchSize1)
				nXSize1 = nOutputXSize1 - nBeginXSize1 + nDeltaXSize1;
			nYSize1 = nPatchSize1;
			if (nOutputYSize1 - nBeginYSize1 + nDeltaYSize1 < nPatchSize1)
				nYSize1 = nOutputYSize1 - nBeginYSize1 + nDeltaYSize1;

			nXSize2 = nPatchSize2;
			if (nOutputXSize2 - nBeginXSize2 + nDeltaXSize2 < nPatchSize2)
				nXSize2 = nOutputXSize2 - nBeginXSize2 + nDeltaXSize2;
			nYSize2 = nPatchSize2;
			if (nOutputYSize2 - nBeginYSize2 + nDeltaYSize2 < nPatchSize2)
				nYSize2 = nOutputYSize2 - nBeginYSize2 + nDeltaYSize2;
			
			pRangeData = new unsigned char[nXSize1 * nYSize1];
			memset(pRangeData, 255, sizeof(unsigned char) * nXSize1 * nYSize1);

			pImageFloatData1 = new float[nXSize1 * nYSize1 * nRasterCount1];
			pImageFloatData2 = new float[nXSize2 * nYSize2 * nRasterCount2];
			pDataset1->RasterIO(GF_Read, nBeginXSize1, nBeginYSize1, nXSize1, nYSize1, pImageFloatData1,
				nXSize1, nYSize1, GDT_Float32, nRasterCount1, NULL, 0, 0, 0);
			pDataset2->RasterIO(GF_Read, nBeginXSize2, nBeginYSize2, nXSize2, nYSize2, pImageFloatData2,
				nXSize2, nYSize2, GDT_Float32, nRasterCount2, NULL, 0, 0, 0);
			for (int y = 0; y < nYSize1; y++)
			{
				for (int x = 0; x < nXSize1; x++)
				{
					x1 = x;
					y1 = y;
					x2 = (int)(x * nXSize2 * 1.0 / nXSize1 + 0.5);
					if (x2 >= nXSize2)
						x2 = nXSize2 - 1;
					y2 = (int)(y * nYSize2 * 1.0 / nYSize2 + 0.5);
					if (y2 >= nYSize2)
						y2 = nYSize2 - 1;
					bZero1 = true;
					bZero2 = true;
					for (int n = 0; n < nRasterCount1; n++)
					{
						if (pImageFloatData1[n * nXSize1 * nYSize1 + y1 * nXSize1 + x1] != 0)
						{
							bZero1 = false;
							break;
						}
					}
					for (int n = 0; n < nRasterCount2; n++)
					{
						if (pImageFloatData2[n * nXSize2 * nYSize2 + y2 * nXSize2 + x2] != 0)
						{
							bZero2 = false;
							break;
						}
					}
					if (bZero1 && bZero2)
					{
						pRangeData[y1 * nXSize1 + x1] = 0;
					}
				}
			}
			delete[]pImageFloatData1;
			delete[]pImageFloatData2;

			pRangeDataset->RasterIO(GF_Write, nBeginXSize1 - nDeltaXSize1, nBeginYSize1 - nDeltaYSize1, nXSize1,
				nYSize1, pRangeData, nXSize1, nYSize1, GDT_Byte, 1, NULL, 0, 0, 0);
			delete[]pRangeData;

			nNewPercent = (r * nCols1 + c) * 100 / nRows1 / nCols1;
			if (nNewPercent != nOldPercent)
			{
				cout << nOldPercent << "%" << endl;
				nOldPercent = nNewPercent;
			}
		}
	}
	GDALClose(pDataset1);
	GDALClose(pDataset2);
	delete pRangeDataset;
	return "";
}

string AddClassToShp(string sShpFile, string sAlterFile, string sOriginalClassField, string sClassField)
{
	GDALDataset *pDataset = (GDALDataset*)GDALOpenEx(sShpFile.c_str(), GDAL_OF_UPDATE, NULL, NULL, NULL);
	if (pDataset == NULL)
		return "打开文件" + sShpFile + "失败！";
	OGRLayer *pLayer = pDataset->GetLayer(0);
	if (pLayer == NULL)
	{
		return "获取图层失败！";
	}

	int index = pLayer->FindFieldIndex(sClassField.c_str(), 0);
	if (index == -1)
	{
		OGRFieldDefn *fieldDefn = new OGRFieldDefn(sClassField.c_str(), OFTString);
		pLayer->CreateField(fieldDefn, 0);
		delete fieldDefn;
	}
	index = pLayer->FindFieldIndex(sClassField.c_str(), 0);
	if (index == -1)
	{
		return "创建字段" + sClassField + "失败！";
	}

	GIntBig nFeatureCount = pLayer->GetFeatureCount(0);
	OGRFeature *pFeature = NULL;
	for (int i = 0; i < nFeatureCount; i++)
	{
		pFeature = pLayer->GetFeature(i);
		if (pFeature == NULL)
			return "获取Feature失败！";
		pFeature->SetField(index, "");
		pLayer->SetFeature(pFeature);
		delete pFeature;
	}

	ifstream in(sAlterFile.c_str());
	if (!in.is_open())
	{
		return "打开文件" + sAlterFile + "失败！";
	}
	int nFID = 0;
	string sClass = "";
	while (in >> nFID >> sClass)
	{
		pFeature = pLayer->GetFeature(nFID);
		if(pFeature == NULL)
		{
			cout << nFID << endl;
			return "获取Feature失败！";
		}
		if (sClass == "")
			return "读取数据错误！";
		string sOriginalClass = pFeature->GetFieldAsString(sOriginalClassField.c_str());
		if ((sOriginalClass == "设施农用地" && sClass == "建设用地") ||
			(sOriginalClass == "园地" && sClass == "林地") ||
			(sOriginalClass == "交通运输用地" && sClass == "建设用地"))
			continue;
		else
		{
			pFeature->SetField(index, sClass.c_str());
			pLayer->SetFeature(pFeature);
		}
		delete pFeature;
	}
	in.close();

	for (int i = 0; i < nFeatureCount; i++)
	{
		pFeature = pLayer->GetFeature(i);
		if(pFeature == NULL)
			return "获取Feature失败！";
		sClass = pFeature->GetFieldAsString(index);
		if(sClass == "")
		{
			pFeature->SetField(index, pFeature->GetFieldAsString(sOriginalClassField.c_str()));
			pLayer->SetFeature(pFeature);
		}
		delete pFeature;
	}

	GDALClose(pDataset);

	return "";
}

string DeleteShpByFID(string sShpFile, string sFIDFile)
{
	GDALDataset *pDataset = (GDALDataset*)GDALOpenEx(sShpFile.c_str(), GDAL_OF_UPDATE, NULL, NULL, NULL);
	if (pDataset == NULL)
		return "打开文件" + sShpFile + "失败！";
	OGRLayer *pLayer = pDataset->GetLayer(0);
	if (pLayer == NULL)
	{
		return "获取图层失败！";
	}

	ifstream in(sFIDFile.c_str());
	if (!in.is_open())
		return "打开文件失败！";
	int nFID = 0;
	OGRFeature *pFeature = NULL;
	while (in >> nFID)
	{
		pFeature = pLayer->GetFeature(nFID);
		if (pFeature == NULL)
			return "获取Feature失败！";
		delete pFeature;
		pLayer->DeleteFeature(nFID);
	}
	in.close();
	GDALClose(pDataset);
	return "";
}

string GenerateSamples(string sShpFile, string sTrainingShpFile, string sTestingShpFile, string sClassField, double dRatio)
{
	GDALDataset *pDataset = (GDALDataset*)GDALOpenEx(sShpFile.c_str(), GDAL_OF_READONLY, NULL, NULL, NULL);
	if (pDataset == NULL)
		return "打开文件" + sShpFile + "失败！";

	OGRLayer *pLayer = pDataset->GetLayer(0);
	if (pLayer == NULL)
		return "获取图层失败！";

	CopyShp(sShpFile, sTrainingShpFile);
	CopyShp(sShpFile, sTestingShpFile);

	GDALDataset *pTrainingDataset = (GDALDataset*)GDALOpenEx(sTrainingShpFile.c_str(), GDAL_OF_UPDATE, NULL, NULL, NULL);
	if (pTrainingDataset == NULL)
		return "打开文件" + sTrainingShpFile + "失败！";

	OGRLayer *pTrainingLayer = pTrainingDataset->GetLayer(0);
	if (pTrainingLayer == NULL)
		return "获取图层失败！";

	GDALDataset *pTestingDataset = (GDALDataset*)GDALOpenEx(sTestingShpFile.c_str(), GDAL_OF_UPDATE, NULL, NULL, NULL);
	if (pTestingDataset == NULL)
		return "打开文件" + sTestingShpFile + "失败！";
	OGRLayer *pTestingLayer = pTestingDataset->GetLayer(0);
	if (pTestingLayer == NULL)
		return "获取图层失败！";

	map<string, vector<int>> mClassFID;
	int nFeatureCount = pLayer->GetFeatureCount(0);
	OGRFeature *pFeature = NULL;
	for (int i = 0; i < nFeatureCount; i++)
	{
		pFeature = pLayer->GetFeature(i);
		string sClass = pFeature->GetFieldAsString(sClassField.c_str());
		delete pFeature;
		if (mClassFID.find(sClass) == mClassFID.end())
			mClassFID.insert(pair<string, vector<int>>(sClass, vector<int>()));
		mClassFID[sClass].push_back(i);
	}
	vector<int> vTrainingFID, vTestingFID;
	for (map<string, vector<int>>::iterator it = mClassFID.begin(); it != mClassFID.end(); it++)
	{
		vector<int> vFID = it->second;
		int n = (int)vFID.size();
		Random((int)(n * dRatio), n, vTrainingFID, vTestingFID);
		for (int i = 0; i < (int)vTrainingFID.size(); i++)
		{
			pTestingLayer->DeleteFeature(vFID[vTrainingFID[i]]);
		}
		for(int i = 0; i < (int)vTestingFID.size(); i++)
		{
			pTrainingLayer->DeleteFeature(vFID[vTestingFID[i]]);
		}
	}
	GDALClose(pDataset);
	GDALClose(pTestingDataset);
	GDALClose(pTrainingDataset);
	return "";
}

string GetFileNameWithoutExtension(string sFileName)
{
	int nIndex1 = sFileName.find_last_of('\\', sFileName.size());
	int nIndex2 = sFileName.find_last_of('.', sFileName.size());
	if(nIndex1 < nIndex2)
	{
		return sFileName.substr(nIndex1 + 1, nIndex2 - nIndex1 - 1);
	}
	else
	{
		return sFileName.substr(nIndex1 + 1, sFileName.size() - nIndex1 - 1);
	}
}

string GetDirectory(string sFileName)
{
	int nIndex = sFileName.find_last_of('\\', sFileName.size());
	return sFileName.substr(0, nIndex);
}

void Random(int m, int n, vector<int>& v1, vector<int>& v2)
{
	if(m >= n)
	{
		cout << "m要比n小" << endl;
		return;
	}
	v1.clear();
	v2.clear();
	vector<int> v;
	for (int i = 0; i < n; i++)
		v.push_back(i);
	random_device rd;
	mt19937 g(rd());
	shuffle(v.begin(), v.end(), g);
	for (int i = 0; i < m; i++)
		v1.push_back(v[i]);
	for (int i = m; i < n; i++)
		v2.push_back(v[i]);
}

void CopyShp(string sShp1, string sShp2)
{
	CopyFile(stringToLPCWSTR(sShp1), stringToLPCWSTR(sShp2), FALSE);
	string sTemp1 = sShp1.substr(0, (int)sShp1.size() - 4);
	string sTemp2 = sShp2.substr(0, (int)sShp2.size() - 4);
	string s = sTemp1 + ".dbf";
	if (IsFileExist(s))
		CopyFile(stringToLPCWSTR(s), stringToLPCWSTR(sTemp2 + ".dbf"), FALSE);
	s = sTemp1 + ".prj";
	if (IsFileExist(s))
		CopyFile(stringToLPCWSTR(s), stringToLPCWSTR(sTemp2 + ".prj"), FALSE);
	s = sTemp1 + ".sbn";
	if (IsFileExist(s))
		CopyFile(stringToLPCWSTR(s), stringToLPCWSTR(sTemp2 + ".sbn"), FALSE);
	s = sTemp1 + ".sbx";
	if (IsFileExist(s))
		CopyFile(stringToLPCWSTR(s), stringToLPCWSTR(sTemp2 + ".sbx"), FALSE);
	s = sTemp1 + ".shx";
	if (IsFileExist(s))
		CopyFile(stringToLPCWSTR(s), stringToLPCWSTR(sTemp2 + ".shx"), FALSE);
}

bool IsFileExist(string sFile)
{
	ifstream in(sFile.c_str());
	bool bResult = in.is_open();
	in.close();
	return bResult;
}

LPCWSTR stringToLPCWSTR(string orig)
{
	size_t origsize = orig.length() + 1;
	size_t convertedChars = 0;
	wchar_t *wcstring = (wchar_t *)malloc(sizeof(wchar_t)*(orig.length() - 1));
	mbstowcs_s(&convertedChars, wcstring, origsize, orig.c_str(), _TRUNCATE);
	return wcstring;
}

string CheckShp(string sShpFile)
{
	GDALDataset *pDataset = (GDALDataset*)GDALOpenEx(sShpFile.c_str(), GDAL_OF_READONLY, NULL, NULL, NULL);
	if(pDataset == NULL)
	{
		return "打开文件" + sShpFile + "失败！";
	}
	OGRLayer *pLayer = pDataset->GetLayer(0);
	if (pLayer == NULL)
		return "获取图层失败！";

	OGRFeature *pFeature = pLayer->GetNextFeature();
	while(pFeature != NULL)
	{
		OGRGeometry *pGeometry = pFeature->GetGeometryRef();
		if (pGeometry == NULL)
			cout << "要素" << pFeature->GetFID() << "为空！" << endl;
		delete pFeature;
		pFeature = pLayer->GetNextFeature();
	}
	GDALClose(pDataset);
}

string AddFeatureToShp(string sShpFile, string sFeatureFile)
{
	GDALDataset *pDataset = (GDALDataset*)GDALOpenEx(sShpFile.c_str(), GDAL_OF_UPDATE, NULL, NULL, NULL);
	if (pDataset == NULL)
		return "打开文件" + sShpFile + "失败！";
	OGRLayer *pLayer = pDataset->GetLayer(0);
	if (pLayer == NULL)
		return "获取图层失败！";

	
}

void DeleteTif(string sFile)
{
	ifstream in(sFile.c_str());
	if (in.is_open())
	{
		in.close();
		remove(sFile.c_str());
	}
	string sTempFile = sFile.substr(0, (int)sFile.size() - 4) + ".ovr";
	in.open(sTempFile.c_str());
	if (in.is_open())
	{
		in.close();
		remove(sTempFile.c_str());
	}

	sTempFile = sFile.substr(0, (int)sFile.size() - 4) + ".tfw";
	in.open(sTempFile.c_str());
	if (in.is_open())
	{
		in.close();
		remove(sTempFile.c_str());
	}

	sTempFile = sFile.substr(0, (int)sFile.size() - 4) + ".tif.aux.xml";
	in.open(sTempFile.c_str());
	if (in.is_open())
	{
		in.close();
		remove(sTempFile.c_str());
	}

	sTempFile = sFile.substr(0, (int)sFile.size() - 4) + ".tif.ovr";
	in.open(sTempFile.c_str());
	if (in.is_open())
	{
		in.close();
		remove(sTempFile.c_str());
	}

	sTempFile = sFile.substr(0, (int)sFile.size() - 4) + ".xml";
	in.open(sTempFile.c_str());
	if (in.is_open())
	{
		in.close();
		remove(sTempFile.c_str());
	}

	sTempFile = sFile.substr(0, (int)sFile.size() - 4) + ".tif.vat.dbf";
	in.open(sTempFile.c_str());
	if (in.is_open())
	{
		in.close();
		remove(sTempFile.c_str());
	}

	sTempFile = sFile.substr(0, (int)sFile.size() - 4) + ".tif.vat.cpg";
	in.open(sTempFile.c_str());
	if (in.is_open())
	{
		in.close();
		remove(sTempFile.c_str());
	}

	sTempFile = sFile.substr(0, (int)sFile.size() - 4) + ".hdr";
	in.open(sTempFile.c_str());
	if (in.is_open())
	{
		in.close();
		remove(sTempFile.c_str());
	}

	sTempFile = sFile.substr(0, (int)sFile.size() - 4) + ".tif.xml";
	in.open(sTempFile.c_str());
	if (in.is_open())
	{
		in.close();
		remove(sTempFile.c_str());
	}
}