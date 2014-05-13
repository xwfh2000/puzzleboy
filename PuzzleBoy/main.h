#pragma once

#include <vector>

const float SQRT_1_2=0.707106781f;

const float m_fBorderSize=0.125f;
const float m_fOverlaySize=0.166666667f;
const float m_fRotateBlockCenterSize=0.2f;
const float m_fPlayerSize=0.375f;

//ad-hoc
void SetProjectionMatrix(int idx=0);

void ClearScreen();
void OnVideoResize(int width, int height);

void AddScreenKeyboard(float x,float y,float w,float h,int index,std::vector<float>& v,std::vector<unsigned short>& idx);
void DrawScreenKeyboard(const std::vector<float>& v,const std::vector<unsigned short>& idx);

bool IsTouchscreen();

const int SCREEN_KEYBOARD_UP=0x1;
const int SCREEN_KEYBOARD_DOWN=0x101;
const int SCREEN_KEYBOARD_LEFT=0x100;
const int SCREEN_KEYBOARD_RIGHT=0x102;
const int SCREEN_KEYBOARD_UNDO=0x0;
const int SCREEN_KEYBOARD_REDO=0x2;
const int SCREEN_KEYBOARD_SWITCH=0x103;
const int SCREEN_KEYBOARD_RESTART=0x3;
const int SCREEN_KEYBOARD_YES=0x200;
const int SCREEN_KEYBOARD_NO=0x201;
const int SCREEN_KEYBOARD_OPEN=0x202;

extern int screenWidth;
extern int screenHeight;
extern int m_nResizeTime;
extern float screenAspectRatio;

struct SDL_Window;

extern SDL_Window *mainWindow;

extern bool m_bRun;
extern bool m_bTouchscreen;
extern bool m_bShowYesNoScreenKeyboard;

class SimpleFontFile;
class SimpleBaseFont;

extern SimpleFontFile *mainFontFile;
extern SimpleBaseFont *mainFont;
extern SimpleBaseFont *titleFont;

void WaitForNextFrame();
