#include "Utils.h"
#include "SQLite.h"
#include "nbsg.h"
#include "PathLib.h"
#include "Str.h"
#include "ChildIndexDlg.h"
#include "AddDlg.h"

#include <Uilib.h>

using namespace DuiLib;

class CAddDlgImpl : public WindowImplBase
{
public:
	CAddDlgImpl(const char* table,CAddDlg::TYPE type,LPARAM lParam)
	{
		m_type = type;
		m_lParam = lParam;
		m_pCurTable = table;
	}
protected:
	virtual CDuiString GetSkinFolder()
	{
		return "skin/";
	}
	virtual CDuiString GetSkinFile()
	{
		return "AddDlg.xml";
	}
	virtual LPCTSTR GetWindowClassName(void) const
	{
		return "Ů������";
	}
	virtual void OnClick(TNotifyUI& msg);
	virtual void InitWindow();


private:
	void initFromParam()
	{
		string path(""),args(""),desc("");
		if(m_type == CAddDlg::TYPE::TYPE_PATH){
			const char* file = (const char*)m_lParam;
			DWORD dwAttr = ::GetFileAttributes(file);
			if(dwAttr == INVALID_FILE_ATTRIBUTES) return;

			if(dwAttr & FILE_ATTRIBUTE_DIRECTORY){
				path = file;
				APathLib::getNameString(file,desc);
			}else{
				if(APathLib::IsLink(file)){
					if(APathLib::ParseLnk(AStr(file,false).toWchar(),path,args,desc)){
						if(desc == ""){
							APathLib::getFileDescription(path.c_str(),desc);
							if(desc == ""){
								APathLib::getNameString(file,desc);
							}
						}
					}
				}else{
					path = file;
					APathLib::getFileDescription(file,desc);
					if(desc == ""){
						APathLib::getNameString(file,desc);
					}
				}
			}

			preIndex	->SetText("");
			preComment	->SetText(desc.c_str());
			prePath		->SetText(path.c_str());
			preParam	->SetText(args.c_str());
			preTimes	->SetText("0");
		}else if(m_type == CAddDlg::TYPE::TYPE_MODIFY){
			auto param = reinterpret_cast<AChildIndexDlg::LPARAM_STRUCT*>(m_lParam);

			preIndex	->SetText(param->si.index.c_str());
			preComment	->SetText(param->si.comment.c_str());
			prePath		->SetText(param->si.path.c_str());
			preParam	->SetText(param->si.param.c_str());
			preTimes	->SetText(param->si.times.c_str());
		}else if(m_type == CAddDlg::TYPE::TYPE_NEW){
			assert(m_lParam == 0);
			preIndex	->SetText("");
			preComment	->SetText("");
			prePath		->SetText("");
			preParam	->SetText("");
			preTimes	->SetText("0");
		}
	}
	void initTables()
	{
		ASettingsSqlite set;
		int size;
		char* tables;
		set.attach(GetHWND(),g_pSqliteBase->getPdb());
		set.getSetting("index_list",(void**)&tables,&size);

		std::string str(tables);
		str += "\r\n";

		//ȡ�ø���
		int n=1;
		std::string::size_type pos=-1;
		while((pos=str.find('\n',pos+1))!=std::string::npos){
			n++;
		}
		//���ݸ�������ռ�,ǰ����ָ������,����������
		m_zTables = new char[size+(n+1)*sizeof(char*)];

		try{
			int cindex=0;
			int cpos = 0;
			char* cptr=m_zTables+(n+1)*sizeof(char*);

			std::string::size_type pos=0,last_pos=0;
			while((pos=str.find_first_of('\n',last_pos))!=std::string::npos){
				if(pos-last_pos==1){
					last_pos = pos+1;
					continue;
				}

				std::string all  = str.substr(last_pos,pos-last_pos);
				all[all.length()-1]='\0';

				std::string::size_type pos_2 = all.find_first_of(',');
				std::string data_base = all.substr(0,pos_2);
				std::string data_name = all.substr(pos_2+1);

				((char**)m_zTables)[cindex] = cptr;
				memcpy(cptr,data_base.c_str(),data_base.size()+1);

				cindex++;
				cptr += data_base.size()+1;

				//ComboBox_AddString(hComboType,all.c_str());
				CListLabelElementUI* ele = new CListLabelElementUI;
				ele->SetText(all.c_str());
				ele->SetPadding(CDuiRect(3,0,0,0));
				pcboClass->Add(ele);

				last_pos = pos+1;
			}
			delete[] tables;

			((char**)m_zTables)[cindex] = NULL;
		}
		catch(...){
			AUtils::msgbox(GetHWND(),MB_ICONERROR,g_pApp->getAppName(),"�����б���ȷ!");
		}

		if(!m_pCurTable || m_pCurTable[0] == '\0'){
			pcboClass->SelectItem(0);
		}else{
			int i=0;
			char** p = (char**)m_zTables;
			while(p[i] != NULL){
				if(strcmp(p[i],m_pCurTable) == 0){
					pcboClass->SelectItem(i);
					break;
				}
				i++;
			}
		}
	}

private:
	CButtonUI* pbtnBrowse;
	CButtonUI* pbtnSave;
	CButtonUI* pbtnClose;
	CRichEditUI* preIndex;
	CRichEditUI* preComment;
	CRichEditUI* prePath;
	CRichEditUI* preParam;
	CRichEditUI* preTimes;
	CComboBoxUI* pcboClass;

private:
	CAddDlg::TYPE m_type;
	LPARAM m_lParam;
	const char* m_pCurTable;	//��ʼ��ʱ���ݽ����ı���
	char* m_zTables;			//���еı���

private:
	bool bNeedFree;				//�����ĸ��ط�Ҫ�õ���,...��������
};

