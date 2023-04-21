//
// Created by hussein on 4/20/23.
//

#include <fixed_point.h>
#define F 16384   // 2^14


int makeFirstFP_thenADD(int x, int y){
    return x * F + y;
}
int makeFirstFP_thenSup(int x, int y){
    return x * F - y;
}
int addTwoFP(int x, int y){
    return x + y;
}
int subTwoFP(int x, int y){
    return x - y;
}

int FirstFPSecondINT_thenMultiply(int x, int y){
    return x * y;
}
int multiplyTwoFP(int x, int y){
    return ((int64_t)x) * y / F;
}
int makeFirstFP_thenDivide(int x, int y){
    return (x * F / y) ;
}

int FirstFPSecondINT_thenDivide(int x, int y){
    return x / y;
}
int divideTwoFP(int x, int y){
    return ((int64_t)x * F / y);
}
int convertToFP(int x){
    return x * F;
}
int convertTOInt(int x){
    if(x < 0) return (x - F / 2) / F;
    return  (x + F / 2) / F;
}

