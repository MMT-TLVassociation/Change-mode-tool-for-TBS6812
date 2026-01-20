#pragma once

//define the product version
//#define  TBS6921
//#define  TBS5921
//#define  TBS8921


//6925和 6922 是一样的
//#define  TBS6925
//#define  TBS6922
//#define  TBS6928
#define TBS6982

//6921产品
#ifdef TBS6921
//added 2010 11 11 liuy
static WCHAR *TunerName = L"TBS 6921 BDA DVBS/S2 Tuner/Demod";
static WCHAR *CaptuerName = L"TBS 6921 DVBS/S2 AVStream TS Capture";
static CString strTBSProductName = _T("TBS6921	MAC Tool");

#endif

//5921产品
#ifdef TBS5921
//added 2010 11 11 liuy
static WCHAR *TunerName = L"TBS QBOX DVB-S2 Tuner";
static WCHAR *CaptuerName = L"TBS QBOX DVB-S2 Capture";
static CString strTBSProductName = _T("TBS5921 MAC Tool");

#endif

//8921产品
#ifdef TBS8921
//added 2010 11 11 liuy
static WCHAR *TunerName = L"TBS 8921 BDA Tuner/Demod";
static WCHAR *CaptuerName = L"TBS 8921 AVStream TS Capture";
static CString strTBSProductName = _T("TBS8921 MAC Tool");

#endif

//6925产品
#ifdef TBS6925
//added 2010 11 11 liuy
static WCHAR *TunerName = L"TBS 6925 DVBS/S2 Tuner";
static WCHAR *CaptuerName = L"TBS 6925 BDA Digital Capture DVBS";
static CString strTBSProductName = _T("TBS6925 MAC Tool");

#endif

//6922产品
#ifdef TBS6922
//added 2010 11 11 liuy
static WCHAR *TunerName = L"TBS 6922 DVBS/S2 Tuner";
static WCHAR *CaptuerName = L"TBS 6922 BDA Digital Capture DVBS";
static CString strTBSProductName = _T("TBS6922 MAC Tool");

#endif

//6928产品
#ifdef TBS6928
//added 2010 11 11 liuy
static WCHAR *TunerName = L"TBS 6928 DVBS/S2 Tuner";
static WCHAR *CaptuerName = L"TBS 6928 BDA Digital Capture DVBS";
static CString strTBSProductName = _T("TBS6928 MAC Tool");

#endif


//6982 TBS 6982 DVBS/S2 Tuner
#ifdef TBS6982
//added 2010 11 11 liuy
static WCHAR *TunerName = L"TBS 6812 ISDB-T/S/S3 Tuner 0";
static WCHAR *CaptuerName = L"TBS 6812 ISDB-T/S/S3 Capture 0";
static CString strTBSProductName = _T("TBS6812 Change Mode Tool");

#endif
