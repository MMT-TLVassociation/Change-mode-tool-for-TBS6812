
// MACToolDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MACTool.h"
#include "MACToolDlg.h"


#include "Dbt.h"
#include "strsafe.h"
#include "ini.h"
#include "TBSStringFnc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//added 2010 11 18 liuy
// this is defined in bda tuner/demod driver 
typedef enum {
	KSPROPERTY_BDA_DISEQC_MESSAGE = 0,  //Custom property for Diseqc messaging
	KSPROPERTY_BDA_DISEQC_INIT,         //Custom property for Intializing Diseqc.
	KSPROPERTY_BDA_SCAN_FREQ,           //Not supported 
	KSPROPERTY_BDA_CHANNEL_CHANGE,      //Custom property for changing channel
	KSPROPERTY_BDA_DEMOD_INFO,          //Custom property for returning demod FW state and version
	KSPROPERTY_BDA_EFFECTIVE_FREQ,      //Not supported 
	KSPROPERTY_BDA_SIGNAL_STATUS,       //Custom property for returning signal quality, strength, BER and other attributes
	KSPROPERTY_BDA_LOCK_STATUS,         //Custom property for returning demod lock indicators 
	KSPROPERTY_BDA_ERROR_CONTROL,       //Custom property for controlling error correction and BER window
	KSPROPERTY_BDA_CHANNEL_INFO,        //Custom property for exposing the locked values of frequency,symbol rate etc after
	//corrections and adjustments
	KSPROPERTY_BDA_NBC_PARAMS,
	KSPROPERTY_BDA_BLIND_SCAN,
	KSPROPERTY_BDA_GET_MEDIAINFO,
	KSPROPERTY_BDA_STREAMTYPE_PARAMS,
	KSPROPERTY_BDA_INPUTMULTISTREAMID,//added 2011 01 27
	KSPROPERTY_BDA_MACACCESS,//added 2010 12 07	
	
	KSPROPERTY_BDA_SETHV,					//added 2011 05 21 control HV , 22k , h=1;v=0;	//6925;6984
	KSPROPERTY_BDA_SET22K,					//22k 0 off; 1 on //6925;6984
	KSPROPERTY_BDA_CI_ACCESS,				//BOB ci /6992,6618,6928
	KSPROPERTY_BDA_UNICABLE,			//******added 2011 10 19 interface for all card*****/
	KSPROPERTY_BDA_PMT_ACCESS	,		// ci mce  /6992,6618,6928
	KSPROPERTY_BDA_TBSACCESS,

	KSPROPERTY_BDA_PLPINFO,//added 2012 11 09 liuy DVBT2 PLP interface
    KSPROPERTY_BDA_GOLDCODE,//added 2013 02 25 liuy
    KSPROPERTY_BDA_MODCODES, 
    KSPROPERTY_BDA_TBSI2CACCESS,//added 20140617 luchy For TBS PCI-E bridge I2C interface

	KSPROPERTY_BDA_GSE_LABEL = 40,
	KSPROPERTY_BDA_GSE_PROTO,

	KSPROPERTY_BDA_CTRL_DemodMode = 60, 
	
	KSPROPERTY_BDA_SETTSID = 96,  // added 20190704 for ISDB-S/S3 for BonBDA drivers	
	KSPROPERTY_BDA_GETTMCC = 97,  // added 20190704 for ISDB-S/S3 for BonBDA drivers
	KSPROPERTY_BDA_DemodSTATUS = 98,
	
	KSPROPERTY_BDA_FPGA_RDID = 100,
	KSPROPERTY_BDA_FPGA_ERASE_WriteCtrl = 101,
	KSPROPERTY_BDA_FPGA_WRITEONEPAGE = 102,
	KSPROPERTY_BDA_FPGA_VERIFY = 103,
} KSPROPERTY_BDA_TUNER_EXTENSION;


static MAC_DATA mac_data_read;
static MAC_DATA mac_data_write;

// CMACToolDlg dialog

CComPtr<IGraphBuilder> builder;
CComPtr<IMediaControl> control;
CComPtr<IBaseFilter> CaptureFilter,TunerFilter,TSDump;
CComPtr<IBaseFilter> provider;
CComPtr<IBaseFilter> demux, infinite, tif;
HANDLE hIOMutex;

IKsPropertySet *m_pKsCtrl;
IKsPropertySet *m_pVCKsCtrl;
//CComPtr <IPin> m_pTunerPin;                  // the tuner pin on the tuner/demod filter

#define DVBS_TUNING_SPACE_NAME		L"turbosight dvb-s"
#define DVBC_TUNING_SPACE_NAME		L"turbosight dvb-c"
#define DVBT_TUNING_SPACE_NAME		L"turbosight dvb-t"

int eSTRtoUINT(CString str, unsigned char *hex_array)
{
	char buffer[512];
	memcpy(buffer,LPCTSTR(str),str.GetLength());

	int j=0;
	for(int i=0;i<str.GetLength()/2;i++)
	{
		UINT temp1,temp2;

		if(buffer[2*i]>='0' && buffer[2*i]<='9')
			temp1=buffer[2*i]-'0';
		else if(buffer[2*i]>='a' && buffer[2*i]<='f')
			temp1=10+buffer[2*i]-'a';
		else if(buffer[2*i]>='A' && buffer[2*i]<='F')
			temp1=10+buffer[2*i]-'A';
		else break;

		if(buffer[2*i+1]>='0' && buffer[2*i+1]<='9')
			temp2=buffer[2*i+1]-'0';
		else if(buffer[2*i+1]>='a' && buffer[2*i+1]<='f')
			temp2=10+buffer[2*i+1]-'a';
		else if(buffer[2*i+1]>='A' && buffer[2*i+1]<='F')
			temp2=10+buffer[2*i+1]-'A';
		else break;

		hex_array[j++]=temp1*16+temp2;
	}
	return j;
}

