#include <iterator>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <iostream>
#include <bitset>

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif


std::string  hamming_encode(const  char *);
unsigned int hamming_decode(std::string frame,unsigned int , unsigned int , unsigned int *, unsigned int *);
int postionInfraeme(int );


#ifdef __cplusplus
}
#endif

