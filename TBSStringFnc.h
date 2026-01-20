#pragma once

//  [8/3/2010 liuy]
//字符串操作函数集合

//从全路径中获取文件名
_inline CString GetFileNameFormFullPathName(CString FullPath)
{
	CString strTemp = FullPath;
	int nPos = strTemp.ReverseFind('\\');
	nPos += 1;//向后移动以为去掉\符号

	strTemp = strTemp.Right(strTemp.GetLength() - nPos);
	return strTemp;
};

_inline CString GetPathFolderFormFullPath(CString FullPath)
{
	CString strTemp = FullPath;
	strTemp = strTemp.Left(strTemp.ReverseFind('\\'));
	return strTemp;
};

//获得当前程序执行的路径
_inline CString GetFileExcuteFolder()
{
	CString strTemp(_T(""));
	TCHAR _szPath[MAX_PATH]={0};
	if(GetModuleFileName(GetModuleHandle(NULL),  _szPath, MAX_PATH) == 0)
	{
		return NULL;
	}
	strTemp.Format("%s",_szPath);
	int nIndex = strTemp.ReverseFind('\\');
	strTemp = strTemp.Left(nIndex);

	return strTemp;
}