IDVBTuningSpace2* CreateDVBSTuningSpace(void)
{
	IDVBSLocator *locator;
	IDVBTuningSpace2 *tuning;

	// Create locator
	CoCreateInstance(CLSID_DVBSLocator,NULL,CLSCTX_INPROC_SERVER,IID_IDVBSLocator,(void**)&locator);

	// Create tuning space
	CoCreateInstance(CLSID_DVBSTuningSpace,NULL,CLSCTX_INPROC_SERVER,IID_IDVBTuningSpace2,(void**)&tuning);
	// Set ITuningSpace variables
	tuning->put__NetworkType(CLSID_DVBSNetworkProvider);
	// Set IDVBTuningSpace variables
	tuning->put_SystemType(DVB_Satellite);
	// Set IDVBTuningSpace2 variables
	//	tuning->put_NetworkID(9018);
	// Set ITuningSpace variables again
	tuning->put_DefaultLocator(locator);
	tuning->put_FrequencyMapping(L"");
	tuning->put_FriendlyName(L"Local DVBS Tuner");
	tuning->put_UniqueName(DVBS_TUNING_SPACE_NAME);
	CComQIPtr<IDVBSTuningSpace> pDVBSTuningSpace(tuning) ;
	if( pDVBSTuningSpace != NULL ) {
		pDVBSTuningSpace->put_HighOscillator (-1); //Sets the high oscillator frequency. 
		pDVBSTuningSpace->put_InputRange (CComBSTR("-1")); //Sets an integer indicating which option or switch contains the requested signal source. 
		pDVBSTuningSpace->put_LNBSwitch (-1); //Sets the LNB switch frequency. 
		pDVBSTuningSpace->put_LowOscillator (-1); //Sets the low oscillator frequency. 
		pDVBSTuningSpace->put_SpectralInversion (BDA_SPECTRAL_INVERSION_NOT_SET);//Sets an integer indicating the spectral inversion. 
	}

	// Done
	return tuning;
}

IDVBTuningSpace2* CreateDVBCTuningSpace(void)
{
	IDVBCLocator *locator;
	IDVBTuningSpace2 *tuning;

	// Create locator
	CoCreateInstance(CLSID_DVBCLocator,NULL,CLSCTX_INPROC_SERVER,IID_IDVBCLocator,(void**)&locator);

	// Create tuning space
	CoCreateInstance(CLSID_DVBTuningSpace,NULL,CLSCTX_INPROC_SERVER,IID_IDVBTuningSpace2,(void**)&tuning);
	// Set ITuningSpace variables
	tuning->put__NetworkType(CLSID_DVBCNetworkProvider);
	// Set IDVBTuningSpace variables
	tuning->put_SystemType(DVB_Cable);
	// Set IDVBTuningSpace2 variables
	//	tuning->put_NetworkID(9018);
	// Set ITuningSpace variables again
	tuning->put_DefaultLocator(locator);
	tuning->put_FrequencyMapping(L"");
	tuning->put_FriendlyName(L"Local DVB-C digital antenna");
	tuning->put_UniqueName(DVBC_TUNING_SPACE_NAME);
	// Done
	return tuning;
}




IDVBTuningSpace2* CreateDVBTTuningSpace(void)
{
	IDVBCLocator *locator;
	IDVBTuningSpace2 *tuning;

	// Create locator
	CoCreateInstance(CLSID_DVBTLocator,NULL,CLSCTX_INPROC_SERVER,IID_IDVBTLocator,(void**)&locator);

	// Create tuning space
	CoCreateInstance(CLSID_DVBTuningSpace,NULL,CLSCTX_INPROC_SERVER,IID_IDVBTuningSpace2,(void**)&tuning);
	// Set ITuningSpace variables
	tuning->put__NetworkType(CLSID_DVBTNetworkProvider);
	// Set IDVBTuningSpace variables
	tuning->put_SystemType(DVB_Terrestrial);
	// Set IDVBTuningSpace2 variables
	//	tuning->put_NetworkID(9018);
	// Set ITuningSpace variables again
	tuning->put_DefaultLocator(locator);
	tuning->put_FrequencyMapping(L"");
	tuning->put_FriendlyName(L"Local DVB-T digital antenna");
	tuning->put_UniqueName(DVBT_TUNING_SPACE_NAME);
	// Done
	return tuning;
}


//Load a filter for CLSID
IBaseFilter* LoadFilter(CLSID clsid)
{
	IBaseFilter *filter;

	CoCreateInstance(clsid,NULL,CLSCTX_INPROC_SERVER,IID_IBaseFilter,reinterpret_cast<void**>(&filter));
	return filter;
}

// Finds a filter based on the name
IBaseFilter* FindFilter(CLSID clsid,WCHAR *name)
{
	HRESULT hr;
	CComPtr <IMoniker>      pIMoniker;
	CComPtr <IEnumMoniker>  pIEnumMoniker;
	CComPtr <ICreateDevEnum> m_pICreateDevEnum;
	int nl;

	if(name!=NULL) nl=wcslen(name);
	if(!m_pICreateDevEnum) 
	{
		hr=m_pICreateDevEnum.CoCreateInstance(CLSID_SystemDeviceEnum);
		if(FAILED(hr)) return NULL;
	}

	hr=m_pICreateDevEnum->CreateClassEnumerator(clsid,&pIEnumMoniker,0);
	if(FAILED(hr)||(S_OK!=hr))
		return NULL;

	while(pIEnumMoniker->Next(1,&pIMoniker,0)==S_OK)
	{
		CComPtr <IPropertyBag>  pBag;
		hr=pIMoniker->BindToStorage(NULL,NULL,IID_IPropertyBag,reinterpret_cast<void**>(&pBag));
		if(FAILED(hr)) 
		{
			OutputDebugString("FindFilter(): Cannot BindToStorage");
			return NULL;
		}

		CComVariant varBSTR;
		hr=pBag->Read(L"FriendlyName",&varBSTR,NULL);
		if(FAILED(hr))
		{
			OutputDebugString("FindFilter(): IPropertyBag->Read method failed");
			pIMoniker=NULL;
			continue;
		}

		// bind the filter
		IBaseFilter*   pFilter;
		hr=pIMoniker->BindToObject(NULL,NULL,IID_IBaseFilter,reinterpret_cast<void**>(&pFilter));
		if(FAILED(hr)) 
		{
			pIMoniker=NULL;
			pFilter=NULL;
			continue;
		}

		if((name==NULL)||(memcmp(varBSTR.bstrVal,name,nl)==0)) 
			return pFilter;

		pIMoniker=NULL;
		pFilter=NULL;
	}
	return NULL;
}

