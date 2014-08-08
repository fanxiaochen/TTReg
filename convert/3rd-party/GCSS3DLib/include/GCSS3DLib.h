//#include <afx.h>
#include "windows.h"


int DecodeImgs(LPCSTR filepath, int Threshold_Contrast, int Threshold_Saturation);

unsigned char** Create2DArrayChar(int h, int w);

double** Create2DArrayDouble(int h, int w);

int ** Create2DArrayInt(int h, int w);

void CreatBMPfromArrayInt(unsigned char *lpimg, int **array, int imgW, int imgH, double scale);

void CreatBMPfromArrayDouble(unsigned char *lpimg, double **array, int imgW, int imgH, double scale);

void ReleaseMemory();

void SaveData(LPCTSTR savefilename, bool b_outputXC, bool outputXP);

void SaveData_Img(LPCTSTR savefilename);

double **GetXC();

double **GetXP();

double *GetImg_x();

double *GetImg_y();