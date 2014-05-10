#if _MSC_VER == 1200
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0502
#endif

#include "PathLib.h"
#include "Utils.h"
#include "nbsg.h"
#include "res/resource.h"
#include <cassert>

string APathLib::expandEnvString(const char* src)
{
	if(!strchr(src,'%')){
		return src;
	}

	int len = ExpandEnvironmentStrings(src,NULL,0);
	char* expand = new char[len+1+1];
	ExpandEnvironmentStrings(src,expand,len+1+1);

	string s = expand;
	delete expand;
	return s;
}

bool APathLib::isFileExists(const char* pszFile,bool bExpandEnvVar)
{
	bool b = GetFileAttributes(pszFile)!=INVALID_FILE_ATTRIBUTES;
	if(!b && bExpandEnvVar){
		string s = APathLib::expandEnvString(pszFile);
		b = GetFileAttributes(s.c_str()) != INVALID_FILE_ATTRIBUTES;
	}
	return b;
}

HICON APathLib::getFileIcon(const char* filespec)
{
	SHFILEINFO shfi = {0};
	SHGetFileInfo(APathLib::expandEnvString(filespec).c_str(), 0, &shfi, sizeof(shfi), SHGFI_ICON|SHGFI_LARGEICON);
	return shfi.hIcon;
}

HICON APathLib::GetIndexedFileIcon(const char* filespec)
{
	string icon_str(filespec);
	string res,index;
	string::size_type pos = icon_str.find_first_of(',');
	if(pos!=string::npos){
		res = icon_str.substr(0,pos);
		index = icon_str.substr(pos+1);
	}else{
		res = icon_str;
		index = "0";
	}
	return ExtractIcon(g_pApp->getInstance(),res.c_str(),atoi(index.c_str()));
}

string APathLib::getSpecialFolder(int csidl)
{
	char folder[MAX_PATH]={0};
	SHGetSpecialFolderPath(NULL,folder,csidl,FALSE);
	return folder;
}

/***********************************************************************
����:getFileDir
����:��·������Ŀ¼
����:src-Դ����·��
����:û��Ŀ¼���ؿ�("")
˵��:���ص�Ŀ¼����'\\'
***********************************************************************/
string APathLib::getFileDir(const char* src)
{
	char* pch = NULL;
	char ch;
	string dst;

	if(!src) return dst;
	pch = (char*)strrchr(src,'\\');
	if(!pch) return dst;
	ch = *(pch+1);
	*(pch+1) = 0;
	dst = src;
	*(pch+1) = ch;
	return dst;
}

/***********************************************************************
����:trim
����:����ַ���ǰ��Ŀո�
����:src - Դ�ַ���
����:Դ�ַ���
˵��:
***********************************************************************/
char* APathLib::trim(char* src)
{
	char* psrc = NULL;
	char *pstart=NULL,*pend=NULL;
	int len = 0;

	if(!src) return NULL;
	len = strlen(src);
	pstart = src;
	psrc = src;

	while(*pstart && (*pstart==0x20||*pstart==0x09))
		pstart++;
	if(!*pstart){
		*src = 0;
		return src;
	}
	pend = src+len-1;
	while(*pend==0x20||*pend==0x09)
		pend--;
	len = ((unsigned int)pend-(unsigned int)pstart)/sizeof(char)+1;
	while(len--)
		*psrc++ = *pstart++;
	*psrc = 0;
	return src;
}

/***********************************************************************
����:show_dir
����:��ָ������·����Ӧ��Ŀ¼��������Ӧ�Ĵ�����Ϣ(�������Ļ�)
����:filespec - ����Ŀ¼���ļ�
����:!0-�ɹ�; 0-ʧ��
˵��:
***********************************************************************/
bool APathLib::showDir(HWND hWnd,const char* filespec)
{
	DWORD dwAttributes;
	const char* p = filespec;
	char argv[MAX_PATH+32];
	int ret;

	if(!isFileExists(filespec)){
		AUtils::msgbox(hWnd,MB_ICONERROR,g_pApp->getAppName(),"ָ�����ļ���Ŀ¼������:\n%s",filespec);
		return 0;
	}
	string s = APathLib::expandEnvString(filespec);
	dwAttributes = GetFileAttributes(s.c_str());
	if(dwAttributes & FILE_ATTRIBUTE_DIRECTORY){
		_snprintf(argv,sizeof(argv),s.c_str());
	}else{
		_snprintf(argv,sizeof(argv),"/select,\"%s\"",s.c_str());
	}
	ret = (int)ShellExecute(NULL,"open","explorer",argv,NULL,SW_SHOW);
	if(ret>32){
		return 1;
	}else{
		AUtils::msgerr(hWnd,"��Ŀ¼ʱ��������");
		return 0;
	}
}