IBaseFilter* CreateCaptureDevice(void)
{
	return FindFilter(KSCATEGORY_BDA_RECEIVER_COMPONENT,CaptuerName);
}

IBaseFilter* CreateMPEG2Demultiplexer(void)
{
	return LoadFilter(CLSID_MPEG2Demultiplexer);
}

IBaseFilter* CreateTunerDevice(void)
{
	return FindFilter(KSCATEGORY_BDA_NETWORK_TUNER,TunerName);
}

void ConnectPins(IGraphBuilder *builder,IBaseFilter *out_from,IBaseFilter *in_to,WCHAR *from_name=NULL,WCHAR *to_name=NULL);
void ConnectPins(IGraphBuilder *builder,IBaseFilter *out_from,IBaseFilter *in_to,WCHAR *from_name,WCHAR *to_name)
{
	CComPtr<IEnumPins> pins;
	CComPtr<IPin> out_pin;
	CComPtr<IPin> in_pin;
	CComPtr<IPin> temp;
	PIN_INFO info;
	HRESULT hResult;
	BOOL bConnected;

	if(builder == NULL)
	{
		MessageBox(NULL, "ConnectPins builder == NULL","error",MB_OK);
		return ;
	}

	if(out_from == NULL)
	{
		MessageBox(NULL, "ConnectPins out_from == NULL","error",MB_OK);
		return ;
	}

	if(in_to == NULL)
	{
		MessageBox(NULL, "ConnectPins in_to == NULL","error",MB_OK);
		return ;
	}

	// Find output pin of out_from
	out_from->EnumPins(&pins);
	pins->Reset();
	while(pins->Next(1,&out_pin,NULL)==S_OK) 
	{
		out_pin->ConnectedTo(&temp);
		bConnected=temp.p!=NULL;
		temp.Release();
		if(!bConnected) 
		{
			out_pin->QueryPinInfo(&info);
			if((info.dir==PINDIR_OUTPUT)&&((from_name==NULL)?TRUE:(wcscmp(info.achName,from_name)==0))) break;
		}
		out_pin.Release();
	}
	pins.Release();

	if(!out_pin) 
	{
		AfxMessageBox("CBDAGraph::ConnectPins() - No output pin found on filter\r\n");
		return;
	}

	// Find input pin of in_to
	in_to->EnumPins(&pins);
	pins->Reset();
	while(pins->Next(1,&in_pin,NULL)==S_OK) 
	{
		in_pin->ConnectedTo(&temp);
		bConnected=temp.p!=NULL;
		temp.Release();
		if(!bConnected) 
		{
			in_pin->QueryPinInfo(&info);
			if((info.dir==PINDIR_INPUT)&&((to_name==NULL)?TRUE:(wcscmp(info.achName,to_name)==0))) break;
		}
		in_pin.Release();
	}

	pins.Release();
	if(!in_pin) 
	{
		AfxMessageBox("CBDAGraph::ConnectPins() - No input pin found on filter\r\n");
		return;
	}
	// Join them together
	if((hResult=builder->Connect(out_pin,in_pin))!=S_OK) 
	{
		//AfxMessageBox("CBDAGraph::ConnectPins() - Could not connect pins together. Error was: ");
	}
}

// Render an output pin
void RenderOutput(IGraphBuilder *builder,IBaseFilter *out_from)
{
	CComPtr<IEnumPins> pins;
	CComPtr<IPin> out_pin;
	PIN_INFO info;

	if(builder == NULL)
	{
		MessageBox(NULL, "RenderOutput builder == NULL","error",MB_OK);
		return ;
	}

	if(out_from == NULL)
	{
		MessageBox(NULL, "RenderOutput out_from == NULL","error",MB_OK);
		return ;
	}

	// Find output pin of out_from
	out_from->EnumPins(&pins);
	pins->Reset();
	while(pins->Next(1,&out_pin,NULL)==S_OK) 
	{
		out_pin->QueryPinInfo(&info);
		if(info.dir==PINDIR_OUTPUT) 
			break;
		out_pin.Release();
	}
	pins.Release();
	if(!out_pin) 
	{
		AfxMessageBox("CBDAGraph::RenderOutput() - No output pin found on device filter");
		return;
	}
	// Render graph
	builder->Render(out_pin);
}

IBaseFilter* CreateInfinitePinTee(void)
{
	return LoadFilter(CLSID_InfTee);
}

IBaseFilter* CreateTransportInformationFilter(void)
{
	return FindFilter(KSCATEGORY_BDA_TRANSPORT_INFORMATION,NULL);
}

