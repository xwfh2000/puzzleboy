#include "SimpleScrollView.h"
#include "main.h"

#include <math.h>

#include "include_gl.h"

SimpleScrollView::SimpleScrollView()
:m_bAutoResize(false),m_fMinZoomPerScreen(1.0f)
,m_nOrientation(0)
,m_zoom(1.0f/48.0f),m_x(0.0f),m_y(0.0f)
,m_zoom2(1.0f/48.0f),m_x2(0.0f),m_y2(0.0f)
,m_nMyResizeTime(-1)
,m_nScrollBarIdleTime(0)
{
	m_virtual.x=0;
	m_virtual.y=0;
	m_virtual.w=0;
	m_virtual.h=0;

	m_screen.x=0;
	m_screen.y=0;
	m_screen.w=0;
	m_screen.h=0;

	m_fAutoResizeScale[0]=0;
	m_fAutoResizeScale[1]=0;
	m_fAutoResizeScale[2]=0;
	m_fAutoResizeScale[3]=0;

	m_nAutoResizeOffset[0]=0;
	m_nAutoResizeOffset[1]=0;
	m_nAutoResizeOffset[2]=0;
	m_nAutoResizeOffset[3]=0;

	m_nScissorOffset[0]=0;
	m_nScissorOffset[1]=0;
	m_nScissorOffset[2]=0;
	m_nScissorOffset[3]=0;
}

bool SimpleScrollView::OnTimer(){
	bool bDirty=false;

	//check screen resize
	if(m_bAutoResize){
		if(m_nMyResizeTime!=m_nResizeTime){
			m_nMyResizeTime=m_nResizeTime;

			SDL_Rect oldScreen=m_screen;

			m_screen.x=int(float(screenWidth)*m_fAutoResizeScale[0]+0.5f)+m_nAutoResizeOffset[0];
			m_screen.y=int(float(screenHeight)*m_fAutoResizeScale[1]+0.5f)+m_nAutoResizeOffset[1];
			m_screen.w=int(float(screenWidth)*m_fAutoResizeScale[2]+0.5f)+m_nAutoResizeOffset[2]-m_screen.x;
			m_screen.h=int(float(screenHeight)*m_fAutoResizeScale[3]+0.5f)+m_nAutoResizeOffset[3]-m_screen.y;

			if(m_virtual.w>0 && m_virtual.h>0 && m_screen.w>0 && m_screen.h>0 && oldScreen.w>0 && oldScreen.h>0){
				//TODO: update zoom pos
			}

			ConstraintView(true);

			bDirty=true;
		}
	}else{
		m_nMyResizeTime=-1;
	}

	if(m_virtual.w>0 && m_virtual.h>0 && m_screen.w>0 && m_screen.h>0){
		//TODO: update zoom pos
	}

	//fake animation
	m_zoom2=(m_zoom+m_zoom2)*0.5f;
	m_x2=(m_x+m_x2)*0.5f;
	m_y2=(m_y+m_y2)*0.5f;

	//scroll bar animation (experimental)
	if(fabs(m_zoom-m_zoom2)<1E-3f && fabs(m_x-m_x2)<1E-3f && fabs(m_y-m_y2)<1E-3f){
		if(m_nScrollBarIdleTime<32){
			m_nScrollBarIdleTime++;
			bDirty=true;
		}
	}else{
		if(m_nScrollBarIdleTime>0) m_nScrollBarIdleTime-=4;
		bDirty=true;
	}

	return bDirty;
}

void SimpleScrollView::OnMultiGesture(float fx,float fy,float dx,float dy,float zoom){
	//translate coordinate if orientation is set
	switch(m_nOrientation){
	case 2:
		fx=float(m_screen.x*2+m_screen.w)/float(screenHeight)-fx;
		fy=float(m_screen.y*2+m_screen.h)/float(screenHeight)-fy;
		dx=-dx;
		dy=-dy;
		break;
	default:
		break;
	}

	//move
	float f=float(screenHeight)*m_zoom;
	m_x-=dx*f;
	m_y-=dy*f;

	//zoom
	float new_zoom=m_zoom/zoom;

	//ad-hoc range check

	//check minimal zoom factor (maximal zoom in)
	f=m_fMinZoomPerScreen/float(m_screen.w>m_screen.h?m_screen.w:m_screen.h);
	if(new_zoom<f) new_zoom=f;

	//check maximal zoom factor (maximal zoom out)
	if(m_virtual.w*m_screen.h>m_virtual.h*m_screen.w){ // w/h>m_screen.w/m_screen.h
		f=float(m_virtual.w)/float(m_screen.w);
	}else{
		f=float(m_virtual.h)/float(m_screen.h);
	}
	if(new_zoom>f) new_zoom=f;

	//apply zoom
	f=float(screenHeight)*(m_zoom-new_zoom);
	m_x+=fx*f;
	m_y+=fy*f;
	m_zoom=new_zoom;

	//move view if necessary
	ConstraintView(false);
}