char* APathLib::toLowerCase(char* src)
{
	char* pch = src;
	if(src==NULL) return NULL;
	while(*pch){
		if(*pch>=L'A' && *pch<=L'Z'){
			*pch += 32;
		}
		pch++;
	}
	return src;
}

/***************************************************
��  ��:getFilName
��  ��:��ʾ���ļ��Ի���
��  ��:
	hwnd:������
	title:�Ի������
	filter:�Ի������ѡ��
	buffer:�ļ������ص�����,����MAX_PATH
����ֵ:
	!0-��ѡ��
	0-δѡ��
˵  ��:
***************************************************/
bool APathLib::getOpenFileName(HWND hwnd,char* title, char* filter,char* buffer)
{
	OPENFILENAME ofn = {0};
	*buffer = 0;
	ofn.lStructSize = sizeof(ofn);
	ofn.Flags = OFN_ENABLESIZING|OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = &buffer[0];
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = title;
	return (bool)(GetOpenFileName(&ofn)>0);
}

/***********************************************************************
����:shellExec
����:ִ��ָ���ĳ���
����:exec-��ִ���ļ�; param-����; dir-����Ŀ¼
����:!0-�ɹ�; 0-ʧ��
˵��:���dirΪNULL�Ļ�, ȡexec����Ŀ¼(����еĻ�)
***********************************************************************/
bool APathLib::shellExec(HWND hWnd,const char* exec,const char* param,const char* dir)
{
	int ret;
	string folder;

	//�ж��Ƿ���GUID
	string guid = exec;
	if(guid[0]=='{' && guid[guid.length()-1]=='}'){
		guid = "shell:::";
		guid += exec;
		int ret = (int)ShellExecute(hWnd,"open",guid.c_str(),NULL,NULL,SW_SHOWNORMAL);
		if(ret<=32){
			AUtils::msgerr(hWnd,(char*)exec);
			return false;
		}
		return true;
	}


// 	if(!isFileExists(exec)){
// 		AUtils::msgbox(hWnd,MB_ICONEXCLAMATION,g_pApp->getAppName(),
// 			"%s\n\nָ�����ļ�������!",exec);
// 		return false;
// 	}
	string s = APathLib::expandEnvString(exec);
	if(dir==NULL){
		folder = APathLib::getFileDir(s.c_str());
		if(folder.length() < 3){
			dir = NULL;
		}else{
			dir = folder.c_str();
		}
	}
	if(GetFileAttributes(s.c_str())&FILE_ATTRIBUTE_DIRECTORY){
		ret = (int)ShellExecute(NULL,"open","explorer",s.c_str(),NULL,SW_SHOWNORMAL);
	}else{
		//lpFile��lpDirectory����ͬʱΪ���Ŀ¼
		if(s.find(':')==string::npos //û��':',˵�������·��
			&& dir && dir[2]!=':'	//Ŀ¼�Ѿ�ָ��,��Ҳ�����·��
		)
		{
			//���Ŀ¼��ʽΪ: �� .\dir\ �� dir\ ;
			char cur[MAX_PATH];
			::GetCurrentDirectory(MAX_PATH,cur);
			string tmp = cur;
			if(dir[0] == '\\'){
				
			}else{
				tmp += '\\';
			}
			tmp += dir;
			dir = tmp.c_str();
		}
		ret = (int)ShellExecute(NULL,"open",s.c_str(),param,dir,SW_SHOWNORMAL);
	}
	if(ret>32){
		return true;
	}else{
		if(GetLastError()==ERROR_NO_ASSOCIATION){
			string str = "shell32.dll,OpenAs_RunDLL ";
			str += s;
			ShellExecute(NULL,"open","rundll32",str.c_str(),NULL,SW_SHOW);
			return true;
		}else{
			AUtils::msgerr(hWnd,(char*)s.c_str());
			return false;
		}
	}
}