IDVBTuningSpace2* LoadDVBSTuningSpace(void)
{
	IEnumTuningSpaces *spaces;
	CComPtr<ITuningSpaceContainer> pITuningSpaceContainer;
	HRESULT hr;
	ITuningSpace *space;
	ULONG l;
	BSTR name;
	BOOL bFound;

	hr=pITuningSpaceContainer.CoCreateInstance(CLSID_SystemTuningSpaces);
	if(FAILED(hr)) 
		return NULL;
	// Check if our tuning space is in the container, and add it if not
	pITuningSpaceContainer->get_EnumTuningSpaces(&spaces);
	spaces->Reset();
	bFound=FALSE;
	while((!bFound)&&(spaces->Next(1,&space,&l)==S_OK)) 
	{
		space->get_UniqueName(&name);
		if(wcscmp(name,DVBS_TUNING_SPACE_NAME)==0) 
			bFound=TRUE;
		else 
			space->Release();
		SysFreeString(name);
	}
	spaces->Release();
	if(!bFound) 
	{
		VARIANT v;

		space=CreateDVBSTuningSpace();
		pITuningSpaceContainer->Add(space,&v);
	}
	return (IDVBTuningSpace2*)space;
}


IDVBTuningSpace2* LoadDVBCTuningSpace(void)
{
	IEnumTuningSpaces *spaces;
	CComPtr<ITuningSpaceContainer> pITuningSpaceContainer;
	HRESULT hr;
	ITuningSpace *space;
	ULONG l;
	BSTR name;
	BOOL bFound;

	hr=pITuningSpaceContainer.CoCreateInstance(CLSID_SystemTuningSpaces);
	if(FAILED(hr)) return NULL;
	// Check if our tuning space is in the container, and add it if not
	pITuningSpaceContainer->get_EnumTuningSpaces(&spaces);
	spaces->Reset();
	bFound=FALSE;
	while((!bFound)&&(spaces->Next(1,&space,&l)==S_OK)) {
		space->get_UniqueName(&name);
		if(wcscmp(name,DVBC_TUNING_SPACE_NAME)==0) bFound=TRUE;
		else space->Release();
		SysFreeString(name);
	}
	spaces->Release();
	if(!bFound) {
		VARIANT v;

		space=CreateDVBCTuningSpace();
		pITuningSpaceContainer->Add(space,&v);
	}
	return (IDVBTuningSpace2*)space;
}

IDVBTuningSpace2* LoadDVBTTuningSpace(void)
{
	IEnumTuningSpaces *spaces;
	CComPtr<ITuningSpaceContainer> pITuningSpaceContainer;
	HRESULT hr;
	ITuningSpace *space;
	ULONG l;
	BSTR name;
	BOOL bFound;

	hr=pITuningSpaceContainer.CoCreateInstance(CLSID_SystemTuningSpaces);
	if(FAILED(hr)) 
		return NULL;
	// Check if our tuning space is in the container, and add it if not
	pITuningSpaceContainer->get_EnumTuningSpaces(&spaces);
	spaces->Reset();
	bFound=FALSE;
	while((!bFound)&&(spaces->Next(1,&space,&l)==S_OK)) 
	{
		space->get_UniqueName(&name);
		if(wcscmp(name, DVBT_TUNING_SPACE_NAME)==0) 
			bFound=TRUE;
		else 
			space->Release();
		SysFreeString(name);
	}
	spaces->Release();
	if(!bFound) 
	{
		VARIANT v;

		space=CreateDVBTTuningSpace();
		pITuningSpaceContainer->Add(space,&v);
	}
	return (IDVBTuningSpace2*)space;
}

BOOL CheckDeviceReady()
{
	TunerFilter=CreateTunerDevice();
	if(TunerFilter)
	{
		return TRUE;
	}
	return FALSE;
}

CMACToolDlg::CMACToolDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMACToolDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMACToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_TunerASelMode, m_TunerASelMode);
	DDX_Control(pDX, IDC_COMBO_TunerBSelMode, m_TunerBSelMode);


}

BEGIN_MESSAGE_MAP(CMACToolDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BTN_READ, &CMACToolDlg::OnBnClickedBtnRead)
	ON_BN_CLICKED(IDC_BTN_WRITE, &CMACToolDlg::OnBnClickedBtnWrite)
	ON_WM_LBUTTONDOWN()
	ON_CBN_SELCHANGE(IDC_COMBO_TunerASelMode, &CMACToolDlg::OnCbnSelchangeComboTuneraselmode)
	ON_CBN_SELCHANGE(IDC_COMBO_TunerBSelMode, &CMACToolDlg::OnCbnSelchangeComboTunerbselmode)
END_MESSAGE_MAP()


// CMACToolDlg message handlers

BOOL CMACToolDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	//初始化过程
	////////////////////////////////////////////////////////////////////////////

	m_pTunerDevice = NULL;
	tuningspace = NULL;
	builder = NULL;
	provider = NULL;
	CaptureFilter = NULL;
	TSDump = NULL;
	control = NULL;
	demux = NULL;
	tif = NULL;
	infinite = NULL;


	unsigned long BytesRead=0;
	DWORD type_support=0 ;

	CreateDVBSFilter();

	HRESULT hr = m_pKsCtrl->QuerySupported(KSPROPSETID_BdaTunerExtensionProperties,
	 KSPROPERTY_BDA_CTRL_DemodMode, 
	 &type_support);

	 if FAILED(hr)
 	{

		CreateDVBTFilter();	
		hr = m_pKsCtrl->QuerySupported(KSPROPSETID_BdaTunerExtensionProperties,
		  KSPROPERTY_BDA_CTRL_DemodMode, 
		  &type_support);
		if FAILED(hr)
		{
				CreateDVBCFilter();	
		}
		
	}


//////////////////////////////////////////////////////////////////////////
//	CString strTemp(_T(""));
//	int nTemp = sizeof(MAC_DATA);
//	strTemp.Format("size is %d",nTemp);
//	MessageBox(strTemp);

	//
//	SetWindowText(strTBSProductName);

	OnBnClickedBtnRead();//刚运行时就读MAC数据



	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMACToolDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMACToolDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// Returns IPin pointer of a pin on a filter.
