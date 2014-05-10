#include <string>
#include <vector>
#include <map>
#include <list>
#include <sstream>
#include <windows.h>

using namespace std;

#include <UIlib.h>

using namespace DuiLib;

#include "res/resource.h"

#include "Utils.h"
#include "SQLite.h"
#include "PathLib.h"
#include "Except.h"

#include "AddDlg.h"
#include "MainDlg.h"
#include "InputBox.h"

class CMouseWheelOptionUI : public COptionUI
{
public:
	virtual LPCTSTR GetClass() const override
	{
		return _T("MouseWheelOptionUI");
	}
	virtual LPVOID GetInterface(LPCTSTR pstrName) override
	{
		if( _tcscmp(pstrName, _T("MouseWheelOption")) == 0 ) return this;
		return __super::GetInterface(pstrName);
	}
	virtual void DoEvent(TEventUI& event) override
	{
		if(IsMouseEnabled() && event.Type>UIEVENT__MOUSEBEGIN && event.Type<UIEVENT__MOUSEEND)
		{
			if(event.Type == UIEVENT_SCROLLWHEEL){
				TNotifyUI msg;
				msg.pSender = this;
				msg.sType = DUI_MSGTYPE_SCROLL;
				msg.wParam = event.wParam;	//SB_LINEDOWN && SB_LINEUP
				msg.lParam = event.lParam;
				GetManager()->SendNotify(msg);
				return;	//__super������,����ֱ�ӷ�����
			}
		}
		return __super::DoEvent(event);
	}
};

class CIconButtonUI : public CButtonUI
{
public:
	CIconButtonUI():
		m_hIcon(0)
	{

	}
	~CIconButtonUI()
	{
		::DestroyIcon(m_hIcon);
	}

	virtual LPCTSTR GetClass() const
	{
		return _T("IconButtonUI");
	}
	virtual LPVOID GetInterface(LPCTSTR pstrName)
	{
		if( _tcscmp(pstrName, "IconButton") == 0 ) return this;
		return __super::GetInterface(pstrName);
	}
	virtual UINT GetControlFlags() const
	{
		return (IsKeyboardEnabled() ? UIFLAG_TABSTOP : 0) | (IsEnabled() ? UIFLAG_SETCURSOR : 0);
	}

	virtual void SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue)
	{
		if(_tcscmp(pstrName,_T("path"))==0) m_strPath = pstrValue;

		else return __super::SetAttribute(pstrName,pstrValue);
	}

	void PaintText(HDC hDC)
	{

	}
	void PaintStatusImage(HDC hDC)
	{

		if(m_hIcon==NULL){
			m_hIcon = APathLib::getFileIcon(m_strPath);
			if(!m_hIcon)
				m_hIcon = APathLib::GetClsidIcon(m_strPath.GetData());
			if(!m_hIcon)
				m_hIcon = (HICON)INVALID_HANDLE_VALUE;
		}
		if(m_hIcon == (HICON)INVALID_HANDLE_VALUE)
			return;
		::DrawIconEx(hDC,m_rcPaint.left,m_rcPaint.top,m_hIcon,GetFixedWidth(),GetFixedHeight(),0,nullptr,DI_NORMAL);
		return __super::PaintStatusImage(hDC);

	}
private:
	HICON m_hIcon;
	CDuiString m_strPath;
};

class CIndexListUI : public CTileLayoutUI,public IDialogBuilderCallback
{
private:
	virtual CControlUI* CreateControl(LPCTSTR pstrClass)
	{
		if(_tcscmp(pstrClass,"IconButton")==0) return new CIconButtonUI;
		return nullptr;
	}

private:
	CDialogBuilder	m_builder;
	CSQLite*		m_db;

public:
	CIndexListUI(CSQLite* db,const char* cat,CPaintManagerUI& pm):
		m_db(db)
	{
		SetItemSize(CSize(80,80));
		SetManager(&pm,0,false);
		delete m_builder.Create("ListItem.xml",0,this);

		db->QueryCategory(cat,&m_iiVec);

		auto s = m_iiVec.begin();
		auto e = m_iiVec.end();

		for(; s != e; s++){
			AddItem(*s);
		}
		SetAttribute("vscrollbar","true");
	}