void SimpleScrollView::ConstraintView(bool zoom){
	float f;

	//reset zoom if necessary
	if(zoom){
		float new_zoom=m_zoom;

		//check minimal zoom factor (maximal zoom in)
		f=m_fMinZoomPerScreen/float(m_screen.w>m_screen.h?m_screen.w:m_screen.h);
		if(new_zoom<f) new_zoom=f;

		//check maximal zoom factor (maximal zoom out)
		if(m_virtual.w*m_screen.h>m_virtual.h*m_screen.w){ // w/h>m_screen.w/m_screen.h
			f=float(m_virtual.w)/float(m_screen.w);
		}else{
			f=float(m_virtual.h)/float(m_screen.h);
		}
		if(new_zoom>f) new_zoom=f;

		float fx=float(m_screen.x)+float(m_screen.w)*0.5f;
		float fy=float(m_screen.y)+float(m_screen.h)*0.5f;
		f=m_zoom-new_zoom;
		m_x+=fx*f;
		m_y+=fy*f;
		m_zoom=new_zoom;
	}

	//move view if necessary
	if(float(m_screen.w)*m_zoom>float(m_virtual.w)){
		m_x=float(m_virtual.x)+float(m_virtual.w)*0.5f-(float(m_screen.x)+float(m_screen.w)*0.5f)*m_zoom;
	}else{
		f=float(m_virtual.x)-float(m_screen.x)*m_zoom;
		if(m_x<f) m_x=f;
		f+=float(m_virtual.w)-float(m_screen.w)*m_zoom;
		if(m_x>f) m_x=f;
	}
	if(float(m_screen.h)*m_zoom>float(m_virtual.h)){
		m_y=float(m_virtual.y)+float(m_virtual.h)*0.5f-(float(m_screen.y)+float(m_screen.h)*0.5f)*m_zoom;
	}else{
		f=float(m_virtual.y)-float(m_screen.y)*m_zoom;
		if(m_y<f) m_y=f;
		f+=float(m_virtual.h)-float(m_screen.h)*m_zoom;
		if(m_y>f) m_y=f;
	}
}

void SimpleScrollView::SetProjectionMatrix(){
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	//ad-hoc test
	switch(m_nOrientation){
	case 2:
		{
			float x3=float(m_screen.x*2+m_screen.w)*m_zoom2+m_x2;
			float y3=float(m_screen.y*2+m_screen.h)*m_zoom2+m_y2;
			glOrtho(x3,x3-float(screenWidth)*m_zoom2,
				y3-float(screenHeight)*m_zoom2,y3,-1.0f,1.0f);
		}
		break;
	default:
		glOrtho(m_x2,m_x2+float(screenWidth)*m_zoom2,
			m_y2+float(screenHeight)*m_zoom2,m_y2,-1.0f,1.0f);
		break;
	}
	glMatrixMode(GL_MODELVIEW);
}

void SimpleScrollView::EnableScissorRect(){
	glEnable(GL_SCISSOR_TEST);
	if(m_screen.w>0 && m_screen.h>0){
		glScissor(m_screen.x+m_nScissorOffset[0],
			screenHeight-m_screen.y-m_screen.h-m_nScissorOffset[3],
			m_screen.w-m_nScissorOffset[0]+m_nScissorOffset[2],
			m_screen.h-m_nScissorOffset[1]+m_nScissorOffset[3]);
	}else{
		glScissor(0,0,0,0);
	}
}

void SimpleScrollView::DisableScissorRect(){
	glDisable(GL_SCISSOR_TEST);
}