//
IPin* CMACToolDlg::FindPinOnFilter(IBaseFilter* pBaseFilter, char* pPinName)
{
	HRESULT hr;
	IEnumPins *pEnumPin = NULL;
	ULONG CountReceived = 0;
	IPin *pPin = NULL, *pThePin = NULL;
	char String[80];
	char* pString;
	PIN_INFO PinInfo;
	int length;

	if (!pBaseFilter || !pPinName)
		return NULL;

	// enumerate of pins on the filter 
	hr = pBaseFilter->EnumPins(&pEnumPin);
	if (hr == S_OK && pEnumPin)
	{
		pEnumPin->Reset();
		while (pEnumPin->Next( 1, &pPin, &CountReceived) == S_OK && pPin)
		{
			memset(String, NULL, sizeof(String));

			hr = pPin->QueryPinInfo(&PinInfo);
			if (hr == S_OK)
			{
				length = wcslen (PinInfo.achName) + 1;
				pString = new char [length];

				// get the pin name 
				WideCharToMultiByte(CP_ACP, 0, PinInfo.achName, -1, pString, length,
					NULL, NULL);

				//strcat (String, pString);
				StringCbCat(String,strlen(String) + strlen(pString)+1,pString);

				// is there a match
				if (strstr(String, pPinName))
					pThePin = pPin;	  // yes
				else
					pPin = NULL;	  // no

				delete pString;

			}
			else
			{
				// need to release this pin
				pPin->Release();
			}


		}	// end if have pin

		// need to release the enumerator
		pEnumPin->Release();
	}

	// return address of pin if found on the filter
	return pThePin;

}

IBaseFilter* CMACToolDlg::CreateDVBSNetworkProvider(void)
{

	BSTR name;
	CLSID clsid;
	IBaseFilter *provider;

	tuningspace=LoadDVBSTuningSpace();
	tuningspace->get_NetworkType(&name);
	CLSIDFromString(name,&clsid);
	SysFreeString(name);
	CoCreateInstance(clsid,NULL,CLSCTX_INPROC_SERVER,IID_IBaseFilter,reinterpret_cast<void**>(&provider));
	{
		CComPtr<ITuner> tuner;
		CComPtr<ITuneRequest> request;

		tuningspace->CreateTuneRequest(&request);
		provider->QueryInterface(IID_ITuner,(void**)&tuner);
		tuner->put_TuningSpace(tuningspace);
		tuner->put_TuneRequest(request);
		//
	}
	return provider;
}

IBaseFilter* CMACToolDlg::CreateDVBCNetworkProvider(void)
{

	BSTR name;
	CLSID clsid;
	IBaseFilter *provider;

	tuningspace=LoadDVBCTuningSpace();
	tuningspace->get_NetworkType(&name);
	CLSIDFromString(name,&clsid);
	SysFreeString(name);
	CoCreateInstance(clsid,NULL,CLSCTX_INPROC_SERVER,IID_IBaseFilter,reinterpret_cast<void**>(&provider));
	{
		CComPtr<ITuner> tuner;
		CComPtr<ITuneRequest> request;

		tuningspace->CreateTuneRequest(&request);
		provider->QueryInterface(IID_ITuner,(void**)&tuner);
		tuner->put_TuningSpace(tuningspace);
		tuner->put_TuneRequest(request);
		//
	}
	return provider;
}


IBaseFilter* CMACToolDlg::CreateDVBTNetworkProvider(void)
{

	BSTR name;
	CLSID clsid;
	IBaseFilter *provider;

	tuningspace=LoadDVBTTuningSpace();
	tuningspace->get_NetworkType(&name);
	CLSIDFromString(name,&clsid);
	SysFreeString(name);
	CoCreateInstance(clsid,NULL,CLSCTX_INPROC_SERVER,IID_IBaseFilter,reinterpret_cast<void**>(&provider));
	{
		CComPtr<ITuner> tuner;
		CComPtr<ITuneRequest> request;

		tuningspace->CreateTuneRequest(&request);
		provider->QueryInterface(IID_ITuner,(void**)&tuner);
		tuner->put_TuningSpace(tuningspace);
		tuner->put_TuneRequest(request);
		//
	}
	return provider;
}

