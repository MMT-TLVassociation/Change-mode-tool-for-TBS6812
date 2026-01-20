
// MACToolDlg.h : header file
//

#pragma once


const GUID KSPROPSETID_BdaTunerExtensionProperties =
{0xfaa8f3e5, 0x31d4, 0x4e41, {0x88, 0xef, 0xd9, 0xeb, 0x71, 0x6f, 0x6e, 0xc9}};

typedef struct _mac_data
{
	int  nIndexMac;//标记4个口的 写哪一个
	unsigned char write_bytes[6];//写MAC 数据
	unsigned char read_bytes[6];//读到的MAC 数据

} MAC_DATA;

// CMACToolDlg dialog
class CMACToolDlg : public CDialog
{
// Construction
public:
	CMACToolDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_MACTOOL_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnRead();
	afx_msg void OnBnClickedBtnWrite();

protected:
	CComPtr <IBaseFilter> m_pTunerDevice;
	IDVBTuningSpace2 * tuningspace;
	CComPtr <IPin> m_pTunerPin;                  // the tuner pin on the tuner/demod filter

public:
	IBaseFilter* CreateDVBSNetworkProvider(void);
	IBaseFilter* CreateDVBCNetworkProvider(void);
	IBaseFilter* CreateDVBTNetworkProvider(void);
	IPin* FindPinOnFilter(IBaseFilter* pBaseFilter, char* pPinName);
	BOOL CreateDVBSFilter();
	BOOL CreateDVBTFilter();
	BOOL CreateDVBCFilter();

	virtual BOOL DestroyWindow();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnCbnSelchangeComboTuneraselmode();
	afx_msg void OnCbnSelchangeComboTunerbselmode();
	CComboBox m_TunerASelMode;
	CComboBox m_TunerBSelMode;
	int m_TunerACurMode;
	int m_TunerBCurMode;
};
