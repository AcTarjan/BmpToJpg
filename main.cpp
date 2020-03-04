//
// Created by 96351 on 2020/3/4.
//
#include "bmp.h"
#include <time.h>
using namespace std;
int main(){
    Bmp bmp = Bmp("E:\\Project_clion\\BmpToJpg\\resource\\w2.bmp");
    bmp.show();
    long now=clock();//存储图像处理开始时间
    bmp.encode2JPG("E:\\Project_clion\\BmpToJpg\\JPG\\w2.jpg",50,0x21,0x11,0x11);
    cout<<int(((double)(clock()-now))/CLOCKS_PER_SEC*1000)<<"ms"<<endl;
    bmp.save();
    return 0;
}