BOOL CMACToolDlg::CreateDVBSFilter()
{
	int ll_init=0;
	HRESULT hr = S_OK;

		//初始化COM接口
		CoCreateInstance(CLSID_FilterGraph,NULL,CLSCTX_INPROC_SERVER,IID_IGraphBuilder,(void**)&builder);
		//找到其中的接口  控制接口
		builder->QueryInterface(IID_IMediaControl,(void**)&control);
	
		// 建立DVB的Filter
		provider=CreateDVBSNetworkProvider();
	
		//添加刚建立的Filter
		if(provider)
		{
			builder->AddFilter(provider, L"Network DVBS Provider");
		}
		else
		{
			MessageBox("CreateDVBSNetworkProvider Failed","error",MB_OK);
			return FALSE;
		}
	
		
		//Tuner
		//这里为什么要判断这么多次
		if(!TunerFilter)	//TunerFilter has been created in CheckDeviceReady() !!!
		{
			TunerFilter=CreateTunerDevice();
		}
	
		if(TunerFilter)
		{
			builder->AddFilter(TunerFilter,TunerName);
		}
		else
		{
			Sleep(3000);
	
			TunerFilter=CreateTunerDevice();//再次判断添加
	
			if(TunerFilter)
			{
				builder->AddFilter(TunerFilter,TunerName);
			}
			else
			{
				MessageBox("CreateTunerDevice Failed","error",MB_OK);
				return FALSE;
			}
		}
	
		//CaptureDevice
		CaptureFilter=CreateCaptureDevice();
		if(CaptureFilter)
		{
			builder->AddFilter(CaptureFilter,CaptuerName);
		}
		else
		{
			Sleep(3000);
	
			CaptureFilter=CreateCaptureDevice();
			if(CaptureFilter)
			{
				builder->AddFilter(CaptureFilter,CaptuerName);
			}
			else
			{
				//MessageBox("CreateCaptureDevice Failed","error",MB_OK);
				return FALSE;
			}
		}
	
		//添加编码 LoadFilter(CLSID_MPEG2Demultiplexer);
		demux=CreateMPEG2Demultiplexer();
		if(demux)
		{
			builder->AddFilter(demux,L"MPEG2 Demultiplexer");
		}
		else
		{
			MessageBox("CreateMPEG2Demultiplexer Failed","error",MB_OK);
			return FALSE;
		}
	
	
		//这里什么意思
		infinite=CreateInfinitePinTee();
		if(infinite)
		{
			builder->AddFilter(infinite,L"Infinite pin tee splitter");
		}
		else
		{
			MessageBox("CreateInfinitePinTee Failed","error",MB_OK);
			return FALSE;
		}
	
	
		//传输Pin?
		tif=CreateTransportInformationFilter();
		if(tif)
		{
			builder->AddFilter(tif, L"transport information filter");
		}
		else
		{
			MessageBox("Createtransport Failed","error",MB_OK);
			return FALSE;
		}
	
	
	
	ConnectPins(builder,provider,TunerFilter);	
	ConnectPins(builder,TunerFilter,CaptureFilter);
	ConnectPins(builder, CaptureFilter, infinite);
	ConnectPins(builder,infinite, demux);
	ConnectPins(builder,demux, tif);
		

	m_pTunerDevice = TunerFilter;
	m_pKsCtrl = NULL;
	m_pTunerPin=NULL;
	m_pTunerPin = FindPinOnFilter(m_pTunerDevice, "Input0");
	if(!ll_init && m_pTunerPin != NULL)
	{
		hr = m_pTunerPin->QueryInterface(IID_IKsPropertySet,
			reinterpret_cast<void**>(&m_pKsCtrl));
		if (FAILED(hr))
		{
			ll_init = 1;
			MessageBox("m_pTunerDevice QueryInterface Failed","error",MB_OK);
			m_pKsCtrl = NULL;
			return FALSE;			
		}
	}
	m_pVCKsCtrl = NULL;

	return TRUE;

}

BOOL CMACToolDlg::CreateDVBTFilter()
{
	int ll_init=0;
	HRESULT hr = S_OK;

		//初始化COM接口
		CoCreateInstance(CLSID_FilterGraph,NULL,CLSCTX_INPROC_SERVER,IID_IGraphBuilder,(void**)&builder);
		//找到其中的接口  控制接口
		builder->QueryInterface(IID_IMediaControl,(void**)&control);
	
		// 建立DVB的Filter

	
	provider=CreateDVBTNetworkProvider();
		
			//添加刚建立的Filter
	if(provider)
		{
			builder->AddFilter(provider, L"Network DVBT Provider");
		}
		else
		{
			MessageBox("CreateDVBTNetworkProvider Failed","error",MB_OK);
			return FALSE;
		}
		
		//Tuner
		//这里为什么要判断这么多次
		if(!TunerFilter)	//TunerFilter has been created in CheckDeviceReady() !!!
		{
			TunerFilter=CreateTunerDevice();
		}
	
		if(TunerFilter)
		{
			builder->AddFilter(TunerFilter,TunerName);
		}
		else
		{
			Sleep(3000);
	
			TunerFilter=CreateTunerDevice();//再次判断添加
	
			if(TunerFilter)
			{
				builder->AddFilter(TunerFilter,TunerName);
			}
			else
			{
				MessageBox("CreateTunerDevice Failed","error",MB_OK);
				return FALSE;
			}
		}
	
		//CaptureDevice
		CaptureFilter=CreateCaptureDevice();
		if(CaptureFilter)
		{
			builder->AddFilter(CaptureFilter,CaptuerName);
		}
		else
		{
			Sleep(3000);
	
			CaptureFilter=CreateCaptureDevice();
			if(CaptureFilter)
			{
				builder->AddFilter(CaptureFilter,CaptuerName);
			}
			else
			{
				//MessageBox("CreateCaptureDevice Failed","error",MB_OK);
				return FALSE;
			}
		}
	
		//添加编码 LoadFilter(CLSID_MPEG2Demultiplexer);
		demux=CreateMPEG2Demultiplexer();
		if(demux)
		{
			builder->AddFilter(demux,L"MPEG2 Demultiplexer");
		}
		else
		{
			MessageBox("CreateMPEG2Demultiplexer Failed","error",MB_OK);
			return FALSE;
		}
	
	
		//这里什么意思
		infinite=CreateInfinitePinTee();
		if(infinite)
		{
			builder->AddFilter(infinite,L"Infinite pin tee splitter");
		}
		else
		{
			MessageBox("CreateInfinitePinTee Failed","error",MB_OK);
			return FALSE;
		}
	
	
		//传输Pin?
		tif=CreateTransportInformationFilter();
		if(tif)
		{
			builder->AddFilter(tif, L"transport information filter");
		}
		else
		{
			MessageBox("Createtransport Failed","error",MB_OK);
			return FALSE;
		}
	
	

		ConnectPins(builder,provider,TunerFilter);
		ConnectPins(builder,TunerFilter,CaptureFilter);
		ConnectPins(builder, CaptureFilter, infinite);
		ConnectPins(builder,infinite, demux);
		ConnectPins(builder,demux, tif);
		m_pTunerDevice = TunerFilter;

	m_pKsCtrl = NULL;
	m_pTunerPin=NULL;
	m_pTunerPin = FindPinOnFilter(m_pTunerDevice, "Input0");
	if(!ll_init && m_pTunerPin != NULL)
	{
		hr = m_pTunerPin->QueryInterface(IID_IKsPropertySet,
			reinterpret_cast<void**>(&m_pKsCtrl));
		if (FAILED(hr))
		{
			ll_init = 1;
			MessageBox("m_pTunerDevice QueryInterface Failed","error",MB_OK);
			m_pKsCtrl = NULL;
			return FALSE;			
		}
	}
	m_pVCKsCtrl = NULL;

	return TRUE;

}

