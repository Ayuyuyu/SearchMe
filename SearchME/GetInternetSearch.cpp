#include "StdAfx.h"
#include "GetInternetSearch.h"
#include "SearchMEDlg.h"

HANDLE CGetInternetSearch::g_hThreadEvent =NULL;
CGetInternetSearch::CGetInternetSearch(void)
{
	g_hThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

CGetInternetSearch::~CGetInternetSearch(void)
{
}
//初始化
vector<SearchListInfo> CGetInternetSearch::vListInfo;


/*******************************************
*函数名称：SearchItem
*函数功能：访问网络 搜索关键字
*参数列表: 输入：搜索关键字，输出：获取的网页信息
*搜索字符有时间出问题，xp有不正常情况。
********************************************/
BOOL CGetInternetSearch::SearchItem(CString strKeyWord,CString &html)
{
		CString strBaiduSearch;
		strBaiduSearch.Format(_T("%s/search/%s.html"), MAIN_URL,strKeyWord);
		//MessageBox(NULL,strBaiduSearch,NULL,NULL);
		CInternetSession session;
		CHttpFile * file = NULL;
		try
		{
			file = (CHttpFile *)session.OpenURL(strBaiduSearch);
			if(file)
			{
				TCHAR line[1024 * 2] = {0};
				while(file->ReadString(line,1024*2) != NULL)
				{
					int nBufferSize = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)line, -1, NULL, 0);
					wchar_t *pBuffer = new wchar_t[nBufferSize+1];
					MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)line, -1 , pBuffer, nBufferSize*sizeof(wchar_t));  
					html += pBuffer;
					delete[] pBuffer;
				}
				file->Close();
			}
		}
		catch (CInternetException* e)
		{
			e->Delete();
			html = "CInternet Exception";
		}
		file = NULL;
		session.Close();
		//printf("%d\n",html.GetLength());
		return 1;
}

unsigned int __stdcall CGetInternetSearch::ParserInsertThread(void *param)
{
	
	SearchPageTmp *pItem = NULL;
	CGetInternetSearch* pthis = NULL;
	if (param == NULL)
		return -1;
	pItem = (SearchPageTmp *)param;
	pthis = (CGetInternetSearch*)(pItem->param);
	
	CString strTmp,strTmpPage;
	CString strKeyword(pItem->strSearchItem);
	CString nPageItem[5] = { _T("1-3"), _T("2-3"), _T("3-3"), _T("4-3"), _T("5-3") };
	int i = 0;
	while (i <5)
	{
		WaitForSingleObject(g_hThreadEvent, INFINITE);
		strTmpPage.Empty();
		strTmp.Empty();
		strTmpPage.Format(_T("%s/%s"), strKeyword, nPageItem[i]);

		pthis->SearchItem(strTmpPage, strTmp);
		pthis->ParserHtmlListName(strTmp);
		SetEvent(g_hThreadEvent);
		i++;
	}
	
	pSeachMEDlg->SendMessage(MSG_SEARCH_SUCESS, NULL, NULL);
	return 0;
}
/************************************
* 函数名称:    NextOrMorePage
* 功能描述:   读取bthand获取的数据 打开下几页 
* 参数列表:  输入 搜索关键字  输出 页面信息
* 返回值:  
/************************************/
BOOL CGetInternetSearch::NextOrMorePage(CString strSearchItem)
{

	CString nPageItem[5] = {_T("1-3"),_T("2-3"),_T("3-3"),_T("4-3"),_T("5-3")};
	CString strtmp,strhtml;
	//解析网页
	/*
	strtmp.Format(_T("%s/1-3"),strSearchItem);
	SearchItem(strtmp,strhtml);
	ParserHtmlListName(strhtml);
	*/
	SetEvent(g_hThreadEvent);
	HANDLE  handle;

	SearchPageTmp sitem;
	sitem.strSearchItem = strSearchItem;

	sitem.param = this;
	handle =(HANDLE)_beginthreadex(NULL, 0, ParserInsertThread, &sitem, 0, NULL);
	//handle[i] = AfxBeginThread((AFX_THREADPROC)ParserInsertThread,&sitem,0,0,0,NULL);
	//handle[i]=CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ParserInsertThread, &sitem, 0, NULL);
	Sleep(50);
	//WaitForMultipleObjects(5, handle, TRUE, INFINITE); 
	return 1;
}
/************************************
* 函数名称:    ParserHtmlListName
* 功能描述:   解析网页数据
* 参数列表:  输入 网页数据 保存到vector中
* 返回值:  
/************************************/
BOOL CGetInternetSearch::ParserHtmlListName(CString testbuffer)
{
	while(testbuffer.Find(_T("<li>")) >=0 )
	{
		SearchListInfo m_Search;


		cout<<"***********************"<<endl;

		//Link
		int smagnet = testbuffer.Find(_T("a href=\"/wiki"));
		int emagnet = testbuffer.Mid(smagnet).Find(_T("target=\"_blank\""));
		if (smagnet != -1 && emagnet != -1)	{ 
			CString Link = GetParseMagnet(testbuffer.Mid(smagnet + 8, emagnet - 10));
			m_Search.SearchLink = Link;
		}
		
		// ListName
		CString strTmpName;
		int splace = testbuffer.Find(_T("target=\"_blank\">"));
		int eplace = testbuffer.Mid(splace).Find(_T("</script>"));
		if (splace != -1 && eplace != -1)	{ 
			strTmpName = testbuffer.Mid(splace + 82, eplace - 86);
			strTmpName.Replace(_T("\"+\""), _T(""));
			CString strDecodeName = UrlDecode(strTmpName);
			strDecodeName.Replace(_T("</b>"), _T(""));
			strDecodeName.Replace(_T("<b>"), _T(""));
			m_Search.SearchListName = strDecodeName;
		}
		

		//Size
		int sSize = testbuffer.Find(_T("cpill yellow-pill"));
		int eSize = testbuffer.Mid(sSize).Find(_T("</b>"));
		if (sSize != -1 && eSize != -1)	{ 
			//cout<<testbuffer.Mid(sSize+13,eSize-sSize-13)<<endl;
			m_Search.SearchSize = testbuffer.Mid(sSize + 19, eSize-19);
		}
		else{
			int sSize_sub = testbuffer.Find(_T("File Size："));
			CString ss = testbuffer.Mid(sSize_sub);
			int eSize_sub = testbuffer.Mid(sSize_sub).Find(_T("</b>"));
			m_Search.SearchSize = testbuffer.Mid(sSize_sub + 38, eSize_sub - 38);
		}
		

		//link_hot
		int shot = testbuffer.Find(_T("Hot：<b>"));
		int ehot = testbuffer.Mid(shot).Find(_T("</b></span>"));
		if (shot != -1 && ehot != -1)
		{//cout<<testbuffer.Mid(shot+19,ehot-shot-19)<<endl;
			m_Search.SearchHotPoint = testbuffer.Mid(shot + 7, ehot - 7);
		}
		
		
		vListInfo.push_back(m_Search);

		testbuffer = testbuffer.Mid(shot +55);
		//testbuffer = testbuffer.Mid(eplace+4);
		//Sleep(1000);
		//EnterCriticalSection(&cs_Search);
		pSeachMEDlg->SendMessage( MSG_INSERTlIST, NULL, (LPARAM)&m_Search);
		//LeaveCriticalSection(&cs_Search);
	}
	
	return TRUE;
}


