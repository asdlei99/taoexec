#include "Utils.h"
#include <cassert>
#include <cstdio>

/***************************************************
��  ��:msgbox
��  ��:��ʾ��Ϣ��
��  ��:
	msgicon:��Ϣ���
	caption:�Ի������
	fmt:��ʽ�ַ���
	...:���
����ֵ:
	�û�����İ�ť��Ӧ��ֵ(MessageBox)
˵  ��:
***************************************************/
int __cdecl AUtils::msgbox(HWND hWnd,UINT msgicon, const char* caption, const char* fmt, ...)
{
	assert(hWnd==NULL || ::IsWindow(hWnd));
	va_list va;
	char smsg[1024]={0};
	va_start(va, fmt);
	_vsnprintf(smsg, sizeof(smsg)/sizeof(*smsg), fmt, va);
	va_end(va);
	return MessageBox(hWnd, smsg, caption, msgicon);
}

/***************************************************
��  ��:msgerr
��  ��:��ʾ��prefixǰ׺��ϵͳ������Ϣ
��  ��:prefix-ǰ׺�ַ���
����ֵ:(��)
˵  ��:
***************************************************/
void AUtils::msgerr(HWND hWnd,char* prefix)
{
	char* buffer = NULL;
	if(!prefix) prefix = "";
	if(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,GetLastError(),MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),(LPSTR)&buffer,0,NULL)) 
	{
		msgbox(hWnd,MB_ICONHAND, NULL, "%s:%s", prefix, buffer);
		LocalFree(buffer);
	}
	else
	{
		msgbox(hWnd,MB_ICONSTOP,NULL,"msg err");
	}
}


/***********************************************************************
����:Assert
����:Debug
����:pv-�κα��ʽ,str-��ʾ
����:
˵��:
***********************************************************************/
void AUtils::myassert(void* pv,char* str)
{
	if(!pv){
		msgbox(NULL,MB_ICONERROR,NULL,"Debug Error:%s\n\n"
			"Ӧ�ó��������ڲ�����,�뱨�����!"
			"����������Ӧ�ó��������!",str==NULL?"<null>":str);
	}
}

/***************************************************
��  ��:set_clip_data
��  ��:����strָ����ַ�����������
��  ��:
	str:�ַ���,��0��β
����ֵ:
	�ɹ�:����
	ʧ��:��
˵  ��:
***************************************************/
bool AUtils::setClipData(const char* str)
{
	HGLOBAL hGlobalMem = NULL;
	char* pMem = NULL;
	int lenstr;

	if(str == NULL) return false;
	if(!OpenClipboard(NULL)) return false;

	lenstr = strlen(str)+sizeof(char);//Makes it null-terminated
	hGlobalMem = GlobalAlloc(GHND, lenstr);
	if(!hGlobalMem) return false;
	pMem = (char*)GlobalLock(hGlobalMem);
	EmptyClipboard();
	memcpy(pMem, str, lenstr);
	SetClipboardData(CF_TEXT, hGlobalMem);
	CloseClipboard();
	GlobalFree(hGlobalMem);
	return true;
}

/**************************************************
��  ��:
��  ��:�ж�һ��������Ƿ���ջ��
��  ��:
��  ��:
˵  ��:�����ж�ȫ�ֶ���
**************************************************/
bool AUtils::isObjOnStack(void* pObjAddr)
{
	bool bOnStack = true;
	MEMORY_BASIC_INFORMATION mbi = {0};
	if(VirtualQuery(&bOnStack,&mbi,sizeof(mbi))){
		bOnStack = pObjAddr>=mbi.BaseAddress && (DWORD)pObjAddr<=(DWORD)mbi.BaseAddress+mbi.RegionSize;
	}else{
		
	}
	return bOnStack;
}