	~CIndexListUI()
	{
		auto s = m_iiVec.begin();
		auto e = m_iiVec.end();
		for(; s != e; s ++){
			delete *s;
		}
	}

	void AddItem(const CIndexItem* pii, bool bAddToVector=false)
	{
		CIconButtonUI* pBtn;
		CTextUI* pText;
		CContainerUI* pContainer = CreateContainer(&pBtn,&pText);
		pBtn->SetAttribute("path",pii->path.c_str());
		pBtn->SetTag(pii->idx);
		pText->SetText(pii->comment.c_str());
		Add(pContainer);
		if(bAddToVector){
			m_iiVec.push_back(const_cast<CIndexItem*>(pii));
		}
	}

	void RemoveItem(CContainerUI* pContainer, int tag)
	{
		CIndexItem* pii = FindIndexList(tag);
		for(auto s=m_iiVec.begin(); s!=m_iiVec.end(); ++s){
			if(*s == pii){
				m_db->DeleteItem(pii->idx); //throw
				m_iiVec.erase(s);
				delete pii;
				this->Remove(pContainer);
				return;
			}
		}
	}

	CContainerUI* CreateContainer(CIconButtonUI** ppBtn,CTextUI** ppText)
	{
		auto pContainer = static_cast<CContainerUI*>(m_builder.Create(this));
		*ppBtn = (CIconButtonUI*)static_cast<CHorizontalLayoutUI*>(pContainer->GetItemAt(0))->GetItemAt(0);
		*ppText = (CTextUI*)static_cast<CHorizontalLayoutUI*>(pContainer->GetItemAt(1));
		return pContainer;
	}

	CIndexItem* FindIndexList(int tag)
	{
		for(auto s=m_iiVec.begin(),e=m_iiVec.end(); s!=e; ++s){
			if((*s)->idx == tag){
				return *s;
			}
		}
		return nullptr;
	}

	CTextUI* GetTextControlFromButton(CButtonUI* pBtn)
	{
		auto pVert = static_cast<CVerticalLayoutUI*>(pBtn->GetParent()->GetParent());
		return static_cast<CTextUI*>(pVert->GetItemAt(1));
	}

	void RenameIndexItemsCategory(const char* to)
	{
		for(auto s=m_iiVec.begin(),e=m_iiVec.end(); s!=e; ++s){
			(*s)->category = to;
		}
	}

	vector<CIndexItem*> m_iiVec;

private:
};

class CMainDlgImpl : public WindowImplBase
{
public:
	CMainDlgImpl(CSQLite* db):
		m_tag(0)
	{
		m_db = db;
	}
protected:
	virtual CDuiString GetSkinFolder()
	{
		return "skin/";
	}
	virtual CDuiString GetSkinFile()
	{
		return "MainDlg.xml";
	}
	virtual LPCTSTR GetWindowClassName(void) const
	{
		return "Ů������";
	}

	virtual void InitWindow();
	virtual LRESULT OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual void OnFinalMessage( HWND hWnd )
	{
		__super::OnFinalMessage(hWnd);
		delete this;
	}

	DUI_DECLARE_MESSAGE_MAP()
	virtual void OnClick(TNotifyUI& msg);
	virtual void OnSelectChanged(TNotifyUI& msg);
	virtual void OnTimer(TNotifyUI& msg);
	virtual void OnMenu(TNotifyUI& msg);
	virtual void OnScroll(TNotifyUI& msg);

private:
	bool addTab(const char* name,int tag,const char* group);
	UINT GetTag()
	{
		return m_tag++;
	}

private:
	CSQLite*		m_db;
	UINT			m_tag;

	CHorizontalLayoutUI*	m_pTabList;
	CTabLayoutUI*			m_pTabPage;

	CButtonUI* m_pbtnClose;
	CButtonUI* m_pbtnMin;
	CButtonUI* m_pbtnRestore;
	CButtonUI* m_pbtnMax;