CString CGetInternetSearch::GetParseMagnet(CString wiki)
{
	if (wiki.GetLength() <= 0)
	{
		return NULL;
	}
	CString strURL, strHTML;
	strURL = MAIN_URL + wiki;
	//SearchItem(strURL, strHTML);
	DownloadStrings(strURL, strHTML);		//上面读不出来 
	if (strHTML.GetLength() >100)
	{
		int splace = strHTML.Find(_T("a href=\"magnet"));
		int eplace = strHTML.Mid(splace).Find(_T(">magnet"));
		if (splace == -1 || eplace == -1)	{ return FALSE; }
		CString strTmpLINK= strHTML.Mid(splace+7 , eplace -7);
		strTmpLINK.Replace(_T("\""), _T(""));
		return strTmpLINK;
	}
	return NULL;
}
bool CGetInternetSearch::DownloadStrings(LPCTSTR szUrl, CString &str)
{

	HINTERNET hInet = NULL;
	HINTERNET hUrl = NULL;
	DWORD dwBuf = 1024 * 1024, dwRead = 0; //1M
	auto_ptr<char> szBuf(new char[dwBuf]);
	memset(szBuf.get(), 0, dwBuf);
	bool bRet = false;
	try
	{
		hInet = InternetOpen(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		if (hInet == NULL)
			throw "error";
		hUrl = InternetOpenUrl(hInet, szUrl, NULL, 0,
			INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
		if (hUrl == NULL)
			throw "error";
		for (;;)
		{
			if (!InternetReadFile(hUrl, szBuf.get(), dwBuf, &dwRead))
			{
				bRet = false;
				break;
			}
			if (dwRead == 0)
			{
				bRet = true;
				break;
			}


			char *szBufx = (char*)malloc(dwRead + 1);
			memset(szBufx, 0, dwRead + 1);
			memcpy(szBufx, szBuf.get(), dwRead);

			CString strBufx(szBufx);
			str = str + strBufx;

			free(szBufx);

			//strTmp += std::string(szBuf,dwRead);
		}
		throw "ok";
	}
	catch (...)
	{
		if (hUrl != NULL)
			InternetCloseHandle(hUrl);
		if (hInet != NULL)
			InternetCloseHandle(hInet);
	}

	return bRet;
}

CString CGetInternetSearch::Utf8ToStringT(LPSTR str)
{
	_ASSERT(str);
	USES_CONVERSION;
	WCHAR *buf;
	int length = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	buf = new WCHAR[length + 1];
	ZeroMemory(buf, (length + 1) * sizeof(WCHAR));
	MultiByteToWideChar(CP_UTF8, 0, str, -1, buf, length);
	CString str_ret = (W2T(buf));
	delete[] buf;
	return str_ret;
}


CString CGetInternetSearch::UrlDecode(LPCTSTR url)
{
	_ASSERT(url);
	USES_CONVERSION;
	LPSTR _url = T2A(const_cast<LPTSTR>(url));
	int i = 0;
	int length = (int)strlen(_url);
	CHAR *buf = new CHAR[length];
	ZeroMemory(buf, length);
	LPSTR p = buf;
	char tmp[4] = { 0 };
	while (i < length)
	{
		if (i <= length - 3 && _url[i] == '%' && IsHexNum(_url[i + 1]) && IsHexNum(_url[i + 2]))
		{
			memset(tmp, 0, sizeof(tmp));
			memcpy(tmp, _url + i + 1, 2);
			sscanf(tmp, "%x", p++);
			i += 3;
		}
		else
		{
			*(p++) = _url[i++];
		}
	}
	CString strbuf = Utf8ToStringT(buf);
	delete[] buf;
	return strbuf;
}