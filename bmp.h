//
// Created by 96351 on 2020/3/1.
//

#ifndef BMPTOJPG_BMP_H
#define BMPTOJPG_BMP_H
#include <stdio.h>
#include <iostream>
#include "table.h"
#include <math.h>
#include <windows.h>
#include <fstream>
using namespace std;
struct Pixel{//BMP图片的像素点
    unsigned char byBlue;
    unsigned char byGreen;
    unsigned char byRed;
};
struct Node{//转换色域后的数据
    short int nY;
    short int nCb;
    short int nCr;
};
struct HV{
    short int H;
    short int V;
    short int hInterval;
    short int vInterval;
};
struct BitString{
    int length;
    int value;
};
class Bmp{//创建BMP图片类
public:
    Bmp(char* sFileName);
    bool encode2JPG(char* fileName,int qualityScale, unsigned char YHV=0x11, unsigned char CbHV=0x11, unsigned char CrHV=0x11);
    void show();
    void save();
private:
    char* fileName;
    int size;//图片大小
    int height;//图片高度
    int width;//图片宽度
    int _height;//8的倍数
    int _width;//8的倍数
    short nBitCount;//图片位数
    float dctTable[8][8]={};
    HV yHV,cbHV,crHV;
    Pixel** rgb;//存储图片的RGB值
    Node** ycbcr;//存储转换后的YCbCr值
    unsigned char QyTable[64],QcTable[64];
    BitString hY_DC[16],hY_AC[256],hC_DC[16],hC_AC[256];
    bool readBMP();//读取BMP图片
    void convert2YCbCr(int x,int y,int flag);//将rgb转换为ycbcr
    void forwardDCT(int x,int y,int flag,int* data);//将ycbcr从(x,y)处进行8*8的dct变换
    void _init(unsigned char YHV, unsigned char CbHV, unsigned char CrHV);//分配内存
    void _initQualityTable(int qualityScale);
    void _initHuffmanTable();
    void getHuffmanTable(const char* numTable,const unsigned char* posTable,BitString* huffmanTable);
    int huffmanCoding(int&last,int* data,BitString* DC,BitString* AC,BitString* output);
    void handleMCU(int flag, int &x, int& y, int H, int V, int _H, int _V,int &last,int& byte,int& bytepos,FILE* fp);
    BitString getValueBit(int value);
    void writeBitString(BitString* data,int count,int& byte,int& bytepos,FILE* fp);
    void writeJpgHeader(FILE* fp, unsigned char YHV, unsigned char CbHV, unsigned char CrHV);
    void _writeByte(unsigned char value,FILE* fp);
    void _writeWord(unsigned short value,FILE* fp);
    void _write(const void* p,int size,FILE* fp);
};
#endif //BMPTOJPG_BMP_H
