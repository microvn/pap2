////////////////////////////////////////////////////////////////////////////////
//
//  FileName    : KGSFXBingPage.cpp
//  Version     : 1.0
//  Creator     : Chiyer (Zhao XinYu)
//  Create Date : 2006-8-31 12:21:35
//  Comment     :
//              Fuck !! 今天写着写着突然停电,写了半天的cpp竟然乱码了
//
////////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "SceneEditor.h"
#include "KGSFXBingPage.h"
#include "KSceneSFXEditorFrame.h"
#include "IEKG3DSFX.h"
#include "IEKG3DModelTable.h"
#include "IEKG3DSceneEntity.h"
//#include "KG3DSceneEditorBase.h"
#include "KSceneHelperFuncs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////////////////////////////////////////////////////


//------------------------------------------------------------------------------
// Name : KGSFXBindTree
// Desc : list for sfx
//------------------------------------------------------------------------------
//IMPLEMENT_DYNCREATE(KGSFXBindTree, CTreeCtrl)
KGSFXTree::KGSFXTree()
{
}
KGSFXTree::~KGSFXTree()
{
}
void KGSFXTree::Init()
{
}
BOOL KGSFXTree::PreTranslateMessage(MSG* pMsg)
{
    return CTreeCtrl::PreTranslateMessage(pMsg);
}
BEGIN_MESSAGE_MAP(KGSFXTree, CTreeCtrl)
END_MESSAGE_MAP()

//------------------------------------------------------------------------------
// Name : KGSFXList
// Desc :
//------------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(KGSFXList, KGListCtrl)
    ON_MESSAGE(WM_EDIT_RECEIVE_ENTER, OnSetScanl)
END_MESSAGE_MAP()
LRESULT KGSFXList::OnSetScanl(WPARAM wParam, LPARAM lParam)
{
    CWnd* pWnd = GetParent();
    if (pWnd)
        pWnd->PostMessage(WM_EDIT_RECEIVE_ENTER, wParam, lParam);
    return 1;
}
//------------------------------------------------------------------------------
// Name : KGSFXBindPage
// Desc : Page for bind
//------------------------------------------------------------------------------
IMPLEMENT_DYNAMIC(KGSFXBindPage, CPropertyPage)
KGSFXBindPage::KGSFXBindPage()
    : CPropertyPage(KGSFXBindPage::IDD)
{
    m_nCurEditIndex = -1;
    m_nEditSFXScanlIndex = -1;
    m_nBindNum      =  0;
    m_strSelPath    = "";
}
KGSFXBindPage::~KGSFXBindPage()
{
}
BEGIN_MESSAGE_MAP(KGSFXBindPage, CPropertyPage)
    ON_NOTIFY(TVN_SELCHANGED, IDC_TREE, &KGSFXBindPage::OnTvnSelchangedTree)
    ON_NOTIFY(NM_DBLCLK, IDC_TREE, &KGSFXBindPage::OnNMDblclkTree)
    ON_BN_CLICKED(IDC_SFX_BIND_DEL, &KGSFXBindPage::OnBnClickedSfxBindDel)
    ON_BN_CLICKED(IDC_SFX_BIND_ADD, &KGSFXBindPage::OnBnClickedSfxBindAdd)
    ON_CBN_SELCHANGE(IDC_COMBO, &KGSFXBindPage::OnCbnSelchangeCombo)
    ON_NOTIFY(LVN_ITEMACTIVATE, IDC_LIST, &KGSFXBindPage::OnLvnItemActivateList)
    ON_MESSAGE(WM_EDIT_RECEIVE_ENTER, OnSetScanl)
    ON_WM_SIZE()