	class TabManager
	{
	private:
		struct IndexListOptionMap
		{
			CIndexListUI*			list;
			CMouseWheelOptionUI*	option;
			IndexListOptionMap(CIndexListUI* li,CMouseWheelOptionUI* op)
			{
				list = li;
				option = op;
			}
		};
	public:
		void Add(CMouseWheelOptionUI* op, CIndexListUI* li)
		{
			m_Pages.push_back(IndexListOptionMap(li,op));
		}
		UINT Size() const 
		{
			return m_Pages.size();
		}
		IndexListOptionMap GetAt(UINT index)
		{
			int i=0;
			auto s=m_Pages.begin();
			for(;i!=index;){
				++s;
				++i;
			}
			return *s;
		}
		CIndexListUI* FindList(CMouseWheelOptionUI* op)
		{
			for(auto s=m_Pages.begin(); s!=m_Pages.end(); ++s){
				if(s->option == op){
					return s->list;
				}
			}
			return nullptr;
		}
		CMouseWheelOptionUI* FindOption(CIndexListUI* li)
		{
			for(auto s=m_Pages.begin(); s!=m_Pages.end(); ++s){
				if(s->list == li){
					return s->option;
				}
			}
			return nullptr;
		}
		void Remove(CMouseWheelOptionUI* op)
		{
			for(auto s=m_Pages.begin(); s!=m_Pages.end(); ++s){
				if(s->option == op){
					m_Pages.erase(s);
					break;
				}
			}
		}
	private:

		list<IndexListOptionMap> m_Pages;
	};

	TabManager				m_tm;

};

CMainDlg::CMainDlg(CSQLite* db)
{
	CMainDlgImpl* pFrame = new CMainDlgImpl(db);
	pFrame->Create(NULL,"Software Manager",UI_WNDSTYLE_FRAME|WS_SIZEBOX,WS_EX_WINDOWEDGE);
	pFrame->CenterWindow();
	pFrame->ShowWindow(true);
}

CMainDlg::~CMainDlg()
{

}

DUI_BEGIN_MESSAGE_MAP(CMainDlgImpl, WindowImplBase)
	DUI_ON_MSGTYPE(DUI_MSGTYPE_CLICK,OnClick)
	DUI_ON_MSGTYPE(DUI_MSGTYPE_SELECTCHANGED,OnSelectChanged)
	DUI_ON_MSGTYPE(DUI_MSGTYPE_TIMER,OnTimer)
	DUI_ON_MSGTYPE(DUI_MSGTYPE_MENU,OnMenu)
	DUI_ON_MSGTYPE(DUI_MSGTYPE_SCROLL,OnScroll)
DUI_END_MESSAGE_MAP()

void CMainDlgImpl::OnClick(TNotifyUI& msg)
{
	if(msg.pSender->GetName() == "listItemBtn"){
		int tag = msg.pSender->GetTag();
		auto pList = static_cast<CIndexListUI*>(m_pTabPage->GetItemAt(m_pTabPage->GetCurSel()));
		auto elem = pList->FindIndexList(tag);
		APathLib::shellExec(GetHWND(),elem->path.c_str(),elem->param.c_str(),0);
	}


	CDuiString n = msg.pSender->GetName();
	if(n == "closebtn"){
		Close(0);
		return;
	}
	else if(n == "minbtn"){
		SendMessage(WM_SYSCOMMAND,SC_MINIMIZE);
	}
	else if(n == "maxbtn"){
		SendMessage(WM_SYSCOMMAND,SC_MAXIMIZE);
	}
	else if(n == "restorebtn"){
		SendMessage(WM_SYSCOMMAND,SC_RESTORE);
	}
}

void CMainDlgImpl::OnSelectChanged(TNotifyUI& msg)
{
	if(msg.pSender->GetUserData() == "index_list_option"){
		auto pOpt = static_cast<CMouseWheelOptionUI*>(msg.pSender);
		m_pTabPage->SelectItem(m_tm.FindList(pOpt));
	}
}

void CMainDlgImpl::OnTimer(TNotifyUI& msg)
{
	//MessageBox(GetHWND(),0,0,0);
}

