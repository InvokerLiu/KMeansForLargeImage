#pragma once
#include <iostream>
#include <fstream>
#include<string>
#include <map>
#include <random>
#include <sstream>
#include <algorithm>
#include <Windows.h>
using namespace std;

#include "gdal_priv.h"
#include "cpl_conv.h"
#include "gdal_alg.h"
#include "ogrsf_frmts.h"


//获取两景遥感影像空间上相交的部分
string GetImageUnions(string sImageFile1, string sImageFile2, string sOutputFile1, string sOutputFile2, string sRangeFile = "", GDALDataType type = GDT_Byte);

//获取两景遥感影像空间上相交的部分
string GetImageUnions(string sImageFile1, string sImageFile2, string sRangeFile);


string AddClassToShp(string sShpFile, string sAlterFile, string sOriginalClassField, string sClassField = "类别");

string DeleteShpByFID(string sShpFile, string sFIDFile);

string GenerateSamples(string sShpFile, string sTrainingShpFile, string sTestingShpFile, string sClassField, double dRatio = 0.5);

string GetFileNameWithoutExtension(string sFileName);

string GetDirectory(string sFileName);

void Random(int m, int n, vector<int>& v1, vector<int>& v2);

void CopyShp(string sShp1, string sShp2);

bool IsFileExist(string sFile);

string CheckShp(string sShpFile);

LPCWSTR stringToLPCWSTR(string orig);

string AddFeatureToShp(string sShpFile, string sCSVFile);

void DeleteTif(string sFile);

//string ReadCSV(string sCSVFile, vector<string>& vCSVData);
//
//vector<string> Split(string s, char c);