END_MESSAGE_MAP()
void KGSFXBindPage::OnSize(UINT nType, int cx, int cy)
{
    CPropertyPage::OnSize(nType, cx, cy);
    RECT rectClient;
    RECT rectContral;
    GetClientRect(&rectClient);
    rectContral = rectClient;
    rectContral.top    += 27;
    rectContral.left   += 2;
    rectContral.right  -= 2;
    rectContral.bottom = rectClient.bottom  - 5;
    if (::IsWindow(m_list.m_hWnd))
        m_list.MoveWindow(&rectContral);
    //RECT rectCleint;
    //RECT rectTree;
    //GetClientRect(&rectCleint);
    //if (::IsWindow(m_tree.m_hWnd))
    //{
    //    m_tree.GetWindowRect(&rectTree);
    //    ScreenToClient(&rectTree);
    //    rectTree.bottom = rectCleint.bottom - 5;
    //    m_tree.MoveWindow(&rectTree);
    //}
}
LRESULT KGSFXBindPage::OnSetScanl(WPARAM wParam, LPARAM lParam)
{
    HRESULT hRetCode = E_FAIL;
    POSITION pos =  m_list.GetFirstSelectedItemPosition();
    int nSel = m_list.GetNextSelectedItem(pos);
    LPKGLISTITEM pFindItem  = NULL;
    DWORD_PTR dwPtr;
    IEKG3DModel* pModel  = NULL;
    CString strScanl = TEXT("");
    float fScanl = 1.0f;
    KG_PROCESS_ERROR(nSel != -1);
    pFindItem = (LPKGLISTITEM)m_list.GetItemData(nSel);
    pFindItem->GetStructData(&dwPtr, sizeof(DWORD_PTR));
    pModel = (IEKG3DModel*)dwPtr;
    m_editScanl.GetWindowText(strScanl);
    fScanl = (float)atof(strScanl.GetBuffer());
    KG_PROCESS_ERROR(fScanl > 0.0f);
    DWORD dwType = 0;
    hRetCode = pModel->GetType(&dwType);
    KGLOG_COM_PROCESS_ERROR(hRetCode);
    if (dwType == MESHTYPE_SFX)
        dynamic_cast<IEKG3DSFX*>(pModel)->SetScaleSize(fScanl);
    pFindItem->arryItemText[1].szItemText.Format(TEXT("%f"), fScanl);
    m_list.Update(pFindItem);
    m_editScanl.ShowWindow(SW_HIDE);
    //UpdateSFXScanl();
Exit0:
    return 1;
}
void KGSFXBindPage::UpdateSFXScanl()
{
    LPKGLISTITEM pTitleItem = NULL;
    LPKGLISTITEM pFindItem  = NULL;
   /* TCHAR szFileName[MAX_PATH];
    LPSTR pExt = NULL;
    TCHAR szInfo[128];
    KG_PROCESS_ERROR(m_nEditSFXScanlIndex != -1);
    KG_PROCESS_ERROR(m_nEditSFXScanlIndex < m_nBindNum);
    sprintf(szFileName, m_strSelPath.GetBuffer());
    pExt = strrchr(szFileName, '.');
    KG_PROCESS_ERROR(m_nCurEditIndex != -1);
    KG_PROCESS_ERROR(pExt);
    KG_PROCESS_ERROR(!strcmpi(pExt, TEXT(".sfx")));
    sprintf(szInfo, TEXT("绑定-%d"), m_nEditSFXScanlIndex);
    m_list.FindItemByText(szInfo);
    pTitleItem = m_list.FindNextItem();
    KG_PROCESS_ERROR(pTitleItem);
    GET_SFX_EDITOR();
    {
        KG3DSFXSuede* pSFXSuede = pScene->GetSFXSuede();
        KG3DSFX* pSFX = (KG3DSFX*)pSFXSuede->GetSFX(m_nEditSFXScanlIndex);
        CString strScanl = TEXT("");
        float fScanl = 1.0f;
        KG_PROCESS_ERROR(pSFXSuede);
        KG_PROCESS_ERROR(pSFX);
        sprintf(szInfo, TEXT("放缩"));
        m_list.FindItemByText(szInfo, pTitleItem);
        pFindItem = m_list.FindNextItem();
        KG_PROCESS_ERROR(pFindItem);
        m_editScanl.GetWindowText(strScanl);
        fScanl = (float)atof(strScanl.GetBuffer());
        KG_PROCESS_ERROR(fScanl > 0.0f);
        pSFX->SetScaleSize(fScanl);
        pFindItem->arryItemText[1].szItemText.Format(TEXT("%f"), pSFX->GetScaleSize());
        m_list.Update(pFindItem);
        m_editScanl.ShowWindow(SW_HIDE);
    }*/
//Exit0:
    return;
}
void KGSFXBindPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //DDX_Control(pDX, IDC_SFX_BIND_ADD, m_buttonAdd);
    //DDX_Control(pDX, IDC_SFX_BIND_DEL, m_buttonDel);
    DDX_Control(pDX, IDC_TREE, m_tree);
    DDX_Control(pDX, IDC_LIST, m_list);
    DDX_Control(pDX, IDC_COMBO, m_combo);
    DDX_Control(pDX, IDC_EDIT_SCANL, m_editScanl);
}
BOOL KGSFXBindPage::OnInitDialog()
{
    int nResult  = CPropertyPage::OnInitDialog();
    m_tree.SetTextColor(RGB(255, 255, 255));
    m_tree.SetLineColor(RGB(122, 122, 122));
    m_tree.SetBkColor(RGB(92,  92, 92));
    m_imageListTree.Create(16, 16, ILC_COLOR24, 4, 4);
    m_imageListTree.Add(AfxGetApp()->LoadIcon(IDC_FC));
    m_imageListTree.Add(AfxGetApp()->LoadIcon(IDC_GLEAN));
    m_imageListTree.Add(AfxGetApp()->LoadIcon(IDC_TALK));
    m_imageListTree.Add(AfxGetApp()->LoadIcon(IDC_OFFER));
    m_imageListTree.Add(AfxGetApp()->LoadIcon(IDC_POS));
    m_tree.SetImageList(&m_imageListTree, TVSIL_NORMAL);
    m_list.InsertColumn(0, "Suede");
    m_list.InsertColumn(1, "绑定信息");
    m_list.SetColumnWidth(0, 80);
    m_list.SetColumnWidth(1, 400);
    m_list.SetBkColor(RGB(92, 92, 92));
    m_list.SetupToolTip(200, RGB(68, 68, 68), RGB(255, 255, 255));
    m_list.SetHoverTime(100);
    TCHAR path[MAX_PATH];
    sprintf(path, "%s%s", g_szDefWorkDirectory, SFX_FILE_PATH);
    FillTree(NULL, path);
    RECT rectClient;
    RECT rectContral;
    GetClientRect(&rectClient);
    rectContral = rectClient;
    rectContral.top    += 27;
    rectContral.left   += 2;
    rectContral.right  -= 2;
    rectContral.bottom = rectClient.bottom  - 5;
    m_list.MoveWindow(&rectContral);
    rectContral.top    = rectContral.bottom + 2;
    rectContral.bottom = rectClient.bottom  - 2;
    m_tree.MoveWindow(&rectContral);
    m_editScanl.SetParent(&m_list);
    m_editScanl.ShowWindow(SW_HIDE);
    // new a
    return nResult;
}
int KGSFXBindPage::FillTree(HTREEITEM hTreeItem, LPCTSTR szPath)
{
    int nResult  = false;
    int nRetCode = false;
    TCHAR szFilePath[MAX_PATH];
    sprintf(szFilePath, "%s%s", szPath, TEXT("\\*.*"));
    CFileFind fileFind;
    BOOL bWorking = fileFind.FindFile(szFilePath);
    while (bWorking)
    {
        bWorking = fileFind.FindNextFile();
        if (fileFind.IsDots())
            continue;
        CString strPath = fileFind.GetFilePath();
        CString strName = fileFind.GetFileName();
        if (fileFind.IsDirectory())
        {
            FillTree(m_tree.InsertItem(strName.GetBuffer(), hTreeItem), strPath.GetBuffer());
        }
        else
        {
            TCHAR  szName[MAX_PATH];
            strncpy(szName, strName.GetBuffer(), sizeof(szName));
            TCHAR* pszExt = strrchr(szName, '.');
            if (!pszExt)
                continue;
            if (!stricmp(pszExt, TEXT(".sfx")))
                m_tree.InsertItem(szName, 3, 3, hTreeItem);
            else
                continue;
        }
    }
    fileFind.Close();
    nResult = true;
//Exit0:
    return nResult;
}
int KGSFXBindPage::UpdateBondInfo(LPSTR pszBoneName)
{
    int nResult  = false;
    int nRetCode = false;
    LPKGLISTITEM pTitleItem = NULL;
    LPKGLISTITEM pFindItem  = NULL;
    TCHAR szFileName[MAX_PATH];
    LPSTR pExt = NULL;
    TCHAR szInfo[128];
    ASSERT(pszBoneName);
    KG_PROCESS_ERROR(pszBoneName);
    sprintf(szFileName, m_strSelPath.GetBuffer());
    pExt = strrchr(szFileName, '.');
    KG_PROCESS_ERROR(m_nCurEditIndex != -1);
    KG_PROCESS_ERROR(pExt);
    KG_PROCESS_ERROR(!strcmpi(pExt, TEXT(".sfx")));
    sprintf(szInfo, TEXT("绑定-%d"), m_nCurEditIndex);
    m_list.FindItemByText(szInfo);
    pTitleItem = m_list.FindNextItem();
    KG_PROCESS_ERROR(pTitleItem);
    sprintf(szInfo, TEXT("骨骼"));
    m_list.FindItemByText(szInfo, pTitleItem);
    pFindItem = m_list.FindNextItem();
    KG_PROCESS_ERROR(pFindItem);
    KG_PROCESS_ERROR(pFindItem->arryItemText.GetCount() == 2);
    pFindItem->arryItemText[1].szItemText = pszBoneName;
    m_list.Update(pFindItem);
    nResult = true;
Exit0:
    return true;
}
void KGSFXBindPage::UpdateSFXSuedeList()
{
    HRESULT hRetCode = E_FAIL;
    SCENEENTITYTYPE EntityType = SCENEENTITY_NONE;
    GET_SFX_EDITOR();
    IEKG3DSceneEntity *piEntity = NULL;
    IEKG3DModel*   pSelModel = NULL;
    DWORD dwType = 0;
    IEKG3DModel *pChildModel = NULL;
    IEKG3DSceneEditorBase *piSceneEditorBase = NULL;
    pScene->GetSceneEditBase(&piSceneEditorBase);
    m_combo.ResetContent();
    m_list.RemoveAll();
    //KG_PROCESS_ERROR(piSceneEditorBase->m_listSelectedEntity.size());

	GETSELECT(piSceneEditorBase);

    if (pScene->IsModifyMode())
    {
        pScene->GetRefrenceModel(0, &pSelModel);
    }
    else
    {
	   /* KG_PROCESS_ERROR(pSelector->GetSelectedEntitySize() > 0);
        piEntity = NULL;

	    pSelector->GetSelectedEntity(0, &piEntity);
        ASSERT(piEntity);*/

		IEKG3DSceneSelectionToolAccessProxy* pProxy = KSF::GetSelectionProxy(piSceneEditorBase);
		KG_PROCESS_ERROR(NULL != pProxy);

		piEntity = pProxy->Reset();
		KG_PROCESS_ERROR(NULL != piEntity);

        hRetCode = piEntity->GetType(&EntityType);
        KGLOG_COM_PROCESS_ERROR(hRetCode);
        KG_PROCESS_ERROR(EntityType == SCENEENTITY_MODEL);
        hRetCode = piEntity->GetPointer((VOID **)&pSelModel);
        KGLOG_COM_PROCESS_ERROR(hRetCode);
    }


    KG_PROCESS_ERROR(pSelModel);
    //vecChilds = pSelModel->GetAllChilds();
    hRetCode = pSelModel->GetFirstChildModel(&pChildModel);
    KGLOG_COM_PROCESS_ERROR(hRetCode);
    while (pChildModel != NULL)
    {
        IEKG3DModel* pModel     = pChildModel;
        CString strModelName  = TEXT("");
        CString strTagetName  = TEXT("");
        CString strSFXScanl   = TEXT("");
        LPCTSTR pszName = NULL;
        float   fSFXScanl  = 1.0f;
        string  strName;
        DWORD   dwBindType = 0;
        int     index = 0;
        enuModelBindType enuType = ENUM_BIND_NONE;
        pModel->IEGetBindType(&enuType);
        dwBindType = enuType;
        hRetCode = pModel->GetName(&pszName);
        KGLOG_COM_PROCESS_ERROR(hRetCode);
        strModelName = pszName;
        if (dwBindType == ENUM_BIND_BONE || dwBindType == ENUM_BIND_SOCKET)
        {
            LPCTSTR pszName = NULL;
            pModel->IEGetBindBoneName(&pszName);
            strTagetName = pszName;
        }
        else
        {
            strTagetName = TEXT("原点");
        }
        hRetCode = pModel->GetType(&dwType);
        KGLOG_COM_PROCESS_ERROR(hRetCode);
        if (dwType == MESHTYPE_SFX)
            fSFXScanl = dynamic_cast<IEKG3DSFX*>(pModel)->GetScaleSize();
        strSFXScanl.Format(TEXT("%f"), fSFXScanl);
        LPKGLISTITEM pTitleItem = NULL;
        LPKGLISTITEM pListItem  = NULL;
        TCHAR szInfo[128];
        sprintf(szInfo, TEXT("绑定-%d"), index++);
        m_combo.AddString(szInfo);
        pTitleItem = new KGLISTITEM;
        ITEMTEXT   itemText;
        itemText.colorBackSel  = RGB(0, 255, 255);
        itemText.szItemText = szInfo;
        pTitleItem->arryItemText.Add(itemText);
        m_list.InsertDepend(NULL, pTitleItem);
        pListItem  = new KGLISTITEM;
        itemText.szItemText = TEXT("特效");
        pListItem->arryItemText.Add(itemText);
        itemText.szItemText = strModelName;
        pListItem->arryItemText.Add(itemText);
        m_list.InsertDepend(pTitleItem, pListItem);
        pListItem  = new KGLISTITEM;
        itemText.szItemText = TEXT("骨骼");
        pListItem->arryItemText.Add(itemText);
        itemText.szItemText = strTagetName;
        pListItem->arryItemText.Add(itemText);
        m_list.InsertDepend(pTitleItem, pListItem);
        /*
        pListItem  = new KGLISTITEM;
        itemText.szItemText = TEXT("放缩");
        pListItem->arryItemText.Add(itemText);
        itemText.szItemText = strSFXScanl;
        pListItem->arryItemText.Add(itemText);
        DWORD_PTR dwPtr = (DWORD_PTR)pModel;
        pListItem->SetStructData(&dwPtr);
        m_list.InsertDepend(pTitleItem, pListItem);
        */
        /*int index = 0;
        if (*i && (*i)->GetType() == MESHTYPE_SFX)
        {
            KG3DSFX* pSFX = (KG3DSFX*)*i;
            TCHAR szRetPath[MAX_PATH];
            float  fSFXScanl   = 1.0f;
            string strName;
            strSFXName = pSFX->m_scName.c_str();
            if (pSFX->GetBindType() == ENUM_BIND_BONE)
            {
                pSFX->GetBindBoneName(strName);
                strBoneName = strName.c_str();
            }
            fSFXScanl = pSFX->GetScaleSize();
            strSFXScanl.Format(TEXT("%f"), fSFXScanl);
            LPKGLISTITEM pTitleItem = NULL;
            LPKGLISTITEM pListItem  = NULL;
            TCHAR szInfo[128];
            sprintf(szInfo, TEXT("绑定-%d"), index++);
            m_combo.AddString(szInfo);
            pTitleItem = new KGLISTITEM;
            ITEMTEXT   itemText;
            itemText.colorBackSel  = RGB(0, 255, 255);
            itemText.szItemText = szInfo;
            pTitleItem->arryItemText.Add(itemText);
            m_list.InsertDepend(NULL, pTitleItem);
            pListItem  = new KGLISTITEM;
            itemText.szItemText = TEXT("特效");
            pListItem->arryItemText.Add(itemText);
            itemText.szItemText = strSFXName;
            pListItem->arryItemText.Add(itemText);
            m_list.InsertDepend(pTitleItem, pListItem);
            pListItem  = new KGLISTITEM;
            itemText.szItemText = TEXT("骨骼");
            pListItem->arryItemText.Add(itemText);
            itemText.szItemText = strBoneName;
            pListItem->arryItemText.Add(itemText);
            m_list.InsertDepend(pTitleItem, pListItem);
            pListItem  = new KGLISTITEM;
            itemText.szItemText = TEXT("放缩");
            pListItem->arryItemText.Add(itemText);
            itemText.szItemText = strSFXScanl;
            pListItem->arryItemText.Add(itemText);
            DWORD_PTR dwPtr = (DWORD_PTR)pSFX;
            pListItem->SetStructData(&dwPtr);
            m_list.InsertDepend(pTitleItem, pListItem);
        }*/
        hRetCode = pSelModel->GetNextChildModel(&pChildModel);
        KGLOG_COM_PROCESS_ERROR(hRetCode);
    }

    //KG3DSFXSuede* pSFXSuede = pScene->GetSFXSuede();
    //DWORD i = 0;
    //KG_PROCESS_ERROR(pSFXSuede);
    //m_combo.ResetContent();
    //m_list.RemoveAll();
    //for (i = 0; i < pSFXSuede->GetSFXCount(); i++)
    //{
    //    CString strSFXName  = TEXT("");
    //    CString strBoneName = TEXT("");
    //    CString strSFXScanl = TEXT("");
    //    KG3DSFX* pSFX = dynamic_cast<KG3DSFX*>(pSFXSuede->GetSFX(i));
    //    if (pSFX)
    //    {
    //        TCHAR szRetPath[MAX_PATH];
    //        float  fSFXScanl   = 1.0f;
    //        string strName;
    //        //g_GetFilePathFromFullPath(szRetPath, pSFX->m_scName.c_str());
    //        strSFXName = pSFX->m_scName.c_str();
    //        if (pSFX->GetBindType() == ENUM_BIND_BONE)
    //        {
    //            pSFX->GetBindBoneName(strName);
    //            strBoneName = strName.c_str();
    //        }
    //        fSFXScanl = pSFX->GetScaleSize();
    //        strSFXScanl.Format(TEXT("%f"), fSFXScanl);
    //    }
    //    LPKGLISTITEM pTitleItem = NULL;
    //    LPKGLISTITEM pListItem  = NULL;
    //    TCHAR szInfo[128];
    //    sprintf(szInfo, TEXT("绑定-%d"), i);
    //    m_combo.AddString(szInfo);
    //    pTitleItem = new KGLISTITEM;
    //    ITEMTEXT   itemText;
    //    itemText.colorBackSel  = RGB(0, 255, 255);
    //    itemText.szItemText = szInfo;
    //    pTitleItem->arryItemText.Add(itemText);
    //    m_list.InsertDepend(NULL, pTitleItem);
    //    pListItem  = new KGLISTITEM;
    //    itemText.szItemText = TEXT("特效");
    //    pListItem->arryItemText.Add(itemText);
    //    itemText.szItemText = strSFXName;
    //    pListItem->arryItemText.Add(itemText);
    //    m_list.InsertDepend(pTitleItem, pListItem);
    //    pListItem  = new KGLISTITEM;
    //    itemText.szItemText = TEXT("骨骼");
    //    pListItem->arryItemText.Add(itemText);
    //    itemText.szItemText = strBoneName;
    //    pListItem->arryItemText.Add(itemText);
    //    m_list.InsertDepend(pTitleItem, pListItem);
    //    pListItem  = new KGLISTITEM;
    //    itemText.szItemText = TEXT("放缩");
    //    pListItem->arryItemText.Add(itemText);
    //    itemText.szItemText = strSFXScanl;
    //    pListItem->arryItemText.Add(itemText);
    //    m_list.InsertDepend(pTitleItem, pListItem);
    //}
    //m_nBindNum = i;
    //m_nCurEditIndex = -1;
    //m_combo.SetCurSel(0);
    //OnCbnSelchangeCombo();
Exit0:
    return;
}
void KGSFXBindPage::UpdateModelPage()
{
    GET_SFX_MDLVIEW_PAGE();
    pMDLViewPage->UpdateInterface();
}
BOOL KGSFXBindPage::OnSetActive()
{
    //UpdateSFXSceneMode(1);
    UpdateModelPage();
    UpdateSFXSuedeList();
    return CPropertyPage::OnSetActive();
}
BOOL KGSFXBindPage::OnKillActive()
{
    UpdateSFXSceneMode(0);
    UpdateModelPage();
    return CPropertyPage::OnKillActive();
}
void KGSFXBindPage::UpdateSceneSFXIndex()
{
    GET_SFX_EDITOR();
    pScene->UpdateSFXBindIndex(m_nCurEditIndex);
}
void KGSFXBindPage::UpdateSFXSceneMode(DWORD dwMode)
{
    GET_SFX_EDITOR();
    pScene->UpdateSceneMode(dwMode);
    KSceneSFXEditorView* pView =
        dynamic_cast<KSceneSFXEditorView*>(pFrame->GetActiveView());
    if (pView)
        pView->UpdateUI();
}
void KGSFXBindPage::UpdateSFXSuedeSFX(int nFlage)
{
    //TCHAR szFullPath[MAX_PATH];
    //KG3DSFXSuede* pSFXSuede = NULL;
    //size_t sizeSuede = 0;
    //ASSERT(nFlage == 0 || nFlage == 1);
    //KG_PROCESS_ERROR(m_nCurEditIndex < m_nBindNum);
    //KG_PROCESS_ERROR(m_nCurEditIndex != -1);
    //KG_PROCESS_ERROR(m_nBindNum);
    //GET_SFX_EDITOR();
    //
    //pSFXSuede = pScene->GetSFXSuede();
    //KG_PROCESS_ERROR(pSFXSuede);
    //if (nFlage)
    //{
    //    g_GetFullPath(szFullPath, m_strSelPath.GetBuffer());
    //    pScene->UpdateSFXSuedeSFX(szFullPath);
    //}
    //else
    //{
    //    pSFXSuede->AddSFX(NULL);
    //}
//Exit0:
    return;
}
CString KGSFXBindPage::GetSelSFXName()
{
    return m_strSelPath;
}
void KGSFXBindPage::OnTvnSelchangedTree(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
    HTREEITEM hTreeItem = pNMTreeView->itemNew.hItem;
    CString strFilePath = TEXT("");
    KG_PROCESS_ERROR(hTreeItem);
    while (hTreeItem)
    {
        strFilePath = TEXT("\\") + m_tree.GetItemText(hTreeItem)+ strFilePath;
        hTreeItem   = m_tree.GetParentItem(hTreeItem);
    }
    m_strSelPath = SFX_FILE_PATH + strFilePath;
Exit0:
    *pResult = 0;
}
void KGSFXBindPage::OnNMDblclkTree(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPKGLISTITEM pTitleItem = NULL;
    LPKGLISTITEM pFindItem  = NULL;
   /* TCHAR szFileName[MAX_PATH];
    LPSTR pExt = NULL;
    TCHAR szInfo[128];
    sprintf(szFileName, m_strSelPath.GetBuffer());
    pExt = strrchr(szFileName, '.');
    KG_PROCESS_ERROR(m_nCurEditIndex != -1);
    KG_PROCESS_ERROR(pExt);
    KG_PROCESS_ERROR(!strcmpi(pExt, TEXT(".sfx")));
    sprintf(szInfo, TEXT("绑定-%d"), m_nCurEditIndex);
    m_list.FindItemByText(szInfo);
    pTitleItem = m_list.FindNextItem();
    KG_PROCESS_ERROR(pTitleItem);
    sprintf(szInfo, TEXT("特效"));
    m_list.FindItemByText(szInfo, pTitleItem);
    pFindItem = m_list.FindNextItem();
    KG_PROCESS_ERROR(pFindItem);
    KG_PROCESS_ERROR(pFindItem->arryItemText.GetCount() == 2);
    pFindItem->arryItemText[1].szItemText = m_strSelPath;
    m_list.Update(pFindItem);
    UpdateSFXSuedeSFX(1);
    GET_SFX_EDITOR();
    {
        KG3DSFXSuede* pSFXSuede = pScene->GetSFXSuede();
        KG3DSFX* pSFX = (KG3DSFX*)pSFXSuede->GetSFX(m_nCurEditIndex);
        KG_PROCESS_ERROR(pSFXSuede);
        KG_PROCESS_ERROR(pSFX);
        sprintf(szInfo, TEXT("放缩"));
        m_list.FindItemByText(szInfo, pTitleItem);
        pFindItem = m_list.FindNextItem();
        KG_PROCESS_ERROR(pFindItem);
        KG_PROCESS_ERROR(pFindItem->arryItemText.GetCount() == 2);
        pFindItem->arryItemText[1].szItemText.Format(TEXT("%f"), pSFX->GetScaleSize());
        m_list.Update(pFindItem);
    }*/
//Exit0:
    *pResult = 0;
}
void KGSFXBindPage::OnBnClickedSfxBindDel()
{
   /* GET_SFX_EDITOR();
    KG3DSFXSuede* pSFXSuede = NULL;
    KG3DSFX*      pCufSFX   = NULL;
    KG_PROCESS_ERROR(m_nCurEditIndex < m_nBindNum);
    KG_PROCESS_ERROR(m_nCurEditIndex != -1);
    pSFXSuede = pScene->GetSFXSuede();
    KG_PROCESS_ERROR(pSFXSuede);
    pCufSFX = dynamic_cast<KG3DSFX*>(pSFXSuede->GetSFX(m_nCurEditIndex));
    {
        KG3DSceneEntity entity(SCENEENTITY_MODEL, pCufSFX);
        pScene->m_listPostRenderEntity.remove(entity);
        pScene->m_listRenderEntity.remove(entity);
        pScene->m_listSelectedEntity.remove(entity);
        pScene->m_listSelectableEntity.remove(entity);
    }
    pSFXSuede->DelSFX(m_nCurEditIndex);
    UpdateSFXSuedeList();*/
//Exit0:
    return;
}
void KGSFXBindPage::OnBnClickedSfxBindAdd()
{
    LPKGLISTITEM pTitleItem = NULL;
    LPKGLISTITEM pListItem  = NULL;
    TCHAR szInfo[128];
    m_nBindNum++;
    int nCurEditIndex = m_nBindNum - 1;
    sprintf(szInfo, TEXT("绑定-%d"), nCurEditIndex);
    m_combo.AddString(szInfo);
    pTitleItem = new KGLISTITEM;
    ITEMTEXT   itemText;
    itemText.colorBackSel  = RGB(0, 255, 255);
    itemText.szItemText = szInfo;
    pTitleItem->arryItemText.Add(itemText);
    m_list.InsertDepend(NULL, pTitleItem);
    pListItem  = new KGLISTITEM;
    itemText.szItemText = TEXT("特效");
    pListItem->arryItemText.Add(itemText);
    itemText.szItemText = TEXT("");
    pListItem->arryItemText.Add(itemText);
    m_list.InsertDepend(pTitleItem, pListItem);
    pListItem  = new KGLISTITEM;
    itemText.szItemText = TEXT("骨骼");
    pListItem->arryItemText.Add(itemText);
    itemText.szItemText = TEXT("");
    pListItem->arryItemText.Add(itemText);
    m_list.InsertDepend(pTitleItem, pListItem);
    /*
    pListItem  = new KGLISTITEM;
    itemText.szItemText = TEXT("放缩");
    pListItem->arryItemText.Add(itemText);
    itemText.szItemText = TEXT("");
    pListItem->arryItemText.Add(itemText);
    m_list.InsertDepend(pTitleItem, pListItem);
    */
    m_combo.SetCurSel(nCurEditIndex);
    OnCbnSelchangeCombo();
    UpdateSFXSuedeSFX(0);
}
void KGSFXBindPage::OnCbnSelchangeCombo()
{
    LPKGLISTITEM pTitleItem = NULL;
    LPKGLISTITEM pFindItem  = NULL;
    TCHAR szInfo[128];
    int nSelIndex = m_combo.GetCurSel();
    KG_PROCESS_ERROR(nSelIndex != m_nCurEditIndex);
    if (m_nCurEditIndex == -1)
        m_nCurEditIndex = nSelIndex;
    sprintf(szInfo, TEXT("绑定-%d"), m_nCurEditIndex);
    m_list.FindItemByText(szInfo);
    pTitleItem = m_list.FindNextItem();
    KG_PROCESS_ERROR(pTitleItem);
    sprintf(szInfo, TEXT("特效"));
    m_list.FindItemByText(szInfo, pTitleItem);
    pFindItem = m_list.FindNextItem();
    KG_PROCESS_ERROR(pFindItem);
    KG_PROCESS_ERROR(pFindItem->arryItemText.GetCount() == 2);
    pFindItem->arryItemText[0].colorBack = RGB(255, 255, 255);
    pFindItem->arryItemText[1].colorBack = RGB(255, 255, 255);
    m_list.Update(pFindItem);
    sprintf(szInfo, TEXT("骨骼"));
    m_list.FindItemByText(szInfo, pTitleItem);
    pFindItem = m_list.FindNextItem();
    KG_PROCESS_ERROR(pFindItem);
    KG_PROCESS_ERROR(pFindItem->arryItemText.GetCount() == 2);
    pFindItem->arryItemText[0].colorBack = RGB(255, 255, 255);
    pFindItem->arryItemText[1].colorBack = RGB(255, 255, 255);
    m_list.Update(pFindItem);
    sprintf(szInfo, TEXT("放缩"));
    m_list.FindItemByText(szInfo, pTitleItem);
    pFindItem = m_list.FindNextItem();
    KG_PROCESS_ERROR(pFindItem);
    KG_PROCESS_ERROR(pFindItem->arryItemText.GetCount() == 2);
    pFindItem->arryItemText[0].colorBack = RGB(255, 255, 255);
    pFindItem->arryItemText[1].colorBack = RGB(255, 255, 255);
    m_list.Update(pFindItem);
    m_nCurEditIndex = nSelIndex;
    sprintf(szInfo, TEXT("绑定-%d"), m_nCurEditIndex);
    m_list.FindItemByText(szInfo);
    pTitleItem = m_list.FindNextItem();
    KG_PROCESS_ERROR(pTitleItem);
    sprintf(szInfo, TEXT("特效"));
    m_list.FindItemByText(szInfo, pTitleItem);
    pFindItem = m_list.FindNextItem();
    KG_PROCESS_ERROR(pFindItem);
    KG_PROCESS_ERROR(pFindItem->arryItemText.GetCount() == 2);
    pFindItem->arryItemText[0].colorBack = RGB(240, 200, 85);
    pFindItem->arryItemText[1].colorBack = RGB(240, 200, 85);
    m_list.Update(pFindItem);
    sprintf(szInfo, TEXT("骨骼"));
    m_list.FindItemByText(szInfo, pTitleItem);
    pFindItem = m_list.FindNextItem();
    KG_PROCESS_ERROR(pFindItem);
    KG_PROCESS_ERROR(pFindItem->arryItemText.GetCount() == 2);
    pFindItem->arryItemText[0].colorBack = RGB(240, 200, 85);
    pFindItem->arryItemText[1].colorBack = RGB(240, 200, 85);
    m_list.Update(pFindItem);
    sprintf(szInfo, TEXT("放缩"));
    m_list.FindItemByText(szInfo, pTitleItem);
    pFindItem = m_list.FindNextItem();
    KG_PROCESS_ERROR(pFindItem);
    KG_PROCESS_ERROR(pFindItem->arryItemText.GetCount() == 2);
    pFindItem->arryItemText[0].colorBack = RGB(240, 200, 85);
    pFindItem->arryItemText[1].colorBack = RGB(240, 200, 85);
    m_list.Update(pFindItem);
    UpdateSceneSFXIndex();
Exit0:
    return;
}
void KGSFXBindPage::OnLvnItemActivateList(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMIA = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    int nSelRow = pNMIA->iItem;
    int nSelCol = pNMIA->iSubItem;
    CString strSelText = TEXT("");
    CRect rectEdit;
    LPKGLISTITEM pListItem = NULL;
    LPKGLISTITEM pDependItem = NULL;
    int index = 0;
    pListItem = reinterpret_cast<LPKGLISTITEM>(m_list.GetItemData(nSelRow));
    KG_PROCESS_ERROR(pListItem);
    m_editScanl.ShowWindow(SW_HIDE);
    KG_PROCESS_ERROR(nSelCol == 1);
    m_list.GetSubItemRect(nSelRow, nSelCol, LVIR_BOUNDS, rectEdit);
    strSelText = m_list.GetItemText(nSelRow, 0);
    KG_PROCESS_ERROR(strSelText == TEXT("放缩"));
    m_editScanl.MoveWindow(&rectEdit);
    m_editScanl.ShowWindow(SW_SHOW);
    pDependItem = pListItem->pPerantItem;
    KG_PROCESS_ERROR(pDependItem);
    pListItem = pDependItem->pNexSiblingItem;
    while (pListItem)
    {
        index++;
        pListItem = pListItem->pNexSiblingItem;
    }
    m_nEditSFXScanlIndex = m_nBindNum - index - 1;
Exit0:
    *pResult = 0;
}
void KGSFXBindPage::OnOK()
{
}