void CMainDlgImpl::OnMenu(TNotifyUI& msg)
{
	if(msg.pSender->GetUserData() == "index_list_option"){
		auto pOpt = static_cast<CMouseWheelOptionUI*>(msg.pSender);
		auto pList = m_tm.FindList(pOpt);

		HMENU hMenu = ::LoadMenu(CPaintManagerUI::GetInstance(),MAKEINTRESOURCE(IDM_TABMENU)); //TODO:destroy
		HMENU hSub0 = ::GetSubMenu(hMenu,0);
		::ClientToScreen(GetHWND(),&msg.ptMouse);
		UINT id = (UINT)::TrackPopupMenu(hSub0,TPM_LEFTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD,msg.ptMouse.x,msg.ptMouse.y,0,GetHWND(),nullptr);
		if(id==0) return;
		if(id == MENU_TAB_RENAME){
			class CRenameCallback : public IInputBoxCallback
			{
			private:
				virtual bool CheckReturn(LPCTSTR str,LPCTSTR* msg) override
				{
					if(_tcschr(str,_T('\''))){
						*msg = _T("�������в��ܰ����������ַ�!");
						return false;
					}
					for(auto s=m_V->begin(),e=m_V->end(); s!=e; ++s){
						if(_tcscmp(str,s->c_str()) == 0
							&& str != m_filter)
						{
							*msg = _T("�����ֲ������������ظ�!");
							return false;
						}
					}
					return true;
				}
			public:
				void SetCats(vector<string>* V,string filter)
				{
					m_V = V;
					m_filter = filter;
				}
			private:
				vector<string>* m_V;
				string m_filter;
			};
			vector<string>	catVec;
			m_db->GetCategories(&catVec);
			CRenameCallback rcb;
			rcb.SetCats(&catVec,pOpt->GetText().GetData());
			CInputBox input(GetHWND(),
				_T("������"),
				_T("������������"),
				pOpt->GetText(),
				&rcb);
			if(rcb.GetDlgCode() == CInputBox::kCancel || rcb.GetDlgCode()== CInputBox::kClose)
				return;

			if(rcb.GetStr() == pOpt->GetText())
				return;
			
			try{
				m_db->RenameCategory(pOpt->GetText(),rcb.GetStr());
				pOpt->SetText(rcb.GetStr());
				m_tm.FindList(pOpt)->RenameIndexItemsCategory(rcb.GetStr());
			}
			catch(CExcept* e)
			{
				::MessageBox(GetHWND(),e->desc.c_str(),nullptr,MB_ICONEXCLAMATION);
			}
			return;
		}
		else if(id == MENU_TAB_CLOSETAB){
			auto pList = m_tm.FindList(pOpt);
			m_tm.Remove(pOpt);
			m_pTabList->Remove(pOpt);
			m_pTabPage->Remove(pList);
		}
		else if(id == MENU_TAB_NEWTAB){
			class CNewTabCallback : public IInputBoxCallback
			{
			private:
				virtual bool CheckReturn(LPCTSTR str,LPCTSTR* msg) override
				{
					if(_tcschr(str,_T('\''))){
						*msg = _T("��ǩ���в��ܰ����������ַ�!");
						return false;
					}
					for(auto s=m_V->begin(),e=m_V->end(); s!=e; ++s){
						if(_tcscmp(str,s->c_str()) == 0){
							*msg = _T("�����ֲ������������ظ�!");
							return false;
						}
					}
					return true;
				}
			public:
				void SetCats(vector<string>* V)
				{
					m_V = V;
				}
			private:
				vector<string>* m_V;
			};

			vector<string>	catVec;
			m_db->GetCategories(&catVec);
			CNewTabCallback cb;
			cb.SetCats(&catVec);

			CInputBox input(GetHWND(),
				_T("�±�ǩ"),
				_T("�������µı�ǩ��:"),
				_T(""),
				&cb);
			if(cb.GetDlgCode() != CInputBox::kOK)
				return;
			addTab(cb.GetStr(),GetTag(),"index_list_option");
			return;
		}
		else{

		}
		return;
	}

	if(msg.pSender->GetName() == "listItemBtn"){
		auto pList = static_cast<CIndexListUI*>(m_pTabPage->GetItemAt(m_pTabPage->GetCurSel()));
		auto elem = pList->FindIndexList(msg.pSender->GetTag());

		HMENU hMenu = ::LoadMenu(CPaintManagerUI::GetInstance(),MAKEINTRESOURCE(IDM_MENU_MAIN)); //TODO:destroy
		HMENU hSub0 = ::GetSubMenu(hMenu,0);
		::ClientToScreen(GetHWND(),&msg.ptMouse);
		UINT id = (UINT)::TrackPopupMenu(hSub0,TPM_LEFTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD,msg.ptMouse.x+1,msg.ptMouse.y+1,0,GetHWND(),nullptr);
		if(id==0) return;
		switch(id)
		{
		case IDM_INDEX_ADD:
			{
				//Ϊaddʱ,CindexItem*ָ�������
				CAddDlg dlg(GetHWND(),CAddDlg::TYPE_NEW,(CIndexItem*)elem->category.c_str(),m_db);
				if(dlg.GetDlgCode() == CAddDlg::kCancel || dlg.GetDlgCode() == CAddDlg::kClose)
					return;
				CIndexItem* pii = dlg.GetIndexItem();
				pList->AddItem(pii,true);

				return;
			}
		case IDM_INDEX_MODIFY:
			{
				CAddDlg dlg(GetHWND(),CAddDlg::TYPE_MODIFY,elem,m_db);
				if(dlg.GetDlgCode() == CAddDlg::kOK){
					if(msg.pSender->GetText() != elem->comment.c_str()){
						pList->GetTextControlFromButton((CButtonUI*)msg.pSender)->SetText(elem->comment.c_str());
					}
				}
				return;
			}
		case IDM_INDEX_REMOVE_MULTI:
			{
				CContainerUI* p = (CContainerUI*)msg.pSender->GetParent()->GetParent();
				try{
					pList->RemoveItem(p,msg.pSender->GetTag());
				}
				catch(CExcept* e)
				{
					::MessageBox(GetHWND(),e->desc.c_str(), nullptr, MB_ICONEXCLAMATION);
				}
				return;
			}
		case IDM_VIEW_DETAIL:
			{
				string detail(4096,0);
				CIndexItem& si = *elem;
				char tmp[128];
				sprintf(tmp,"%d",si.idx);
				detail = "�������: ";
				detail += tmp;
				detail += "\n��������: ";
				detail += si.idxn;
				detail += "\n��������: ";
				detail += si.category;
				detail += "\n����˵��: ";
				detail += si.comment;
				detail += "\n�ļ�·��: ";
				detail += si.path;
				detail += "\nĬ�ϲ���: ";
				detail += si.param;
				detail += "\nʹ�ô���: ";
				detail += si.times;
				detail += "\n�Ƿ�ɼ�: ";
				detail += si.visible?"�ɼ�":"���ɼ�";

				::MessageBox(GetHWND(),detail.c_str(),"",MB_OK);
				return ;
			}
		case IDM_VIEW_DIR:
			{
				CIndexItem& ii = *elem;
				APathLib::showDir(GetHWND(),ii.path.c_str());
				return;
			}
		case IDM_VIEW_COPYDIR:
		case IDM_VIEW_COPYPARAM:
			{
				AUtils::setClipData(id==IDM_VIEW_COPYDIR?elem->path.c_str():elem->param.c_str());
				return;
			}
		}//switch(id)
	}// if btn

	if(msg.pSender->GetName() == "switch"){
		auto pList = static_cast<CIndexListUI*>(m_pTabPage->GetItemAt(m_pTabPage->GetCurSel()));
		auto pOpt = m_tm.FindOption(pList);

		HMENU hMenu = ::LoadMenu(CPaintManagerUI::GetInstance(),MAKEINTRESOURCE(IDM_INDEXTAB_MENU)); //TODO:destroy
		HMENU hSub0 = ::GetSubMenu(hMenu,0);
		::ClientToScreen(GetHWND(),&msg.ptMouse);
		UINT id = (UINT)::TrackPopupMenu(hSub0,TPM_LEFTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD,msg.ptMouse.x+1,msg.ptMouse.y+1,0,GetHWND(),nullptr);
		if(id==0) return;
		switch(id)
		{
		case MENU_TABINDEX_NEWINDEX:
			{
				//Ϊaddʱ,CindexItem*ָ�������
				CAddDlg dlg(GetHWND(),CAddDlg::TYPE_NEW,(CIndexItem*)pOpt->GetText().GetData(),m_db);
				if(dlg.GetDlgCode() == CAddDlg::kCancel || dlg.GetDlgCode() == CAddDlg::kClose)
					return;
				CIndexItem* pii = dlg.GetIndexItem();
				pList->AddItem(pii,true);

				return;
			}
		}
		return;
	}
}

