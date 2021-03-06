// MainFrm.h : CMainFrame 类的接口
//


#pragma once

class CMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();

// 属性
public:

// 操作
public:

// 重写
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// 实现
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

public:  // 控件条嵌入成员
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;
	CToolBar    m_wndEditToolBar;

// 生成的消息映射函数
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnUpdateEditToolbar(CCmdUI *pCmdUI);
	afx_msg void OnEditToolbar();
	afx_msg void OnToolbarDropdown(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLanguage(UINT);
	afx_msg void OnUpdateLanguage(CCmdUI *pCmdUI);
};