void SimpleScrollView::TranslateCoordinate(int x,int y,int& out_x,int& out_y){
	switch(m_nOrientation){
	case 2:
		out_x=(int)floor(float(m_screen.x*2+m_screen.w-x)*m_zoom2+m_x2);
		out_y=(int)floor(float(m_screen.y*2+m_screen.h-y)*m_zoom2+m_y2);
		break;
	default:
		out_x=(int)floor(float(x)*m_zoom2+m_x2);
		out_y=(int)floor(float(y)*m_zoom2+m_y2);
		break;
	}
}

void SimpleScrollView::CenterView(int x,int y,int w,int h){
	if(w<0 || h<0){
		x=m_virtual.x;
		y=m_virtual.y;
		w=m_virtual.w;
		h=m_virtual.h;
	}

	if(w<0 || h<0) return;

	if(w==0 && h==0){ //centering only
		m_x=float(x)-float(m_screen.x)*m_zoom;
		m_y=float(y)-float(m_screen.y)*m_zoom;
	}else if(w*m_screen.h>h*m_screen.w){ // w/h>m_screen.w/m_screen.h
		m_zoom=float(w)/float(m_screen.w);
		m_x=float(x)-float(m_screen.x)*m_zoom;
		m_y=float(y)+float(h)*0.5f-(float(m_screen.y)+float(m_screen.h)*0.5f)*m_zoom;
	}else{
		m_zoom=float(h)/float(m_screen.h);
		m_x=float(x)+float(w)*0.5f-(float(m_screen.x)+float(m_screen.w)*0.5f)*m_zoom;
		m_y=float(y)-float(m_screen.y)*m_zoom;
	}

	ConstraintView(true);
}

void SimpleScrollView::EnsureVisible(int x,int y,int w,int h){
	if(float(m_screen.x)*m_zoom2+m_x2>float(x)
		|| float(m_screen.y)*m_zoom2+m_y2>float(y)
		|| float(m_screen.x+m_screen.w)*m_zoom2+m_x2<float(x+w)
		|| float(m_screen.y+m_screen.h)*m_zoom2+m_y2<float(y+h))
	{
		m_x=float(x)+float(w)*0.5f-(float(m_screen.x)+float(m_screen.w)*0.5f)*m_zoom;
		m_y=float(y)+float(h)*0.5f-(float(m_screen.y)+float(m_screen.h)*0.5f)*m_zoom;

		ConstraintView(true);
	}
}

void SimpleScrollView::DrawScrollBar(){
	//draw scrollbar (experimental)
	if(m_nScrollBarIdleTime<32 && m_virtual.w>0 && m_virtual.h>0 && m_screen.w>0 && m_screen.h>0){
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		glOrtho(0.0f,float(screenWidth),float(screenHeight),0.0f,-1.0f,1.0f);

		glMatrixMode(GL_MODELVIEW);

		float x=float(m_screen.w)/float(m_virtual.w);
		float x2=float(m_screen.w)*x*m_zoom2;
		x*=(m_x2+float(m_screen.x)*m_zoom2-float(m_virtual.x));
		x+=float(m_screen.x);
		x2+=x;

		float y=float(m_screen.h)/float(m_virtual.h);
		float y2=float(m_screen.h)*y*m_zoom2;
		y*=(m_y2+float(m_screen.y)*m_zoom2-float(m_virtual.y));
		y+=float(m_screen.y);
		y2+=y;

		float v[16]={
			float(m_screen.x+m_screen.w-8),y,
			float(m_screen.x+m_screen.w),y,
			float(m_screen.x+m_screen.w-8),y2,
			float(m_screen.x+m_screen.w),y2,
			x,float(m_screen.y+m_screen.h-8),
			x2,float(m_screen.y+m_screen.h-8),
			x,float(m_screen.y+m_screen.h),
			x2,float(m_screen.y+m_screen.h),
		};

		switch(m_nOrientation){
		case 2:
			for(int i=0;i<8;i++){
				v[i*2]=float(m_screen.x*2+m_screen.w)-v[i*2];
				v[i*2+1]=float(m_screen.y*2+m_screen.h)-v[i*2+1];
			}
			break;
		}

		const unsigned short i[]={0,1,3,0,3,2,4,5,7,4,7,6};

		float transparency=float(32-m_nScrollBarIdleTime)/16.0f;
		if(transparency>1.0f) transparency=1.0f;

		glColor4f(0.5f,0.5f,0.5f,transparency);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2,GL_FLOAT,0,v);
		glDrawElements(GL_TRIANGLES,12,GL_UNSIGNED_SHORT,i);
		glDisableClientState(GL_VERTEX_ARRAY);
	}
}