void CMainDlgImpl::OnScroll(TNotifyUI& msg)
{
	if(msg.pSender->GetUserData() == "index_list_option"){
		int sel = -1;
		UINT sz = m_tm.Size();
		for(UINT i=0; i<sz; ++i){
			if(m_tm.GetAt(i).option->IsSelected()){
				sel = i;
				break;
			}
		}
		if(sel == -1) return;

		if(msg.wParam == SB_LINEUP){
			sel--;
			if(sel<0)
				sel = sz-1;
		}
		else if(msg.wParam == SB_LINEDOWN){
			sel++;
			if(sel>sz-1)
				sel = 0;
		}

		m_tm.GetAt(sel).option->Selected(true);
		m_pTabPage->SelectItem(m_tm.GetAt(sel).list);
	}
}

void CMainDlgImpl::InitWindow()
{	
	struct{
		void* ptr;
		const char*  name;
	} li[] = {
		{&m_pbtnMin,		"minbtn"},
		{&m_pbtnMax,		"maxbtn"},
		{&m_pbtnRestore,	"restorebtn"},
		{&m_pbtnClose,		"closebtn"},
		{0,0}
	};
	for(int i=0;li[i].ptr; i++){
		*(CControlUI**)li[i].ptr = static_cast<CControlUI*>(m_PaintManager.FindControl(li[i].name));
	}

	m_pTabList = static_cast<CHorizontalLayoutUI*>(m_PaintManager.FindControl(_T("tabs")));
	m_pTabPage = static_cast<CTabLayoutUI*>(m_PaintManager.FindControl(_T("switch")));

	vector<string>	catVec;
	m_db->GetCategories(&catVec);

	for(auto it=catVec.begin(); it!=catVec.end(); it++){
		addTab(it->c_str(),GetTag(),"index_list_option");
	}
	
	if(m_tm.Size()){
		m_pTabPage->SelectItem(m_tm.GetAt(0).list);
		m_tm.GetAt(0).option->Selected(true);
	}
}

