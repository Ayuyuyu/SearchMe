
#pragma once
#include <afxinet.h>
#include <iostream>
#include <fstream>
#include <vector>
//#include "html/ParserDom.h"  
//#include "html/utils.h" 

using namespace std;
//using namespace htmlcxx;

class SearchListInfo
{
public:
	CString SearchListName;
	CString SearchSize;
	CString SearchLink;
	CString SearchHotPoint;
};
class SearchPageTmp
{
public:
	CString strSearchItem;
	CString strPage;
};

class CGetInternetSearch
{
public:
	CGetInternetSearch(void);
	~CGetInternetSearch(void);
public:
	static BOOL SearchItem(CString strKeyWord,CString &html);
	static BOOL ParserHtmlListName(CString testbuffer);
	BOOL NextOrMorePage(CString strSearchItem);
	static unsigned int __stdcall CGetInternetSearch::ParserInsertThread(void *param);
public:
	vector<char> v;
	static vector<SearchListInfo> vListInfo;
};