bool APathLib::ParseLnk(const wchar_t* lnk,string& path,string& args,string& description)
{
	HRESULT hres;
	IShellLink* psl=NULL;
	IPersistFile* ppf=NULL;
	::CoInitialize(NULL);
	hres = ::CoCreateInstance(CLSID_ShellLink,NULL,CLSCTX_INPROC_SERVER,IID_IShellLink,(LPVOID*)&psl);
	if(SUCCEEDED(hres)){
		hres = psl->QueryInterface(IID_IPersistFile,(void**)&ppf);
		if(SUCCEEDED(hres)){
			hres = ppf->Load(lnk,STGM_READ);
			if(SUCCEEDED(hres)){
				hres = psl->Resolve(GetDesktopWindow(),0);
				if(SUCCEEDED(hres)){
					char tpath[MAX_PATH]={0};
					char targs[384]={0};
					char tdesc[128]={0};
					WIN32_FIND_DATA fd;
					hres = psl->GetPath(tpath,MAX_PATH,&fd,SLGP_UNCPRIORITY);
					hres = psl->GetArguments(targs,sizeof(targs)/sizeof(*targs));
					hres = psl->GetDescription(tdesc,sizeof(tdesc)/sizeof(*tdesc));
					//if(SUCCEEDED(hres)){
						path = tpath;
						args = targs;
						description = tdesc;
					//}
				}
			}
			ppf->Release();
		}
		psl->Release();
	}
	CoUninitialize();
	return SUCCEEDED(hres);
}

bool APathLib::getFileDescription(const char* file,string& description)
{
	DWORD dwHandle;
	bool result=false;
	DWORD size = GetFileVersionInfoSize(file,&dwHandle);
	if(size){
		void* pv = (void*)new char[size];
		if(GetFileVersionInfo(file,0,size,pv)){
			struct{
				WORD wLanguage;
				WORD wCodePage;
			}*pTranslate;
			UINT cbTranslate;
			if(VerQueryValue(pv,"\\VarFileInfo\\Translation",(void**)&pTranslate,&cbTranslate)){
				if(cbTranslate/sizeof(*pTranslate)>0){
					char sub[64];
					char* pBuffer=NULL;
					UINT cbSizeBuf;
					wsprintf(sub,"StringFileInfo\\%04x%04x\\FileDescription",pTranslate[0].wLanguage,pTranslate[0].wCodePage);
					if(VerQueryValue(pv,sub,(void**)&pBuffer,&cbSizeBuf)){
						description = reinterpret_cast<char*>(pBuffer);
						result = true;
					}
				}
			}
		}
		delete[] pv;
	}
	return result;
}

bool APathLib::getNameString(const char* file,string& name)
{
	string str(file);
	string::size_type pos;
	pos = str.find_last_of('\\');
	if(pos != string::npos){
		name = str.substr(pos+1);
	}else{
		name = file;
	}
	return true;
}

bool APathLib::IsLink(const char* file)
{
	assert(file != NULL);
	bool b = false;
	const char* pdot = strrchr(file,'.');
	if(pdot != NULL){
		if(_stricmp(pdot,".lnk")==0 && !*(pdot+4)){
			b = true;
		}
	}
	return b;
}

HICON APathLib::GetClsidIcon(string guid_str)
{
	HKEY hKey;
	HICON hIcon;
	bool ret=false;
	if(RegOpenKeyEx(HKEY_CLASSES_ROOT,"CLSID",0,KEY_READ,&hKey) == ERROR_SUCCESS){
		char icon[300];
		DWORD dwIcon = sizeof(icon);
#if _MSC_VER>1200
		if(RegGetValue(hKey,string(guid_str+"\\DefaultIcon").c_str(),NULL,RRF_RT_REG_SZ,NULL,icon,&dwIcon) == ERROR_SUCCESS){
#else
		//if(RegQueryValueEx(hKey,string(guid_str+"\\DefaultIcon").c_str(),NULL,NULL,(unsigned char*)icon,&dwIcon) == ERROR_SUCCESS){
		if(RegQueryValue(hKey,string(guid_str+"\\DefaultIcon").c_str(),(LPSTR)icon,(PLONG)&dwIcon) == ERROR_SUCCESS){
#endif
			hIcon = APathLib::GetIndexedFileIcon(icon);
			ret = true;
		}
		RegCloseKey(hKey);
	}
	if(ret == false){
		hIcon = LoadIcon(g_pApp->getInstance(),MAKEINTRESOURCE(IDI_ICON1));
	}
	return hIcon;
}