BOOL CMACToolDlg::CreateDVBCFilter()
{
	int ll_init=0;
	HRESULT hr = S_OK;

		//初始化COM接口
		CoCreateInstance(CLSID_FilterGraph,NULL,CLSCTX_INPROC_SERVER,IID_IGraphBuilder,(void**)&builder);
		//找到其中的接口  控制接口
		builder->QueryInterface(IID_IMediaControl,(void**)&control);
	
		// 建立DVB的Filter

	
	provider=CreateDVBCNetworkProvider();
		
			//添加刚建立的Filter
	if(provider)
		{
			builder->AddFilter(provider, L"Network DVBC Provider");
		}
		else
		{
			MessageBox("CreateDVBCNetworkProvider Failed","error",MB_OK);
			return FALSE;
		}
		
		//Tuner
		//这里为什么要判断这么多次
		if(!TunerFilter)	//TunerFilter has been created in CheckDeviceReady() !!!
		{
			TunerFilter=CreateTunerDevice();
		}
	
		if(TunerFilter)
		{
			builder->AddFilter(TunerFilter,TunerName);
		}
		else
		{
			Sleep(3000);
	
			TunerFilter=CreateTunerDevice();//再次判断添加
	
			if(TunerFilter)
			{
				builder->AddFilter(TunerFilter,TunerName);
			}
			else
			{
				MessageBox("CreateTunerDevice Failed","error",MB_OK);
				return FALSE;
			}
		}
	
		//CaptureDevice
		CaptureFilter=CreateCaptureDevice();
		if(CaptureFilter)
		{
			builder->AddFilter(CaptureFilter,CaptuerName);
		}
		else
		{
			Sleep(3000);
	
			CaptureFilter=CreateCaptureDevice();
			if(CaptureFilter)
			{
				builder->AddFilter(CaptureFilter,CaptuerName);
			}
			else
			{
				//MessageBox("CreateCaptureDevice Failed","error",MB_OK);
				return FALSE;
			}
		}
	
		//添加编码 LoadFilter(CLSID_MPEG2Demultiplexer);
		demux=CreateMPEG2Demultiplexer();
		if(demux)
		{
			builder->AddFilter(demux,L"MPEG2 Demultiplexer");
		}
		else
		{
			MessageBox("CreateMPEG2Demultiplexer Failed","error",MB_OK);
			return FALSE;
		}
	
	
		//这里什么意思
		infinite=CreateInfinitePinTee();
		if(infinite)
		{
			builder->AddFilter(infinite,L"Infinite pin tee splitter");
		}
		else
		{
			MessageBox("CreateInfinitePinTee Failed","error",MB_OK);
			return FALSE;
		}
	
	
		//传输Pin?
		tif=CreateTransportInformationFilter();
		if(tif)
		{
			builder->AddFilter(tif, L"transport information filter");
		}
		else
		{
			MessageBox("Createtransport Failed","error",MB_OK);
			return FALSE;
		}
	
	
		m_pTunerDevice = TunerFilter;
		ConnectPins(builder,provider,TunerFilter);
		ConnectPins(builder,TunerFilter,CaptureFilter);
	
		ConnectPins(builder, CaptureFilter, infinite);
		ConnectPins(builder,infinite, demux);
		ConnectPins(builder,demux, tif);

	m_pKsCtrl = NULL;
	m_pTunerPin=NULL;
	m_pTunerPin = FindPinOnFilter(m_pTunerDevice, "Input0");
	if(!ll_init && m_pTunerPin != NULL)
	{
		hr = m_pTunerPin->QueryInterface(IID_IKsPropertySet,
			reinterpret_cast<void**>(&m_pKsCtrl));
		if (FAILED(hr))
		{
			ll_init = 1;
			MessageBox("m_pTunerDevice QueryInterface Failed","error",MB_OK);
			m_pKsCtrl = NULL;
			return FALSE;			
		}
	}
	m_pVCKsCtrl = NULL;

	return TRUE;

}


BOOL CMACToolDlg::DestroyWindow()
{
	// TODO: Add your specialized code here and/or call the base class

	if(control != NULL)
	{
		control->Stop();
	}

	if(m_pKsCtrl)
	{
		WaitForSingleObject( hIOMutex, INFINITE );
		m_pKsCtrl->Release();
		m_pKsCtrl = NULL;	
		ReleaseMutex(hIOMutex);
	}
	if(CaptureFilter != NULL)
	{
		builder->RemoveFilter(CaptureFilter);		
	}
	if(TSDump != NULL)
	{
		builder->RemoveFilter(TSDump);
	}
	if(provider != NULL)
	{
		builder->RemoveFilter(provider);
	}
	if(control != NULL)
	{
		control.Release();
		control = NULL;
	}
	if(CaptureFilter != NULL)
	{
		CaptureFilter.Release();
		CaptureFilter = NULL;
	}
	if(TSDump != NULL)
	{
		TSDump.Release();
		TSDump = NULL;
	}
	if(demux != NULL)
	{
		demux.Release();
		demux = NULL;
	}
	if(infinite != NULL)
	{
		infinite.Release();
		infinite = NULL;
	}
	if(tif != NULL)
	{
		tif.Release();
		tif = NULL;
	}
	if(provider != NULL)
	{
		provider.Release();
		provider = NULL;
	}	

	if(TunerFilter)
	{
		builder->RemoveFilter(TunerFilter);
		TunerFilter.Release();
	}

	if(builder != NULL)
	{
		builder.Release();
		builder = NULL;
	}	

	CoUninitialize();

	return CDialog::DestroyWindow();
}