CAddDlg::CAddDlg(HWND parent,const char* table,TYPE type,LPARAM lParam)
{
	assert(parent!=NULL && "CAddDlg::CAddDlg()");
	CAddDlgImpl* pFrame = new CAddDlgImpl(table,type,lParam);
	pFrame->Create(parent,NULL, WS_VISIBLE, WS_EX_WINDOWEDGE);	
	pFrame->CenterWindow();
	pFrame->ShowModal();
	delete pFrame;
}

CAddDlg::~CAddDlg()
{

}

void CAddDlgImpl::InitWindow()
{
	struct{
		void* ptr;
		const char*  name;
	} li[] = {
		{&pbtnBrowse,	"btnBrowse"},
		{&pbtnSave,		"btnSave"},
		{&pbtnClose,	"btnClose"},
		{&preIndex,		"richIndex"},
		{&preComment,	"richComment"},
		{&prePath,		"richPath"},
		{&preParam,		"richParam"},
		{&preTimes,		"richTimes"},
		{&pcboClass,	"cboClass"},
		{0,0}
	};
	for(int i=0;li[i].ptr; i++){
		*(CControlUI**)li[i].ptr = static_cast<CControlUI*>(m_PaintManager.FindControl(li[i].name));
	}
	initTables();
	initFromParam();
}

