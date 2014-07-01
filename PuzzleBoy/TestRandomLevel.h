#pragma once

#include <string.h>

class PuzzleBoyLevelData;
class MT19937;

//return value: non-zero=abort (?)
typedef int (*RandomLevelCallback)(void* userData,float progress);

//Generate a random level with rotate block only.
//Note: width and height must <=8
//return value is the best step of generated level, 0=failed
int RandomTest(int width,int height,PuzzleBoyLevelData*& outputLevel,MT19937* rnd,void *userData=NULL,RandomLevelCallback callback=NULL);