void CMACToolDlg::OnBnClickedBtnRead()
{
	// TODO: Add your control notification handler code here
//	HRESULT hr = S_OK;
	unsigned long BytesRead = 0;
	unsigned char mode[2];
	DWORD TypeSupport=0;

	/*HRESULT hr = m_pKsCtrl->QuerySupported(KSPROPSETID_BdaTunerExtensionProperties,
		   KSPROPERTY_BDA_MACACCESS, 
		   &TypeSupport);
   
	   if FAILED(hr)
	   {
		   AfxMessageBox("QuerySupported KSPROPERTY_BDA_I2CACCESS error");
		   return ;    
	   }
*/
   HRESULT hr = m_pKsCtrl->QuerySupported(KSPROPSETID_BdaTunerExtensionProperties,
		KSPROPERTY_BDA_CTRL_DemodMode, 
		&TypeSupport);

	if FAILED(hr)
	{
		AfxMessageBox("QuerySupported KSPROPERTY_BDA_CTRL_DemodMode error");
		return ;	
	}

	if(m_pKsCtrl!=NULL)
	{

	   hr = m_pKsCtrl->Get(KSPROPSETID_BdaTunerExtensionProperties,
		                        KSPROPERTY_BDA_CTRL_DemodMode,
		                        &mode,
		                        sizeof( mode ),
		                        &mode,
		                        sizeof( mode ),
		                        &BytesRead );
 

	   CString strTemp(_T(""));
	   strTemp.Format("TunerA =%d",mode[0]);
	   OutputDebugString(strTemp);
		switch(mode[0])
		{
			case 1:
				strTemp = "ISDB-T";
				break;
			case 2:
				strTemp = "ISDB-S/S3";
				break;
			case 3:
				strTemp = "ATSC";
				break;
			case 4:
				strTemp = "DVB-C";
				break;
			case 5:
				strTemp = "ISDB-T";
				break;
			case 6:
				strTemp = "MCNS/QAMB";
				break;
			case 7:
				strTemp = "DVBC2";
				break;
			default:
				strTemp = "NO Set";
				break;
		
		}
		
	   ((CEdit*)GetDlgItem(IDC_EDIT_TunerA))->SetWindowText(strTemp);

  // 	   CString strTemp(_T(""));
	   strTemp.Format("TunerB =%d",mode[1]);
	   OutputDebugString(strTemp);
		switch(mode[1])
		{
			case 1:
				strTemp = "ISDB-T";
				break;
			case 2:
				strTemp = "ISDB-S/S3";
				break;
			case 3:
				strTemp = "ATSC";
				break;
			case 4:
				strTemp = "DVB-C";
				break;
			case 5:
				strTemp = "ISDB-T";
				break;
			case 6:
				strTemp = "MCNS/QAMB";
				break;
			case 7:
				strTemp = "DVBC2";
				break;
			default:
				strTemp = "NO Set";
				break;
		
		}
		
	   ((CEdit*)GetDlgItem(IDC_EDIT_TunerB))->SetWindowText(strTemp);

		}


	UpdateData(FALSE);


}

void CMACToolDlg::OnBnClickedBtnWrite()
{
	// TODO: Add your control notification handler code here
	HRESULT hr  ;
	unsigned char m_CurMode[2];
	DWORD TypeSupport=0;

   hr = m_pKsCtrl->QuerySupported(KSPROPSETID_BdaTunerExtensionProperties,
		KSPROPERTY_BDA_CTRL_DemodMode, 
		&TypeSupport);

	if FAILED(hr)
	{
		AfxMessageBox("QuerySupported KSPROPERTY_BDA_CTRL_DemodMode error");
		return ;	
	}
	if ((m_TunerACurMode==0)||(m_TunerBCurMode==0))
	{
		MessageBox("Sorry,Please Set all Tuner mode. ","TBS Changed Mode Tool",MB_OK);
		return;
	}
	if ((m_TunerACurMode==3)||(m_TunerBCurMode==3))
	{
		MessageBox("Sorry,TBS6812 can not support ATSC. ","TBS Changed Mode Tool",MB_OK);
		return;
	}
	m_CurMode[0]= m_TunerACurMode;
	m_CurMode[1]= m_TunerBCurMode;

	if(m_pKsCtrl!=NULL)
	{
			
		hr = m_pKsCtrl->Set(KSPROPSETID_BdaTunerExtensionProperties,
			KSPROPERTY_BDA_CTRL_DemodMode,
			&m_CurMode,
			sizeof( m_CurMode ),
			&m_CurMode,
			sizeof( m_CurMode ));
		if (FAILED(hr))
		{
		MessageBox("write mac11 ","error",MB_OK);
		}

		Sleep(1000);
		MessageBox("Attention,please change to corresponding signal cable ","TBS Changed Mode Tool",MB_OK);
		MessageBox("Set finished,Please wait about 8 seconds let device finish changed mode .Thank you ","TBS Changed Mode Tool",MB_OK);

		OnCancel();


	}
	
}



void CMACToolDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	PostMessage(WM_NCLBUTTONDOWN,HTCAPTION,MAKELPARAM(point.x,point.y));

	CDialog::OnLButtonDown(nFlags, point);
}

void CMACToolDlg::OnCbnSelchangeComboTuneraselmode()
{
	// TODO: Add your control notification handler code here
	m_TunerACurMode=m_TunerASelMode.GetCurSel()+1;
	CString strTemp(_T(""));
	strTemp.Format("TunerAMode =%d",m_TunerACurMode);
	OutputDebugString(strTemp);

}

void CMACToolDlg::OnCbnSelchangeComboTunerbselmode()
{
	// TODO: Add your control notification handler code here
	m_TunerBCurMode=m_TunerBSelMode.GetCurSel()+1;
	CString strTemp(_T(""));
	strTemp.Format("TunerBMode =%d",m_TunerBCurMode);
	OutputDebugString(strTemp);
}