void CAddDlgImpl::OnClick(TNotifyUI& msg)
{
	if(msg.pSender == pbtnClose || msg.pSender->GetName()==_T("closebtn")){
		::DestroyWindow(GetHWND());
	}
	else if(msg.pSender == pbtnBrowse){
		char str[MAX_PATH];
		if(!APathLib::getOpenFileName(
			GetHWND(),
			"ѡ���ִ���ļ���·��",
			"Ӧ�ó���(*.exe)\x00*.exe\x00"
			"������(*.bat;*.cmd)\x00*.bat;*.cmd\x00"
			"Windows �ű�(*.vbs;*.vbe)\x00*.vbs;*.vbe)\x00"
			"�����ļ�(*.*)\x00*.*\x00",
			str))
		{	
			return;
		}
		prePath->SetText(str);
	}
	else if(msg.pSender == pbtnSave){
		CDuiString strIndex = preIndex->GetText();

		if(strIndex.GetLength()<1){
			if(AUtils::msgbox(GetHWND(),MB_ICONINFORMATION|MB_OKCANCEL,"��ʾ",
				"��û������������, �����㽫����ͨ�����������������ٴ�Ŀ��, ȷʵ��������ô\?") == IDCANCEL)
			{
				return;
			}
		}

		CDuiString strComment = preComment	->GetText();
		CDuiString strPath    = prePath		->GetText();
		CDuiString strParam   = preParam	->GetText();
		CDuiString strTimes   = preTimes	->GetText();

		if(	strIndex.Find('\'')!=-1
			|| strIndex.Find('\"')!=-1
			|| strIndex.Find(' ') !=-1
			|| strIndex.Find('\t')!=-1)
		{
			AUtils::msgbox(GetHWND(),MB_ICONEXCLAMATION,NULL,"���������Ƿ��ַ�!");
			return;
		}

		if( strComment.Find('\"')!=-1
			|| strComment.Find('\'')!=-1)
		{
			AUtils::msgbox(GetHWND(),MB_ICONEXCLAMATION,NULL,"˵�������Ƿ��ַ�!");
			return;
		}

		if(	strPath.Find('\'')!=-1
			|| strPath.Find('\"')!=-1
			|| strPath.Find('\t')!=-1)
		{
			AUtils::msgbox(GetHWND(),MB_ICONEXCLAMATION,NULL,"·�������Ƿ��ַ�!");
			return;
		}

		if(strPath[0]!='{' && !APathLib::isFileExists(strPath.GetData())){
			AUtils::msgbox(GetHWND(),MB_ICONEXCLAMATION,NULL,"�ļ� \'%s\' ������!",strPath.GetData());
			return;
		}

		char buf_times[12];
		sprintf(buf_times,"%u",atoi(strTimes.GetData()));
		if(strcmp(buf_times,strTimes.GetData())){
			AUtils::msgbox(GetHWND(),MB_ICONEXCLAMATION,NULL,"��ʼ��������ȷ!");
			return ;
		}

		AIndexSqlite::SQLITE_INDEX si;
		AIndexSqlite::SQLITE_INDEX* lastpsi = NULL;

		lastpsi = reinterpret_cast<AIndexSqlite::SQLITE_INDEX*>(m_lParam);

		if(m_type == CAddDlg::TYPE_PATH || m_type == CAddDlg::TYPE_NEW){
			si.idx[0] = '.';
		}else{//�޸�
			//strcpy(si.idx,lastpsi->idx);
			si.idx = lastpsi->idx;
		}

		si.index   = strIndex.GetData();
		si.comment = strComment.GetData();
		si.path    = strPath.GetData();
		si.param   = strParam.GetData();
		si.times   = strTimes.GetData();

		const char* selTable = ((char**)m_zTables)[pcboClass->GetCurSel()];
		BOOL bChangeTable = m_type==CAddDlg::TYPE_MODIFY && strcmp(selTable,m_pCurTable);

		AIndexSqlite is;
		is.setTableName(selTable); //---������ѡ��ı���
		is.attach(GetHWND(),g_pSqliteBase->getPdb());

		if(bChangeTable){
			//�����˱���,��������idx��Ӧ��Ϊ��
			si.idx[0] = '.';
		}

		if(is.add(&si)){
			if(m_type !=  CAddDlg::TYPE_MODIFY){//����������Ļ�,���ı�־Ϊ�޸�
				m_type = CAddDlg::TYPE_MODIFY;
				bNeedFree = TRUE;
				lastpsi = new AIndexSqlite::SQLITE_INDEX;
				*lastpsi = si;
				m_lParam = (LPARAM)lastpsi;
			}else if(m_type == CAddDlg::TYPE_MODIFY){
				if(bChangeTable){
					AIndexSqlite is;
					is.setTableName(m_pCurTable);
					is.attach(GetHWND(),g_pSqliteBase->getPdb());
					is.deleteIndex((char*)lastpsi->idx.c_str());
				}
				//memcpy((void*)m_lParam,&si,sizeof(si));
				*reinterpret_cast<AIndexSqlite::SQLITE_INDEX*>(m_lParam) = si;
			}
			//��ӳɹ�,����Ϊ��ǰ�ı���
			m_pCurTable = selTable;
		}
		return;
	}
}