bool CMainDlgImpl::addTab(const char* name,int tag,const char* group)
{
	auto pOption = new CMouseWheelOptionUI;
	pOption->SetAttribute("text",name);
	pOption->SetAttribute("width","60");
	pOption->SetAttribute("textcolor","0xFF386382");
	pOption->SetAttribute("normalimage","file='tabbar_normal.png' fade='50'");
	pOption->SetAttribute("hotimage","tabbar_hover.png");
	pOption->SetAttribute("pushedimage","tabbar_pushed.png");
	pOption->SetAttribute("selectedimage","file='tabbar_pushed.png' fade='150'");
	pOption->SetAttribute("group","contenttab");
	pOption->SetAttribute("menu","true");

	pOption->SetUserData(group);
	pOption->SetTag(tag);

	auto pList = new CIndexListUI(m_db,name,m_PaintManager);
	pList->SetTag(tag);

	m_pTabList->Add(pOption);
	m_pTabPage->Add(pList);
	m_tm.Add(pOption,pList);
	return true;
}

LRESULT CMainDlgImpl::OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	BOOL bZoomed = ::IsZoomed(m_hWnd);
	LRESULT lRes = CWindowWnd::HandleMessage(uMsg, wParam, lParam);
	if (::IsZoomed(m_hWnd) != bZoomed)
	{
		if (!bZoomed)
		{
			m_pbtnMax->SetVisible(false);
			m_pbtnRestore->SetVisible(true);
		}
		else 
		{
			m_pbtnMax->SetVisible(true);
			m_pbtnRestore->SetVisible(false);
		}
	}
	return lRes;
}
