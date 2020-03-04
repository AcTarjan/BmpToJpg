//
// Created by 96351 on 2020/2/28.
//
#include "bmp.h"
Bmp::Bmp(char *sFileName) {
    fileName = sFileName;
    rgb = NULL;
    ycbcr = NULL;
    readBMP();
}
bool Bmp::readBMP() {
    FILE* fp = fopen(fileName,"rb");
    if(!fp)
        return false;
    BITMAPFILEHEADER fileHeader;//文件头
    BITMAPINFOHEADER infoHeader;//信息头
    fread(&fileHeader, sizeof(BITMAPFILEHEADER),1,fp);
    fread(&infoHeader, sizeof(BITMAPINFOHEADER),1,fp);
    size = fileHeader.bfSize;
    height = infoHeader.biHeight;
    width = infoHeader.biWidth;
    nBitCount = infoHeader.biBitCount;
    _height = (height+31)/32*32;
    _width = (width+31)/32*32;
    cout<<_height<<" "<<_width<<endl;
    rgb = (Pixel**)malloc(sizeof(Pixel*)*_height);
    for(int i=0;i<_height;i++)
        rgb[i] = (Pixel*)malloc(sizeof(Pixel)*_width);
    int offset = ((width*3+3)/4*4) - width*3;
    for(int i=height-1;i>=0;i--) {
        for (int j = 0; j < width; j++) {
            fread(&rgb[i][j], 3, 1, fp);
        }
        fseek(fp,offset,SEEK_CUR);
    }
    fclose(fp);
    return true;
}
void Bmp::show() {
    cout<<"图片大小："<<size<<"Byte"<<endl;
    cout<<"图片高度："<<height<<endl;
    cout<<"图片宽度："<<width<<endl;
    cout<<"图片位数："<<(int)nBitCount<<endl;
}
void Bmp::save() {
    ofstream r("E:\\Project_clion\\BmpToJpg\\data\\r.csv");
    ofstream y("E:\\Project_clion\\BmpToJpg\\data\\g.csv");
    ofstream dct_y("E:\\Project_clion\\BmpToJpg\\data\\b.csv");
    for(int i=0;i<height;i++){
        for(int j=0;j<width;j++) {
            r << ycbcr[i][j].nY << " ";
            y << (int)rgb[i][j].byGreen<<" ";
            dct_y<<(int)rgb[i][j].byBlue<<" ";
        }
       r<<endl;y<<endl;dct_y<<endl;
    }
    r.close();y.close();dct_y.close();
}
void Bmp::convert2YCbCr(int x,int y,int flag) {
    for(int i=x;i<x+8;i++)
        for(int j=y;j<y+8;j++){
            if(flag==1)
                ycbcr[i][j].nY = round(0.299*rgb[i][j].byRed + 0.587*rgb[i][j].byGreen + 0.114*rgb[i][j].byBlue);
            else if(flag==2)
                ycbcr[i][j].nCb = round(-0.1687*rgb[i][j].byRed - 0.3313*rgb[i][j].byGreen + 0.5*rgb[i][j].byBlue+128);
            else if(flag==3)
                ycbcr[i][j].nCr = round(0.5*rgb[i][j].byRed - 0.4187*rgb[i][j].byGreen -0.0813*rgb[i][j].byBlue+128);
        }
}
void Bmp::forwardDCT(int x, int y,int flag,int* data) {
    float s = 1/sqrt(8.0f);
    for(int i=0;i<8;i++)
        for(int j=0;j<8;j++){
            float sum=0;
            for(int u=0;u<8;u++)
                for(int v=0;v<8;v++){
                    if(flag==1)
                        sum+=dctTable[u][i]*dctTable[v][j]*(ycbcr[x+u][y+v].nY-128);
                    else if(flag==2)
                        sum+=dctTable[u][i]*dctTable[v][j]*(ycbcr[x+u][y+v].nCb-128);
                    else if(flag==3)
                        sum+=dctTable[u][i]*dctTable[v][j]*(ycbcr[x+u][y+v].nCr-128);
                }
            float alphaU = i==0?s:0.5f;
            float alphaV = j==0?s:0.5f;
            float temp = alphaU*alphaV;
            if(flag==1)
                data[ZigZag[i][j]] = round(temp*sum/QyTable[ZigZag[i][j]]);
            else
                data[ZigZag[i][j]] = round(temp*sum/QcTable[ZigZag[i][j]]);
        }
}
bool Bmp::encode2JPG(char* fileName,int qualityScale,unsigned char YHV, unsigned char CbHV, unsigned char CrHV) {
    FILE* fp = fopen(fileName,"wb");
    if(!fp) {
        cout<<"false"<<endl;
        return false;
    }
    _initQualityTable(qualityScale);//初始化量化表
    writeJpgHeader(fp,YHV,CbHV,CrHV);//写文件
    int lastY=0,lastCb=0,lastCr=0;
    int byte=0, bytepos=7;
    _init(YHV,CbHV,CrHV);//分配内存
    _initHuffmanTable();//初始化哈夫曼表
    int x1=0,y1=0,x2=0,y2=0,x3=0,y3=0;
    while(1){
        //Y分量
        handleMCU(1,x1,y1,yHV.H,yHV.V,yHV.hInterval,yHV.vInterval,lastY,byte,bytepos,fp);
        //Cb分量
        handleMCU(2,x2,y2,cbHV.H,cbHV.V,cbHV.hInterval,cbHV.vInterval,lastCb,byte,bytepos,fp);
        //Cr分量
        handleMCU(3,x3,y3,crHV.H,crHV.V,crHV.hInterval,crHV.vInterval,lastCr,byte,bytepos,fp);

        if(x1>=_height)
            break;
    }
    //EOI
    _writeWord(0xFFD9,fp); //图片结束标志
    fclose(fp);
    return true;
}
void Bmp::handleMCU(int flag, int &x, int& y, int H, int V, int _H, int _V,int &last,int& byte,int& bytepos,FILE* fp) {
    int data[64];BitString output[128];
    int count;
    for(int i=0;i<H;i++) {
        for (int j = 0; j < V; j++) {
            convert2YCbCr(x + i * 8 * _H, y + 8 * j * _V, flag);
            forwardDCT(x + i * 8 * _H, y + 8 * j * _V, flag, data);
            if (flag == 1)
                count = huffmanCoding(last, data, hY_DC, hY_AC, output);
            else
                count = huffmanCoding(last, data, hC_DC, hC_AC, output);
            writeBitString(output, count, byte, bytepos, fp);
        }
    }
    y+=V*8*_V;
    if(y>=_width){
        y=0;
        x+=H*8*_H;
    }
    return ;
}
void Bmp::_init(unsigned char YHV, unsigned char CbHV, unsigned char CrHV) {
    //初试化YCbCr采样因子
    yHV.H= (YHV&0xf0)>>4;
    cbHV.H = (CbHV&0xf0)>>4;
    crHV.H = (CrHV&0xf0)>>4;
    int maxH = max(max(yHV.H,cbHV.H),crHV.H);
    yHV.hInterval = maxH/yHV.H;
    cbHV.hInterval = maxH/cbHV.H;
    crHV.hInterval = maxH/crHV.H;

    yHV.V = YHV&0x0f;
    cbHV.V = CbHV&0x0f;
    crHV.V = CrHV&0x0f;
    int maxV = max(max(yHV.V,cbHV.V),crHV.V);
    yHV.vInterval = maxV/yHV.V;
    cbHV.vInterval = maxV/cbHV.V;
    crHV.vInterval = maxV/crHV.V;
    _height = (height+maxH*8-1)/(maxH*8)*maxH*8;
    _width = (width+maxV*8-1)/(maxV*8)*maxV*8;
    //为ycbcr分配内存
    ycbcr = (Node**)malloc(sizeof(Node*)*_height);
    for(int i=0;i<_height;i++)
        ycbcr[i] = (Node *) malloc(sizeof(Node) * _width);
    //初始化dctTable
    const float PI = 3.1415926f;
    for(int i=0;i<8;i++)
        for(int j=0;j<8;j++)
            dctTable[i][j] = cos((2*i+1)*j*PI/16);
}
void Bmp::_initQualityTable(int qualityScale) {
    if(qualityScale<=0) qualityScale=1;
    if(qualityScale>=100) qualityScale=99;
    for(int i=0;i<8;i++)
        for(int j=0;j<8;j++){
            int temp = ((int)(yQualityTable[i][j]*qualityScale + 50) / 100);
            if (temp<=0) temp = 1;
            if (temp>0xFF) temp = 0xFF;
            QyTable[ZigZag[i][j]] = (unsigned char)temp;

            temp = ((int)(cQualityTable[i][j]*qualityScale + 50) / 100);
            if (temp<=0) 	temp = 1;
            if (temp>0xFF) temp = 0xFF;
            QcTable[ZigZag[i][j]] = (unsigned char)temp;
        }
}
void Bmp::_initHuffmanTable() {
    memset(hY_DC,0, sizeof(hY_DC));
    getHuffmanTable(Standard_DC_Luminance_NRCodes,Standard_DC_Luminance_Values,hY_DC);

    memset(hY_AC,0, sizeof(hY_AC));
    getHuffmanTable(Standard_AC_Luminance_NRCodes,Standard_AC_Luminance_Values,hY_AC);

    memset(hC_DC, 0, sizeof(hC_DC));
    getHuffmanTable(Standard_DC_Chrominance_NRCodes,Standard_DC_Chrominance_Values,hC_DC);

    memset(hC_AC,0, sizeof(hC_AC));
    getHuffmanTable(Standard_AC_Chrominance_NRCodes,Standard_AC_Chrominance_Values,hC_AC);
}
void Bmp::getHuffmanTable(const char *numTable,const unsigned char *posTable, BitString *huffmanTable) {
    int index = 0;
    int code = 0;
    for(int i=0;i<16;i++) {
        for (int j = 0; j < numTable[i]; j++) {
            huffmanTable[posTable[index]].length = i + 1;
            huffmanTable[posTable[index]].value = code;
            code++;
            index++;
        }
        code<<=1;
    }
}
int Bmp::huffmanCoding(int &last, int *data, BitString* HD, BitString* HA, BitString *output) {
    int index = 0;
    //DC分量处理
    int diff = data[0] - last;
    last  = data[0];
    if(diff == 0){
        output[index++] = HD[0x00];
    }else{
        BitString bs = getValueBit(diff);
        output[index++] = HD[bs.length];
        output[index++] = bs;
    }
    //AC分量处理
    BitString EOB = HA[0x00];
    BitString SIXTEENZEROS = HA[0xf0];
    int endpos = 63;
    while(data[endpos]==0&&endpos>0)
        endpos--;
    for(int i=1;i<=endpos;i++){
        int startpos = i;
        while(data[i]==0&&i<=endpos)
            i++;
        int count = i - startpos;
        while(count>=16){
            output[index++] = SIXTEENZEROS;
            count -= 16;
        }
        BitString bs = getValueBit(data[i]);
        output[index++] = HA[count<<4|bs.length];
        output[index++] = bs;
    }
    if(endpos!=63)
        output[index++] = EOB;
    return index;
}
void Bmp::writeBitString(BitString *data, int count, int &byte, int &bytepos, FILE* fp) {
    unsigned short mask[]  = {1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768};
    for(int i=0;i<count;i++){
        int value = data[i].value;
        int pos = data[i].length - 1;
        while(pos>=0){
            if(value&mask[pos]){
                byte = byte|mask[bytepos];
            }
            pos--;
            bytepos--;
            if(bytepos<0){
                _writeByte((unsigned char)byte,fp);
                if(byte==0xff)
                    _writeByte((unsigned char)0,fp);
                byte=0;
                bytepos=7;
            }
        }
    }
}
BitString Bmp::getValueBit(int value) {
    int v = value>0?value:-value;
    int length = 0;
    while(v>0){
        length++;
        v>>=1;
    }
    BitString bs;
    bs.length = length;
    bs.value = value>0?value:(1<<length)+value-1;
    return bs;
}
void Bmp::_writeByte(unsigned char value, FILE* fp)
{
    _write(&value, 1, fp);
}
void Bmp::_writeWord(unsigned short value, FILE* fp)
{
    unsigned short _value = ((value>>8)&0xFF) | ((value&0xFF)<<8);
    _write(&_value, 2, fp);
}
void Bmp::_write(const void* p, int byteSize, FILE* fp)
{
    fwrite(p, 1, byteSize, fp);
}
void Bmp::writeJpgHeader(FILE *fp, unsigned char YHV, unsigned char CbHV, unsigned char CrHV) {
    //SOI
    _writeWord(0xFFD8, fp);		// marker = 0xFFD8 图像开始标志

    //APPO
    _writeWord(0xFFE0,fp);		// marker = 0xFFE0
    _writeWord(16, fp);			// 应用标志长度
    _write("JFIF", 5, fp);			// 'JFIF\0'
    _writeByte(1, fp);			// 主版本号
    _writeByte(1, fp);			// 次版本号
    _writeByte(1, fp);			// 单位：0表示：无单位；1表示：英寸/点数；2表示：厘米/点数
    _writeWord(0x78, fp);			// 水平分辨率
    _writeWord(0x78, fp);			// 竖直分辨率
    _writeByte(0, fp);			// 水平点数
    _writeByte(0, fp);			// 竖直点数

    //DQT
    _writeWord(0xFFDB, fp);		//marker = 0xFFDB 量化表标志
    _writeWord(132, fp);			//size=132 标志长度
    _writeByte(0x00, fp);			//高4位是量化表精度，0表示8位，1表示16位；低四位定义量化表标识
    _write(QyTable, 64, fp);		//YTable   按z字形存储
    _writeByte(0x01, fp);			//高4位是量化表精度，0表示8位，1表示16位；低四位定义量化表标识
    _write(QcTable, 64, fp);	//CbCrTable

    //SOFO
    _writeWord(0xFFC0, fp);			//marker = 0xFFC0 帧开始标记
    _writeWord(17, fp);				//length = 17 for a truecolor YCbCr JPG 参数长度
    _writeByte(8, fp);				//precision = 8: 8 bits/sample 采样精度
    _writeWord(height&0xFFFF, fp);	//height 行数
    _writeWord(width&0xFFFF, fp);	//width 列数
    _writeByte(3, fp);				//Nr= 3: We encode a truecolor JPG 分量数

    _writeByte(1, fp);				//IdY = 1 分量标志
    _writeByte(YHV, fp);				//HVY sampling factors for Y (bit 0-3 vert., 4-7 hor.)(SubSamp 1x1)竖直水平采样因子
    _writeByte(0, fp);				//QTY  Quantization Table number for Y = 0 使用的量化表

    _writeByte(2, fp);				//IdCb = 2
    _writeByte(CbHV, fp);				//HVCb = 0x11(SubSamp 1x1)
    _writeByte(1, fp);				//QTCb = 1

    _writeByte(3, fp);				//IdCr = 3
    _writeByte(CrHV, fp);				//HVCr = 0x11 (SubSamp 1x1)
    _writeByte(1, fp);				//QTCr Normally equal to QTCb = 1

    //DHT
    _writeWord(0xFFC4, fp);		//marker = 0xFFC4 哈夫曼表标志
    _writeWord(0x01A2, fp);		//length = 0x01A2 标志长度

    _writeByte(0x00, fp);			//高4位：0表示DC，1表示AC；低四位：表标志，0表示Y
    _write(Standard_DC_Luminance_NRCodes, sizeof(Standard_DC_Luminance_NRCodes), fp);	//DC_L_NRC
    _write(Standard_DC_Luminance_Values, sizeof(Standard_DC_Luminance_Values), fp);		//DC_L_VALUE

    _writeByte(0x10, fp);			//HTYAC
    _write(Standard_AC_Luminance_NRCodes, sizeof(Standard_AC_Luminance_NRCodes), fp);
    _write(Standard_AC_Luminance_Values, sizeof(Standard_AC_Luminance_Values), fp); //we'll use the standard Huffman tables

    _writeByte(0x01, fp);			//高4位：0表示DC，1表示AC；低四位：表标志，1表示C
    _write(Standard_DC_Chrominance_NRCodes, sizeof(Standard_DC_Chrominance_NRCodes), fp);
    _write(Standard_DC_Chrominance_Values, sizeof(Standard_DC_Chrominance_Values), fp);

    _writeByte(0x11, fp);			//HCAC
    _write(Standard_AC_Chrominance_NRCodes, sizeof(Standard_AC_Chrominance_NRCodes), fp);
    _write(Standard_AC_Chrominance_Values, sizeof(Standard_AC_Chrominance_Values), fp);

    //SOS
    _writeWord(0xFFDA, fp);		//marker = 0xFFC4 扫描头标志
    _writeWord(12, fp);			//length = 12  扫描头长度
    _writeByte(3, fp);			//nrofcomponents, Should be 3: truecolor JPG 扫描分量

    _writeByte(1, fp);			//Idy=1
    _writeByte(0x00, fp);			//HTY	bits 0..3: AC table (0..3)
    //		bits 4..7: DC table (0..3)
    _writeByte(2, fp);			//IdCb
    _writeByte(0x11, fp);			//HTCb

    _writeByte(3, fp);			//IdCr
    _writeByte(0x11, fp);			//HTCr

    _writeByte(0, fp);			//Ss not interesting, they should be 0,63,0
    _writeByte(0x3F, fp);			//Se
    _writeByte(0, fp);			//Bf
}