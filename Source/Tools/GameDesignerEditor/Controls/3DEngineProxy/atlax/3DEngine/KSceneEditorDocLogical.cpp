#include "stdafx.h"
#include "IEEditor.h"
#include "KSceneEditorDocLogical.h"
#include "KGLog.h"
#include "KSO3World.h"
#include "AtlBase.h"
#include "ieother.h"

#include "KGSTLWrappers.h"
#include "KG3DRepresentNPC.h" // fuck


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


extern TCHAR g_szDefWorkDirectory[MAX_PATH];
// ahpho suntao
extern IEKG3DEngineManager* g_pEngineManager;
IEKG3DSceneSceneEditor* g_pSceneEditor = NULL;
// end ahpho


//[原来在SceneEditorDoc.h,cpp
BOOL KLogicalSceneConnection::CheckObstacle(int nXCell, int nYCell, int nLayer)
{
	if(m_lpLogicalScene)
		return m_lpLogicalScene->CheckObstacle(nXCell,nYCell,nLayer);
	else
		return FALSE;
}
KRegion* KLogicalSceneConnection::GetRegion(int nRegionX, int nRegionY)
{
	if(m_lpLogicalScene)
		return m_lpLogicalScene->GetRegion(nRegionX,nRegionY);
	else
		return NULL;
}
HRESULT KLogicalSceneConnection::CreateLogicalRegionInfo()
{
	SAFE_DELETE_ARRAY(m_lpLogicalRegionInfo);

	int nX = m_lpLogicalScene->GetRegionWidth();
	int nY = m_lpLogicalScene->GetRegionHeight();

	m_lpLogicalRegionInfo = new _KG3DLogicalRegionInfo[nX * nY];
	KGLOG_PROCESS_ERROR(m_lpLogicalRegionInfo);

	ZeroMemory(m_lpLogicalRegionInfo,sizeof(_KG3DLogicalRegionInfo) * nX * nY);

	return S_OK;
Exit0:
	return E_FAIL;
}
//]


float GetAngle(float x,float y)
{
    D3DXVECTOR2 A(x,y);
    float Angle = 0;
    D3DXVec2Normalize(&A,&A);
    if(A.x>=0)
    {
        Angle = -asinf(A.y);
    }
    else
    {
        Angle = asinf(A.y) - D3DX_PI ;
    }
    return Angle;
}

KSceneEditorDocLogical::WayPointSet::~WayPointSet()
{
	DeleteAllWayPoint();
}


HRESULT KSceneEditorDocLogical::WayPointSet::AddWayPoint(WayPoint** ppNewWayPoint,D3DXVECTOR3 vPos,D3DXQUATERNION vRot,PVOID pPointer)
{
	WayPoint* pNewPoint = new WayPoint;
	KGLOG_PROCESS_ERROR(pNewPoint);
	(*ppNewWayPoint) = pNewPoint;

	pNewPoint->vPosition = vPos;
	pNewPoint->vRotation = vRot;

	pNewPoint->pPointer = pPointer;
	pNewPoint->bStayAniLoop = TRUE;
	pNewPoint->bIsRun = FALSE;
	pNewPoint->nStayAnimation = 0;
	pNewPoint->nStayDirection = 0;
	pNewPoint->nStayFrame = 0;

	vectorWayPoint.push_back(pNewPoint);

	return S_OK;
Exit0:
	return E_FAIL;
}

HRESULT KSceneEditorDocLogical::WayPointSet::InsertWayPoint(WayPoint** ppNewWayPoint,int nIndex,D3DXVECTOR3 vPos)
{
	vector<WayPoint*>::iterator t = vectorWayPoint.begin();
	WayPoint* pNewPoint = new WayPoint;
	KGLOG_PROCESS_ERROR(pNewPoint);
	(*ppNewWayPoint) = pNewPoint;

	pNewPoint->vPosition = vPos;

	for(int i=0;i<nIndex;i++)//愚昧的方式
	{
		t++;
	}
	vectorWayPoint.insert(t,pNewPoint);
	
	return S_OK;
Exit0:
	return E_FAIL;
}

HRESULT KSceneEditorDocLogical::WayPointSet::DeleteWayPoint(WayPoint* pWayPoint)
{
	vector<WayPoint*>::iterator i = find(vectorWayPoint.begin(),vectorWayPoint.end(),pWayPoint);
	if(i!=vectorWayPoint.end())
	{
		vectorWayPoint.erase(i);
	}
	SAFE_DELETE(pWayPoint);

	return S_OK;
}

HRESULT KSceneEditorDocLogical::WayPointSet::DeleteAllWayPoint()
{
	for(size_t i=0;i<vectorWayPoint.size();i++)
	{
		WayPoint* pWayPoint = vectorWayPoint[i];
		SAFE_DELETE(pWayPoint);
	}
	vectorWayPoint.clear();
	return S_OK;
}

HRESULT KSceneEditorDocLogical::WayPointSet::FindWayPointByEditorWayPoint(WayPoint** ppWayPoint,PVOID pPointer)
{
	for(size_t i=0;i<vectorWayPoint.size();i++)
	{
		WayPoint* pWayPoint = vectorWayPoint[i];
		if (pWayPoint->pPointer == pPointer)
		{
			(*ppWayPoint) = pWayPoint;
			return S_OK;
		}
	}
	(*ppWayPoint) = NULL;
	return E_FAIL;
}

KSceneEditorDocLogical::TrafficPointSet::~TrafficPointSet()
{
	DeleteAllTrafficPoint();
}

HRESULT KSceneEditorDocLogical::TrafficPointSet::DeleteAllTrafficPoint()
{
	for(size_t i=0;i<vecTrafficPoint.size();i++)
	{
		TrafficLittlePoint* pTrafficPoint = vecTrafficPoint[i];
		SAFE_DELETE(pTrafficPoint);
	}
	vecTrafficPoint.clear();
	return S_OK;
}
HRESULT KSceneEditorDocLogical::TrafficPointSet::AddTrafficPoint(TrafficLittlePoint** ppNewTrafficPoint,D3DXVECTOR3 vPos,PVOID pPointer)															  
{
	TrafficLittlePoint* pNewPoint = new TrafficLittlePoint;
	KGLOG_PROCESS_ERROR(pNewPoint);
	(*ppNewTrafficPoint) = pNewPoint;
	pNewPoint->pPointer = pPointer;
	pNewPoint->vPosition = vPos;
	vecTrafficPoint.push_back(pNewPoint);
	return S_OK;
Exit0:
	return E_FAIL;
}
KSceneEditorDocLogical::TrafficPointSet::TrafficPointSet()
{
//	m_vecCurPos = D3DXVECTOR3(0,0,0); //At First No Point In
	m_dwStep = 1000;
	m_dwCostMoney = 1;
	m_dwVelocity = 48;
	szName = "";
}


HRESULT KSceneEditorDocLogical::TrafficPointSet::InsertTrafficPoint(TrafficLittlePoint** ppNewWayPoint,int nIndex,
																	D3DXVECTOR3 vPos,PVOID pPointer)
{
	vector<TrafficLittlePoint*>::iterator t = vecTrafficPoint.begin();
	TrafficLittlePoint* pNewPoint = new TrafficLittlePoint;
	KGLOG_PROCESS_ERROR(pNewPoint);
	(*ppNewWayPoint) = pNewPoint;

	pNewPoint->vPosition = vPos;
	pNewPoint->pPointer = pPointer;
	for(int i=0;i<nIndex;i++)//愚昧的方式
	{
		t++;
	}
	vecTrafficPoint.insert(t,pNewPoint);

	return S_OK;
Exit0:
	return E_FAIL;
}

HRESULT KSceneEditorDocLogical::TrafficPointSet::DeleteTrafficPoint(TrafficLittlePoint* pWayPoint)
{
	vector<TrafficLittlePoint*>::iterator i = find(vecTrafficPoint.begin(),vecTrafficPoint.end(),pWayPoint);
	if(i!=vecTrafficPoint.end())
	{
		vecTrafficPoint.erase(i);
	}
	SAFE_DELETE(pWayPoint);

	return S_OK;
}

HRESULT KSceneEditorDocLogical::TrafficPointSet::FindTrafficPointByEditorWayPoint(TrafficLittlePoint** ppWayPoint,int* pID,PVOID pPointer)
{
	if (ppWayPoint == NULL) 
		return E_FAIL;
	if (pID == NULL)
		return E_FAIL;
	for(size_t i=0;i<vecTrafficPoint.size();i++)
	{
		TrafficLittlePoint* pWayPoint = vecTrafficPoint[i];
		if (pWayPoint->pPointer == pPointer)
		{
			(*ppWayPoint) = pWayPoint;
			*pID = (int)i;
			return S_OK;
		}
	}
	(*ppWayPoint) = NULL;
	return E_FAIL;
}


KSceneEditorDocLogical::KSceneEditorDocLogical(void)
{
	m_dwCurrentWayPointSetID = 0;
	m_dwCurrentTrafficPointSetID = 0;
	//m_dwCurrentNPCRefreshPointSetID = 0;
	//m_dwCurrentDoodadRefreshPointSetID = 0;
	m_dwCurrentAIGroupID = 0;

	m_fLogicalXStart = 0;
	m_fLogicalZStart = 0;

	m_nSceneID = 0;

	m_lpSceneSceneEditor = NULL;

	m_dwCurrentNpcReviveGroupID = 1;
	m_dwCurrentDoodadReviveGroupID = 1;
	m_dwCurrentRandomGroupID = 1;
	m_dwCurrentAIParameterSetID = 1;
	
	// ahpho suntao
	m_npcHatredGroupIndex = 1;
	// end ahpho

	GetDefaultAIParameterDefine();

	GetNpcAIDefaultValue();
}

KSceneEditorDocLogical::~KSceneEditorDocLogical(void)
{
	DeleteAllNPC();
	DeleteAllWayPointSet();
	DeleteAllDoodad();
	DeleteAllLogicalPoly();
	DeleteAllAIGroup();
	DeleteAllTrafficPointSet();

	m_AIParameterDefine.vecParameters.clear();
	m_mapNpcAISceneParameterSet.clear();
}

HRESULT KSceneEditorDocLogical::FindNPCInfoByEditorNPC(NPCInfo** ppNPC,PVOID pPointer)
{
	list<NPCInfo*>::iterator i = m_listNPCInfo.begin();
	while (i!=m_listNPCInfo.end())
	{
		NPCInfo* pNPC = *i;
		if (pNPC->pPointer == pPointer)
		{
			(*ppNPC) = pNPC;
			return S_OK;
		}
		i++;
	}
	(*ppNPC) = NULL;
	return E_FAIL;
}

HRESULT KSceneEditorDocLogical::FindDoodadInfoByEditorDoodad(DoodadInfo** ppDoodad,PVOID pPointer)
{
	list<DoodadInfo*>::iterator i = m_listDoodadInfo.begin();
	while (i!=m_listDoodadInfo.end())
	{
		DoodadInfo* pDoodad = *i;
		if (pDoodad->pPointer == pPointer)
		{
			(*ppDoodad) = pDoodad;
			return S_OK;
		}
		i++;
	}
	(*ppDoodad) = NULL;
	return E_FAIL;
}

HRESULT KSceneEditorDocLogical::AddOneNPC(NPCInfo** ppNewNPC,LPCTSTR pName,D3DXVECTOR3& vPos,D3DXQUATERNION& vRot,int nDirection,int nTempleteID,PVOID pPointer,
										  int nLevel,int nKind,/*int nAIType,*/ DWORD dwRepresentID ,float fAlertRange,float fAttackRange, /*int nDisableAutoTurn,*/
										  DWORD dwReliveID,DWORD dwRandomID,DWORD dwAISetID)
{
	NPCInfo* pNewNPC = new NPCInfo;
	KGLOG_PROCESS_ERROR(pNewNPC);
	(*ppNewNPC) = pNewNPC;

	pNewNPC->szName = pName;
	pNewNPC->vPosition = vPos;
	pNewNPC->vRotation = vRot;
	pNewNPC->nTempleteID = nTempleteID;
	pNewNPC->pPointer = pPointer;
	pNewNPC->pWayPointSet = NULL;

	pNewNPC->fAlertRange = fAlertRange;
	pNewNPC->fAttackRange = fAttackRange;

	pNewNPC->nX = (int)(vPos.x / 100.0f * 256);
	pNewNPC->nY = (int)(vPos.z / 100.0f * 256);
	pNewNPC->nZ = 0;
	pNewNPC->dwScriptID = 0;
	//pNewNPC->nAIType = nAIType;
	pNewNPC->nLevel = nLevel;
	pNewNPC->nKind = nKind;
	pNewNPC->nFaceDirection = nDirection;
	pNewNPC->dwNpcReliveID = 0;
	pNewNPC->dwNpcRandomID = 0;

	pNewNPC->dwNpcTeamID = 0;
	pNewNPC->dwNpcTeamIndex = 0;
	pNewNPC->nPathIndex = 0;
    pNewNPC->dwRepresentID = dwRepresentID;
    //pNewNPC->nDisableAutoTurn = nDisableAutoTurn;

	pNewNPC->dwNpcReliveID = dwReliveID;
	pNewNPC->dwNpcRandomID = dwRandomID;
	pNewNPC->dwNpcAISet    = dwAISetID;
	
	// 给npc分配一个index区别开来，如果是读取地图时创建的话，在之后会被文件中获得的index覆盖掉
	pNewNPC->nIndex = m_npcIndex;
	m_npcIndex++;

	m_listNPCInfo.push_back(pNewNPC);
	
	return S_OK;
Exit0:
	return E_FAIL;
}


HRESULT KSceneEditorDocLogical::DeleteNPC(NPCInfo* pNewNPC)
{
	//RemoveNPCFromNpcRefreshSet(pNewNPC);

	RemoveNPCFromAIGroup(pNewNPC);

	m_listNPCInfo.remove(pNewNPC);
	SAFE_DELETE(pNewNPC);

	return S_OK;
}

HRESULT KSceneEditorDocLogical::DeleteAllNPC()
{
	list<NPCInfo*>::iterator i = m_listNPCInfo.begin();
	while (i!=m_listNPCInfo.end())
	{
		NPCInfo* pNPC = *i;
		SAFE_DELETE(pNPC);

		i++;
	}
	m_listNPCInfo.clear();

	return S_OK;
}

HRESULT KSceneEditorDocLogical::AddOneDoodad(DoodadInfo** ppNewDoodad,LPCTSTR pName,D3DXVECTOR3& vPos,D3DXQUATERNION& vRot,
											 int nDirection,int nTempleteID,PVOID pPointer,DWORD dwReliveID)
{
	DoodadInfo* pNewDoodad = new DoodadInfo;
	KGLOG_PROCESS_ERROR(pNewDoodad);
	(*ppNewDoodad) = pNewDoodad;

	pNewDoodad->szName = pName;
	pNewDoodad->vPosition = vPos;
	pNewDoodad->vRotation = vRot;
	pNewDoodad->nTempleteID = nTempleteID;
	pNewDoodad->pPointer = pPointer;
	pNewDoodad->dwDoodadReliveID = 0;

	pNewDoodad->nX = (int)(vPos.x / 100.0f * 256);
	pNewDoodad->nY = (int)(vPos.z / 100.0f * 256);
	pNewDoodad->nZ = 0;
	pNewDoodad->dwScriptID = 0;
    //pNewDoodad->nAIType = 0;
	pNewDoodad->nLevel = 0;
	pNewDoodad->nKind = 0;
	pNewDoodad->nFaceDirection = nDirection;
	pNewDoodad->dwDoodadReliveID = dwReliveID;

	m_listDoodadInfo.push_back(pNewDoodad);

	return S_OK;
Exit0:
	return E_FAIL;
}

HRESULT KSceneEditorDocLogical::DeleteDoodad(DoodadInfo* pNewDoodad)
{
	//RemoveDoodadFromDoodadRefreshSet(pNewDoodad);

	m_listDoodadInfo.remove(pNewDoodad);
	SAFE_DELETE(pNewDoodad);

	return S_OK;
}

HRESULT KSceneEditorDocLogical::DeleteAllDoodad()
{
	list<DoodadInfo*>::iterator i = m_listDoodadInfo.begin();
	while (i!=m_listDoodadInfo.end())
	{
		DoodadInfo* pDoodad = *i;
		SAFE_DELETE(pDoodad);

		i++;
	}
	m_listDoodadInfo.clear();

	return S_OK;
}

HRESULT KSceneEditorDocLogical::GetWayPointSetByID(WayPointSet** ppSet,DWORD dwID)
{
	list<WayPointSet*>::iterator i = m_listWayPointSet.begin();
	while (i!=m_listWayPointSet.end())
	{
		WayPointSet* pSet = *i;
		if(pSet->dwSetID==dwID)
		{
			(*ppSet) = pSet;
			return S_OK;
		}

		i++;
	}
	(*ppSet) = NULL;
	return E_FAIL;
}

// ahpho suntao
// 获取npc重生组的信息
HRESULT KSceneEditorDocLogical::GetNpcReviveGroupInfo(LONG dwID, int* nMin, int* nMax, int* nRandom, BSTR* npcString)
{	
	map<DWORD, ReviveGroup>::iterator it = m_mapNpcReviveGroup.find(dwID);
	if (it != m_mapNpcReviveGroup.end())
	{
		ReviveGroup& reviveGroup = it->second;
		*nMin = reviveGroup.nMinNpcCount;
		*nMax = reviveGroup.nMaxNpcCount;
		*nRandom = reviveGroup.nRandom;

		string info = "";
		for (vector<NPCInfo*>::iterator iter = reviveGroup.vecNpcInfo.begin(); iter != reviveGroup.vecNpcInfo.end(); iter++)
		{
			NPCInfo* npcInfo = *iter;
			TCHAR charArray[MAX_PATH];
			wsprintf(charArray, "%s,%d,%d;", npcInfo->szName.c_str(), npcInfo->nTempleteID, npcInfo->nIndex);
			string tempString(charArray);
			info = info + tempString;
		}

		*npcString = STRING_TO_BSTR(info.c_str());
	}

	return S_OK;
}

// 获取doodad重生组的信息
HRESULT KSceneEditorDocLogical::GetDoodadReviveGroupInfo(LONG dwID, int* nMin, int* nMax, int* nRandom, BSTR* doodadString)
{	
	map<DWORD, ReviveGroup>::iterator it = m_mapDoodadReviveGroup.find(dwID);
	if (it != m_mapDoodadReviveGroup.end())
	{
		ReviveGroup& reviveGroup = it->second;
		*nMin = reviveGroup.nMinNpcCount;
		*nMax = reviveGroup.nMaxNpcCount;
		*nRandom = reviveGroup.nRandom;

		string info = "";
		for (vector<NPCInfo*>::iterator iter = reviveGroup.vecNpcInfo.begin(); iter != reviveGroup.vecNpcInfo.end(); iter++)
		{
			NPCInfo* npcInfo = *iter;
			TCHAR charArray[MAX_PATH];
			wsprintf(charArray, "%s,%d,%d;", npcInfo->szName.c_str(), npcInfo->nTempleteID, npcInfo->nIndex);
			string tempString(charArray);
			info = info + tempString;
		}

		*doodadString = STRING_TO_BSTR(info.c_str());
	}

	return S_OK;
}

// 删除仇恨组
HRESULT KSceneEditorDocLogical::DeleteHatredGroup(INT groupID)
{
	map<INT, string>::iterator iFind = m_npcHatredGroupMap.find(groupID);
	if (iFind != m_npcHatredGroupMap.end())
	{		
		m_npcHatredGroupMap.erase(iFind);
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

// 删除AI组
HRESULT KSceneEditorDocLogical::DecoratedDeleteAIGroup(INT groupID)
{
	AIGroup* aiGroup = NULL;
	if (SUCCEEDED(GetAIGroupByID(&aiGroup, groupID)))
	{
		DeleteAIGroup(aiGroup);
	}

	return S_OK;
}

// 重设AISet
HRESULT KSceneEditorDocLogical::ResetAISet(DWORD dwID, AIParameterSet &newAISet)
{
	AIParameterSet NewSet;	

	NewSet.dwID = dwID;
	NewSet.szName = "AUTO_CONVERT_AI_SET";
	NewSet.vecParameters = m_AIParameterDefine.vecParameters;

	m_mapNpcAISceneParameterSet[dwID] = NewSet;
	newAISet = NewSet;

	return S_OK;
}

// 通过顺序编号获取路径点集合
HRESULT KSceneEditorDocLogical::GetWayPointSetByIndex(WayPointSet** ppSet, int index)
{
	int i = 0;
	list<WayPointSet*>::iterator it = m_listWayPointSet.begin();
	while ( it != m_listWayPointSet.end())
	{
		if (i == index)
		{
			WayPointSet* pSet = *it;			
			(*ppSet) = pSet;

			return S_OK;			
		}		

		i++;
		it++;
	}

	(*ppSet) = NULL;
	return E_FAIL;
}

HRESULT KSceneEditorDocLogical::DecoratedAddOneWayPointSet(LONG* ppNewSet, BSTR pName)
{    	
	return AddOneWayPointSet((WayPointSet**)ppNewSet, (LPCTSTR)BSTR_TO_STRING(pName));	
}

HRESULT KSceneEditorDocLogical::DecoratedDeleteWayPointSet(LONG pSet)
{
	return DeleteWayPointSet((WayPointSet*)pSet);
}

HRESULT KSceneEditorDocLogical::AddOneWayPoint(LONG wayPointSetID, BSTR pName)
{		
	return S_OK;
}

HRESULT KSceneEditorDocLogical::RefreshWayPoint(LONG wayPointPtr, INT nType)
{		
	KG3DRepresentWayPoint* representObj = (KG3DRepresentWayPoint*)wayPointPtr;
	IEKG3DRepresentWayPoint* representWayPoint = dynamic_cast<IEKG3DRepresentWayPoint*>(representObj);
	representWayPoint->SetWayPointType(nType);

	return S_OK;
}

HRESULT KSceneEditorDocLogical::AddOneBigTrafficPoint()
{
	return S_OK;
}

HRESULT KSceneEditorDocLogical::DecoratedAddOneTrafficPointSet(int trafficID, INT bAddPoint)
{
	return S_OK;
}

HRESULT KSceneEditorDocLogical::DeleteTrafficPointAndSet(LONG pSelectedObject, INT bDeletePoint)
{
	if(bDeletePoint) // 删除交通点
	{
		TrafficLittlePoint* pPoint = (TrafficLittlePoint*)pSelectedObject;	
		m_lpSceneSceneEditor->ClearSelectedEntityList();
		m_lpSceneSceneEditor->SetSelected(SCENEENTITY_OBJECT, pPoint->pPointer, TRUE);	
		m_lpSceneSceneEditor->DeleteSelectedEntity();
	}
	else // 删除交通路径
	{
		TrafficPointSet* pSet = (TrafficPointSet*)pSelectedObject;		
		m_lpSceneSceneEditor->ClearSelectedEntityList();
		for(size_t s=0;s<pSet->vecTrafficPoint.size();s++)
		{
			TrafficLittlePoint* pPoint = pSet->vecTrafficPoint[s];
			m_lpSceneSceneEditor->SetSelected(SCENEENTITY_OBJECT, pPoint->pPointer, TRUE);
		}
		m_lpSceneSceneEditor->DeleteSelectedEntity();
		g_pSO3World->m_RoadManager.DelTrack(pSet->m_dwID);
		DeleteTrafficPointSet(pSet);
	}

	return S_OK;
}

HRESULT KSceneEditorDocLogical::InsertTrafficPoint()
{
	CString  szText;
	D3DXVECTOR3 vPos1,vPos2;
	SCENEENTITYTYPE EntityType = SCENEENTITY_NONE;
	IEKG3DSceneEntity* pEntityFrom = NULL;
	HRESULT hRetCode = E_FAIL;
	KSceneEditorDocLogical::TrafficLittlePoint* pPoint = NULL;
	IEKG3DRepresentTrafficPoint* pRepFromTrafficPoint = NULL;
	KSceneEditorDocLogical::TrafficPointSet* pSet = NULL;
	IEKG3DRepresentObject* pObject = NULL;
	DWORD dwType = 0;
	int nTempID = 0;
	DWORD dwSetID = 0; 

	//m_lpSceneSceneEditor->GetSelectedEntity(0, &pEntityFrom);
	IEKG3DSceneSelectionTool* pSelectionTool = NULL;
	m_lpSceneSceneEditor->GetSelectionToolInterface(&pSelectionTool);

	IEKG3DSceneSelectionToolAccessProxy* pAccessProxy = NULL;
	pSelectionTool->GetAccessProxy(&pAccessProxy);
	_ASSERTE(NULL != pAccessProxy);

	pEntityFrom = pAccessProxy->Reset();
	_ASSERTE(NULL != pEntityFrom);

	hRetCode = pEntityFrom->GetType(&EntityType);
	KGLOG_COM_PROCESS_ERROR(hRetCode);
	if(EntityType != SCENEENTITY_OBJECT)
	{
		MessageBox(NULL, "交通点选择错误！非物件。", "请先选择一个将在之前插入的小交通点！", MB_OK);
		return S_OK;
	}	
	hRetCode = pEntityFrom->GetPointer((VOID **)&pObject);
	KGLOG_COM_PROCESS_ERROR(hRetCode);
	if(pObject->GetType() != REPRESENTOBJECT_TRAFFICPOINT)
	{
		MessageBox(NULL, "非交通点！", "请先选择一个将在之前插入的小交通点！", MB_OK);
		return S_OK;
	}
	pRepFromTrafficPoint = m_lpSceneSceneEditor->GetIEKG3DRepresentTrafficPoint(pObject);
	dwType = pRepFromTrafficPoint->GetKind();
	if (dwType == 0)
	{
		MessageBox(NULL, "非小交通点！", "请先选择一个将在之前插入的小交通点！", MB_OK);
		return S_OK;
	}

	pRepFromTrafficPoint->GetTranslation(&vPos1);		
	dwSetID = pRepFromTrafficPoint->GetSetID();
	GetTrafficPointSetByID(&pSet, dwSetID);
	KG_PROCESS_ERROR(pSet);
	pSet->FindTrafficPointByEditorWayPoint(&pPoint, &nTempID, pObject);
	if (pPoint != NULL)
	{
		if (nTempID == 0)
		{
			vPos2 = m_mapTrafficPoint[pSet->m_dwFromID].vPosition;
		}
		else
		{
			vPos2 = (pSet->vecTrafficPoint[nTempID - 1])->vPosition;
		}
		IEKG3DRepresentObject* pNewRepPoint = NULL;
		TrafficLittlePoint* pNewPoint = NULL;
		D3DXVECTOR3 vMidPos = (vPos1 + vPos2) / 2;
		m_lpSceneSceneEditor->AddTrafficPoint(&pNewRepPoint, vMidPos, 1, dwSetID);
		pSet->InsertTrafficPoint(&pNewPoint, nTempID, vMidPos, pNewRepPoint);
	}
Exit0:
	return S_OK;
}

//HRESULT KSceneEditorDocLogical::SelectLogicObject(LONG pSelectedObject, BSTR objectType)
//{	
//	if (_strcmpi(BSTR_TO_STRING(objectType), "WayPoint") == 0)
//	{
//		KG3DRepresentWayPoint* representObj = (KG3DRepresentWayPoint*)pSelectedObject;
//		m_lpSceneSceneEditor->ClearSelectedEntityList();
//		m_lpSceneSceneEditor->SetSelected(SCENEENTITY_OBJECT, representObj, TRUE);
//	}
//	else if(_strcmpi(BSTR_TO_STRING(objectType), "TrafficLittlePoint") == 0)
//	{
//		KG3DRepresentTrafficPoint* representObj = (KG3DRepresentTrafficPoint*)pSelectedObject;		
//		m_lpSceneSceneEditor->ClearSelectedEntityList();
//		m_lpSceneSceneEditor->SetSelected(SCENEENTITY_OBJECT, representObj, TRUE);
//	}		
//
//	return S_OK;
//}

HRESULT KSceneEditorDocLogical::RefreshTraffic()
{
	BOOL bUseSO3Logical = FALSE;
	g_pEngineManager->GetUseSO3Logical(&bUseSO3Logical);
	m_lpSceneSceneEditor->ClearSetHelpLine(1);
	
	TCHAR name[MAX_PATH];
	int K = 0;
	list<TrafficPointSet*>::iterator i = m_listTrafficPointSet.begin();
	DWORD dwNum = 0;
	while (i != m_listTrafficPointSet.end())
	{
		TrafficPointSet* pSet = *i;
		wsprintf(name,"%d: %s %d", K, pSet->szName.c_str(), pSet->vecTrafficPoint.size());

		dwNum = m_lpSceneSceneEditor->GetHelpLineSize();
		D3DXVECTOR3 vPos = m_mapTrafficPoint[pSet->m_dwFromID].vPosition + D3DXVECTOR3(0,50,0);
		m_lpSceneSceneEditor->SetHelpLine(dwNum,1, 0, 1, 0xFFFFFFFF,&vPos);
		size_t s = 0;
		for(s = 0; s < pSet->vecTrafficPoint.size(); s++)
		{
			TrafficLittlePoint* pPoint = pSet->vecTrafficPoint[s];
			wsprintf(name, "TrafficPoint%d", s);			
			vPos = pPoint->vPosition + D3DXVECTOR3(0,50,0);
			m_lpSceneSceneEditor->SetHelpLine(dwNum, 1, s + 1, 1, 0xFFFFFFFF, &vPos);
		}
		vPos = m_mapTrafficPoint[pSet->m_dwToID].vPosition + D3DXVECTOR3(0,50,0);
		m_lpSceneSceneEditor->SetHelpLine(dwNum, 1, s + 1, 1, 0xFFFFFFFF, &vPos);
		m_lpSceneSceneEditor->SetHelpLineStart(dwNum,0);
		i++;
		K++;
	}

	return S_OK;
}

HRESULT KSceneEditorDocLogical::RefreshWay()
{
	m_lpSceneSceneEditor->ClearSetHelpLine(0);

	TCHAR name[MAX_PATH];
	DWORD dwNum = 0;
	int K = 0;
	list<WayPointSet*>::iterator i = m_listWayPointSet.begin();
	while (i != m_listWayPointSet.end())
	{
		WayPointSet* pSet = *i;
		wsprintf(name,"%d: %s %d",K,pSet->szName.c_str(),pSet->vectorWayPoint.size());		
		dwNum = m_lpSceneSceneEditor->GetHelpLineSize();
		for(size_t s=0;s<pSet->vectorWayPoint.size();s++)
		{
			WayPoint* pPoint = pSet->vectorWayPoint[s];
			wsprintf(name,"WayPoint %s",pPoint->szInfo.c_str());						

			D3DXVECTOR3 vPos = pPoint->vPosition + D3DXVECTOR3(0,50,0);
			m_lpSceneSceneEditor->SetHelpLine(dwNum,0 ,s, 1, 0xFFFFFFFF, &vPos);
		}

		i++;
		//K++;
	}

	return S_OK;
}

HRESULT KSceneEditorDocLogical::DeleteSelectedObject()
{
	int selectedObjectCount = 0;
	//g_pSceneEditor->GetSelectedEntityCount(&selectedObjectCount);

	IEKG3DSceneSelectionTool* pSelectionTool = NULL;
	m_lpSceneSceneEditor->GetSelectionToolInterface(&pSelectionTool);

	IEKG3DSceneSelectionToolAccessProxy* pAccessProxy = NULL;
	pSelectionTool->GetAccessProxy(&pAccessProxy);
	_ASSERTE(NULL != pAccessProxy);

	IEKG3DSceneEntity* pEntity = pAccessProxy->Reset();

	while (pEntity)
	{
		bool deleteSuccess = false;

		SCENEENTITYTYPE EntityType = SCENEENTITY_NONE;
		pEntity->GetType(&EntityType);

		if (EntityType == SCENEENTITY_OBJECT)
		{
			IEKG3DRepresentObject* pObject = NULL;
			pEntity->GetPointer((VOID**)&pObject);

			if (pObject->GetType() == REPRESENTOBJECT_NPC)
			{
				NPCInfo* pNPCinfo = NULL;
				if (SUCCEEDED(FindNPCInfoByEditorNPC(&pNPCinfo, pObject)))
				{
					if (SUCCEEDED(DeleteNPC(pNPCinfo)))
					{
						deleteSuccess = true;
					}
				}
			}
			else if (pObject->GetType() == REPRESENTOBJECT_DOODAD)
			{
				DoodadInfo* pDoodadInfo = NULL;
				if (SUCCEEDED(FindDoodadInfoByEditorDoodad(&pDoodadInfo, pObject)))
				{
					if (SUCCEEDED(DeleteDoodad(pDoodadInfo)))
					{
						deleteSuccess = true;
					}					
				}
			}
			else if (pObject->GetType() == REPRESENTOBJECT_WAYPOINT)
			{
				IEKG3DRepresentWayPoint* pRepWayPoint = m_lpSceneSceneEditor->GetIEKG3DRepresentWayPoint(pObject);

				DWORD dwSetID = 0;
				dwSetID = pRepWayPoint->GetSetID();
				WayPointSet* pSet = NULL;

				if (SUCCEEDED(GetWayPointSetByID(&pSet, dwSetID)))
				{
					WayPoint* pWayPoint = NULL;
					if (SUCCEEDED(pSet->FindWayPointByEditorWayPoint(&pWayPoint, pObject)))
					{
						if (SUCCEEDED(pSet->DeleteWayPoint(pWayPoint)))
						{
							deleteSuccess = true;
						}
					}
				}
			}
			else if (pObject->GetType() == REPRESENTOBJECT_POLY)
			{
				IEKG3DRepresentPoly* pIEPoly = m_lpSceneSceneEditor->GetSelectedPoly();												
				m_listLogicalPoly.remove(pIEPoly);

				deleteSuccess = true;
			}
			else if (pObject->GetType() == REPRESENTOBJECT_TRAFFICPOINT)
			{
				IEKG3DRepresentTrafficPoint* pRepTrafficPoint = m_lpSceneSceneEditor->GetIEKG3DRepresentTrafficPoint(pObject);

				DWORD dwSetID = 0;
				dwSetID = pRepTrafficPoint->GetSetID();
				TrafficPointSet* pSet = NULL;

				if (SUCCEEDED(GetTrafficPointSetByID(&pSet, dwSetID)))
				{
					TrafficLittlePoint* pTrafficPoint = NULL;
					int nTempID = 0;

					if (SUCCEEDED(pSet->FindTrafficPointByEditorWayPoint(&pTrafficPoint, &nTempID, pObject)))
					{
						if (SUCCEEDED(pSet->DeleteTrafficPoint(pTrafficPoint)))
						{
							deleteSuccess = true;						
						}
					}
				}
			}

			if (deleteSuccess)
			{
				m_lpSceneSceneEditor->RemoveRepresentObject(pObject);
			}
		}

		pEntity = pAccessProxy->Next();
	}

	m_lpSceneSceneEditor->ClearSelectedEntityList();

	return S_OK;
}
// end ahpho

HRESULT KSceneEditorDocLogical::AddOneWayPointSet(WayPointSet** ppNewSet,LPCTSTR pName)
{
	WayPointSet* pSet = new WayPointSet;
	KGLOG_PROCESS_ERROR(pSet);
	(*ppNewSet) = pSet;

	pSet->szName = pName;
	pSet->dwSetID = m_dwCurrentWayPointSetID++;
	pSet->nIndex = 0;
	pSet->nOrderID = 1;
	m_listWayPointSet.push_back(pSet);

	FireEvent2(EVENTDOCLOGIGAL_UPDATEUI,0,0,0);

	return S_OK;
Exit0:
	return E_FAIL;
}

HRESULT KSceneEditorDocLogical::DeleteWayPointSet(WayPointSet* pSet)
{
	list<AIGroup*>::iterator i = m_listAIGroup.begin();
	while (i!=m_listAIGroup.end())
	{
		AIGroup* pGroup = *i;
		if(pGroup->pWayPointSet == pSet)
		{
			pGroup->pWayPointSet = NULL;
		}
		i++;
	}

	list<NPCInfo*>::iterator j= m_listNPCInfo.begin();
	while (j!=m_listNPCInfo.end())
	{
		NPCInfo* pNpc = *j;
		if(pNpc->pWayPointSet == pSet)
		{
			pNpc->pWayPointSet = NULL;
		}
		j++;
	}
	
	// 清掉路径点的表现
	m_lpSceneSceneEditor->ClearSelectedEntityList();

	vector<WayPoint*>::iterator k = pSet->vectorWayPoint.begin();
	while (k != pSet->vectorWayPoint.end())
	{
		WayPoint* pWayPoint = *k;
		IEKG3DRepresentObject* pObject = (IEKG3DRepresentObject*)pWayPoint->pPointer;
		g_pSceneEditor->RemoveRepresentObject(pObject);
		k++;
	}

	m_listWayPointSet.remove(pSet);
	SAFE_DELETE(pSet);

	FireEvent2(EVENTDOCLOGIGAL_UPDATEUI,0,0,0);

	return S_OK;
}

HRESULT KSceneEditorDocLogical::DeleteAllWayPointSet()
{
	list<WayPointSet*>::iterator i = m_listWayPointSet.begin();
	while (i!=m_listWayPointSet.end())
	{
		WayPointSet* pSet = *i;
		SAFE_DELETE(pSet);

		i++;
	}
	m_listWayPointSet.clear();
	return S_OK;
}

/*HRESULT KSceneEditorDocLogical::AddNPCOneRefreshPointSet(NPCRefreshPointSet** ppNewSet,LPCTSTR pName)
{
	NPCRefreshPointSet* pSet = new NPCRefreshPointSet;
	KGLOG_PROCESS_ERROR(pSet);
	(*ppNewSet) = pSet;

	pSet->szName = pName;
	pSet->dwSetID = m_dwCurrentNPCRefreshPointSetID++;

	m_listNPCRefreshPointSet.push_back(pSet);

	return S_OK;
Exit0:
	return E_FAIL;
}

HRESULT KSceneEditorDocLogical::DeleteNPCRefreshPointSet(NPCRefreshPointSet* pSet)
{
	m_listNPCRefreshPointSet.remove(pSet);
	SAFE_DELETE(pSet);

	return S_OK;
}

HRESULT KSceneEditorDocLogical::DeleteAllNPCRefreshPointSet()
{
	list<NPCRefreshPointSet*>::iterator i = m_listNPCRefreshPointSet.begin();
	while (i!=m_listNPCRefreshPointSet.end())
	{
		NPCRefreshPointSet* pSet = *i;
		SAFE_DELETE(pSet);

		i++;
	}
	m_listNPCRefreshPointSet.clear();
	return S_OK;
}

HRESULT KSceneEditorDocLogical::GetNPCRefreshPointSetByID(NPCRefreshPointSet** ppSet,DWORD dwID)
{
	list<NPCRefreshPointSet*>::iterator i = m_listNPCRefreshPointSet.begin();
	while (i!=m_listNPCRefreshPointSet.end())
	{
		NPCRefreshPointSet* pSet = *i;
		if(pSet->dwSetID==dwID)
		{
			(*ppSet) = pSet;
			return S_OK;
		}

		i++;
	}
	(*ppSet) = NULL;
	return E_FAIL;
}

HRESULT KSceneEditorDocLogical::AddDoodadOneRefreshPointSet(DoodadRefreshPointSet** ppNewSet,LPCTSTR pName)
{
	DoodadRefreshPointSet* pSet = new DoodadRefreshPointSet;
	KGLOG_PROCESS_ERROR(pSet);
	(*ppNewSet) = pSet;

	pSet->szName = pName;
	pSet->dwSetID = m_dwCurrentDoodadRefreshPointSetID++;

	m_listDoodadRefreshPointSet.push_back(pSet);

	return S_OK;
Exit0:
	return E_FAIL;
}

HRESULT KSceneEditorDocLogical::DeleteDoodadRefreshPointSet(DoodadRefreshPointSet* pSet)
{
	m_listDoodadRefreshPointSet.remove(pSet);
	SAFE_DELETE(pSet);

	return S_OK;
}

HRESULT KSceneEditorDocLogical::DeleteAllDoodadRefreshPointSet()
{
	list<DoodadRefreshPointSet*>::iterator i = m_listDoodadRefreshPointSet.begin();
	while (i!=m_listDoodadRefreshPointSet.end())
	{
		DoodadRefreshPointSet* pSet = *i;
		SAFE_DELETE(pSet);

		i++;
	}
	m_listDoodadRefreshPointSet.clear();
	return S_OK;
}

HRESULT KSceneEditorDocLogical::GetDoodadRefreshPointSetByID(DoodadRefreshPointSet** ppSet,DWORD dwID)
{
	list<DoodadRefreshPointSet*>::iterator i = m_listDoodadRefreshPointSet.begin();
	while (i!=m_listDoodadRefreshPointSet.end())
	{
		DoodadRefreshPointSet* pSet = *i;
		if(pSet->dwSetID==dwID)
		{
			(*ppSet) = pSet;
			return S_OK;
		}

		i++;
	}
	(*ppSet) = NULL;
	return E_FAIL;
}
*/
HRESULT KSceneEditorDocLogical::RefreshPointlist::FindRefreshPointByPointer(RefreshPoint** ppNewPoint,PVOID pPointer)
{
	list<RefreshPoint*>::iterator i = listRefreshPoint.begin();
	while (i!=listRefreshPoint.end())
	{
		RefreshPoint* pSet = *i;
		if(pSet->pPointer == pPointer)
		{
			(*ppNewPoint) = pSet;
			return S_OK;
		}
		i++;
	}
	return E_FAIL;
}

HRESULT KSceneEditorDocLogical::RefreshPointlist::NewOneRefreshPoint(RefreshPoint** ppNewPoint,PVOID pPointer,D3DXVECTOR3 vPos,D3DXQUATERNION vRot)
{
	RefreshPoint* pPoint = new RefreshPoint;
	KGLOG_PROCESS_ERROR(pPoint);
	(*ppNewPoint) = pPoint;

	pPoint->pPointer = pPointer;
	pPoint->nX = 0;
	pPoint->nY = 0;
	pPoint->nZ = 0;
	pPoint->nDirection = 0;
	pPoint->vPosition = vPos;
	pPoint->vRotation = vRot;
	//pPoint->szName = pName;
	//pPoint->dwSetID = m_dwCurrentDoodadRefreshPointSetID++;

	listRefreshPoint.push_back(pPoint);
	return S_OK;
Exit0:
	return E_FAIL;
}

HRESULT KSceneEditorDocLogical::RefreshPointlist::DeleteRefreshPoint(RefreshPoint* pPoint)
{
	listRefreshPoint.remove(pPoint);
	SAFE_DELETE(pPoint);
	return S_OK;
}

HRESULT KSceneEditorDocLogical::RefreshPointlist::DeleteAllRefreshPoint()
{
	list<RefreshPoint*>::iterator i = listRefreshPoint.begin();
	while (i!=listRefreshPoint.end())
	{
		RefreshPoint* pSet = *i;
		SAFE_DELETE(pSet);
		i++;
	}
	listRefreshPoint.clear();
	return S_OK;
}

void KSceneEditorDocLogical::SinkFunc2(INT nSinkID,INT nsubSinkID,DWORD dwValue,PVOID pPoint)
{
	HRESULT hr = S_OK;
	
	KGLOG_PROCESS_ERROR(m_lpSceneSceneEditor);

	switch(nSinkID)
	{
	case EVENTPLACEDOBJECT:
		{
		}
		break;
	case EVENTEDITORNPCPLACED:
		{
			IEKG3DRepresentNPC* pRepNPC = m_lpSceneSceneEditor->GetIEKG3DRepresentNPC(pPoint);
			KGLOG_PROCESS_ERROR(pRepNPC);

			NPCInfo* pNewNPC = NULL;
			D3DXVECTOR3 Pos(0,0,0);
			D3DXQUATERNION Rot;
			TCHAR Name[MAX_PATH];

			pRepNPC->GetName(Name, _countof(Name));
			pRepNPC->GetNPCTranslation(&Pos);
			pRepNPC->GetNPCRotation(&Rot);

			DWORD dwTemplateID = 0;
            pRepNPC->GetTemplateID(&dwTemplateID);
			int nDirection = 0;//角度转换还没有作

			int nKind = 0;
            pRepNPC->GetKind(&nKind);
			int nLevel = 0;
            pRepNPC->GetLevel(&nLevel);
            //int nAIType = 0;
            //pRepNPC->GetAIKind(&nAIType);

            DWORD dwRepresentID = 0;
            pRepNPC->GetRepresentID(&dwRepresentID);

			float fAlertRange = 0;
			pRepNPC->GetAlertRange(&fAlertRange);
			float fAttackRange = 0;
			pRepNPC->GetAttackRange(&fAttackRange);

            //int nAutoTurn = 0;
            //pRepNPC->GetIsDisableAutoTurn(&nAutoTurn);

			DWORD dwReliveID = pRepNPC->GetReliveID();
			DWORD dwRandomID = pRepNPC->GetRandomID();
			DWORD dwAISetID  = pRepNPC->GetAISetID();

			hr = AddOneNPC(&pNewNPC,Name,Pos,Rot,nDirection,dwTemplateID,
				pPoint,nLevel,nKind,/*nAIType, */
				dwRepresentID,
				fAlertRange,fAttackRange,
				/*nAutoTurn,*/
				dwReliveID,dwRandomID,dwAISetID);
			KGLOG_COM_PROCESS_ERROR(hr);
			UpdateNPC(pNewNPC);	

			UpdataOneNpcAIParameters(pNewNPC);
		}
		break;
	case EVENTEDITORNPCMOVED:
		{
			IEKG3DRepresentNPC* pRepNPC = m_lpSceneSceneEditor->GetIEKG3DRepresentNPC(pPoint);
			KGLOG_PROCESS_ERROR(pRepNPC);

			NPCInfo* pNPC = NULL;
			hr = FindNPCInfoByEditorNPC(&pNPC,pPoint);
			if(SUCCEEDED(hr))
			{
				D3DXVECTOR3 vPos(0,0,0);
				D3DXQUATERNION vRot;
				pRepNPC->GetNPCTranslation(&vPos);
				pRepNPC->GetNPCRotation(&vRot);

				pNPC->vPosition = vPos;
				pNPC->vRotation = vRot;

				UpdateNPC(pNPC);
			}
		}
	    break;
	case EVENTEDITORNPCROTATED:
		{
			IEKG3DRepresentNPC* pRepNPC = m_lpSceneSceneEditor->GetIEKG3DRepresentNPC(pPoint);
			KGLOG_PROCESS_ERROR(pRepNPC);

			NPCInfo* pNPC = NULL;
			hr = FindNPCInfoByEditorNPC(&pNPC,pPoint);
			if(SUCCEEDED(hr))
			{
				D3DXVECTOR3 vPos(0,0,0);
				D3DXQUATERNION vRot;
				pRepNPC->GetNPCTranslation(&vPos);
				pRepNPC->GetNPCRotation(&vRot);

				pNPC->vPosition = vPos;
				pNPC->vRotation = vRot;

				UpdateNPC(pNPC);
			}
		}
	    break;
	case EVENTEDITORNPCDELETED:
		{
			IEKG3DRepresentNPC* pRepNPC = m_lpSceneSceneEditor->GetIEKG3DRepresentNPC(pPoint);
			KGLOG_PROCESS_ERROR(pRepNPC);

			NPCInfo* pNPC = NULL;
			hr = FindNPCInfoByEditorNPC(&pNPC,pPoint);
			if(SUCCEEDED(hr))
			{
				DeleteNPC(pNPC);
				FireEvent2(EVENTDOCLOGIGAL_REFRESHPOINT_ADDED,0,0,0);
			}
		}
		break;
	case EVENTEDITORNPCHIDEATTACKRANGE:
		{
			IEKG3DRepresentNPC* pRepNPC = m_lpSceneSceneEditor->GetIEKG3DRepresentNPC(pPoint);
			KGLOG_PROCESS_ERROR(pRepNPC);
			pRepNPC->HideAttackRange();
		}
		break;
	case EVENTEDITORNPCCOPYED:
	    break;
	//case EVENTEDITORNPCADDTOSET:
	//	{
	//		IEKG3DRepresentNPC* pRepNPC = m_lpSceneSceneEditor->GetIEKG3DRepresentNPC(pPoint);
	//		KGLOG_PROCESS_ERROR(pRepNPC);

	//		NPCInfo* pNPC = NULL;
	//		hr = FindNPCInfoByEditorNPC(&pNPC,pPoint);
	//		if(SUCCEEDED(hr))
	//		{
	//			RemoveNPCFromNpcRefreshSet(pNPC);
	//			RemoveNPCFromAIGroup(pNPC);

	//			DWORD dwSetID = dwValue;
	//			NPCRefreshPointSet* pSet = NULL;
	//			if(SUCCEEDED(GetNPCRefreshPointSetByID(&pSet,dwSetID)))
	//			{
	//				//if(pSet->listNPCInfo.size()<pSet->listRefreshPoint.listRefreshPoint.size())
	//				pSet->listNPCInfo.push_back(pNPC);
	//			}
	//		}
	//		FireEvent2(EVENTDOCLOGIGAL_REFRESHPOINT_ADDED,0,0,0);
	//	}
	//	break;
/*	case EVENTEDITORNPCREMOVEFROMESET:
		{
			IEKG3DRepresentNPC* pRepNPC = m_lpSceneSceneEditor->GetIEKG3DRepresentNPC(pPoint);
			KGLOG_PROCESS_ERROR(pRepNPC);

			NPCInfo* pNPC = NULL;
			hr = FindNPCInfoByEditorNPC(&pNPC,pPoint);
			if(SUCCEEDED(hr))
			{
				DWORD dwSetID = dwValue;
				NPCRefreshPointSet* pSet = NULL;
				if(SUCCEEDED(GetNPCRefreshPointSetByID(&pSet,dwSetID)))
				{
					pSet->listNPCInfo.remove(pNPC);
				}
			}
			FireEvent2(EVENTDOCLOGIGAL_REFRESHPOINT_ADDED,0,0,0);
		}
		break;*/	
	case EVENTEDITORNPCADDTOAIGROUP:
		{
			IEKG3DRepresentNPC* pRepNPC = m_lpSceneSceneEditor->GetIEKG3DRepresentNPC(pPoint);
			KGLOG_PROCESS_ERROR(pRepNPC);

			NPCInfo* pNPC = NULL;
			hr = FindNPCInfoByEditorNPC(&pNPC,pPoint);
			if(SUCCEEDED(hr))
			{
				//RemoveNPCFromNpcRefreshSet(pNPC);
				RemoveNPCFromAIGroup(pNPC);

				DWORD dwSetID = dwValue;
				AIGroup* pSet = NULL;
				if(SUCCEEDED(GetAIGroupByID(&pSet,dwSetID)))
				{
					pSet->vecNpcInfo.push_back(pNPC);
				}
			}
			FireEvent2(EVENTDOCLOGIGAL_UPDATEUI,0,0,0);
		}
		break;
	case EVENTEDITORNPCREMOVEFROMEAIGROUP:
		{
			IEKG3DRepresentNPC* pRepNPC = m_lpSceneSceneEditor->GetIEKG3DRepresentNPC(pPoint);
			KGLOG_PROCESS_ERROR(pRepNPC);

			NPCInfo* pNPC = NULL;
			hr = FindNPCInfoByEditorNPC(&pNPC,pPoint);
			if(SUCCEEDED(hr))
			{
				DWORD dwSetID = dwValue;
				AIGroup* pSet = NULL;
				if(SUCCEEDED(GetAIGroupByID(&pSet,dwSetID)))
				{
					vector<NPCInfo*>::iterator i = find(pSet->vecNpcInfo.begin(),pSet->vecNpcInfo.end(),pNPC);
					if(i!=pSet->vecNpcInfo.end())
					{
						pSet->vecNpcInfo.erase(i);
					}
				}
			}
			FireEvent2(EVENTDOCLOGIGAL_UPDATEUI,0,0,0);
		}
		break;	
		//////////////////////////////////////////////////////////////////////////
	case EVENTEDITORDOODADPLACED:
		{
			IEKG3DRepresentDoodad* pRepDoodad = m_lpSceneSceneEditor->GetIEKG3DRepresentDoodad(pPoint);
			KGLOG_PROCESS_ERROR(pRepDoodad);

			DoodadInfo* pNewDoodad = NULL;
			D3DXVECTOR3 Pos(0,0,0);
			D3DXQUATERNION vRot;
			TCHAR Name[MAX_PATH];

			pRepDoodad->GetName(Name, _countof(Name));
			pRepDoodad->GetTranslation(&Pos);
			pRepDoodad->GetRotation(&vRot);
			DWORD dwTemplateID = 0;
            pRepDoodad->GetTemplateID(&dwTemplateID);
			int nDirection = 0;//角度转换还没有作

			DWORD dwReliveID = pRepDoodad->GetReliveID();

			hr = AddOneDoodad(&pNewDoodad,Name,Pos,vRot,nDirection,dwTemplateID,pPoint,dwReliveID);
			KGLOG_COM_PROCESS_ERROR(hr);
			UpdateDoodad(pNewDoodad);
		}
		break;
	case EVENTEDITORDOODADMOVED:
		{
			IEKG3DRepresentDoodad* pRepDoodad = m_lpSceneSceneEditor->GetIEKG3DRepresentDoodad(pPoint);
			KGLOG_PROCESS_ERROR(pRepDoodad);

			DoodadInfo* pDoodad = NULL;
			hr = FindDoodadInfoByEditorDoodad(&pDoodad,pPoint);
			if(SUCCEEDED(hr))
			{
				D3DXVECTOR3 vPos(0,0,0);
				D3DXQUATERNION vRot;
				pRepDoodad->GetTranslation(&vPos);
				pRepDoodad->GetRotation(&vRot);

				pDoodad->vPosition = vPos;
				pDoodad->vRotation = vRot;

				UpdateDoodad(pDoodad);
			}
		}
		break;
	case EVENTEDITORDOODADROTATED:
		{
			IEKG3DRepresentDoodad* pRepDoodad = m_lpSceneSceneEditor->GetIEKG3DRepresentDoodad(pPoint);
			KGLOG_PROCESS_ERROR(pRepDoodad);

			DoodadInfo* pDoodad = NULL;
			hr = FindDoodadInfoByEditorDoodad(&pDoodad,pPoint);
			if(SUCCEEDED(hr))
			{
				D3DXVECTOR3 vPos(0,0,0);
				D3DXQUATERNION vRot;
				pRepDoodad->GetTranslation(&vPos);
				pRepDoodad->GetRotation(&vRot);

				pDoodad->vPosition = vPos;
				pDoodad->vRotation = vRot;

				UpdateDoodad(pDoodad);
			}
		}
		break;
	case EVENTEDITORDOODADDELETED:
		{
			IEKG3DRepresentDoodad* pRepDoodad = m_lpSceneSceneEditor->GetIEKG3DRepresentDoodad(pPoint);
			KGLOG_PROCESS_ERROR(pRepDoodad);

			DoodadInfo* pDoodad = NULL;
			hr = FindDoodadInfoByEditorDoodad(&pDoodad,pPoint);
			if(SUCCEEDED(hr))
			{
				DeleteDoodad(pDoodad);
				FireEvent2(EVENTDOCLOGIGAL_REFRESHPOINT_ADDED,0,0,0);
			}
		}
		break;
	//case EVENTEDITORDOODADADDTOSET:
	//	{
	//		IEKG3DRepresentDoodad* pRepDoodad = m_lpSceneSceneEditor->GetIEKG3DRepresentDoodad(pPoint);
	//		KGLOG_PROCESS_ERROR(pRepDoodad);

	//		DoodadInfo* pDoodad = NULL;
	//		hr = FindDoodadInfoByEditorDoodad(&pDoodad,pPoint);
	//		if(SUCCEEDED(hr))
	//		{
	//			RemoveDoodadFromDoodadRefreshSet(pDoodad);

	//			DWORD dwSetID = dwValue;
	//			DoodadRefreshPointSet* pSet = NULL;
	//			if(SUCCEEDED(GetDoodadRefreshPointSetByID(&pSet,dwSetID)))
	//			{
	//				//if(pSet->listDoodadInfo.size()<pSet->listRefreshPoint.listRefreshPoint.size())
	//					pSet->listDoodadInfo.push_back(pDoodad);
	//			}
	//		}
	//		FireEvent2(EVENTDOCLOGIGAL_REFRESHPOINT_ADDED,0,0,0);
	//	}
	//	break;
	//case EVENTEDITORDOODADREMOVEFROMESET:
	//	{
	//		IEKG3DRepresentDoodad* pRepDoodad = m_lpSceneSceneEditor->GetIEKG3DRepresentDoodad(pPoint);
	//		KGLOG_PROCESS_ERROR(pRepDoodad);

	//		DoodadInfo* pDoodad = NULL;
	//		hr = FindDoodadInfoByEditorDoodad(&pDoodad,pPoint);
	//		if(SUCCEEDED(hr))
	//		{
	//			DWORD dwSetID = dwValue;
	//			DoodadRefreshPointSet* pSet = NULL;
	//			if(SUCCEEDED(GetDoodadRefreshPointSetByID(&pSet,dwSetID)))
	//			{
	//				pSet->listDoodadInfo.remove(pDoodad);
	//			}
	//		}
	//		FireEvent2(EVENTDOCLOGIGAL_REFRESHPOINT_ADDED,0,0,0);
	//	}
	//	break;	
		//////////////////////////////////////////////////////////////////////////
	case EVENTEDITORWAYPOINTPLACED:
		{
			IEKG3DRepresentWayPoint* pRepWayPoint = m_lpSceneSceneEditor->GetIEKG3DRepresentWayPoint(pPoint);
			KGLOG_PROCESS_ERROR(pRepWayPoint);

			WayPoint* pNewWayPoint = NULL;
			D3DXVECTOR3 Pos(0,0,0);
			D3DXQUATERNION Rot;
			TCHAR Name[MAX_PATH];

			pRepWayPoint->GetName(Name, _countof(Name));
			pRepWayPoint->GetTranslation(&Pos);
			pRepWayPoint->GetRotation(&Rot);
			DWORD dwSetID = 0;
            dwSetID = pRepWayPoint->GetSetID();
			WayPointSet* pSet = NULL;

			if(SUCCEEDED(GetWayPointSetByID(&pSet,dwSetID)))
			{
				hr = pSet->AddWayPoint(&pNewWayPoint,Pos,Rot,pPoint);
				KGLOG_COM_PROCESS_ERROR(hr);

				UpdateWayPoint(pNewWayPoint);

				FireEvent2(EVENTDOCLOGIGAL_WAYPOINT_ADDED,0,0,0);

			}
		}
		break;
	case EVENTEDITORWAYPOINTMOVED:
	case EVENTEDITORWAYPOINTROTATED:
		{
			IEKG3DRepresentWayPoint* pRepWayPoint = m_lpSceneSceneEditor->GetIEKG3DRepresentWayPoint(pPoint);
			KGLOG_PROCESS_ERROR(pRepWayPoint);

			DWORD dwSetID = 0;
            dwSetID = pRepWayPoint->GetSetID();
			WayPointSet* pSet = NULL;

			if(SUCCEEDED(GetWayPointSetByID(&pSet,dwSetID)))
			{
				WayPoint* pWayPoint = NULL;
				if(SUCCEEDED(pSet->FindWayPointByEditorWayPoint(&pWayPoint,pPoint)))
				{
					D3DXVECTOR3 vPos(0,0,0);
					D3DXQUATERNION vRot;
					pRepWayPoint->GetTranslation(&vPos);
					pRepWayPoint->GetRotation(&vRot);

					pWayPoint->vPosition = vPos;
					pWayPoint->vRotation = vRot;

					UpdateWayPoint(pWayPoint);

					FireEvent2(EVENTDOCLOGIGAL_WAYPOINT_MOVED,0,0,0);
				}
			}
		}
		break;
	case EVENTEDITORWAYPOINTDELETED:
		{
			IEKG3DRepresentWayPoint* pRepWayPoint = m_lpSceneSceneEditor->GetIEKG3DRepresentWayPoint(pPoint);
			KGLOG_PROCESS_ERROR(pRepWayPoint);

			DWORD dwSetID = 0;
            dwSetID = pRepWayPoint->GetSetID();
			WayPointSet* pSet = NULL;

			if(SUCCEEDED(GetWayPointSetByID(&pSet,dwSetID)))
			{
				WayPoint* pWayPoint = NULL;
				if(SUCCEEDED(pSet->FindWayPointByEditorWayPoint(&pWayPoint,pPoint)))
				{
					pSet->DeleteWayPoint(pWayPoint);
					FireEvent2(EVENTDOCLOGIGAL_WAYPOINT_DELETED,0,0,0);
				}
			}
		}
		break;
		//////////////////////////////////////////////////////////////////////////
	/*case EVENTEDITORREFRESHPOINTPLACED:
		{
			IEKG3DRepresentRefreshPoint* pRepRefreshPoint = m_lpSceneSceneEditor->GetIEKG3DRepresentRefreshPoint(pPoint);
			KGLOG_PROCESS_ERROR(pRepRefreshPoint);

			D3DXVECTOR3 Pos(0,0,0);
			D3DXQUATERNION Rot;

			TCHAR Name[MAX_PATH];

			pRepRefreshPoint->GetName(Name, _countof(Name));
			pRepRefreshPoint->GetTranslation(&Pos);
			pRepRefreshPoint->GetRotation(&Rot);

			DWORD dwSetID = 0;
            dwSetID = pRepRefreshPoint->GetSetID();
			DWORD dwType = 0;
            dwType = pRepRefreshPoint->GetRefreshPointType();

			if(dwType==0)
			{
				DoodadRefreshPointSet* pSet = NULL;
				if(SUCCEEDED(GetDoodadRefreshPointSetByID(&pSet,dwSetID)))
				{
					RefreshPoint* pNewRefreshPoint = NULL;
					pSet->listRefreshPoint.NewOneRefreshPoint(&pNewRefreshPoint,pPoint,Pos,Rot);
					UpdateRefreshPoint(pNewRefreshPoint);
					FireEvent2(EVENTDOCLOGIGAL_REFRESHPOINT_ADDED,0,0,0);
				}
			}
			else
			{
				NPCRefreshPointSet* pSet = NULL;
				if(SUCCEEDED(GetNPCRefreshPointSetByID(&pSet,dwSetID)))
				{
					RefreshPoint* pNewRefreshPoint = NULL;
					pSet->listRefreshPoint.NewOneRefreshPoint(&pNewRefreshPoint,pPoint,Pos,Rot);
					UpdateRefreshPoint(pNewRefreshPoint);
					FireEvent2(EVENTDOCLOGIGAL_REFRESHPOINT_ADDED,0,0,0);
				}
			}
		}
		break;*/
	/*case EVENTEDITORREFRESHPOINTMOVED:
	case EVENTEDITORREFRESHPOINTROTATED:
		{
			IEKG3DRepresentRefreshPoint* pRepRefreshPoint = m_lpSceneSceneEditor->GetIEKG3DRepresentRefreshPoint(pPoint);
			KGLOG_PROCESS_ERROR(pRepRefreshPoint);

			D3DXVECTOR3 Pos(0,0,0);
			D3DXQUATERNION Rot;
			TCHAR Name[MAX_PATH];

			pRepRefreshPoint->GetName(Name, _countof(Name));
			pRepRefreshPoint->GetTranslation(&Pos);
			pRepRefreshPoint->GetRotation(&Rot);
			DWORD dwSetID = 0;
            dwSetID = pRepRefreshPoint->GetSetID();
			DWORD dwType = 0;
            dwType = pRepRefreshPoint->GetRefreshPointType();

			if(dwType==0)
			{
				DoodadRefreshPointSet* pSet = NULL;
				if(SUCCEEDED(GetDoodadRefreshPointSetByID(&pSet,dwSetID)))
				{
					RefreshPoint* pRefreshPoint = NULL;
					if(SUCCEEDED(pSet->listRefreshPoint.FindRefreshPointByPointer(&pRefreshPoint,pPoint)))
					{
						pRefreshPoint->vPosition = Pos;
						pRefreshPoint->vRotation = Rot;
						UpdateRefreshPoint(pRefreshPoint);
						FireEvent2(EVENTDOCLOGIGAL_REFRESHPOINT_MOVED,0,0,0);
					}
				}
			}
			else
			{
				NPCRefreshPointSet* pSet = NULL;
				if(SUCCEEDED(GetNPCRefreshPointSetByID(&pSet,dwSetID)))
				{
					RefreshPoint* pRefreshPoint = NULL;
					if(SUCCEEDED(pSet->listRefreshPoint.FindRefreshPointByPointer(&pRefreshPoint,pPoint)))
					{
						pRefreshPoint->vPosition = Pos;
						pRefreshPoint->vRotation = Rot;
						UpdateRefreshPoint(pRefreshPoint);
						FireEvent2(EVENTDOCLOGIGAL_REFRESHPOINT_MOVED,0,0,0);
					}
				}
			}
		}
		break;
	case EVENTEDITORREFRESHPOINTDELETED:
		{
			IEKG3DRepresentRefreshPoint* pRepRefreshPoint = m_lpSceneSceneEditor->GetIEKG3DRepresentRefreshPoint(pPoint);
			KGLOG_PROCESS_ERROR(pRepRefreshPoint);

			D3DXVECTOR3 Pos(0,0,0);
			TCHAR Name[MAX_PATH];

			pRepRefreshPoint->GetName(Name, _countof(Name));
			pRepRefreshPoint->GetTranslation(&Pos);
			DWORD dwSetID = 0;
            dwSetID = pRepRefreshPoint->GetSetID();
			DWORD dwType = 0;
            dwType = pRepRefreshPoint->GetRefreshPointType();

			if(dwType==0)
			{
				DoodadRefreshPointSet* pSet = NULL;
				if(SUCCEEDED(GetDoodadRefreshPointSetByID(&pSet,dwSetID)))
				{
					RefreshPoint* pRefreshPoint = NULL;
					if(SUCCEEDED(pSet->listRefreshPoint.FindRefreshPointByPointer(&pRefreshPoint,pPoint)))
					{
						pSet->listRefreshPoint.DeleteRefreshPoint(pRefreshPoint);
						FireEvent2(EVENTDOCLOGIGAL_REFRESHPOINT_DELETED,0,0,0);
					}
				}
			}
			else
			{
				NPCRefreshPointSet* pSet = NULL;
				if(SUCCEEDED(GetNPCRefreshPointSetByID(&pSet,dwSetID)))
				{
					RefreshPoint* pRefreshPoint = NULL;
					if(SUCCEEDED(pSet->listRefreshPoint.FindRefreshPointByPointer(&pRefreshPoint,pPoint)))
					{
						pSet->listRefreshPoint.DeleteRefreshPoint(pRefreshPoint);
						FireEvent2(EVENTDOCLOGIGAL_REFRESHPOINT_DELETED,0,0,0);
					}
				}
			}
		}
		break;*/
		//////////////////////////////////////////////////////////////////////////
	case EVENTEDITORPOLYPLACED:
		{
			IEKG3DRepresentPoly* pIEPoint = static_cast<IEKG3DRepresentPoly*>(pPoint);
			AddOneNewLogicalPoly(pIEPoint);
			break;
		}
	case EVENTEDITORPOLYDELETED:
		{
			IEKG3DRepresentPoly* pIEPoint = static_cast<IEKG3DRepresentPoly*>(pPoint);
			DeleteLogicalPoly(pIEPoint);
			break;
		}
		///////////////////////////////////////////////////////////////////////////
	case EVENTEDITORTRAFFICPOINTPLACED:
		{
			IEKG3DRepresentTrafficPoint* pRepWayPoint = m_lpSceneSceneEditor->GetIEKG3DRepresentTrafficPoint(pPoint);
			KGLOG_PROCESS_ERROR(pRepWayPoint);
			
			D3DXVECTOR3 Pos(0,0,0);
			D3DXQUATERNION Rot;
			DWORD dwType = 0;
			pRepWayPoint->GetTranslation(&Pos);
			dwType = pRepWayPoint->GetKind();
			if (dwType == 0)
			{
				int nTempID = 0;
				g_pSO3World->m_RoadManager.AddNode(&nTempID);
				pRepWayPoint->SetSetID((DWORD)nTempID);
				m_mapTrafficPoint[nTempID].pPointer = pPoint;
				m_mapTrafficPoint[nTempID].vPosition = Pos;
				m_mapTrafficPoint[nTempID].m_strDescription = "";
				m_mapTrafficPoint[nTempID].m_dwCityID = 0;
				m_mapTrafficPoint[nTempID].m_dwForceID = 0;
				m_mapTrafficPoint[nTempID].m_dwTongID = 0;
				m_mapTrafficPoint[nTempID].m_nLinkID = -2;
				m_mapTrafficPoint[nTempID].m_bNeedOpen = 1;
				m_mapTrafficPoint[nTempID].m_strScriptFile = "";
				m_mapTrafficPoint[nTempID].m_dwNodeID = nTempID;
				m_mapTrafficPoint[nTempID].m_dwMapID = 0;
			}
			else
			{
				DWORD dwSetID = 0;
				dwSetID = pRepWayPoint->GetSetID();
				TrafficPointSet* pSet = NULL;
				if(SUCCEEDED(GetTrafficPointSetByID(&pSet,dwSetID)))
				{
					TrafficLittlePoint* pNewWayPoint = NULL;
					hr = pSet->AddTrafficPoint(&pNewWayPoint,Pos,pPoint);
				}
			}
			FireEvent2(EVENTDOCLOGIGAL_TRAFFICPOINT_ADDED,0,0,0);			
		}
		break;
	case EVENTEDITORTRAFFICPOINTMOVED:	
		{
			IEKG3DRepresentTrafficPoint* pRepWayPoint = m_lpSceneSceneEditor->GetIEKG3DRepresentTrafficPoint(pPoint);
			KGLOG_PROCESS_ERROR(pRepWayPoint);

			D3DXVECTOR3 Pos(0,0,0);
			D3DXQUATERNION Rot;
			DWORD dwType = 0;
			pRepWayPoint->GetTranslation(&Pos);
			dwType = pRepWayPoint->GetKind();
			if (dwType == 0)
			{
				DWORD nTempID = 0;
				nTempID = pRepWayPoint->GetSetID();
				if (m_mapTrafficPoint.find(nTempID) != m_mapTrafficPoint.end())
					m_mapTrafficPoint[nTempID].vPosition = Pos;
			}
			else
			{
				DWORD dwSetID = 0;
				dwSetID = pRepWayPoint->GetSetID();
				TrafficPointSet* pSet = NULL;
				TrafficPoint* pNewWayPoint = NULL;
				int nTempID = 0;
				if(SUCCEEDED(GetTrafficPointSetByID(&pSet,dwSetID)))
				{
					TrafficLittlePoint* pTrafficPoint = NULL;
					int i = 0;
					if(SUCCEEDED(pSet->FindTrafficPointByEditorWayPoint(&pTrafficPoint,&i,pPoint)))	
						pTrafficPoint->vPosition = Pos;

				}
			}
			FireEvent2(EVENTDOCLOGIGAL_TRAFFICPOINT_MOVED,0,0,0);
		}
		break;
	case EVENTEDITORTRAFFICPOINTDELETED:
		{
	   	    IEKG3DRepresentTrafficPoint* pRepTrafficPoint = m_lpSceneSceneEditor->GetIEKG3DRepresentTrafficPoint(pPoint);
			KGLOG_PROCESS_ERROR(pRepTrafficPoint);
			DWORD dwType = 0;
			dwType = pRepTrafficPoint->GetKind();
			if (dwType == 0)
			{
				DWORD dwID = pRepTrafficPoint->GetSetID();				
				list<TrafficPointSet*>::iterator it = m_listTrafficPointSet.begin();
				while (it != m_listTrafficPointSet.end())
				{
					KSceneEditorDocLogical::TrafficPointSet*& pSet = *it;
					if (pSet->m_dwFromID == dwID || pSet->m_dwToID == dwID)
					{
						/*
						先把东西放到选择列表中，然后删除，这样会当机，因为这里可能也是删除那个函数触发的
						*/
						//m_lpSceneSceneEditor->ClearSelectedEntityList();
						for(size_t s=0;s<pSet->vecTrafficPoint.size();s++)
						{
							KSceneEditorDocLogical::TrafficLittlePoint* pPoint = pSet->vecTrafficPoint[s];
							m_lpSceneSceneEditor->RemoveRenderEntityAndNotifyLogic(SCENEENTITY_OBJECT, pPoint->pPointer);

							//m_lpSceneSceneEditor->SetSelected(SCENEENTITY_OBJECT,pPoint->pPointer, TRUE);
						}
						//m_lpSceneSceneEditor->DeleteSelectedEntity();
						it++;
						DeleteTrafficPointSet(pSet);
						pSet = NULL;
					}
					else
						it++;
				}
				g_pSO3World->m_RoadManager.DelNodeWithTrack(dwID);

				map<DWORD,TrafficPoint>::iterator iPoint = m_mapTrafficPoint.find(dwID);
				if (iPoint != m_mapTrafficPoint.end())
				{
					if ((iPoint->second).m_nLinkID == -1)
					{
						list<TrafficPointSet*>::iterator iSet = m_listOverMapPointSet.begin();
						while (iSet != m_listOverMapPointSet.end())
						{
							KSceneEditorDocLogical::TrafficPointSet* pSet = *iSet;						
							if (pSet->m_dwFromID == iPoint->first || pSet->m_dwToID == iPoint->first)
							{				
								iSet++;
								map<DWORD,string>::iterator itTrackByWhat = m_mapTrackByWhat.find(pSet->m_dwID);
								if (itTrackByWhat != m_mapTrackByWhat.end())
									m_mapTrackByWhat.erase(itTrackByWhat);
								m_listOverMapPointSet.remove(pSet);
								SAFE_DELETE(pSet);
							}
							else
								iSet++;	
						}	
						m_mapLinkAbleList.erase(m_mapLinkAbleList.find(dwID));
					}
					m_mapTrafficPoint.erase(iPoint);
				}

			}
			else
			{
				DWORD dwSetID = 0;
				dwSetID = pRepTrafficPoint->GetSetID();
				TrafficPointSet* pSet = NULL;
				if(SUCCEEDED(GetTrafficPointSetByID(&pSet,dwSetID)))
				{
					TrafficLittlePoint* pTrafficPoint = NULL;
					int nTempID = 0;
					if(SUCCEEDED(pSet->FindTrafficPointByEditorWayPoint(&pTrafficPoint,&nTempID,pPoint)))
					{					
						pSet->DeleteTrafficPoint(pTrafficPoint);	
					}
				}
			}
			FireEvent2(EVENTDOCLOGIGAL_TRAFFICPOINT_DELETED,0,0,0);
		}
		break;
	default:
	    break;
	}

Exit0:
	;
}

/*
HRESULT KSceneEditorDocLogical::RemoveNPCFromNpcRefreshSet(NPCInfo* pNPC)
{
	list<NPCRefreshPointSet*>::iterator i = m_listNPCRefreshPointSet.begin();
	while (i!=m_listNPCRefreshPointSet.end())
	{
		NPCRefreshPointSet* pSet = *i;
		pSet->listNPCInfo.remove(pNPC);
		i++;
	}
	return S_OK;
}
*/

// ahpho suntao
// 将npc从AI组中清除
HRESULT KSceneEditorDocLogical::ClearNpcAIGroup(INT npcID)
{	
	for (list<NPCInfo*>::iterator it = m_listNPCInfo.begin(); it != m_listNPCInfo.end(); it++)
	{
		NPCInfo* sourceinfo = *it;
		if (sourceinfo->nIndex == npcID)
		{
			RemoveNPCFromAIGroup(sourceinfo);
			break;
		}
	}

	return S_OK;
}
// end ahpho

HRESULT KSceneEditorDocLogical::RemoveNPCFromAIGroup(NPCInfo* pNPC)
{
	list<AIGroup*>::iterator i = m_listAIGroup.begin();
	while (i!=m_listAIGroup.end())
	{
		AIGroup* pSet = *i;

		vector<NPCInfo*>::iterator j = find(pSet->vecNpcInfo.begin(),pSet->vecNpcInfo.end(),pNPC);
		if(j!=pSet->vecNpcInfo.end())
		{
			pSet->vecNpcInfo.erase(j);
		}
		i++;
	}
	return S_OK;
}

/*
HRESULT KSceneEditorDocLogical::RemoveDoodadFromDoodadRefreshSet(DoodadInfo* pDoodad)
{
	list<DoodadRefreshPointSet*>::iterator i = m_listDoodadRefreshPointSet.begin();
	while (i!=m_listDoodadRefreshPointSet.end())
	{
		DoodadRefreshPointSet* pSet = *i;
		pSet->listDoodadInfo.remove(pDoodad);
		i++;
	}
	return S_OK;
}
*/

// ahpho suntao
// 设置AI组阵型信息
HRESULT KSceneEditorDocLogical::SetAIGroupOrder(INT groupID, INT npcOrderID, BSTR npcString, BSTR orderString)
{
	vector<NPCInfo*> npcVector;
	string tempString;
	string npcStr = BSTR_TO_STRING(npcString);;
	INT npcIndex;
	NPCInfo* npcInfo = NULL;
	AIParameterSet aiParameterSet;
	vector<INT> orderVector;
	INT npcOrder;
	string orderStr = BSTR_TO_STRING(orderString);
	AIGroup* groupInfo = NULL;
	
	list<AIGroup*>::iterator i = m_listAIGroup.begin();
	while (i != m_listAIGroup.end())
	{
		AIGroup* pInfo = *i;
		if (pInfo->dwSetID == groupID)
		{
			groupInfo = pInfo;
			break;
		}
		i++;
	}
	
	if (groupInfo != NULL)
	{
		groupInfo->npcOrderID = npcOrderID;
		groupInfo->npcOrder = BSTR_TO_STRING(orderString);
		WayPointSet* pWayPointSet = groupInfo->pWayPointSet;
		
		// 初始化阵型	
		int index = orderStr.find(",");
		while (index != -1)
		{
			tempString = orderStr.substr(0, index);
			npcOrder = atoi(tempString.c_str());
			orderVector.push_back(npcOrder);
			orderStr = orderStr.substr(index + 1, orderStr.length() - index - 1);
			index = orderStr.find(",");
		}
		tempString = orderStr;
		npcOrder = atoi(tempString.c_str());
		orderVector.push_back(npcOrder);

		// 设置npc
		int i = 0;
		index = npcStr.find(",");
		while (index != -1)
		{
			tempString = npcStr.substr(0, index);
			npcIndex = atoi(tempString.c_str());
			npcInfo = NULL;
			GetNpcInfoByIndex(&npcInfo, npcIndex);
			npcInfo->pWayPointSet = pWayPointSet;

			// 更新npc的AI参数
			if (npcInfo->dwNpcAISet)
			{
				HRESULT success = GetAISet(npcInfo->dwNpcAISet, aiParameterSet);
				if (success == S_OK)
				{
					UpdateAIParameter(aiParameterSet, "PatrolPathID", pWayPointSet->nIndex);
					UpdateAIParameter(aiParameterSet, "PatrolOrderIndex", orderVector[i]);
				}
				else
				{
					ResetAISet(npcInfo->dwNpcAISet, aiParameterSet);
					UpdateAIParameter(aiParameterSet, "PatrolPathID", pWayPointSet->nIndex);
					UpdateAIParameter(aiParameterSet, "PatrolOrderIndex", orderVector[i]);
					npcInfo->m_cAIParameterSet = aiParameterSet;
				}
			}
			else
			{
				DWORD aiSetID = 0;
				NewOneAISet(aiSetID, "AUTO_CONVERT_AI_SET");

				GetAISet(aiSetID, aiParameterSet);
				UpdateAIParameter(aiParameterSet, "PatrolPathID", pWayPointSet->nIndex);
				UpdateAIParameter(aiParameterSet, "PatrolOrderIndex", orderVector[i]);

				npcInfo->dwNpcAISet = aiSetID;
				npcInfo->m_cAIParameterSet = aiParameterSet;
			}		

			SetAISet(npcInfo->dwNpcAISet, aiParameterSet);
			npcVector.push_back(npcInfo);		
			npcStr = npcStr.substr(index + 1, npcStr.length() - index - 1);
			index = npcStr.find(",");
			i++;
		}

		tempString = npcStr;
		npcIndex = atoi(tempString.c_str());
		npcInfo = NULL;
		GetNpcInfoByIndex(&npcInfo, npcIndex);
		npcInfo->pWayPointSet = pWayPointSet;
		// 更新npc的AI参数
		if (npcInfo->dwNpcAISet)
		{
			HRESULT success = GetAISet(npcInfo->dwNpcAISet, aiParameterSet);
			if (success == S_OK)
			{
				UpdateAIParameter(aiParameterSet, "PatrolPathID", pWayPointSet->nIndex);
				UpdateAIParameter(aiParameterSet, "PatrolOrderIndex", orderVector[i]);
			}
			else
			{
				ResetAISet(npcInfo->dwNpcAISet, aiParameterSet);
				UpdateAIParameter(aiParameterSet, "PatrolPathID", pWayPointSet->nIndex);
				UpdateAIParameter(aiParameterSet, "PatrolOrderIndex", orderVector[i]);
				npcInfo->m_cAIParameterSet = aiParameterSet;
			}			
		}
		else
		{
			DWORD aiSetID = 0;
			NewOneAISet(aiSetID, "AUTO_CONVERT_AI_SET");

			GetAISet(aiSetID, aiParameterSet);
			UpdateAIParameter(aiParameterSet, "PatrolPathID", pWayPointSet->nIndex);
			UpdateAIParameter(aiParameterSet, "PatrolOrderIndex", orderVector[i]);

			npcInfo->dwNpcAISet = aiSetID;
			npcInfo->m_cAIParameterSet = aiParameterSet;
		}	
		SetAISet(npcInfo->dwNpcAISet, aiParameterSet);
		npcVector.push_back(npcInfo);

		groupInfo->vecNpcInfo.clear();
		groupInfo->vecNpcInfo = npcVector;
	}	

	return S_OK;
}

// 更新AISet参数
HRESULT KSceneEditorDocLogical::UpdateAIParameter(AIParameterSet& aiSet, string field, int newValue)
{
	vector<AIParameter> parameterVector = aiSet.vecParameters;	

	for(int i = 0; i < aiSet.vecParameters.size(); i++)
	{
		AIParameter aiParameter = aiSet.vecParameters[i];
		if (aiParameter.strName == field)
		{
			aiParameter.nValue = newValue;
			aiSet.vecParameters[i] = aiParameter;
			break;
		}
	}

	return S_OK;
}

// 新建AI组
HRESULT KSceneEditorDocLogical::DecoratedAddOneAIGroup(BSTR groupName, INT patrolPathID, INT npcOrderID, BOOL reviveTeammate, BSTR npcString, BSTR orderString)
{	
	vector<NPCInfo*> npcVector;
	string tempString;
	string npcStr = BSTR_TO_STRING(npcString);;
	INT npcIndex;
	NPCInfo* npcInfo = NULL;
	AIParameterSet aiParameterSet;
	vector<INT> orderVector;
	INT npcOrder;
	string orderStr = BSTR_TO_STRING(orderString);

	AIGroup* pSet = new AIGroup;
	KGLOG_PROCESS_ERROR(pSet);

	pSet->szName = BSTR_TO_STRING(groupName);
	pSet->dwSetID = m_dwCurrentAIGroupID++;
	pSet->nKing = 0;
	pSet->nArrayIndex = 0;
	pSet->npcOrderID = npcOrderID;
	pSet->reviveTeammate = reviveTeammate;
	
	// 设置巡逻路径
	WayPointSet* pWayPointSet;
	GetWayPointSetByIndex(&pWayPointSet, patrolPathID);
	pSet->pWayPointSet = pWayPointSet;

	// 初始化阵型	
	int index = orderStr.find(",");
	while (index != -1)
	{
		tempString = orderStr.substr(0, index);
		npcOrder = atoi(tempString.c_str());
		orderVector.push_back(npcOrder);
		orderStr = orderStr.substr(index + 1, orderStr.length() - index - 1);
		index = orderStr.find(",");
	}
	tempString = orderStr;
	npcOrder = atoi(tempString.c_str());
	orderVector.push_back(npcOrder);

	// 设置npc和npc阵型	
	int i = 0;
	index = npcStr.find(",");
	while (index != -1)
	{
		tempString = npcStr.substr(0, index);
		npcIndex = atoi(tempString.c_str());
		npcInfo = NULL;
		GetNpcInfoByIndex(&npcInfo, npcIndex);
		npcInfo->pWayPointSet = pWayPointSet;

		// 更新npc的AI参数
		if (npcInfo->dwNpcAISet)
		{
			HRESULT success = GetAISet(npcInfo->dwNpcAISet, aiParameterSet);
			if (success == S_OK)
			{
				UpdateAIParameter(aiParameterSet, "PatrolPathID", pWayPointSet->nIndex);
				UpdateAIParameter(aiParameterSet, "PatrolOrderIndex", orderVector[i]);
				UpdateAIParameter(aiParameterSet, "ReviveTeammate", reviveTeammate);
			}
			else
			{
				ResetAISet(npcInfo->dwNpcAISet, aiParameterSet);
				UpdateAIParameter(aiParameterSet, "PatrolPathID", pWayPointSet->nIndex);
				UpdateAIParameter(aiParameterSet, "PatrolOrderIndex", orderVector[i]);
				UpdateAIParameter(aiParameterSet, "ReviveTeammate", reviveTeammate);
				npcInfo->m_cAIParameterSet = aiParameterSet;
			}
		}
		else
		{
			DWORD aiSetID = 0;
			NewOneAISet(aiSetID, "AUTO_CONVERT_AI_SET");

			GetAISet(aiSetID, aiParameterSet);
			UpdateAIParameter(aiParameterSet, "PatrolPathID", pWayPointSet->nIndex);
			UpdateAIParameter(aiParameterSet, "PatrolOrderIndex", orderVector[i]);
			UpdateAIParameter(aiParameterSet, "ReviveTeammate", reviveTeammate);

			npcInfo->dwNpcAISet = aiSetID;
			npcInfo->m_cAIParameterSet = aiParameterSet;
		}		
		
		SetAISet(npcInfo->dwNpcAISet, aiParameterSet);
		npcVector.push_back(npcInfo);		
		npcStr = npcStr.substr(index + 1, npcStr.length() - index - 1);
		index = npcStr.find(",");
		i++;
	}
	
	tempString = npcStr;
	npcIndex = atoi(tempString.c_str());
	npcInfo = NULL;
	GetNpcInfoByIndex(&npcInfo, npcIndex);
	npcInfo->pWayPointSet = pWayPointSet;
	// 更新npc的AI参数
	if (npcInfo->dwNpcAISet)
	{
		HRESULT success = GetAISet(npcInfo->dwNpcAISet, aiParameterSet);
		if (success == S_OK)
		{
			UpdateAIParameter(aiParameterSet, "PatrolPathID", pWayPointSet->nIndex);
			UpdateAIParameter(aiParameterSet, "PatrolOrderIndex", orderVector[i]);
			UpdateAIParameter(aiParameterSet, "ReviveTeammate", reviveTeammate);
		}
		else
		{
			ResetAISet(npcInfo->dwNpcAISet, aiParameterSet);
			UpdateAIParameter(aiParameterSet, "PatrolPathID", pWayPointSet->nIndex);
			UpdateAIParameter(aiParameterSet, "PatrolOrderIndex", orderVector[i]);
			UpdateAIParameter(aiParameterSet, "ReviveTeammate", reviveTeammate);
			npcInfo->m_cAIParameterSet = aiParameterSet;
		}			
	}
	else
	{
		DWORD aiSetID = 0;
		NewOneAISet(aiSetID, "AUTO_CONVERT_AI_SET");

		GetAISet(aiSetID, aiParameterSet);
		UpdateAIParameter(aiParameterSet, "PatrolPathID", pWayPointSet->nIndex);
		UpdateAIParameter(aiParameterSet, "PatrolOrderIndex", orderVector[i]);
		UpdateAIParameter(aiParameterSet, "ReviveTeammate", reviveTeammate);

		npcInfo->dwNpcAISet = aiSetID;
		npcInfo->m_cAIParameterSet = aiParameterSet;
	}	
	SetAISet(npcInfo->dwNpcAISet, aiParameterSet);
	npcVector.push_back(npcInfo);

	pSet->vecNpcInfo = npcVector;
	pSet->npcOrder = BSTR_TO_STRING(orderString);
	
	m_listAIGroup.push_back(pSet);

	return S_OK;
Exit0:
	return E_FAIL;	
}

// 通过nIndex获取对应的npc
HRESULT KSceneEditorDocLogical::GetNpcInfoByIndex(NPCInfo** npcInfo, INT nIndex)
{
	list<NPCInfo*>::iterator i = m_listNPCInfo.begin();
	while (i != m_listNPCInfo.end())
	{
		NPCInfo* pInfo = *i;
		if (pInfo->nIndex == nIndex)
		{
			(*npcInfo) = pInfo;
			break;
		}
		i++;
	}

	return S_OK;
}

// 通过nIndex获取对应的doodad
HRESULT KSceneEditorDocLogical::GetDoodadInfoByIndex(DoodadInfo** doodadInfo, INT nIndex)
{
	list<DoodadInfo*>::iterator i = m_listDoodadInfo.begin();
	while (i != m_listDoodadInfo.end())
	{
		DoodadInfo* pInfo = *i;
		if (pInfo->nIndex == nIndex)
		{
			(*doodadInfo) = pInfo;
			break;
		}
		i++;
	}

	return S_OK;
}
// end ahpho

HRESULT KSceneEditorDocLogical::AddOneAIGroup(AIGroup** ppNewSet,LPCTSTR pName)
{
	AIGroup* pSet = new AIGroup;
	KGLOG_PROCESS_ERROR(pSet);
	(*ppNewSet) = pSet;

	pSet->szName = pName;
	pSet->dwSetID = m_dwCurrentAIGroupID++;
	pSet->nKing = 0;
	pSet->pWayPointSet = NULL;
	pSet->nArrayIndex = 0;

	m_listAIGroup.push_back(pSet);

	return S_OK;
Exit0:
	return E_FAIL;
}

HRESULT KSceneEditorDocLogical::DeleteAIGroup(AIGroup* pSet)
{
	m_listAIGroup.remove(pSet);
	SAFE_DELETE(pSet);

	return S_OK;
}

HRESULT KSceneEditorDocLogical::DeleteAllAIGroup()
{
	list<AIGroup*>::iterator i = m_listAIGroup.begin();
	while (i!=m_listAIGroup.end())
	{
		AIGroup* pSet = *i;
		SAFE_DELETE(pSet);

		i++;
	}
	m_listAIGroup.clear();
	return S_OK;
}

HRESULT KSceneEditorDocLogical::GetAIGroupByID(AIGroup** ppSet,DWORD dwID)
{
	list<AIGroup*>::iterator i = m_listAIGroup.begin();
	while (i!=m_listAIGroup.end())
	{
		AIGroup* pSet = *i;
		if(pSet->dwSetID==dwID)
		{
			(*ppSet) = pSet;
			return S_OK;
		}

		i++;
	}
	(*ppSet) = NULL;
	return E_FAIL;
}

HRESULT KSceneEditorDocLogical::UpdateNPC(NPCInfo* pNpc)
{
	SceneToLogical(pNpc->nX,pNpc->nY,pNpc->nZ,pNpc->vPosition,0);
	if(pNpc->nFaceDirection!=-1)
	{
		pNpc->nFaceDirection = SceneToLogicalAngle(pNpc->vRotation);
	}

	if(pNpc->szScriptName.size()==0)
		;//pNpc->dwScriptID = 0;
	else
		pNpc->dwScriptID = g_FileNameHash(pNpc->szScriptName.c_str());

	return S_OK;
}

HRESULT KSceneEditorDocLogical::UpdateDoodad(DoodadInfo* pDoodad)
{
	SceneToLogical(pDoodad->nX,pDoodad->nY,pDoodad->nZ,pDoodad->vPosition,0);
	if(pDoodad->nFaceDirection!=-1)
	{
		pDoodad->nFaceDirection = SceneToLogicalAngle(pDoodad->vRotation);
	}

	if(pDoodad->szScriptName.size() == 0)
		;//pDoodad->dwScriptID = 0;
	else
		pDoodad->dwScriptID = g_FileNameHash(pDoodad->szScriptName.c_str());

	return S_OK;
}

HRESULT KSceneEditorDocLogical::UpdateWayPoint(WayPoint* pWayPoint)
{
	SceneToLogical(pWayPoint->nX,pWayPoint->nY,pWayPoint->nZ,pWayPoint->vPosition,0);
	if(pWayPoint->nStayDirection!=-1)
	{
		pWayPoint->nStayDirection = SceneToLogicalAngle(pWayPoint->vRotation);
	}

	return S_OK;
}

HRESULT KSceneEditorDocLogical::UpdateRefreshPoint(RefreshPoint* pPoint)
{
	SceneToLogical(pPoint->nX,pPoint->nY,pPoint->nZ,pPoint->vPosition,0);
	if(pPoint->nDirection!=-1)
	{
		pPoint->nDirection = SceneToLogicalAngle(pPoint->vRotation);
	}

	return S_OK;
}


void KSceneEditorDocLogical::SceneToLogical(int& X,int& Y,int& Z,D3DXVECTOR3 vPos,float fGroundHeight)
{
	X = (int)((vPos.x - m_fLogicalXStart) / 50.0F * 32);
	Y = (int)((vPos.z - m_fLogicalZStart) / 50.0F * 32);
	//Z = (int)((vPos.y - fGroundHeight) / 50.0F * 32);
	Z = (int)((vPos.y - TERRAIN_MIN_HEIGHT) * POINT_PER_ALTITUDE / ALTITUDE_UNIT);
}

int KSceneEditorDocLogical::SceneToLogicalAngle(D3DXQUATERNION vRot)
{
	D3DXQuaternionNormalize(&vRot,&vRot);
	D3DXMATRIX mat;
	D3DXMatrixRotationQuaternion(&mat,&vRot);
	D3DXVECTOR3 vDir(1,0,0);
	D3DXVec3TransformCoord(&vDir,&vDir,&mat);

	float fAngle = - GetAngle(vDir.x,vDir.z) - D3DX_PI * 0.5F;
	while (fAngle<0)
	{
		fAngle += D3DX_PI * 2;
	}

	int nAngle = (int)(fAngle / ( D3DX_PI * 2) * 256 );
	return nAngle;
}

HRESULT KSceneEditorDocLogical::FitDoodadGroup(DoodadRefreshPointSet* pSet)
{
	list<RefreshPoint*>listPoint = pSet->listRefreshPoint.listRefreshPoint;
	list<DoodadInfo*>listDoodad = pSet->listDoodadInfo;

	DWORD dwTime = (DWORD)listPoint.size();
	for(DWORD i=0;i<dwTime;i++)
	{
		if(listPoint.size()>0)
		{
			int K = rand() % (int)listPoint.size();
			int M = 0;

			list<RefreshPoint*>::iterator itPoint = listPoint.begin();
			while (itPoint!=listPoint.end())
			{
				RefreshPoint* pPoint = *itPoint;
				if(M==K)//选取到随机的刷新点
				{
					if(listDoodad.size()>0)
					{
						int S = rand() % (int)listDoodad.size();
						int T = 0;

						list<DoodadInfo*>::iterator itDoodad = listDoodad.begin();
						while (itDoodad!=listDoodad.end())
						{
							DoodadInfo* pDoodad = *itDoodad;
							if(T == S)//选取到随机的doodad
							{
								pDoodad->vPosition = pPoint->vPosition;
								pDoodad->vRotation = pPoint->vRotation;
								pDoodad->nFaceDirection = pPoint->nDirection;

								UpdateDoodad(pDoodad);

								listDoodad.erase(itDoodad);
								break;
							}
							itDoodad++;
							T++;
						}
					}
					listPoint.remove(pPoint);
					break;
				}

				itPoint++;
				M++;
			}
		}
		
	}
	return S_OK;
}

HRESULT KSceneEditorDocLogical::FitNpcGroup(NPCRefreshPointSet* pSet)
{
	list<RefreshPoint*>listPoint = pSet->listRefreshPoint.listRefreshPoint;
	list<NPCInfo*>listNpc = pSet->listNPCInfo;

	DWORD dwTime = (DWORD)listPoint.size();
	for(DWORD i=0;i<dwTime;i++)
	{
		if(listPoint.size()>0)
		{
			int K = rand() % (int)listPoint.size();
			int M = 0;

			list<RefreshPoint*>::iterator itPoint = listPoint.begin();
			while (itPoint!=listPoint.end())
			{
				RefreshPoint* pPoint = *itPoint;
				if(M==K)//选取到随机的刷新点
				{
					if(listNpc.size()>0)
					{
						int S = rand() % (int)listNpc.size();
						int T = 0;

						list<NPCInfo*>::iterator itNpc = listNpc.begin();
						while (itNpc!=listNpc.end())
						{
							NPCInfo* pNpc = *itNpc;
							if(T == S)//选取到随机的doodad
							{
								pNpc->vPosition = pPoint->vPosition;
								pNpc->vRotation = pPoint->vRotation;
								pNpc->nFaceDirection = pPoint->nDirection;

								UpdateNPC(pNpc);

								listNpc.erase(itNpc);
								break;
							}
							itNpc++;
							T++;
						}
					}
					listPoint.remove(pPoint);
					break;
				}

				itPoint++;
				M++;
			}
		}

	}
	return S_OK;
}

HRESULT KSceneEditorDocLogical::SaveToIniFile(LPCTSTR pFileName)
{
	TCHAR szSectionName[MAX_PATH];

    ResortNpcAISet();
	//////////////////////////////////////////////////////////////////////

	{
		TCHAR szBakFile[MAX_PATH];
		wsprintf(szBakFile,"%sbak",pFileName);
		CopyFile(pFileName,szBakFile,TRUE);
	}

	DeleteFile(pFileName);

	IIniFile* pFile = g_OpenIniFile(pFileName,false,true);
	if (!pFile)
		return E_FAIL;
	//////////////////////////////////////////////////////////////////////////
	//MAIN
	pFile->WriteInteger("MAIN","NumNPC",(int)m_listNPCInfo.size());
	pFile->WriteInteger("MAIN","NumDoodad",(int)m_listDoodadInfo.size());

	//pFile->WriteInteger("MAIN","NumNpcRefreshSet",(int)m_listNPCRefreshPointSet.size());
	//pFile->WriteInteger("MAIN","NumNpcRefreshSetMaxID",(int)m_dwCurrentNPCRefreshPointSetID);
	//pFile->WriteInteger("MAIN","NumDoodadRefreshSet",(int)m_listDoodadRefreshPointSet.size());
	//pFile->WriteInteger("MAIN","NumDoodadRefreshSetMaxID",(int)m_dwCurrentDoodadRefreshPointSetID);
	pFile->WriteInteger("MAIN","NumAIGroup",(int)m_listAIGroup.size());
	pFile->WriteInteger("MAIN","NumAIGroupMaxID",(int)m_dwCurrentAIGroupID);

	pFile->WriteInteger("MAIN","NumWapPointSet",(int)m_listWayPointSet.size());
	pFile->WriteInteger("MAIN","NumWapPointSetMaxID",(int)m_dwCurrentWayPointSetID);

	pFile->WriteInteger("MAIN","LogicalSceneID",m_nSceneID);
	pFile->WriteFloat("MAIN","LogicalSceneXStart",m_fLogicalXStart);
	pFile->WriteFloat("MAIN","LogicalSceneZStart",m_fLogicalZStart);
	
	pFile->WriteInteger("MAIN","NumLogicalPoly",(int)m_listLogicalPoly.size());

	//pFile->WriteInteger("MAIN","NumTrafficPointSet",(int)m_listTrafficPointSet.size());
	//pFile->WriteInteger("MAIN","NumTrafficPointSetMaxID",(int)m_dwCurrentTrafficPointSetID);
	pFile->WriteInteger("MAIN","NumTrafficPointSet",(int)m_listTrafficPointSet.size());
	pFile->WriteInteger("MAIN","NumTrafficPoint",(int)m_mapTrafficPoint.size());
	
	// ahpho suntao
	SaveNpcHatredGroups(pFile);		
	// end ahpho

	//////////////////////////////////////////////////////////////////////////
	//WayPointSet
	{
		int S = 0;
		list<WayPointSet*>::iterator i = m_listWayPointSet.begin();
		while (i!=m_listWayPointSet.end())
		{
			WayPointSet* pWayPointSet = *i;

			wsprintf(szSectionName,"WayPointSet%d",S);
			SaveWayPointSetToIniFile(pFile,szSectionName,pWayPointSet);

			i++;
			S++;
		}
	}
	//////////////////////////////////////////////////////////////////////////
	//NPC
	{
		int S = 0;
		list<NPCInfo*>::iterator i = m_listNPCInfo.begin();
		while (i!=m_listNPCInfo.end())
		{
			NPCInfo* pNpcInfo = *i;
			pNpcInfo->nIndex = S;

			wsprintf(szSectionName,"NPC%d",S);
			SaveNPCToIniFile(pFile,szSectionName,pNpcInfo);

			i++;
			S++;
		}
	}
	//////////////////////////////////////////////////////////////////////////
	//Doodad
	{
		int S = 0;
		list<DoodadInfo*>::iterator i = m_listDoodadInfo.begin();
		while (i!=m_listDoodadInfo.end())
		{
			DoodadInfo* pDoodad = *i;
			pDoodad->nIndex = S;

			wsprintf(szSectionName,"Doodad%d",S);
			SaveDoodadToIniFile(pFile,szSectionName,pDoodad);

			i++;
			S++;
		}
	}
	//////////////////////////////////////////////////////////////////////////
	//NPCRefreshPointSet
	/*{
		int S = 0;
		list<NPCRefreshPointSet*>::iterator i = m_listNPCRefreshPointSet.begin();
		while (i!=m_listNPCRefreshPointSet.end())
		{
			NPCRefreshPointSet* pRefreshSet = *i;

			wsprintf(szSectionName,"NPCRefreshPointSet%d",S);
			SaveNPCRefreshPointSetToIniFile(pFile,szSectionName,pRefreshSet);

			i++;
			S++;
		}
	}*/
	//////////////////////////////////////////////////////////////////////////
	//DoodadRefreshPointSet
	/*{
		int S = 0;
		list<DoodadRefreshPointSet*>::iterator i = m_listDoodadRefreshPointSet.begin();
		while (i!=m_listDoodadRefreshPointSet.end())
		{
			DoodadRefreshPointSet* pRefreshSet = *i;

			wsprintf(szSectionName,"DoodadRefreshPointSet%d",S);
			SaveDoodadRefreshPointSetToIniFile(pFile,szSectionName,pRefreshSet);

			i++;
			S++;
		}
	}*/
	//////////////////////////////////////////////////////////////////////////
	//AIGroup
	{
		int S = 0;
		list<AIGroup*>::iterator i = m_listAIGroup.begin();
		while (i!=m_listAIGroup.end())
		{
			AIGroup* pAI = *i;

			wsprintf(szSectionName,"AIGroup%d",S);
			SaveAIGroupToIniFile(pFile,szSectionName,pAI);

			i++;
			S++;
		}
	}
	//////////////////////////////////////////////////////////////////////////
	//Logical Poly
	{
		int S = 0;
		list<IEKG3DRepresentPoly*>::iterator i = m_listLogicalPoly.begin();
		while (i!=m_listLogicalPoly.end())
		{
			IEKG3DRepresentPoly* pPoly = *i;

			wsprintf(szSectionName,"LogicalPoly%d",S);
			SaveLogicalPolyIniFile(pFile,szSectionName,pPoly);

			i++;
			S++;
		}
	}


	//////////////////////////////////////////////////////////////////////////
	//TrafficPoint
	{
		int S = 0;
		int nSize = (int)m_mapTrafficPoint.size();
		map<DWORD,TrafficPoint>::iterator i = m_mapTrafficPoint.begin();
		while (i!=m_mapTrafficPoint.end())
		{
			TrafficPoint* pTrafficPoint = &(i->second);
			wsprintf(szSectionName,"TrafficPoint%d",S);
			SaveTrafficPointToIniFile(pFile,szSectionName,pTrafficPoint);
			S++;
			i++;		
		}
		//pFile->WriteInteger("MAIN","NumTrafficPointSet",(int)S);
	}

	//////////////////////////////////////////////////////////////////////////
	//TrafficPointSet
	{
		int S = 0;
		int nSize = (int)m_listTrafficPointSet.size();
		list<TrafficPointSet*>::iterator i = m_listTrafficPointSet.begin();
		while (i!=m_listTrafficPointSet.end())
		{
			TrafficPointSet* pTrafficPointSet = *i;
			wsprintf(szSectionName,"TrafficPointSet%d",S);
			SaveTrafficPointSetToIniFile(pFile,szSectionName,pTrafficPointSet);
			S++;
			i++;		
		}
		//pFile->WriteInteger("MAIN","NumTrafficPointSet",(int)S);
	}
	//////////////////////////////////////////////////////////////////////////
	SaveNpcReviveGroups(pFile);

	SaveDoodadReviveGroups(pFile);

	SaveNpcRandomGroups(pFile);

	SaveAISetToFile(pFile);
		
	//////////////////////////////////////////////////////////////////////////
	pFile->Save(pFileName);
	pFile->Release();

	TCHAR szRoadFile[MAX_PATH];
	wsprintf(szRoadFile,"%s%s\\Road\\RoadPointOverMaps.ini",g_szDefWorkDirectory,SETTING_DIR);
	DeleteFile(szRoadFile);

	IIniFile* pRoadFile = g_OpenIniFile(szRoadFile,false,true);
	if (!pRoadFile)
		return E_FAIL;
	SaveOverMapTrafficPointToIniFile(pRoadFile);
	pRoadFile->Save(szRoadFile);
	pRoadFile->Release();

	TCHAR szRoadByWhatFile[MAX_PATH];
//	TCHAR szWriteByte[MAX_PATH];
	wsprintf(szRoadByWhatFile,"%s%s\\Road\\RoadByWhat.ini",g_szDefWorkDirectory,SETTING_DIR);
	DeleteFile(szRoadByWhatFile);
	ITabFile* pRoadByWhatFile = g_OpenTabFile(szRoadByWhatFile,false,true);	
	if (!pRoadByWhatFile)
		return E_FAIL;
	pRoadByWhatFile->InsertNewCol("RoadID");
	pRoadByWhatFile->InsertNewCol("RidesID");
	int nCurRow = 0;
	map<DWORD,string>::iterator itRoadByWhat = m_mapTrackByWhat.begin();
	while (itRoadByWhat != m_mapTrackByWhat.end())
	{
		pRoadByWhatFile->WriteInteger(nCurRow+2,"RoadID",itRoadByWhat->first);
		pRoadByWhatFile->WriteString(nCurRow+2,"RidesID",itRoadByWhat->second.c_str());
		nCurRow++;
		itRoadByWhat++;
	}
	pRoadByWhatFile->Save(szRoadByWhatFile);
	pRoadByWhatFile->Release();
	return S_OK;
}

// ahpho suntao
// 更新AI字段参数
HRESULT KSceneEditorDocLogical::SetAIParameterValue(int aiSetID, string paraName, int newValue)
{
	AIParameterSet aiSet = m_mapNpcAISceneParameterSet[aiSetID];
	
	vector<AIParameter> paramVector = aiSet.vecParameters;
	for(vector<AIParameter>::iterator it = paramVector.begin(); it != paramVector.end(); it++)
	{		
		AIParameter aiParameter = *it;
		if(paraName == it->strName)
		{
			aiParameter.nValue = newValue;
			break;
		}
	}
	
	return S_OK;
}

// 创建仇恨组
HRESULT KSceneEditorDocLogical::AddOneNpcHatredGroup(BSTR groupName)
{	
	string strGroupName = BSTR_TO_STRING(groupName);
	m_npcHatredGroupMap[m_npcHatredGroupIndex] = strGroupName;
	m_npcHatredGroupIndex++;

	return S_OK;
}

// 加载npc仇恨组信息
HRESULT KSceneEditorDocLogical::LoadNpcHatredGroups(IIniFile *pFile)
{	
	INT nCount = 0;
	pFile->GetInteger("MAIN", "NumNpcHatredGroup", 0, &nCount);

	for (int i = 0; i < nCount; i++)
	{
		int groupID = i + 1;
		TCHAR szSectionName[MAX_PATH];
		wsprintf(szSectionName, "HatredGroup%d", i);

		TCHAR szGroupName[MAX_PATH];		
		pFile->GetString(szSectionName, "Name", "", szGroupName, MAX_PATH);
				
		string strGroupName(szGroupName);
		m_npcHatredGroupMap[groupID] = strGroupName;
		
		if (groupID >= m_npcHatredGroupIndex)
		{
			m_npcHatredGroupIndex = groupID + 1;
		}
	}

	return S_OK;	
}

// 保存npc仇恨组
HRESULT KSceneEditorDocLogical::SaveNpcHatredGroups(IIniFile* pFile)
{
	pFile->WriteInteger("MAIN", "NumNpcHatredGroup", m_npcHatredGroupMap.size());
	
	// 重新分配仇恨组的ID
	INT i = 0;
	for (map<INT, string>::iterator it = m_npcHatredGroupMap.begin(); it != m_npcHatredGroupMap.end(); it++)
	{
		int newID = i + 1;
		TCHAR Group[MAX_PATH];
		wsprintf(Group, "HatredGroup%d", i);

		INT groupID = it->first;
		string groupName = it->second;		
		pFile->WriteString(Group, "Name", groupName.c_str());
		
		// 更新npc的仇恨组信息
		for (list<NPCInfo*>::iterator iter = m_listNPCInfo.begin(); iter != m_listNPCInfo.end(); iter++)
		{
			NPCInfo* npcInfo = *iter;
			if (npcInfo->dwNpcTeamID == groupID)
			{
				npcInfo->dwNpcTeamID = newID;
			}
		}

		i++;
	}	

	return S_OK;
}

// 保存npcAI参数
HRESULT KSceneEditorDocLogical::SaveNpcAIParameters()
{
	TCHAR charArray[MAX_PATH];
	wsprintf(charArray, "%s\\settings\\map\\%s\\NpcAIParameter\\NpcAIParameterList.tab", g_szDefWorkDirectory, m_SceneName);
	string filePath(charArray);

	// 先删除文件
    DeleteFile(filePath.c_str());	
	ITabFile* pFile = g_OpenTabFile(filePath.c_str(), true, true);
	
	for (map<DWORD,AIParameterSet>::iterator it = m_mapNpcAISceneParameterSet.begin(); it != m_mapNpcAISceneParameterSet.end(); it++)
	{		
		
	}

	pFile->Save(filePath.c_str());
	pFile->Release();

	return S_OK;
}
// end ahpho
HRESULT KSceneEditorDocLogical::SaveLogicalPolyIniFile(IIniFile* pFile,LPCTSTR pSectionName,IEKG3DRepresentPoly* pPoly)
{
	pFile->WriteString(pSectionName,"szFileName",pPoly->GetScriptFileName());
	pFile->WriteString(pSectionName,"szPolyName",pPoly->GetPolyName());
	pFile->WriteString(pSectionName,"szLogicalSetName",pPoly->GetLogicalSetName());
	pFile->WriteInteger(pSectionName,"nLogicalSetID",pPoly->GetLogicalSetID());
	pFile->WriteInteger(pSectionName,"dwModifyState",pPoly->GetModifyState());
	pFile->WriteInteger(pSectionName,"Color",pPoly->GetColor());
	pFile->WriteInteger(pSectionName,"NumVectex",(int)pPoly->GetVertexCount());
	pFile->WriteFloat(pSectionName,"fHeight",pPoly->GetVertexHeight());

	for (int i=0;i<pPoly->GetVertexCount();i++)
	{
		TCHAR Name[MAX_PATH];
		wsprintf(Name,"Vertex%d",i);
		pFile->WriteMultiFloat(pSectionName,Name,(float*)&pPoly->GetVertex(i),3);
	}

	return S_OK;
}

HRESULT KSceneEditorDocLogical::SaveAIGroupToIniFile(IIniFile* pFile,LPCTSTR pSectionName,AIGroup* pAI)
{
	pFile->WriteString(pSectionName,"szName",pAI->szName.c_str());
	pFile->WriteString(pSectionName,"szFileName",pAI->szFileName.c_str());
	pFile->WriteInteger(pSectionName,"dwSetID",(int)pAI->dwSetID);
	pFile->WriteInteger(pSectionName,"nKing",pAI->nKing);
	pFile->WriteInteger(pSectionName,"nArrayIndex",pAI->nArrayIndex);
	pFile->WriteInteger(pSectionName,"NumNPC",(int)pAI->vecNpcInfo.size());

	for (int i=0;i<(int)pAI->vecNpcInfo.size();i++)
	{
		NPCInfo* pNpc = pAI->vecNpcInfo[i];

		TCHAR keyName[MAX_PATH];
		wsprintf(keyName,"NPC%dIndex",i);
		pFile->WriteInteger(pSectionName,keyName,pNpc->nIndex);
	}

	if(pAI->pWayPointSet)
		pFile->WriteInteger(pSectionName,"WayPointSetID",(int)pAI->pWayPointSet->dwSetID);
	else
		pFile->WriteInteger(pSectionName,"WayPointSetID",-1);

	// ahpho suntao
	pFile->WriteInteger(pSectionName, "npcOrderID", pAI->npcOrderID);
	pFile->WriteInteger(pSectionName, "reviveTeammate", pAI->reviveTeammate);
	pFile->WriteString(pSectionName, "npcOrder", pAI->npcOrder.c_str());
	// end ahpho

	return S_OK;
}

HRESULT KSceneEditorDocLogical::SaveDoodadRefreshPointSetToIniFile(IIniFile* pFile,LPCTSTR pSectionName,DoodadRefreshPointSet* pRefreshSet)
{
	pFile->WriteString(pSectionName,"szName",pRefreshSet->szName.c_str());
	pFile->WriteInteger(pSectionName,"dwSetID",(int)pRefreshSet->dwSetID);
	pFile->WriteInteger(pSectionName,"NumPoint",(int)pRefreshSet->listRefreshPoint.listRefreshPoint.size());
	pFile->WriteInteger(pSectionName,"NumDoodad",(int)pRefreshSet->listDoodadInfo.size());

	{
		int i = 0;
		list<RefreshPoint*>::iterator j = pRefreshSet->listRefreshPoint.listRefreshPoint.begin();
		while (j!=pRefreshSet->listRefreshPoint.listRefreshPoint.end())
		{
			RefreshPoint* pPoint = *j;

			TCHAR keyName[MAX_PATH];
			wsprintf(keyName,"Point%dvPosition",i);
			pFile->WriteMultiFloat(pSectionName,keyName,(float*)&pPoint->vPosition,3);
			wsprintf(keyName,"Point%dvRotation",i);
			pFile->WriteMultiFloat(pSectionName,keyName,(float*)&pPoint->vRotation,4);
			wsprintf(keyName,"Point%dnX",i);
			pFile->WriteInteger(pSectionName,keyName,pPoint->nX);
			wsprintf(keyName,"Point%dnY",i);
			pFile->WriteInteger(pSectionName,keyName,pPoint->nY);
			wsprintf(keyName,"Point%dnZ",i);
			pFile->WriteInteger(pSectionName,keyName,pPoint->nZ);
			wsprintf(keyName,"Point%dnDirection",i);
			pFile->WriteInteger(pSectionName,keyName,pPoint->nDirection);

			j++;
			i++;
		}
	}
	{
		int i = 0;
		list<DoodadInfo*>::iterator j = pRefreshSet->listDoodadInfo.begin();
		while (j!=pRefreshSet->listDoodadInfo.end())
		{
			DoodadInfo* pDoodad = *j;

			TCHAR keyName[MAX_PATH];
			wsprintf(keyName,"Doodad%dIndex",i);
			pFile->WriteInteger(pSectionName,keyName,pDoodad->nIndex);

			j++;
			i++;
		}
	}
	return S_OK;
}

HRESULT KSceneEditorDocLogical::SaveNPCRefreshPointSetToIniFile(IIniFile* pFile,LPCTSTR pSectionName,NPCRefreshPointSet* pRefreshSet)
{
	pFile->WriteString(pSectionName,"szName",pRefreshSet->szName.c_str());
	pFile->WriteInteger(pSectionName,"dwSetID",(int)pRefreshSet->dwSetID);
	pFile->WriteInteger(pSectionName,"NumPoint",(int)pRefreshSet->listRefreshPoint.listRefreshPoint.size());
	pFile->WriteInteger(pSectionName,"NumNpc",(int)pRefreshSet->listNPCInfo.size());

	{
		int i = 0;
		list<RefreshPoint*>::iterator j = pRefreshSet->listRefreshPoint.listRefreshPoint.begin();
		while (j!=pRefreshSet->listRefreshPoint.listRefreshPoint.end())
		{
			RefreshPoint* pPoint = *j;

			TCHAR keyName[MAX_PATH];
			wsprintf(keyName,"Point%dvPosition",i);
			pFile->WriteMultiFloat(pSectionName,keyName,(float*)&pPoint->vPosition,3);
			wsprintf(keyName,"Point%dvRotation",i);
			pFile->WriteMultiFloat(pSectionName,keyName,(float*)&pPoint->vRotation,4);
			wsprintf(keyName,"Point%dnX",i);
			pFile->WriteInteger(pSectionName,keyName,pPoint->nX);
			wsprintf(keyName,"Point%dnY",i);
			pFile->WriteInteger(pSectionName,keyName,pPoint->nY);
			wsprintf(keyName,"Point%dnZ",i);
			pFile->WriteInteger(pSectionName,keyName,pPoint->nZ);
			wsprintf(keyName,"Point%dnDirection",i);
			pFile->WriteInteger(pSectionName,keyName,pPoint->nDirection);

			j++;
			i++;
		}
	}
	{
		int i = 0;
		list<NPCInfo*>::iterator j = pRefreshSet->listNPCInfo.begin();
		while (j!=pRefreshSet->listNPCInfo.end())
		{
			NPCInfo* pNpc = *j;

			TCHAR keyName[MAX_PATH];
			wsprintf(keyName,"NPC%dIndex",i);
			pFile->WriteInteger(pSectionName,keyName,pNpc->nIndex);

			j++;
			i++;
		}
	}
	return S_OK;
}

HRESULT KSceneEditorDocLogical::SaveWayPointSetToIniFile(IIniFile* pFile,LPCTSTR pSectionName,WayPointSet* pWayPointSet)
{
	pFile->WriteString(pSectionName,"szName",pWayPointSet->szName.c_str());
	pFile->WriteInteger(pSectionName,"dwSetID",(int)pWayPointSet->dwSetID);
	//////////add by huangjinshou 2008-11-17//////////////////////////
	pFile->WriteInteger(pSectionName,"nOrderID",pWayPointSet->nOrderID);
	//////////////////////////////////////////////////////////////////
	pFile->WriteInteger(pSectionName,"NumPoint",(int)pWayPointSet->vectorWayPoint.size());

	for (int i=0;i<(int)pWayPointSet->vectorWayPoint.size();i++)
	{
		WayPoint* pPoint = pWayPointSet->vectorWayPoint[i];

		TCHAR keyName[MAX_PATH];
		wsprintf(keyName,"Point%dvPosition",i);
		pFile->WriteMultiFloat(pSectionName,keyName,(float*)&pPoint->vPosition,3);
		wsprintf(keyName,"Point%dvRotation",i);
		pFile->WriteMultiFloat(pSectionName,keyName,(float*)&pPoint->vRotation,4);
		wsprintf(keyName,"Point%dnX",i);
		pFile->WriteInteger(pSectionName,keyName,pPoint->nX);
		wsprintf(keyName,"Point%dnY",i);
		pFile->WriteInteger(pSectionName,keyName,pPoint->nY);
		wsprintf(keyName,"Point%dnZ",i);
		pFile->WriteInteger(pSectionName,keyName,pPoint->nZ);
		wsprintf(keyName,"Point%dnStayFrame",i);
		pFile->WriteInteger(pSectionName,keyName,pPoint->nStayFrame);
		wsprintf(keyName,"Point%dnStayDirection",i);
		pFile->WriteInteger(pSectionName,keyName,pPoint->nStayDirection);
		wsprintf(keyName,"Point%dnStayAnimation",i);
		pFile->WriteInteger(pSectionName,keyName,pPoint->nStayAnimation);
		wsprintf(keyName,"Point%dbStayAniLoop",i);
		pFile->WriteInteger(pSectionName,keyName,(int)pPoint->bStayAniLoop);
		wsprintf(keyName,"Point%dbIsRun",i);
		pFile->WriteInteger(pSectionName,keyName,(int)pPoint->bIsRun);
		wsprintf(keyName,"Point%dszScriptName",i);
		pFile->WriteString(pSectionName,keyName,pPoint->szScriptName.c_str());
		wsprintf(keyName,"Point%dszInfo",i);
		pFile->WriteString(pSectionName,keyName,pPoint->szInfo.c_str());
	}
	return S_OK;
}
HRESULT KSceneEditorDocLogical::SaveTrafficPointToIniFile(IIniFile* pFile,LPCTSTR pSectionName,TrafficPoint* pTrafficPoint)
{
	pFile->WriteInteger(pSectionName,"nType",0);
	pFile->WriteMultiFloat(pSectionName,"vPosition",(float*)&pTrafficPoint->vPosition,3);
	pFile->WriteInteger(pSectionName,"nLinkID",(int)pTrafficPoint->m_nLinkID);
	pFile->WriteInteger(pSectionName,"nID",(int)pTrafficPoint->m_dwNodeID);
	pFile->WriteInteger(pSectionName,"nMapID",(int)pTrafficPoint->m_dwMapID);
	pFile->WriteInteger(pSectionName,"nCityID",(int)pTrafficPoint->m_dwCityID);
	pFile->WriteInteger(pSectionName,"nForceID",(int)pTrafficPoint->m_dwForceID);
	pFile->WriteInteger(pSectionName,"nTongID",(int)pTrafficPoint->m_dwTongID);
	pFile->WriteInteger(pSectionName,"bNeedOpen",(int)pTrafficPoint->m_bNeedOpen);		
	pFile->WriteString(pSectionName,"szScriptName",pTrafficPoint->m_strScriptFile.c_str());
	pFile->WriteString(pSectionName,"szDescription",pTrafficPoint->m_strDescription.c_str());
	return S_OK;
}

HRESULT KSceneEditorDocLogical::SaveTrafficPointSetToIniFile(IIniFile* pFile,LPCTSTR pSectionName,TrafficPointSet* pTrafficPointSet)
{
	pFile->WriteString(pSectionName,"szName",pTrafficPointSet->szName.c_str());
	pFile->WriteInteger(pSectionName,"dwSetID",(int)pTrafficPointSet->m_dwID);
	pFile->WriteInteger(pSectionName,"NumPoint",(int)pTrafficPointSet->vecTrafficPoint.size());
	pFile->WriteInteger(pSectionName,"CostMoney",(int)pTrafficPointSet->m_dwCostMoney);
	pFile->WriteInteger(pSectionName,"Velocity",(int)pTrafficPointSet->m_dwVelocity);
	pFile->WriteInteger(pSectionName,"Step",(int)pTrafficPointSet->m_dwStep);
	pFile->WriteInteger(pSectionName,"FromID",(int)pTrafficPointSet->m_dwFromID);
	pFile->WriteInteger(pSectionName,"ToID",(int)pTrafficPointSet->m_dwToID);

	for (int i=0;i<(int)pTrafficPointSet->vecTrafficPoint.size();i++)
	{
		TrafficLittlePoint* pPoint = pTrafficPointSet->vecTrafficPoint[i];

		TCHAR keyName[MAX_PATH];
		wsprintf(keyName,"Point%dvPosition",i);
		pFile->WriteMultiFloat(pSectionName,keyName,(float*)&pPoint->vPosition,3);
	}
	return S_OK;
}

HRESULT KSceneEditorDocLogical::SaveOverMapTrafficPointToIniFile(IIniFile* pFile)
{
	TCHAR szSectionName[MAX_PATH];


	pFile->WriteInteger("MAIN","NumPoint",(int)m_mapLinkAbleList.size());
	pFile->WriteInteger("MAIN","NumTrack",(int)m_listOverMapPointSet.size());

	int S = 0;
	map<DWORD,string>::iterator i = m_mapLinkAbleList.begin();
	while (i!=m_mapLinkAbleList.end())
	{

		wsprintf(szSectionName,"RoadPoint%d",S);
		pFile->WriteInteger(szSectionName,"ID",(int)(i->first));
		pFile->WriteString(szSectionName,"Name",(i->second).c_str());
		S++;
		i++;
	}

	S = 0;
	list<TrafficPointSet*>::iterator iSet = m_listOverMapPointSet.begin();
	while (iSet != m_listOverMapPointSet.end())
	{	
		TrafficPointSet* pTrafficPointSet = *iSet;
		wsprintf(szSectionName,"RoadTrack%d",S);
		pFile->WriteInteger(szSectionName,"dwSetID",(int)pTrafficPointSet->m_dwID);
		pFile->WriteInteger(szSectionName,"CostMoney",(int)pTrafficPointSet->m_dwCostMoney);
		pFile->WriteInteger(szSectionName,"Velocity",(int)pTrafficPointSet->m_dwVelocity);
		pFile->WriteInteger(szSectionName,"FromID",(int)pTrafficPointSet->m_dwFromID);
		pFile->WriteInteger(szSectionName,"ToID",(int)pTrafficPointSet->m_dwToID);
		S++;
		iSet++;
	}
	return S_OK;
}

HRESULT KSceneEditorDocLogical::SaveDoodadToIniFile(IIniFile* pFile,LPCTSTR pSectionName,DoodadInfo* pDoodad)
{
	pFile->WriteString(pSectionName,"szName",pDoodad->szName.c_str());
	pFile->WriteString(pSectionName,"szNickName",pDoodad->szNickName.c_str());
	pFile->WriteString(pSectionName,"szScriptName",pDoodad->szScriptName.c_str());

	pFile->WriteInteger(pSectionName,"nIndex",pDoodad->nIndex);
	pFile->WriteInteger(pSectionName,"nTempleteID",pDoodad->nTempleteID);
	pFile->WriteInteger(pSectionName,"nX",pDoodad->nX);
	pFile->WriteInteger(pSectionName,"nY",pDoodad->nY);
	pFile->WriteInteger(pSectionName,"nZ",pDoodad->nZ);
	pFile->WriteInteger(pSectionName,"nKind",pDoodad->nKind);
	pFile->WriteInteger(pSectionName,"nFaceDirection",pDoodad->nFaceDirection);
	//pFile->WriteInteger(pSectionName,"nAIType",pDoodad->nAIType);
    //pFile->WriteInteger(pSectionName, "nAIMainState", pDoodad->nAIMainState);
    pFile->WriteStruct(pSectionName,"dwScriptID",(void*)&(pDoodad->dwScriptID), sizeof(DWORD));

	pFile->WriteMultiFloat(pSectionName,"vPosition",(float*)&pDoodad->vPosition,3);
	pFile->WriteMultiFloat(pSectionName,"vRotation",(float*)&pDoodad->vRotation,4);

	pFile->WriteInteger(pSectionName, "ReliveID", pDoodad->dwDoodadReliveID);

	return S_OK;

}

HRESULT KSceneEditorDocLogical::SaveNPCToIniFile(IIniFile* pFile,LPCTSTR pSectionName,NPCInfo* pNpc)
{
	pFile->WriteString(pSectionName,"szName",pNpc->szName.c_str());
	pFile->WriteString(pSectionName,"szNickName",pNpc->szNickName.c_str());
	pFile->WriteString(pSectionName,"szScriptName",pNpc->szScriptName.c_str());

	pFile->WriteInteger(pSectionName,"nIndex",pNpc->nIndex);
	pFile->WriteInteger(pSectionName,"nTempleteID",pNpc->nTempleteID);
	pFile->WriteInteger(pSectionName,"nX",pNpc->nX);
	pFile->WriteInteger(pSectionName,"nY",pNpc->nY);
	pFile->WriteInteger(pSectionName,"nZ",pNpc->nZ);
	pFile->WriteInteger(pSectionName,"nKind",pNpc->nKind);
	pFile->WriteInteger(pSectionName,"nFaceDirection",pNpc->nFaceDirection);
    //pFile->WriteInteger(pSectionName, "nDisableAutoTurn", pNpc->nDisableAutoTurn);
	//pFile->WriteInteger(pSectionName,"nAIType",pNpc->nAIType);
    //pFile->WriteInteger(pSectionName, "nAIMainState", pNpc->nAIMainState);
	pFile->WriteInteger(pSectionName, "nLevel", pNpc->nLevel);
	//pFile->WriteInteger(pSectionName,"nAlertRange",pNpc->nAlertRange);
	//pFile->WriteInteger(pSectionName, "nAttackRange", pNpc->nAttackRange);
    pFile->WriteStruct(pSectionName,"dwScriptID",(void*)&(pNpc->dwScriptID), sizeof(DWORD));
    pFile->WriteStruct(pSectionName,"dwRepresentID",(void*)&(pNpc->dwRepresentID), sizeof(DWORD));
    

	pFile->WriteMultiFloat(pSectionName,"vPosition",(float*)&pNpc->vPosition,3);
	pFile->WriteMultiFloat(pSectionName,"vRotation",(float*)&pNpc->vRotation,4);

	if(pNpc->pWayPointSet)
		pFile->WriteInteger(pSectionName,"WayPointSetID",(int)pNpc->pWayPointSet->dwSetID);
	else
		pFile->WriteInteger(pSectionName,"WayPointSetID",-1);

	pFile->WriteInteger(pSectionName, "ReliveID", pNpc->dwNpcReliveID);
	pFile->WriteInteger(pSectionName, "RandomID", pNpc->dwNpcRandomID);
	pFile->WriteInteger(pSectionName, "AISetID" , pNpc->dwNpcAISet);

	// ahpho suntao
	pFile->WriteInteger(pSectionName, "TeamID", pNpc->dwNpcTeamID);
	// end ahpho

	return S_OK;
}



HRESULT KSceneEditorDocLogical::LoadFromIniFile(LPCTSTR pFileName,FUNC_LOADPROGRESS pProgress,BOOL bAddToScene)
{
	TCHAR szSectionName[MAX_PATH];

	int nNumNPC = 0;
	int nNumDoodad = 0;
	int nNumNpcRefreshSet = 0;
	int nNumNpcRefreshSetMaxID = 0;
	int nNumDoodadRefreshSet = 0;
	int nNumDoodadRefreshSetMaxID = 0;
	int nNumAIGroup = 0;
	int nNumAIGroupMaxID = 0;
	int nNumWapPointSet = 0;
	int nNumWapPointSetMaxID = 0;
	int nNumLogicalPoly = 0;
	int nNumTrafficPointSet = 0;
	int nNumTrafficPoint = 0;
	int nNumTrafficPointSetMaxID = 0;

	// ahpho suntao 获取当前的场景名
	string fileName(pFileName);
	int splitIndex = fileName.find_last_of("\\");
	m_SceneName = fileName.substr(splitIndex, fileName.length() - splitIndex);
	// end ahpho

	IIniFile* pFile = g_OpenIniFile(pFileName,false,false);
	if (!pFile)
		return E_FAIL;	
	//////////////////////////////////////////////////////////////////////////
	//MAIN
	pFile->GetInteger("MAIN","NumNPC",0,&nNumNPC);
	pFile->GetInteger("MAIN","NumDoodad",0,&nNumDoodad);

	pFile->GetInteger("MAIN","NumNpcRefreshSet",0,&nNumNpcRefreshSet);
	pFile->GetInteger("MAIN","NumNpcRefreshSetMaxID",0,&nNumNpcRefreshSetMaxID);
	pFile->GetInteger("MAIN","NumDoodadRefreshSet",0,&nNumDoodadRefreshSet);
	pFile->GetInteger("MAIN","NumDoodadRefreshSetMaxID",0,&nNumDoodadRefreshSetMaxID);
	pFile->GetInteger("MAIN","NumAIGroup",0,&nNumAIGroup);
	pFile->GetInteger("MAIN","NumAIGroupMaxID",0,&nNumAIGroupMaxID);

	pFile->GetInteger("MAIN","NumWapPointSet",0,&nNumWapPointSet);
	pFile->GetInteger("MAIN","NumWapPointSetMaxID",0,&nNumWapPointSetMaxID);

	pFile->GetInteger("MAIN","LogicalSceneID",0,&m_nSceneID);
	pFile->GetFloat("MAIN","LogicalSceneXStart",0,&m_fLogicalXStart);
	pFile->GetFloat("MAIN","LogicalSceneZStart",0,&m_fLogicalZStart);

	pFile->GetInteger("MAIN","NumLogicalPoly",0,&nNumLogicalPoly);

	//pFile->GetInteger("MAIN","NumTrafficPointSet",0,&nNumTrafficPointSet);
	//pFile->GetInteger("MAIN","NumTrafficPointSetMaxID",0,&nNumTrafficPointSetMaxID);
	pFile->GetInteger("MAIN","NumTrafficPointSet",0,&nNumTrafficPointSet);
	pFile->GetInteger("MAIN","NumTrafficPoint",0,&nNumTrafficPoint);		

	// 把 ai 提前
	LoadAISetFromFile(pFile);

	//////////////////////////////////////////////////////////////////////////
	//WayPointSet
	{
		for (int S=0;S<nNumWapPointSet;S++)
		{
			wsprintf(szSectionName,"WayPointSet%d",S);
			WayPointSet* pWayPointSet = NULL;

			AddOneWayPointSet(&pWayPointSet,"");

			LoadWayPointSetFromIniFile(pFile,szSectionName,pWayPointSet);

			if(pProgress)
			{
				(*pProgress)("Load WayPointSet",S * 1.0F / nNumWapPointSet);
			}
		}
		m_dwCurrentWayPointSetID = (DWORD) nNumWapPointSetMaxID;
	}
	//////////////////////////////////////////////////////////////////////////
	//NPC
	{
		for (int S=0;S<nNumNPC;S++)
		{
			wsprintf(szSectionName,"NPC%d",S);
			NPCInfo* pNpcInfo = NULL;

			D3DXVECTOR3 vPos;
			D3DXQUATERNION vRot;

			AddOneNPC(&pNpcInfo,"",vPos,vRot,0,0,NULL,0,0,/*0,*/0,0,0,/*0,*/0,0,0);

			LoadNPCFromIniFile(pFile,szSectionName,pNpcInfo);

			if(pProgress)
			{
				(*pProgress)("Load NPC",S * 1.0F / nNumNPC);
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////
	//Doodad
	{
		for (int S=0;S<nNumDoodad;S++)
		{
			wsprintf(szSectionName,"Doodad%d",S);
			DoodadInfo* pDoodad = NULL;

			D3DXVECTOR3 vPos;
			D3DXQUATERNION vRot;

			AddOneDoodad(&pDoodad,"",vPos,vRot,0,0,NULL,0);

			LoadDoodadFromIniFile(pFile,szSectionName,pDoodad);

			if(pProgress)
			{
				(*pProgress)("Load Doodad",S * 1.0F / nNumDoodad);
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////
	//AIGroup
	{
		for (int S=0;S<nNumAIGroup;S++)
		{
			wsprintf(szSectionName,"AIGroup%d",S);
			AIGroup* pAI = NULL;

			AddOneAIGroup(&pAI,"");

			LoadAIGroupFromIniFile(pFile,szSectionName,pAI);

			if(pProgress)
			{
				(*pProgress)("Load AIGroup",S * 1.0F / nNumAIGroup);
			}
		}
		m_dwCurrentAIGroupID = (DWORD) nNumAIGroupMaxID;
	}
	//////////////////////////////////////////////////////////////////////////
	//Logical Poly
	{
		for (int S=0;S<nNumLogicalPoly;S++)
		{
			wsprintf(szSectionName,"LogicalPoly%d",S);
			LoadLogicalPolyIniFile(pFile,szSectionName,bAddToScene);

			if(pProgress)
			{
				(*pProgress)("Load LogicalPoly",S * 1.0F / nNumLogicalPoly);
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////
	//Traffic Point Set
	{
		for (int S=0;S<nNumTrafficPoint;S++)
		{
			wsprintf(szSectionName,"TrafficPoint%d",S);
			TrafficPointSet* pTrafficPointSet = NULL;

			//AddOneTrafficPointSet(&pTrafficPointSet,"");

			LoadTrafficPointFromIniFile(pFile,szSectionName);	
			//pTrafficPointSet->m_dwID = S;
			if(pProgress)
			{
				(*pProgress)("Load TrafficPoint",S * 1.0F / nNumTrafficPoint);
			}
		}
		//m_dwCurrentTrafficPointSetID = (DWORD)nNumTrafficPointSet;
	}
	//////////////////////////////////////////////////////////////////////////
	//Traffic Point Set
	{
		for (int S=0;S<nNumTrafficPointSet;S++)
		{
			wsprintf(szSectionName,"TrafficPointSet%d",S);
			TrafficPointSet* pTrafficPointSet = NULL;

			AddOneTrafficPointSet(&pTrafficPointSet,0,"");
			LoadTrafficPointSetFromIniFile(pFile,szSectionName,pTrafficPointSet);	
			if(pProgress)
			{
				(*pProgress)("Load TrafficPointSet",S * 1.0F / nNumTrafficPointSet);
			}
		}
		//m_dwCurrentTrafficPointSetID = (DWORD)nNumTrafficPointSet;
	}
	//////////////////////////////////////////////////////////////////////////
	LoadNpcReviveGroups(pFile);

	LoadDoodadReviveGroups(pFile);

	LoadNpcRandomGroups(pFile);

//	LoadAISetFromFile(pFile);
	// ahpho suntao
	// 加载npc仇恨组信息
	LoadNpcHatredGroups(pFile);

	// 初始化npc索引
	m_npcIndex = nNumNPC;
	// end ahpho

	pFile->Release();

	TCHAR szRoadFile[MAX_PATH];
	wsprintf(szRoadFile,"%s%s\\Road\\RoadPointOverMaps.ini",g_szDefWorkDirectory,SETTING_DIR);
	IIniFile* pRoadFile = g_OpenIniFile(szRoadFile,false,false);
	if (!pRoadFile)
		return E_FAIL;
	LoadOverMapTrafficPointToIniFile(pRoadFile);
	pRoadFile->Release();

	TCHAR szRoadByWhatFile[MAX_PATH];
	char szReadBuffer[MAX_PATH];
	wsprintf(szRoadByWhatFile,"%s%s\\Road\\RoadByWhat.ini",g_szDefWorkDirectory,SETTING_DIR);
	ITabFile* pRoadByWhatFile = g_OpenTabFile(szRoadByWhatFile,false,false);	
	if (!pRoadByWhatFile)
		return E_FAIL;
	int nRows = pRoadByWhatFile->GetHeight();
	for (int nRow = 2; nRow <= nRows; nRow++)
	{
		int nIndex = 0;
		pRoadByWhatFile->GetInteger(nRow,"RoadID",0,&nIndex);
		pRoadByWhatFile->GetString(nRow,"RidesID","",szReadBuffer,MAX_PATH);
		m_mapTrackByWhat[nIndex] = szReadBuffer;
	}


	pRoadByWhatFile->Release();

	//////////////////////////////////////////////////////////////////////////
	UpdataNpcAIParameters();
	return S_OK;
}


HRESULT KSceneEditorDocLogical::LoadLogicalPolyIniFile(IIniFile* pFile,LPCTSTR pSectionName,BOOL bAddToScene)
{
	D3DXVECTOR3* vPos = NULL;
	TCHAR szFileName[MAX_PATH];
	TCHAR szPolyName[MAX_PATH];
	TCHAR szLogicalSetName[MAX_PATH];
	int nLogicalSetIndex = 0;
	DWORD dwModifyState = 0;
	DWORD dwColor = 0;
	int nNumVertex = 0;
	float fHeight = 0;

	pFile->GetString(pSectionName,"szFileName","",szFileName,MAX_PATH);
	pFile->GetString(pSectionName,"szPolyName","",szPolyName,MAX_PATH);
	pFile->GetString(pSectionName,"szLogicalSetName","",szLogicalSetName,MAX_PATH);//add by huangjinshou 2008-12-03
	pFile->GetInteger(pSectionName,"nLogicalSetID",0,&nLogicalSetIndex);
	pFile->GetInteger(pSectionName,"dwModifyState",0,(int*)&dwModifyState);
	pFile->GetInteger(pSectionName,"Color",0,(int*)&dwColor);
	pFile->GetInteger(pSectionName,"NumVectex",0,&nNumVertex);
	pFile->GetFloat(pSectionName,"fHeight",0,&fHeight);

	vPos = new D3DXVECTOR3[nNumVertex];
	ASSERT(vPos);
	for (int i=0;i<nNumVertex;i++)
	{
		TCHAR Name[MAX_PATH];
		wsprintf(Name,"Vertex%d",i);
		pFile->GetMultiFloat(pSectionName,Name,(float*)&vPos[i],3);
	}

	
	m_lpSceneSceneEditor->IELoadPoly(szFileName,szPolyName,dwColor,szLogicalSetName,nLogicalSetIndex,dwModifyState,fHeight,nNumVertex,vPos);
	SAFE_DELETE_ARRAY(vPos);
	return S_OK;
}

HRESULT KSceneEditorDocLogical::LoadAIGroupFromIniFile(IIniFile* pFile,LPCTSTR pSectionName,AIGroup* pAI)
{
	string szName;
	string szFileName;

	vector<NPCInfo*>vecNpcInfo;
	TCHAR str[MAX_PATH];

	pFile->GetString(pSectionName,"szName","",str,MAX_PATH);
	pAI->szName = str;
	pFile->GetString(pSectionName,"szFileName","",str,MAX_PATH);
	pAI->szFileName = str;
	int nSetID = 0;
	pFile->GetInteger(pSectionName,"dwSetID",0,&nSetID);
	pAI->dwSetID = (int)nSetID;
	pFile->GetInteger(pSectionName,"nKing",0,&pAI->nKing);
	pFile->GetInteger(pSectionName,"nArrayIndex",0,&pAI->nArrayIndex);
	int nNumNpc = 0;
	pFile->GetInteger(pSectionName,"NumNPC",0,&nNumNpc);

	for (int i=0;i<nNumNpc;i++)
	{
		TCHAR keyName[MAX_PATH];
		wsprintf(keyName,"NPC%dIndex",i);
		int nIndex = 0;
		pFile->GetInteger(pSectionName,keyName,-1,&nIndex);

		NPCInfo* pNpc = NULL;
		if(SUCCEEDED(FindNpcInfoByIndex(&pNpc,nIndex)))
		{
			pAI->vecNpcInfo.push_back(pNpc);
		}
	}

	int WayPointSetID = 0;
	pFile->GetInteger(pSectionName,"WayPointSetID",-1,&WayPointSetID);

	if(WayPointSetID==-1)
	{
		pAI->pWayPointSet = NULL;
	}
	else
	{
		GetWayPointSetByID(&pAI->pWayPointSet,WayPointSetID);
	}

	// ahpho suntao
	int npcOrderID = 0;
	pFile->GetInteger(pSectionName, "npcOrderID", 0, &npcOrderID);
	pAI->npcOrderID = npcOrderID;	
	pFile->GetString(pSectionName, "npcOrder", "", str, MAX_PATH);;
	string npcOrder(str);
	pAI->npcOrder = npcOrder;
	int reviveTeammate = 0;
	pFile->GetInteger(pSectionName, "reviveTeammate", 0, &reviveTeammate);
	pAI->reviveTeammate = reviveTeammate;
	// end ahpho

	return S_OK;
}

HRESULT KSceneEditorDocLogical::LoadDoodadRefreshPointSetFromIniFile(IIniFile* pFile,LPCTSTR pSectionName,DoodadRefreshPointSet* pRefreshSet)
{
	TCHAR str[MAX_PATH];

	pFile->GetString(pSectionName,"szName","",str,MAX_PATH);
	pRefreshSet->szName = str;
	int nSetID = 0;
	pFile->GetInteger(pSectionName,"dwSetID",0,&nSetID);
	pRefreshSet->dwSetID = (int)nSetID;
	int nNumPoint = 0;
	pFile->GetInteger(pSectionName,"NumPoint",0,&nNumPoint);
	int nNumDoodad = 0;
	pFile->GetInteger(pSectionName,"NumDoodad",0,&nNumDoodad);

	{
		for (int i=0;i<nNumPoint;i++)
		{
			RefreshPoint* pPoint = NULL;

			D3DXVECTOR3 vPos;
			D3DXQUATERNION vRot;

			pRefreshSet->listRefreshPoint.NewOneRefreshPoint(&pPoint,NULL,vPos,vRot);

			TCHAR keyName[MAX_PATH];
			wsprintf(keyName,"Point%dvPosition",i);
			pFile->GetMultiFloat(pSectionName,keyName,(float*)&pPoint->vPosition,3);
			wsprintf(keyName,"Point%dvRotation",i);
			pFile->GetMultiFloat(pSectionName,keyName,(float*)&pPoint->vRotation,4);
			wsprintf(keyName,"Point%dnX",i);
			pFile->GetInteger(pSectionName,keyName,0,&pPoint->nX);
			wsprintf(keyName,"Point%dnY",i);
			pFile->GetInteger(pSectionName,keyName,0,&pPoint->nY);
			wsprintf(keyName,"Point%dnZ",i);
			pFile->GetInteger(pSectionName,keyName,0,&pPoint->nZ);
			wsprintf(keyName,"Point%dnDirection",i);
			pFile->GetInteger(pSectionName,keyName,0,&pPoint->nDirection);

		}
	}
	{
		for (int i=0;i<nNumDoodad;i++)
		{
			TCHAR keyName[MAX_PATH];
			wsprintf(keyName,"Doodad%dIndex",i);
			int nIndex = 0;
			pFile->GetInteger(pSectionName,keyName,-1,&nIndex);

			DoodadInfo* pDoodad = NULL;
			if(SUCCEEDED(FindDoodadInfoByIndex(&pDoodad,nIndex)))
			{
				pRefreshSet->listDoodadInfo.push_back(pDoodad);
			}
		}
	}

	return S_OK;
}

HRESULT KSceneEditorDocLogical::LoadNPCRefreshPointSetFromIniFile(IIniFile* pFile,LPCTSTR pSectionName,NPCRefreshPointSet* pRefreshSet)
{
	TCHAR str[MAX_PATH];

	pFile->GetString(pSectionName,"szName","",str,MAX_PATH);
	pRefreshSet->szName = str;
	int nSetID = 0;
	pFile->GetInteger(pSectionName,"dwSetID",0,&nSetID);
	pRefreshSet->dwSetID = (int)nSetID;
	int nNumPoint = 0;
	pFile->GetInteger(pSectionName,"NumPoint",0,&nNumPoint);
	int nNumNpc = 0;
	pFile->GetInteger(pSectionName,"NumNpc",0,&nNumNpc);

	{
		for (int i=0;i<nNumPoint;i++)
		{
			RefreshPoint* pPoint = NULL;

			D3DXVECTOR3 vPos;
			D3DXQUATERNION vRot;

			pRefreshSet->listRefreshPoint.NewOneRefreshPoint(&pPoint,NULL,vPos,vRot);

			TCHAR keyName[MAX_PATH];
			wsprintf(keyName,"Point%dvPosition",i);
			pFile->GetMultiFloat(pSectionName,keyName,(float*)&pPoint->vPosition,3);
			wsprintf(keyName,"Point%dvRotation",i);
			pFile->GetMultiFloat(pSectionName,keyName,(float*)&pPoint->vRotation,4);
			wsprintf(keyName,"Point%dnX",i);
			pFile->GetInteger(pSectionName,keyName,0,&pPoint->nX);
			wsprintf(keyName,"Point%dnY",i);
			pFile->GetInteger(pSectionName,keyName,0,&pPoint->nY);
			wsprintf(keyName,"Point%dnZ",i);
			pFile->GetInteger(pSectionName,keyName,0,&pPoint->nZ);
			wsprintf(keyName,"Point%dnDirection",i);
			pFile->GetInteger(pSectionName,keyName,0,&pPoint->nDirection);

		}
	}
	{
		for (int i=0;i<nNumNpc;i++)
		{
			TCHAR keyName[MAX_PATH];
			wsprintf(keyName,"NPC%dIndex",i);
			int nIndex = 0;
			pFile->GetInteger(pSectionName,keyName,-1,&nIndex);

			NPCInfo* pNpc = NULL;
			if(SUCCEEDED(FindNpcInfoByIndex(&pNpc,nIndex)))
			{
				pRefreshSet->listNPCInfo.push_back(pNpc);
			}
		}
	}

	return S_OK;
}

HRESULT KSceneEditorDocLogical::FindNpcInfoByIndex(NPCInfo** ppNpc,int nIndex)
{
	list<NPCInfo*>::iterator i = m_listNPCInfo.begin();
	while (i!=m_listNPCInfo.end())
	{
		NPCInfo* pNpc = *i;
		if(pNpc->nIndex == nIndex)
		{
			(*ppNpc) = pNpc;
			return S_OK;
		}
		i++;
	}
	return E_FAIL;
}

HRESULT KSceneEditorDocLogical::FindDoodadInfoByIndex(DoodadInfo** ppDoodad,int nIndex)
{
	list<DoodadInfo*>::iterator i = m_listDoodadInfo.begin();
	while (i!=m_listDoodadInfo.end())
	{
		DoodadInfo* pDoodad = *i;
		if(pDoodad->nIndex == nIndex)
		{
			(*ppDoodad) = pDoodad;
			return S_OK;
		}
		i++;
	}
	return E_FAIL;
}

HRESULT KSceneEditorDocLogical::LoadWayPointSetFromIniFile(IIniFile* pFile,LPCTSTR pSectionName,WayPointSet* pWayPointSet)
{
	TCHAR str[MAX_PATH];

	pFile->GetString(pSectionName,"szName","",str,MAX_PATH);
	pWayPointSet->szName = str;
	int nSetID = 0;
	pFile->GetInteger(pSectionName,"dwSetID",0,&nSetID);
	pWayPointSet->dwSetID = (int)nSetID;
	//////////add by huangjinshou 2008-11-17//////////////////////////
	int nOrderID = 0;
	pFile->GetInteger(pSectionName,"nOrderID",1,&nOrderID);
	pWayPointSet->nOrderID = nOrderID;
	//////////////////////////////////////////////////////////////////
	int nNumPoint = 0;
	pFile->GetInteger(pSectionName,"NumPoint",0,&nNumPoint);

	for (int i=0;i<nNumPoint;i++)
	{
		WayPoint* pPoint = NULL;

		D3DXVECTOR3 vPos;
		D3DXQUATERNION vRot;
		pWayPointSet->AddWayPoint(&pPoint,vPos,vRot,NULL);

		TCHAR keyName[MAX_PATH];
		wsprintf(keyName,"Point%dvPosition",i);
		pFile->GetMultiFloat(pSectionName,keyName,(float*)&pPoint->vPosition,3);
		wsprintf(keyName,"Point%dvRotation",i);
		pFile->GetMultiFloat(pSectionName,keyName,(float*)&pPoint->vRotation,4);
		wsprintf(keyName,"Point%dnX",i);
		pFile->GetInteger(pSectionName,keyName,0,&pPoint->nX);
		wsprintf(keyName,"Point%dnY",i);
		pFile->GetInteger(pSectionName,keyName,0,&pPoint->nY);
		wsprintf(keyName,"Point%dnZ",i);
		pFile->GetInteger(pSectionName,keyName,0,&pPoint->nZ);
		wsprintf(keyName,"Point%dnStayFrame",i);
		pFile->GetInteger(pSectionName,keyName,0,&pPoint->nStayFrame);
		wsprintf(keyName,"Point%dnStayDirection",i);
		pFile->GetInteger(pSectionName,keyName,0,&pPoint->nStayDirection);
		wsprintf(keyName,"Point%dnStayAnimation",i);
		pFile->GetInteger(pSectionName,keyName,0,&pPoint->nStayAnimation);
		wsprintf(keyName,"Point%dbStayAniLoop",i);
		int nStayAniLoop = 0;
		pFile->GetInteger(pSectionName,keyName,0,&nStayAniLoop);
		pPoint->bStayAniLoop = (BOOL) nStayAniLoop;
		wsprintf(keyName,"Point%dbIsRun",i);
		int nIsRun = 0;
		pFile->GetInteger(pSectionName,keyName,0,&nIsRun);
		pPoint->bIsRun = (BOOL) nIsRun;

		wsprintf(keyName,"Point%dszScriptName",i);
		pFile->GetString(pSectionName,keyName,"",str,MAX_PATH);
		pPoint->szScriptName = str;
		wsprintf(keyName,"Point%dszInfo",i);
		pFile->GetString(pSectionName,keyName,"",str,MAX_PATH);
		pPoint->szInfo = str;
	}
	return S_OK;
}

HRESULT KSceneEditorDocLogical::LoadTrafficPointFromIniFile(IIniFile* pFile,LPCTSTR pSectionName)
{
	DWORD dwID = 0;
	TCHAR str[MAX_PATH];
	pFile->GetInteger(pSectionName,"nID",0,(int*)(&dwID));
	m_mapTrafficPoint[dwID].m_dwNodeID = dwID;
	pFile->GetInteger(pSectionName,"nType",0,(int*)(&(m_mapTrafficPoint[dwID].m_dwType)));
	pFile->GetMultiFloat(pSectionName,"vPosition",(float*)(&(m_mapTrafficPoint[dwID].vPosition)),3);		
	pFile->GetInteger(pSectionName,"nLinkID",-2,(int*)(&(m_mapTrafficPoint[dwID].m_nLinkID)));	
	pFile->GetInteger(pSectionName,"nMapID",0,(int*)(&(m_mapTrafficPoint[dwID].m_dwMapID)));
	pFile->GetInteger(pSectionName,"nCityID",0,(int*)(&(m_mapTrafficPoint[dwID].m_dwCityID)));
	pFile->GetInteger(pSectionName,"nForceID",0,(int*)(&(m_mapTrafficPoint[dwID].m_dwForceID)));
	pFile->GetInteger(pSectionName,"nTongID",0,(int*)(&(m_mapTrafficPoint[dwID].m_dwTongID)));

	int nNeedOpen = 0;
	pFile->GetInteger(pSectionName,"bNeedOpen",0,&nNeedOpen);
	m_mapTrafficPoint[dwID].m_bNeedOpen = (BOOL)nNeedOpen;


	pFile->GetString(pSectionName,"szScriptName","",str,MAX_PATH);
	m_mapTrafficPoint[dwID].m_strScriptFile = str;

	pFile->GetString(pSectionName,"szDescription","",str,MAX_PATH);
	m_mapTrafficPoint[dwID].m_strDescription = str;
	return S_OK;
}

HRESULT KSceneEditorDocLogical::LoadTrafficPointSetFromIniFile(IIniFile* pFile,LPCTSTR pSectionName,TrafficPointSet* pTrafficPointSet)
{
	TCHAR str[MAX_PATH];

	pFile->GetString(pSectionName,"szName","",str,MAX_PATH);
	pTrafficPointSet->szName = str;
	int nSetID = 0;
	pFile->GetInteger(pSectionName,"dwSetID",0,&nSetID);
	pTrafficPointSet->m_dwID = (DWORD)nSetID;
	int nNumPoint = 0;
	pFile->GetInteger(pSectionName,"NumPoint",0,&nNumPoint);
	int nCostMoney = 0;
	pFile->GetInteger(pSectionName,"CostMoney",0,&nCostMoney);
	pTrafficPointSet->m_dwCostMoney = (DWORD)nCostMoney;
	int nVelocity = 0;
	pFile->GetInteger(pSectionName,"Velocity",0,&nVelocity);
	pTrafficPointSet->m_dwVelocity = (DWORD)nVelocity;
	int nStep = 0;
	pFile->GetInteger(pSectionName,"Step",0,&nStep);
    pTrafficPointSet->m_dwStep = (DWORD)nStep;

	int nFromID = 0;
	pFile->GetInteger(pSectionName,"FromID",0,&nFromID);
	pTrafficPointSet->m_dwFromID = (DWORD)nFromID;
	int nToID = 0;
	pFile->GetInteger(pSectionName,"ToID",0,&nToID);
	pTrafficPointSet->m_dwToID = (DWORD)nToID;
	for (int i=0;i<nNumPoint;i++)
	{
		TrafficLittlePoint* pPoint = NULL;

		D3DXVECTOR3 vPos;
		//D3DXQUATERNION vRot;
		pTrafficPointSet->AddTrafficPoint(&pPoint,vPos,NULL);

		TCHAR keyName[MAX_PATH];
		wsprintf(keyName,"Point%dvPosition",i);
		pFile->GetMultiFloat(pSectionName,keyName,(float*)&pPoint->vPosition,3);		
	}
	m_mapTrackByWhat[nSetID] = "";
	return S_OK;
}
//HRESULT KSceneEditorDocLogical::LoadTrafficPointSetFromIniFile(IIniFile* pFile,LPCTSTR pSectionName,TrafficPointSet* pTrafficPointSet)
//{
//	TCHAR str[MAX_PATH];
//	DWORD dwID = 0;
//	TCHAR keyName[MAX_PATH];
//	int nNeedOpen = 0;
//
//	pFile->GetString(pSectionName,"szName","",str,MAX_PATH);
//	pTrafficPointSet->szName = str;
//	int nSetID = 0;
//	pFile->GetInteger(pSectionName,"dwSetID",0,&nSetID);
//	pTrafficPointSet->m_dwID = (DWORD)nSetID;
//	int nNumPoint = 0;
//	pFile->GetInteger(pSectionName,"NumPoint",0,&nNumPoint);
//	int nCostMoney = 0;
//	pFile->GetInteger(pSectionName,"CostMoney",0,&nCostMoney);
//	pTrafficPointSet->m_dwCostMoney = (DWORD)nCostMoney;
//	int nVelocity = 0;
//	pFile->GetInteger(pSectionName,"Velocity",0,&nVelocity);
//	pTrafficPointSet->m_dwVelocity = (DWORD)nVelocity;
//	int nStep = 0;
//	pFile->GetInteger(pSectionName,"Step",0,&nStep);
//	pTrafficPointSet->m_dwStep = (DWORD)nStep;
//
//	int nFromID = 0;
//	pFile->GetInteger(pSectionName,"FromID",0,&nFromID);
//	pTrafficPointSet->m_dwFromID = (DWORD)nFromID;
//	int nToID = 0;
//	pFile->GetInteger(pSectionName,"ToID",0,&nToID);
//	pTrafficPointSet->m_dwToID = (DWORD)nToID;
//
//
//	wsprintf(keyName,"Point%dnID",0);
//	pFile->GetInteger(pSectionName,keyName,0,(int*)(&dwID));
//	m_mapTrafficPoint[dwID].m_dwNodeID = dwID;
//	wsprintf(keyName,"Point%dnType",0);
//	pFile->GetInteger(pSectionName,keyName,0,(int*)(&(m_mapTrafficPoint[dwID].m_dwType)));
//	wsprintf(keyName,"Point%dvPosition",0);
//	pFile->GetMultiFloat(pSectionName,keyName,(float*)(&(m_mapTrafficPoint[dwID].vPosition)),3);		
//	wsprintf(keyName,"Point%dnLinkID",0);
//	pFile->GetInteger(pSectionName,keyName,-2,(int*)(&(m_mapTrafficPoint[dwID].m_nLinkID)));	
//	wsprintf(keyName,"Point%dnMapID",0);
//	pFile->GetInteger(pSectionName,keyName,0,(int*)(&(m_mapTrafficPoint[dwID].m_dwMapID)));
//	wsprintf(keyName,"Point%dnCityID",0);
//	pFile->GetInteger(pSectionName,keyName,0,(int*)(&(m_mapTrafficPoint[dwID].m_dwCityID)));
//	wsprintf(keyName,"Point%dnForceID",0);
//	pFile->GetInteger(pSectionName,keyName,0,(int*)(&(m_mapTrafficPoint[dwID].m_dwForceID)));
//	wsprintf(keyName,"Point%dnTongID",0);
//	pFile->GetInteger(pSectionName,keyName,0,(int*)(&(m_mapTrafficPoint[dwID].m_dwTongID)));
//
//
//	wsprintf(keyName,"Point%dbNeedOpen",0);
//	pFile->GetInteger(pSectionName,keyName,0,&nNeedOpen);
//	m_mapTrafficPoint[dwID].m_bNeedOpen = (BOOL)nNeedOpen;
//	wsprintf(keyName,"Point%dszScriptName",0);
//	pFile->GetString(pSectionName,keyName,"",str,MAX_PATH);
//	m_mapTrafficPoint[dwID].m_strScriptFile = str;
//	wsprintf(keyName,"Point%dszDescription",0);
//	pFile->GetString(pSectionName,keyName,"",str,MAX_PATH);
//	m_mapTrafficPoint[dwID].m_strDescription = str;
//
//	for (int i = 1;i < nNumPoint - 1;i++)
//	{
//		TrafficLittlePoint* pPoint = NULL;
//		D3DXVECTOR3 vPos;
//		pTrafficPointSet->AddTrafficPoint(&pPoint,vPos,NULL);	
//		wsprintf(keyName,"Point%dvPosition",i);
//		pFile->GetMultiFloat(pSectionName,keyName,(float*)&pPoint->vPosition,3);		
//	}
//
//	wsprintf(keyName,"Point%dnID",nNumPoint - 1);
//	pFile->GetInteger(pSectionName,keyName,0,(int*)(&dwID));
//	m_mapTrafficPoint[dwID].m_dwNodeID = dwID;
//	wsprintf(keyName,"Point%dnType",nNumPoint - 1);
//	pFile->GetInteger(pSectionName,keyName,0,(int*)(&(m_mapTrafficPoint[dwID].m_dwType)));
//	wsprintf(keyName,"Point%dvPosition",nNumPoint - 1);
//	pFile->GetMultiFloat(pSectionName,keyName,(float*)(&(m_mapTrafficPoint[dwID].vPosition)),3);		
//	wsprintf(keyName,"Point%dnLinkID",nNumPoint - 1);
//	pFile->GetInteger(pSectionName,keyName,-2,(int*)(&(m_mapTrafficPoint[dwID].m_nLinkID)));	
//	wsprintf(keyName,"Point%dnMapID",nNumPoint - 1);
//	pFile->GetInteger(pSectionName,keyName,0,(int*)(&(m_mapTrafficPoint[dwID].m_dwMapID)));
//	wsprintf(keyName,"Point%dnCityID",nNumPoint - 1);
//	pFile->GetInteger(pSectionName,keyName,0,(int*)(&(m_mapTrafficPoint[dwID].m_dwCityID)));
//	wsprintf(keyName,"Point%dnForceID",nNumPoint - 1);
//	pFile->GetInteger(pSectionName,keyName,0,(int*)(&(m_mapTrafficPoint[dwID].m_dwForceID)));
//	wsprintf(keyName,"Point%dnTongID",nNumPoint - 1);
//	pFile->GetInteger(pSectionName,keyName,0,(int*)(&(m_mapTrafficPoint[dwID].m_dwTongID)));
//
//	
//	wsprintf(keyName,"Point%dbNeedOpen",nNumPoint - 1);
//	pFile->GetInteger(pSectionName,keyName,0,&nNeedOpen);
//	m_mapTrafficPoint[dwID].m_bNeedOpen = (BOOL)nNeedOpen;
//	wsprintf(keyName,"Point%dszScriptName",nNumPoint - 1);
//	pFile->GetString(pSectionName,keyName,"",str,MAX_PATH);
//	m_mapTrafficPoint[dwID].m_strScriptFile = str;
//	wsprintf(keyName,"Point%dszDescription",nNumPoint - 1);
//	pFile->GetString(pSectionName,keyName,"",str,MAX_PATH);
//	m_mapTrafficPoint[dwID].m_strDescription = str;
//	return S_OK;
//}

HRESULT KSceneEditorDocLogical::LoadOverMapTrafficPointToIniFile(IIniFile* pFile)
{
	TCHAR szSectionName[MAX_PATH];
	TCHAR szStr[MAX_PATH];
	int nID = 0;
	int nNumOfPointOverMaps = 0;
	int nNumOfTrackOverMaps = 0;
	HRESULT hr = E_FAIL;

	pFile->GetInteger("MAIN","NumPoint",0,&nNumOfPointOverMaps);
	for (int i = 0;i < nNumOfPointOverMaps;i++)
	{
		wsprintf(szSectionName,"RoadPoint%d",i);
		pFile->GetInteger(szSectionName,"ID",0,&nID);
		pFile->GetString(szSectionName,"Name","",szStr,MAX_PATH);
		m_mapLinkAbleList[nID] = szStr;
	}
	
	pFile->GetInteger("MAIN","NumTrack",0,&nNumOfTrackOverMaps);
	for (int i = 0;i < nNumOfTrackOverMaps;i++)
	{
		wsprintf(szSectionName,"RoadTrack%d",i);
		TrafficPointSet* pTrafficPointSet = new TrafficPointSet;
		KGLOG_PROCESS_ERROR(pTrafficPointSet);	
		m_listOverMapPointSet.push_back(pTrafficPointSet);

		int nSetID = 0;
		pFile->GetInteger(szSectionName,"dwSetID",0,&nSetID);
		pTrafficPointSet->m_dwID = (DWORD)nSetID;
		int nCostMoney = 0;
		pFile->GetInteger(szSectionName,"CostMoney",0,&nCostMoney);
		pTrafficPointSet->m_dwCostMoney = (DWORD)nCostMoney;
		int nVelocity = 0;
		pFile->GetInteger(szSectionName,"Velocity",0,&nVelocity);
		pTrafficPointSet->m_dwVelocity = (DWORD)nVelocity;
		int nFromID = 0;
		pFile->GetInteger(szSectionName,"FromID",0,&nFromID);
		pTrafficPointSet->m_dwFromID = (DWORD)nFromID;
		int nToID = 0;
		pFile->GetInteger(szSectionName,"ToID",0,&nToID);
		pTrafficPointSet->m_dwToID = (DWORD)nToID;
	}
	hr = S_OK;
Exit0:
	return hr;
}
HRESULT KSceneEditorDocLogical::LoadDoodadFromIniFile(IIniFile* pFile,LPCTSTR pSectionName,DoodadInfo* pDoodad)
{
	TCHAR str[MAX_PATH];

	pFile->GetString(pSectionName,"szName","",str,MAX_PATH);
	pDoodad->szName = str;
	pFile->GetString(pSectionName,"szNickName","",str,MAX_PATH);
	pDoodad->szNickName = str;
	pFile->GetString(pSectionName,"szScriptName","",str,MAX_PATH);
	pDoodad->szScriptName = str;

	pFile->GetInteger(pSectionName,"nIndex",0,&pDoodad->nIndex);
	pFile->GetInteger(pSectionName,"nTempleteID",0,&pDoodad->nTempleteID);
	pFile->GetInteger(pSectionName,"nX",0,&pDoodad->nX);
	pFile->GetInteger(pSectionName,"nY",0,&pDoodad->nY);
	pFile->GetInteger(pSectionName,"nZ",0,&pDoodad->nZ);
	pFile->GetInteger(pSectionName,"nKind",0,&pDoodad->nKind);
	pFile->GetInteger(pSectionName,"nFaceDirection",0,&pDoodad->nFaceDirection);
	//pFile->GetInteger(pSectionName,"nAIType",0,&pDoodad->nAIType);
    //pFile->GetInteger(pSectionName, "nAIMainState", 0, &pDoodad->nAIMainState);
    pFile->GetStruct(pSectionName, "dwScriptID", (void*)&(pDoodad->dwScriptID), sizeof(DWORD));

	pFile->GetMultiFloat(pSectionName,"vPosition",(float*)&pDoodad->vPosition,3);
	pFile->GetMultiFloat(pSectionName,"vRotation",(float*)&pDoodad->vRotation,4);

	int nReliveID;
	pFile->GetInteger(pSectionName, "ReliveID", 0,&nReliveID);
	pDoodad->dwDoodadReliveID = nReliveID;

	return S_OK;
}

HRESULT KSceneEditorDocLogical::LoadNPCFromIniFile(IIniFile* pFile,LPCTSTR pSectionName,NPCInfo* pNpc)
{
	TCHAR str[MAX_PATH];

	pFile->GetString(pSectionName,"szName","",str,MAX_PATH);
	pNpc->szName = str;
	pFile->GetString(pSectionName,"szNickName","",str,MAX_PATH);
	pNpc->szNickName = str;
	pFile->GetString(pSectionName,"szScriptName","",str,MAX_PATH);
	pNpc->szScriptName = str;

	pFile->GetInteger(pSectionName,"nIndex",0,&pNpc->nIndex);
	pFile->GetInteger(pSectionName,"nTempleteID",0,&pNpc->nTempleteID);
	pFile->GetInteger(pSectionName,"nX",0,&pNpc->nX);
	pFile->GetInteger(pSectionName,"nY",0,&pNpc->nY);
	pFile->GetInteger(pSectionName,"nZ",0,&pNpc->nZ);
	pFile->GetInteger(pSectionName,"nKind",0,&pNpc->nKind);
	pFile->GetInteger(pSectionName,"nFaceDirection",0,&pNpc->nFaceDirection);
    //pFile->GetInteger(pSectionName, "nDisableAutoTurn", 0, &pNpc->nDisableAutoTurn);
	//pFile->GetInteger(pSectionName,"nAIType",0,&pNpc->nAIType);
    //pFile->GetInteger(pSectionName, "nAIMainState", 0, &pNpc->nAIMainState);
    pFile->GetStruct(pSectionName, "dwScriptID", (void*)&(pNpc->dwScriptID), sizeof(DWORD));
    pFile->GetStruct(pSectionName, "dwRepresentID", (void*)&(pNpc->dwRepresentID), sizeof(DWORD));
	pFile->GetInteger(pSectionName, "nLevel", 0, &pNpc->nLevel);
	pFile->GetMultiFloat(pSectionName,"vPosition",(float*)&pNpc->vPosition,3);
	pFile->GetMultiFloat(pSectionName,"vRotation",(float*)&pNpc->vRotation,4);

	int WayPointSetID = 0;
	pFile->GetInteger(pSectionName,"WayPointSetID",-1,&WayPointSetID);

	if(WayPointSetID==-1)
	{
		pNpc->pWayPointSet = NULL;
	}
	else
	{
		GetWayPointSetByID(&pNpc->pWayPointSet,WayPointSetID);
	}

	int nReliveID,nRandomID,nAisetID;
	pFile->GetInteger(pSectionName, "ReliveID",0,&nReliveID);
	pFile->GetInteger(pSectionName, "RandomID",0,&nRandomID);
	pFile->GetInteger(pSectionName, "AISetID" ,0,&nAisetID);

	pNpc->dwNpcReliveID = nReliveID;
	pNpc->dwNpcRandomID = nRandomID;
	pNpc->dwNpcAISet    = nAisetID;

	// ahpho suntao
	int nTeamID = 0;
	pFile->GetInteger(pSectionName, "TeamID", 0, &nTeamID);
	pNpc->dwNpcTeamID = nTeamID;
	// end ahpho

	UpdataOneNpcAIParameters(pNpc);	

	return S_OK;
}


HRESULT KSceneEditorDocLogical::AddOneNewLogicalPoly(IEKG3DRepresentPoly* pScenePoly)
{
	m_listLogicalPoly.push_back(pScenePoly);
	return S_OK;
}

HRESULT KSceneEditorDocLogical::DeleteLogicalPoly(IEKG3DRepresentPoly* pScenePoly)
{
	m_listLogicalPoly.remove(pScenePoly);
	return S_OK;
}

HRESULT KSceneEditorDocLogical::DeleteAllLogicalPoly()
{
	m_listLogicalPoly.clear();
	return S_OK;
}

HRESULT KSceneEditorDocLogical::AddOneTrafficPointSet(TrafficPointSet** ppNewSet,DWORD dwID,LPCTSTR pName)
{
	TrafficPointSet* pSet = new TrafficPointSet;
	KGLOG_PROCESS_ERROR(pSet);
	(*ppNewSet) = pSet;

	pSet->szName = pName;
	pSet->m_dwID = dwID;
	m_listTrafficPointSet.push_back(pSet);

	FireEvent2(EVENTDOCLOGIGAL_UPDATEUI,0,0,0);

	return S_OK;
Exit0:
	return E_FAIL;
}
HRESULT KSceneEditorDocLogical::DeleteTrafficPointSet(TrafficPointSet* pSet)
{

	map<DWORD,string>::iterator it = m_mapTrackByWhat.find(pSet->m_dwID);
	if (it != m_mapTrackByWhat.end())
		m_mapTrackByWhat.erase(it);
	m_listTrafficPointSet.remove(pSet);

	// 删除交通路径两端大点的表现
	INT fromID = pSet->m_dwFromID;
	map<DWORD, TrafficPoint>::iterator iter = m_mapTrafficPoint.find(fromID);
	if (iter != m_mapTrafficPoint.end())
	{
		TrafficPoint fromTrafficPoint = iter->second;
		IEKG3DRepresentObject* pObj = (IEKG3DRepresentObject*)fromTrafficPoint.pPointer;
		m_lpSceneSceneEditor->RemoveRepresentObject(pObj);
		m_mapTrafficPoint.erase(fromID);
	}
	
	INT toID = pSet->m_dwToID;
	iter = m_mapTrafficPoint.find(toID);
	if (iter != m_mapTrafficPoint.end())
	{
		TrafficPoint toTrafficPoint = iter->second;
		IEKG3DRepresentObject* pObj = (IEKG3DRepresentObject*)toTrafficPoint.pPointer;
		m_lpSceneSceneEditor->RemoveRepresentObject(pObj);
		m_mapTrafficPoint.erase(toID);
	}	

	SAFE_DELETE(pSet);
	
	FireEvent2(EVENTDOCLOGIGAL_UPDATEUI,0,0,0);

	return S_OK;
}
HRESULT KSceneEditorDocLogical::DeleteAllTrafficPointSet()
{
	list<TrafficPointSet*>::iterator i = m_listTrafficPointSet.begin();
	while (i!=m_listTrafficPointSet.end())
	{
		TrafficPointSet* pSet = *i;
		SAFE_DELETE(pSet);

		i++;
	}
	m_listTrafficPointSet.clear();
	return S_OK;
}

HRESULT KSceneEditorDocLogical::GetTrafficPointSetByID(TrafficPointSet** ppSet,DWORD dwID)
{
	list<TrafficPointSet*>::iterator i = m_listTrafficPointSet.begin();
	while (i!=m_listTrafficPointSet.end())
	{
		TrafficPointSet* pSet = *i;
		if(pSet->m_dwID == dwID)
		{
			(*ppSet) = pSet;
			return S_OK;
		}

		i++;
	}
	(*ppSet) = NULL;
	return E_FAIL;

}

HRESULT KSceneEditorDocLogical::GetTrafficPointByEditorPoint(TrafficLittlePoint** ppTrafficPoint,PVOID pPointer)
{
	if (ppTrafficPoint == NULL)
		return E_FAIL;
	TrafficLittlePoint* pTrafficPoint = NULL;
	list<TrafficPointSet*>::iterator i = m_listTrafficPointSet.begin();
	while (i!=m_listTrafficPointSet.end())
	{
		TrafficPointSet* pSet = *i;
		int nTempID;
		if(pSet->FindTrafficPointByEditorWayPoint(&pTrafficPoint,&nTempID,pPointer) == S_OK)
		{
			*ppTrafficPoint = pTrafficPoint;
			return S_OK;
		}

		i++;
	}
	(*ppTrafficPoint) = NULL;
	return E_FAIL;
}

HRESULT KSceneEditorDocLogical::GetBigTrafficPointByEditorPoint(TrafficPoint** ppTrafficPoint, PVOID pPointer)
{
	if (ppTrafficPoint == NULL)
		return E_FAIL;
	TrafficPoint* pTrafficPoint = NULL;
	map<DWORD,TrafficPoint>::iterator iter = m_mapTrafficPoint.begin();
	while (iter != m_mapTrafficPoint.end())
	{
		TrafficPoint& sourceinfo = iter->second;
		if (sourceinfo.pPointer == pPointer)
		{
			*ppTrafficPoint = &sourceinfo;
			return S_OK;
		}

		iter++;
	}
	(*ppTrafficPoint) = NULL;
	return E_FAIL;
}


HRESULT KSceneEditorDocLogical::GetNpcReviveGroup(DWORD dwID,LPTSTR pName,int& nMin,int& nMax)
{
	map<DWORD,ReviveGroup>::iterator iFind = m_mapNpcReviveGroup.find(dwID);//npc 重生组
	if (iFind != m_mapNpcReviveGroup.end())
	{
		ReviveGroup& Group = iFind->second;
		wsprintf(pName,"%s",Group.szName.c_str());
		nMin = Group.nMinNpcCount;
		nMax = Group.nMaxNpcCount;

		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

HRESULT KSceneEditorDocLogical::SetNpcReviveGroup(DWORD dwID,LPTSTR pName,int nMin,int nMax)
{
	map<DWORD,ReviveGroup>::iterator iFind = m_mapNpcReviveGroup.find(dwID);//npc 重生组
	if (iFind != m_mapNpcReviveGroup.end())
	{
		ReviveGroup& Group = iFind->second;
		Group.szName = pName;
		Group.nMinNpcCount = nMin;
		Group.nMaxNpcCount = nMax;

		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}	

// ahpho suntao
// 创建doodad重生组
HRESULT KSceneEditorDocLogical::DecoratedNewOneDoodadReviveGroup(LONG* dwID, BSTR pName, int nMin, int nMax, BSTR doodadString)
{	
	vector<DoodadInfo*> doodadVector;
	string tempString;
	string doodadStr = BSTR_TO_STRING(doodadString);;
	INT doodadIndex;
	DoodadInfo* doodadInfo = NULL;

	DWORD groupID = GetCurrentDoodadReviveGroupID();
	ReviveGroup NewGroup;
	NewGroup.szName = BSTR_TO_STRING(pName);
	NewGroup.nMinNpcCount = nMin;
	NewGroup.nMaxNpcCount = nMax;
	NewGroup.dwGroupID = groupID;
	NewGroup.nRandom = 0;
	*dwID = (LONG)groupID;

	int i = 0;
	int index = doodadStr.find(",");
	while (index != -1)
	{
		tempString = doodadStr.substr(0, index);
		doodadIndex = atoi(tempString.c_str());
		doodadInfo = NULL;
		GetDoodadInfoByIndex(&doodadInfo, doodadIndex);							
		doodadVector.push_back(doodadInfo);		
		doodadStr = doodadStr.substr(index + 1, doodadStr.length() - index - 1);
		index = doodadStr.find(",");
		i++;
	}

	tempString = doodadStr;
	doodadIndex = atoi(tempString.c_str());
	doodadInfo = NULL;
	GetDoodadInfoByIndex(&doodadInfo, doodadIndex);
	doodadVector.push_back(doodadInfo);	
	NewGroup.vecDoodadInfo = doodadVector;

	m_mapDoodadReviveGroup[groupID] = NewGroup;

	return S_OK;
}

// 删除doodad重生组
HRESULT KSceneEditorDocLogical::DecoratedDeleteDoodadReviveGroup(LONG dwID)
{
	map<DWORD,ReviveGroup>::iterator iFind = m_mapDoodadReviveGroup.find(dwID);// doodad 重生组
	if (iFind != m_mapDoodadReviveGroup.end())
	{
		ReviveGroup reviveGroup = iFind->second;
		reviveGroup.vecDoodadInfo.clear();
		m_mapDoodadReviveGroup.erase(iFind);
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

// 更新doodad重生组
HRESULT KSceneEditorDocLogical::UpdateDoodadReviveGroup(LONG dwID, BSTR doodadString)
{
	map<DWORD,ReviveGroup>::iterator iFind = m_mapDoodadReviveGroup.find(dwID);// doodad 重生组
	if (iFind != m_mapDoodadReviveGroup.end())
	{
		ReviveGroup reviveGroup = iFind->second;
		reviveGroup.vecDoodadInfo.clear();
		
		vector<DoodadInfo*> doodadVector;
		string tempString;
		string doodadStr = BSTR_TO_STRING(doodadString);;
		INT doodadIndex;
		DoodadInfo* doodadInfo = NULL;

		int i = 0;
		int index = doodadStr.find(",");
		while (index != -1)
		{
			tempString = doodadStr.substr(0, index);
			doodadIndex = atoi(tempString.c_str());
			doodadInfo = NULL;
			GetDoodadInfoByIndex(&doodadInfo, doodadIndex);							
			doodadVector.push_back(doodadInfo);		
			doodadStr = doodadStr.substr(index + 1, doodadStr.length() - index - 1);
			index = doodadStr.find(",");
			i++;
		}

		tempString = doodadStr;
		doodadIndex = atoi(tempString.c_str());
		doodadInfo = NULL;
		GetDoodadInfoByIndex(&doodadInfo, doodadIndex);
		doodadVector.push_back(doodadInfo);	
		reviveGroup.vecDoodadInfo = doodadVector;

		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

// 创建npc重生组
HRESULT KSceneEditorDocLogical::DecoratedNewOneNpcReviveGroup(LONG* dwID, BSTR pName, int nMin, int nMax, BSTR npcString)
{	
	vector<NPCInfo*> npcVector;
	string tempString;
	string npcStr = BSTR_TO_STRING(npcString);;
	INT npcIndex;
	NPCInfo* npcInfo = NULL;

	DWORD groupID = GetCurrentNpcReviveGroupID();
	ReviveGroup NewGroup;
	NewGroup.szName = BSTR_TO_STRING(pName);
	NewGroup.nMinNpcCount = nMin;
	NewGroup.nMaxNpcCount = nMax;
	NewGroup.dwGroupID = groupID;
	NewGroup.nRandom = 0;
	*dwID = (LONG)groupID;

	int i = 0;
	int index = npcStr.find(",");
	while (index != -1)
	{
		tempString = npcStr.substr(0, index);
		npcIndex = atoi(tempString.c_str());
		npcInfo = NULL;
		GetNpcInfoByIndex(&npcInfo, npcIndex);							
		npcVector.push_back(npcInfo);		
		npcStr = npcStr.substr(index + 1, npcStr.length() - index - 1);
		index = npcStr.find(",");
		i++;
	}

	tempString = npcStr;
	npcIndex = atoi(tempString.c_str());
	npcInfo = NULL;
	GetNpcInfoByIndex(&npcInfo, npcIndex);
	npcVector.push_back(npcInfo);	
	NewGroup.vecNpcInfo = npcVector;

	m_mapNpcReviveGroup[groupID] = NewGroup;

	return S_OK;
}

// 删除npc重生组
HRESULT KSceneEditorDocLogical::DecoratedDeleteNpcReviveGroup(LONG dwID)
{
	map<DWORD,ReviveGroup>::iterator iFind = m_mapNpcReviveGroup.find(dwID);// npc 重生组
	if (iFind != m_mapNpcReviveGroup.end())
	{
		ReviveGroup reviveGroup = iFind->second;
		reviveGroup.vecNpcInfo.clear();
		m_mapNpcReviveGroup.erase(iFind);
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

// 更新npc重生组
HRESULT KSceneEditorDocLogical::UpdateNpcReviveGroup(LONG dwID, BSTR npcString)
{
	map<DWORD,ReviveGroup>::iterator iFind = m_mapNpcReviveGroup.find(dwID);// npc 重生组
	if (iFind != m_mapNpcReviveGroup.end())
	{
		ReviveGroup reviveGroup = iFind->second;
		reviveGroup.vecNpcInfo.clear();

		vector<NPCInfo*> npcVector;
		string tempString;
		string npcStr = BSTR_TO_STRING(npcString);;
		INT npcIndex;
		NPCInfo* npcInfo = NULL;

		int i = 0;
		int index = npcStr.find(",");
		while (index != -1)
		{
			tempString = npcStr.substr(0, index);
			npcIndex = atoi(tempString.c_str());
			npcInfo = NULL;
			GetNpcInfoByIndex(&npcInfo, npcIndex);							
			npcVector.push_back(npcInfo);		
			npcStr = npcStr.substr(index + 1, npcStr.length() - index - 1);
			index = npcStr.find(",");
			i++;
		}

		tempString = npcStr;
		npcIndex = atoi(tempString.c_str());
		npcInfo = NULL;
		GetNpcInfoByIndex(&npcInfo, npcIndex);
		npcVector.push_back(npcInfo);	
		reviveGroup.vecNpcInfo = npcVector;

		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

// 获取npc仇恨组信息
HRESULT KSceneEditorDocLogical::GetHatredGroupInfo(INT index, INT* groupID, BSTR* groupName)
{
 	int i = 0;

	for (map<INT, string>::iterator it = m_npcHatredGroupMap.begin(); it != m_npcHatredGroupMap.end(); it++)
	{					
		if (i++ == index)
		{
			*groupID = it->first;
			string strGroupName = it->second;				
			*groupName = STRING_TO_BSTR(strGroupName.c_str());			
			break;
		}
	}
	return S_OK;
}

// 获取npcAI组信息
HRESULT KSceneEditorDocLogical::GetAIGroupInfo(BSTR* infoString)
{
	string info = "";

	for (list<AIGroup*>::iterator it = m_listAIGroup.begin(); it != m_listAIGroup.end(); it++)
	{					
		AIGroup* aiGroup = *it;		
		TCHAR charArray[MAX_PATH];
		wsprintf(charArray, "%d,%s;", aiGroup->dwSetID, aiGroup->szName.c_str());
		string tempString(charArray);
		info = info + tempString;	
	}

	*infoString = STRING_TO_BSTR(info.c_str());

	return S_OK;
}

// 获取AI组成员信息
HRESULT KSceneEditorDocLogical::GetAIGroupMemberInfo(INT groupID, BSTR* memberInfo)
{
	AIGroup* aiGroup = NULL;

	if (SUCCEEDED(GetAIGroupByID(&aiGroup, groupID)))
	{
		string info = "";

		for (vector<NPCInfo*>::iterator it = aiGroup->vecNpcInfo.begin(); it != aiGroup->vecNpcInfo.end(); it++)
		{
			NPCInfo* npcInfo = *it;
			TCHAR charArray[MAX_PATH];
			wsprintf(charArray, "%s,%d,%d;", npcInfo->szName.c_str(), npcInfo->nTempleteID, npcInfo->nIndex);
			string tempString(charArray);
			info = info + tempString;
		}

		*memberInfo = STRING_TO_BSTR(info.c_str());
	}

	return S_OK;
}
// end ahpho

HRESULT KSceneEditorDocLogical::NewOneNpcReviveGroup(DWORD& dwID,LPTSTR pName,int nMin,int nMax)
{
	dwID = GetCurrentNpcReviveGroupID();
	ReviveGroup NewGroup;
	NewGroup.szName = pName;
	NewGroup.nMinNpcCount = nMin;
	NewGroup.nMaxNpcCount = nMax;
	NewGroup.dwGroupID = dwID;
	NewGroup.nRandom = 0;
	m_mapNpcReviveGroup[dwID] = NewGroup;

	return S_OK;
}

HRESULT KSceneEditorDocLogical::DeleteNpcReviveGroup(DWORD dwID)
{
	map<DWORD,ReviveGroup>::iterator iFind = m_mapNpcReviveGroup.find(dwID);//npc 重生组
	if (iFind != m_mapNpcReviveGroup.end())
	{
		m_mapNpcReviveGroup.erase(iFind);
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

//////////////////////////////////////////////////////////////////////////

HRESULT KSceneEditorDocLogical::GetDoodadReviveGroup(DWORD dwID,LPTSTR pName,int& nMin,int& nMax)
{
	map<DWORD,ReviveGroup>::iterator iFind = m_mapDoodadReviveGroup.find(dwID);//npc 重生组
	if (iFind != m_mapDoodadReviveGroup.end())
	{
		ReviveGroup& Group = iFind->second;
		wsprintf(pName,"%s",Group.szName.c_str());
		nMin = Group.nMinNpcCount;
		nMax = Group.nMaxNpcCount;

		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

HRESULT KSceneEditorDocLogical::SetDoodadReviveGroup(DWORD dwID,LPTSTR pName,int nMin,int nMax)
{
	map<DWORD,ReviveGroup>::iterator iFind = m_mapDoodadReviveGroup.find(dwID);//npc 重生组
	if (iFind != m_mapDoodadReviveGroup.end())
	{
		ReviveGroup& Group = iFind->second;
		Group.szName = pName;
		Group.nMinNpcCount = nMin;
		Group.nMaxNpcCount = nMax;

		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

HRESULT KSceneEditorDocLogical::NewOneDoodadReviveGroup(DWORD& dwID,LPTSTR pName,int nMin,int nMax)
{
	dwID = GetCurrentDoodadReviveGroupID();
	ReviveGroup NewGroup;
	NewGroup.szName = pName;
	NewGroup.nMinNpcCount = nMin;
	NewGroup.nMaxNpcCount = nMax;
	NewGroup.dwGroupID = dwID;
	NewGroup.nRandom = 0;
	m_mapDoodadReviveGroup[dwID] = NewGroup;

	return S_OK;
}

HRESULT KSceneEditorDocLogical::DeleteDoodadReviveGroup(DWORD dwID)
{
	map<DWORD,ReviveGroup>::iterator iFind = m_mapDoodadReviveGroup.find(dwID);//npc 重生组
	if (iFind != m_mapDoodadReviveGroup.end())
	{
		m_mapDoodadReviveGroup.erase(iFind);
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

//////////////////////////////////////////////////////////////////////////
HRESULT KSceneEditorDocLogical::GetRandomGroup(DWORD dwID,LPTSTR pName,vector<DWORD>*pvecNpcTempleteID)
{
	map<DWORD,RandomGroup>::iterator iFind = m_mapRandomGroup.find(dwID);//npc 随机群
	if (iFind != m_mapRandomGroup.end())
	{
		RandomGroup& Group = iFind->second;
		wsprintf(pName,"%s",Group.szName.c_str());
		(*pvecNpcTempleteID) = Group.vecNpcTempleteID;

		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

HRESULT KSceneEditorDocLogical::SetRandomGroup(DWORD dwID,LPTSTR pName,vector<DWORD>*pvecNpcTempleteID)
{
	map<DWORD,RandomGroup>::iterator iFind = m_mapRandomGroup.find(dwID);//npc 随机群
	if (iFind != m_mapRandomGroup.end())
	{
		RandomGroup& Group = iFind->second;
		Group.szName = pName;
		Group.vecNpcTempleteID = (*pvecNpcTempleteID);
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

HRESULT KSceneEditorDocLogical::NewOneRandomGroup(DWORD& dwID)
{
	dwID = GetCurrentRandomGroupID();
	RandomGroup NewGroup;
	NewGroup.dwGroupID = dwID;
	TCHAR szName[MAX_PATH];
	wsprintf(szName,"New Npc Random Group %d",dwID);
	NewGroup.szName = szName;
	m_mapRandomGroup[dwID] = NewGroup;
	return S_OK;
}

HRESULT KSceneEditorDocLogical::DeleteRandomGroup(DWORD dwID)
{
	map<DWORD,RandomGroup>::iterator iFind = m_mapRandomGroup.find(dwID);//npc 随机群
	if (iFind != m_mapRandomGroup.end())
	{
		m_mapRandomGroup.erase(iFind);
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

HRESULT KSceneEditorDocLogical::SaveNpcReviveGroups(IIniFile* pFile)
{
	int nCount = (int)m_mapNpcReviveGroup.size();
	pFile->WriteInteger("MAIN","NumNpcReviveGroup",nCount);

	int S=0;
	map<DWORD,ReviveGroup>::iterator i = m_mapNpcReviveGroup.begin();//npc 重生组
	map<DWORD,ReviveGroup>::iterator iend = m_mapNpcReviveGroup.end();//npc 重生组
	while (i!=iend)
	{
		ReviveGroup& vReviveGroup = i->second;

		TCHAR szSectionName[MAX_PATH];
		wsprintf(szSectionName,"NpcReviveSection%d",S);

		pFile->WriteInteger(szSectionName,"GroupID",(int)vReviveGroup.dwGroupID);
		pFile->WriteString(szSectionName,"GroupName",vReviveGroup.szName.c_str());
		pFile->WriteInteger(szSectionName,"MinCount",vReviveGroup.nMinNpcCount);
		pFile->WriteInteger(szSectionName,"MaxCount",vReviveGroup.nMaxNpcCount);
		pFile->WriteInteger(szSectionName,"nRandom" ,vReviveGroup.nRandom);

		// ahpho suntao
		string npcString = "";
		TCHAR szNpcInfo[MAX_PATH];

		for(vector<NPCInfo*>::iterator it = vReviveGroup.vecNpcInfo.begin(); it != vReviveGroup.vecNpcInfo.end(); it++)
		{
			NPCInfo* npcInfo = *it;
			wsprintf(szNpcInfo, "%d,", npcInfo->nIndex);
			npcString = npcString + string(szNpcInfo);
		}

		// 去掉最后的逗号
		if (npcString.find_last_of(",") >= 0)
		{
			npcString = npcString.substr(0, npcString.length() - 1);
		}

		pFile->WriteString(szSectionName, "GroupMember", npcString.c_str());
		// end ahpho

		i++;
		S++;
	}

	return S_OK;
}

HRESULT KSceneEditorDocLogical::LoadNpcReviveGroups(IIniFile* pFile)
{
	int nCount = 0;
	pFile->GetInteger("MAIN","NumNpcReviveGroup",0,&nCount);

	for (int S=0;S<nCount;S++)
	{
		ReviveGroup vReviveGroup;

		TCHAR szSectionName[MAX_PATH];
		wsprintf(szSectionName,"NpcReviveSection%d",S);

		TCHAR szGroupName[MAX_PATH];
		int nID;

		pFile->GetInteger(szSectionName,"GroupID",0,&nID);
		vReviveGroup.dwGroupID = (DWORD)nID;

		pFile->GetString(szSectionName,"GroupName","",szGroupName,MAX_PATH);
		vReviveGroup.szName = szGroupName;
		pFile->GetInteger(szSectionName,"MinCount",0,&vReviveGroup.nMinNpcCount);
		pFile->GetInteger(szSectionName,"MaxCount",0,&vReviveGroup.nMaxNpcCount);
		pFile->GetInteger(szSectionName,"nRandom",0,&vReviveGroup.nRandom);

		// ahpho suntao 初始化npc组信息
		TCHAR szNpcString[MAX_PATH];
		pFile->GetString(szSectionName, "GroupMember", "", szGroupName, MAX_PATH);
		
		vector<NPCInfo*> npcVector;
		string tempString;
		string npcStr(szNpcString);
		INT npcIndex;
		NPCInfo* npcInfo = NULL;		
		
		if (npcStr != "")
		{
			int i = 0;
			int index = npcStr.find(",");
			while (index != -1)
			{
				tempString = npcStr.substr(0, index);
				npcIndex = atoi(tempString.c_str());
				npcInfo = NULL;
				GetNpcInfoByIndex(&npcInfo, npcIndex);							
				npcVector.push_back(npcInfo);		
				npcStr = npcStr.substr(index + 1, npcStr.length() - index - 1);
				index = npcStr.find(",");
				i++;
			}

			tempString = npcStr;
			npcIndex = atoi(tempString.c_str());
			npcInfo = NULL;
			GetNpcInfoByIndex(&npcInfo, npcIndex);
			npcVector.push_back(npcInfo);	
		}
		else // 需要遍历加入member信息
		{
			for(list<NPCInfo*>::iterator it = m_listNPCInfo.begin(); it != m_listNPCInfo.end(); it++)
			{
				npcInfo = *it;
				if (npcInfo->nIndex == nID)
				{
					npcVector.push_back(npcInfo);
				}
			}
		}
		
		vReviveGroup.vecNpcInfo = npcVector;
		// end ahpho
		m_mapNpcReviveGroup[nID] = vReviveGroup;
		m_dwCurrentNpcReviveGroupID = max(m_dwCurrentNpcReviveGroupID,(DWORD)nID+1);
	}

	return S_OK;
}

HRESULT KSceneEditorDocLogical::SaveDoodadReviveGroups(IIniFile* pFile)
{
	int nCount = (int)m_mapDoodadReviveGroup.size();
	pFile->WriteInteger("MAIN","NumDoodadReviveGroup",nCount);

	int S=0;
	map<DWORD,ReviveGroup>::iterator i = m_mapDoodadReviveGroup.begin();//npc 重生组
	map<DWORD,ReviveGroup>::iterator iend = m_mapDoodadReviveGroup.end();//npc 重生组
	while (i!=iend)
	{
		ReviveGroup& vReviveGroup = i->second;

		TCHAR szSectionName[MAX_PATH];
		wsprintf(szSectionName,"DoodadReviveSection%d",S);

		pFile->WriteInteger(szSectionName,"GroupID",(int)vReviveGroup.dwGroupID);
		pFile->WriteString(szSectionName,"GroupName",vReviveGroup.szName.c_str());
		pFile->WriteInteger(szSectionName,"MinCount",vReviveGroup.nMinNpcCount);
		pFile->WriteInteger(szSectionName,"MaxCount",vReviveGroup.nMaxNpcCount);
		pFile->WriteInteger(szSectionName,"nRandom" ,vReviveGroup.nRandom);
		
		// ahpho suntao
		string doodadString = "";
		TCHAR szDoodadInfo[MAX_PATH];

		for(vector<DoodadInfo*>::iterator it = vReviveGroup.vecDoodadInfo.begin(); it != vReviveGroup.vecDoodadInfo.end(); it++)
		{
			DoodadInfo* doodadInfo = *it;
			wsprintf(szDoodadInfo, "%d,", doodadInfo->nIndex);
			doodadString = doodadString + string(szDoodadInfo);
		}

		// 去掉最后的逗号
		if (doodadString.find_last_of(",") >= 0)
		{
			doodadString = doodadString.substr(0, doodadString.length() - 1);
		}

		pFile->WriteString(szSectionName, "GroupMember", doodadString.c_str());
		// end ahpho

		i++;
		S++;
	}

	return S_OK;
}



HRESULT KSceneEditorDocLogical::LoadDoodadReviveGroups(IIniFile* pFile)
{
	int nCount = 0;
	pFile->GetInteger("MAIN","NumDoodadReviveGroup",0,&nCount);

	for (int S=0;S<nCount;S++)
	{
		ReviveGroup vReviveGroup;

		TCHAR szSectionName[MAX_PATH];
		wsprintf(szSectionName,"DoodadReviveSection%d",S);

		TCHAR szGroupName[MAX_PATH];
		int nID;

		pFile->GetInteger(szSectionName,"GroupID",0,&nID);
		vReviveGroup.dwGroupID = (DWORD)nID;

		pFile->GetString(szSectionName,"GroupName","",szGroupName,MAX_PATH);
		vReviveGroup.szName = szGroupName;
		pFile->GetInteger(szSectionName,"MinCount",0,&vReviveGroup.nMinNpcCount);
		pFile->GetInteger(szSectionName,"MaxCount",0,&vReviveGroup.nMaxNpcCount);
		pFile->GetInteger(szSectionName,"nRandom",0,&vReviveGroup.nRandom);
		
		// ahpho suntao 初始化doodad组信息
		TCHAR szDoodadString[MAX_PATH];
		pFile->GetString(szSectionName, "GroupMember", "", szGroupName, MAX_PATH);

		vector<DoodadInfo*> doodadVector;
		string tempString;
		string doodadStr(szDoodadString);
		INT doodadIndex;
		DoodadInfo* doodadInfo = NULL;		
		
		if (doodadStr != "") // 已经保存了member信息
		{
			int i = 0;
			int index = doodadStr.find(",");
			while (index != -1)
			{
				tempString = doodadStr.substr(0, index);
				doodadIndex = atoi(tempString.c_str());
				doodadInfo = NULL;
				GetDoodadInfoByIndex(&doodadInfo, doodadIndex);							
				doodadVector.push_back(doodadInfo);		
				doodadStr = doodadStr.substr(index + 1, doodadStr.length() - index - 1);
				index = doodadStr.find(",");
				i++;
			}

			tempString = doodadStr;
			doodadIndex = atoi(tempString.c_str());
			doodadInfo = NULL;
			GetDoodadInfoByIndex(&doodadInfo, doodadIndex);
			doodadVector.push_back(doodadInfo);	
		}
		else // 需要遍历加入member信息
		{
			for(list<DoodadInfo*>::iterator it = m_listDoodadInfo.begin(); it != m_listDoodadInfo.end(); it++)
			{
				doodadInfo = *it;
				if (doodadInfo->nIndex == nID)
				{
					doodadVector.push_back(doodadInfo);
				}
			}
		}
		
		vReviveGroup.vecDoodadInfo = doodadVector;
		// end ahpho

		m_mapDoodadReviveGroup[nID] = vReviveGroup;

		m_dwCurrentDoodadReviveGroupID = max(m_dwCurrentDoodadReviveGroupID,(DWORD)nID+1);

	}

	return S_OK;
}

HRESULT KSceneEditorDocLogical::SaveNpcRandomGroups(IIniFile* pFile)
{
	int nCount = (int)m_mapRandomGroup.size();
	pFile->WriteInteger("MAIN","NumNpcRandomGroup",nCount);

	int S=0;
	map<DWORD,RandomGroup>::iterator i = m_mapRandomGroup.begin();//npc 重生组
	map<DWORD,RandomGroup>::iterator iend = m_mapRandomGroup.end();//npc 重生组
	while (i!=iend)
	{
		RandomGroup& vRandomGroup = i->second;

		TCHAR szSectionName[MAX_PATH];
		wsprintf(szSectionName,"NpcRandomSection%d",S);

		pFile->WriteInteger(szSectionName,"GroupID",(int)vRandomGroup.dwGroupID);
		pFile->WriteString(szSectionName,"GroupName",vRandomGroup.szName.c_str());

		int nNpcCount = (int)vRandomGroup.vecNpcTempleteID.size();
		pFile->WriteInteger(szSectionName,"NpcCount",nNpcCount);

		for (int K=0;K<nNpcCount;K++)
		{
			TCHAR szKeyName[MAX_PATH];
			wsprintf(szKeyName,"NpcTempleteID%d",K);
			pFile->WriteInteger(szSectionName,szKeyName,(int)vRandomGroup.vecNpcTempleteID[K]);
		}

		i++;
		S++;
	}

	return S_OK;
}

HRESULT KSceneEditorDocLogical::LoadNpcRandomGroups(IIniFile* pFile)
{
	int nCount = 0;
	pFile->GetInteger("MAIN","NumNpcRandomGroup",0,&nCount);

	for (int S=0;S<nCount;S++)
	{
		RandomGroup vRandomGroup;

		TCHAR szSectionName[MAX_PATH];
		TCHAR szGroupName[MAX_PATH];
		wsprintf(szSectionName,"NpcRandomSection%d",S);
		int nID = 0;

		pFile->GetInteger(szSectionName,"GroupID",0,&nID);
		vRandomGroup.dwGroupID = nID;

		pFile->GetString(szSectionName,"GroupName","",szGroupName,MAX_PATH);
		vRandomGroup.szName = szGroupName;

		int nNpcCount = 0;
		pFile->GetInteger(szSectionName,"NpcCount",0,&nNpcCount);

		for (int K=0;K<nNpcCount;K++)
		{
			TCHAR szKeyName[MAX_PATH];
			wsprintf(szKeyName,"NpcTempleteID%d",K);
			int nNpcTempleteID = 0;
			pFile->GetInteger(szSectionName,szKeyName,0,&nNpcTempleteID);

			vRandomGroup.vecNpcTempleteID.push_back(nNpcTempleteID);
		}

		m_mapRandomGroup[nID] = vRandomGroup;

		m_dwCurrentRandomGroupID = max(m_dwCurrentRandomGroupID,(DWORD)nID+1);
	}

	return S_OK;
}

HRESULT KSceneEditorDocLogical::ExchangeNpcAISet(DWORD dwMapID, LPSTR pFileName)
{
    HRESULT     hResult = E_FAIL;
	ITabFile*   pFile   = NULL;
    int         nRow    = 0;

	KGLOG_PROCESS_ERROR(pFileName);
	DeleteFile(pFileName);

	pFile = g_OpenTabFile(pFileName, true, true);
	KGLOG_PROCESS_ERROR(pFile);

	pFile->InsertNewCol("ID");
	for (size_t i = 0; i < m_AIParameterDefine.vecParameters.size(); i++)
	{
		AIParameter& par = m_AIParameterDefine.vecParameters[i];
		pFile->InsertNewCol(par.strName.c_str());
	}

    ASSERT(sizeof(DWORD) >= 4);
    ASSERT(dwMapID <= UCHAR_MAX);

    nRow = 2;
    for (map<DWORD,AIParameterSet>::iterator it = m_mapNpcAISceneParameterSet.begin(), itEnd = m_mapNpcAISceneParameterSet.end(); it != itEnd; ++it, nRow++)
    {
        DWORD           dwID        = it->first;
        DWORD           dwAIMapID   = (dwMapID << 24) | dwID;
        AIParameterSet& rAISet      = it->second;

        pFile->WriteInteger(nRow, 1, (int)dwAIMapID);

        KGLOG_PROCESS_ERROR(rAISet.vecParameters.size() == m_AIParameterDefine.vecParameters.size());

        for (size_t nParamIndex = 0; nParamIndex < m_AIParameterDefine.vecParameters.size(); nParamIndex++)
        {
            AIParameter& rParam = rAISet.vecParameters[nParamIndex];

            ASSERT(rParam.strName == m_AIParameterDefine.vecParameters[nParamIndex].strName);
            pFile->WriteInteger(nRow, (int)nParamIndex + 2, rParam.nValue);
        }
    }

	pFile->Save(pFileName);

	hResult = S_OK;
Exit0:
	SAFE_RELEASE(pFile);
	return hResult;
}

HRESULT KSceneEditorDocLogical::ExchangeNpcReviveGroups(ITabFile* pFile)
{
	int bRetCode;
	int S=2;
	map<DWORD,ReviveGroup>::iterator i = m_mapNpcReviveGroup.begin();//npc 重生组
	map<DWORD,ReviveGroup>::iterator iend = m_mapNpcReviveGroup.end();//npc 重生组
	while (i!=iend)
	{
		ReviveGroup& vReviveGroup = i->second;

		bRetCode = pFile->WriteInteger(S,1, vReviveGroup.dwGroupID);

		bRetCode = pFile->WriteInteger(S,2, vReviveGroup.nMinNpcCount);

		bRetCode = pFile->WriteInteger(S,3, vReviveGroup.nMaxNpcCount);

		bRetCode = pFile->WriteInteger(S,4, vReviveGroup.nRandom);
		i++;
		S++;
	}

	return S_OK;
}

HRESULT KSceneEditorDocLogical::ExchangeDoodadReviveGroups(ITabFile* pFile)
{
	int bRetCode;
	int S=2;
	map<DWORD,ReviveGroup>::iterator i = m_mapDoodadReviveGroup.begin();//npc 重生组
	map<DWORD,ReviveGroup>::iterator iend = m_mapDoodadReviveGroup.end();//npc 重生组
	while (i!=iend)
	{
		ReviveGroup& vReviveGroup = i->second;

		bRetCode = pFile->WriteInteger(S,1, vReviveGroup.dwGroupID);

		bRetCode = pFile->WriteInteger(S,2, vReviveGroup.nMinNpcCount);

		bRetCode = pFile->WriteInteger(S,3, vReviveGroup.nMaxNpcCount);

		bRetCode = pFile->WriteInteger(S,4, vReviveGroup.nRandom);
		i++;
		S++;
	}

	return S_OK;
}

HRESULT KSceneEditorDocLogical::ExchangeNpcRandomGroups(ITabFile* pFile,LPCTSTR pDir)
{
	int bRetCode;
	int S=2;
	map<DWORD,RandomGroup>::iterator i = m_mapRandomGroup.begin();//npc 重生组
	map<DWORD,RandomGroup>::iterator iend = m_mapRandomGroup.end();//npc 重生组
	while (i!=iend)
	{
		RandomGroup& vRandomGroup = i->second;

		bRetCode = pFile->WriteInteger(S,1, vRandomGroup.dwGroupID);

		TCHAR szGroupFileName[MAX_PATH];
		wsprintf(szGroupFileName,"%s.tab",vRandomGroup.szName.c_str());
		bRetCode = pFile->WriteString (S,2, szGroupFileName,MAX_PATH);

		wsprintf(szGroupFileName,"%s%s.tab", pDir,vRandomGroup.szName.c_str());
		{
			ITabFile* pGroupFile = NULL;

			pGroupFile = g_OpenTabFile(szGroupFileName,true,true);
			if(pGroupFile)
			{
				pGroupFile->InsertNewCol("NpcTemplateID");

				for (size_t t=0;t<vRandomGroup.vecNpcTempleteID.size();t++)
				{
					pGroupFile->WriteInteger((int)t+2,1,vRandomGroup.vecNpcTempleteID[t]);
				}
				pGroupFile->Save(szGroupFileName);
			}
			SAFE_RELEASE(pGroupFile);
		}


		i++;
		S++;
	}

	return S_OK;
}

HRESULT KSceneEditorDocLogical::NewOneAISet(DWORD& dwID, LPTSTR pName)
{
    AIParameterSet NewSet;

    dwID = GetCurrentAISetID();

    NewSet.dwID = dwID;
    NewSet.szName = pName;
    NewSet.vecParameters = m_AIParameterDefine.vecParameters;

    m_mapNpcAISceneParameterSet[dwID] = NewSet;

    return S_OK;
}

HRESULT KSceneEditorDocLogical::DeleteAISet(DWORD dwID)
{
	map<DWORD,AIParameterSet>::iterator i = m_mapNpcAISceneParameterSet.find(dwID);
	KGLOG_PROCESS_ERROR(i!=m_mapNpcAISceneParameterSet.end());

	m_mapNpcAISceneParameterSet.erase(i);

	return S_OK;
Exit0:
	return E_FAIL;
}

HRESULT KSceneEditorDocLogical::GetAISet(DWORD dwID, AIParameterSet& set)
{
    HRESULT hResult = E_FAIL;
	map<DWORD,AIParameterSet>::iterator it;
    
    KGLOG_PROCESS_ERROR(!m_mapNpcAISceneParameterSet.empty());
    
    it = m_mapNpcAISceneParameterSet.find(dwID);
	KGLOG_PROCESS_ERROR(it != m_mapNpcAISceneParameterSet.end());

	set = it->second;

	hResult = S_OK;
Exit0:
	return hResult;
}

HRESULT KSceneEditorDocLogical::SetAISet(DWORD dwID, AIParameterSet& set)
{
    HRESULT hResult = E_FAIL;
    map<DWORD,AIParameterSet>::iterator it;

    KGLOG_PROCESS_ERROR(!m_mapNpcAISceneParameterSet.empty());

    it = m_mapNpcAISceneParameterSet.find(dwID);
	KGLOG_PROCESS_ERROR(it != m_mapNpcAISceneParameterSet.end());

	it->second = set;

    hResult = S_OK;
Exit0:
    return hResult;
}

HRESULT KSceneEditorDocLogical::GetDefaultAIParameterDefine()
{
    HRESULT     hResult         = E_FAIL;
	TCHAR       szFileName[]    = "settings\\AIDefine.ini";
	IIniFile*   pFile           = g_OpenIniFile(szFileName,true,false);
    int         nCount          = 0;

    if (!pFile)
    {
        KGLogPrintf(KGLOG_ERR, "[AI] Can not open AI define file: \"settings\\AIDefine.ini\", please check your resource. ");
        goto Exit0;
    }

    m_AIParameterDefine.dwID = 0;
    m_AIParameterDefine.szName = "AI_PARAM_DEFINE";
    m_AIParameterDefine.vecParameters.clear();

    KGLOG_PROCESS_ERROR(pFile);

    pFile->GetInteger("MAIN", "Count", 0, &nCount);

    for (int i = 0; i < nCount; i++)
    {
        AIParameter par;

        LoadAIParameters(pFile, "MAIN", par, i);

        m_AIParameterDefine.vecParameters.push_back(par);
    }

    hResult = S_OK;
Exit0:
    SAFE_RELEASE(pFile);
    return hResult;
}

HRESULT KSceneEditorDocLogical::GetNpcAIDefaultValue()
{
    HRESULT             hResult         = E_FAIL;
	TCHAR               szFileName[]    = "settings\\AIType.tab";
	ITabFile*           pFile           = g_OpenTabFile(szFileName);
    int                 nHeight         = 0;
    int                 nWidth          = 0;
    std::vector<int >   vecRealIndex;

    KGLOG_PROCESS_ERROR(pFile);

	nHeight = pFile->GetHeight();
	nWidth  = pFile->GetWidth();

    vecRealIndex.clear();
    vecRealIndex.resize(nWidth - 2);
    for (int nCol = 3; nCol <= nWidth; nCol++)
    {
        int     nValue  = 0;
        int     nIndex  = -1;
        char    szColName[MAX_PATH];

        pFile->GetString(1, nCol, "", szColName, MAX_PATH);
        szColName[MAX_PATH - 1] = '\0';

        nIndex = m_AIParameterDefine.FindParameterByName(szColName);

        vecRealIndex[nCol - 3] = nIndex;
    }

	for (int nRow = 2; nRow <= nHeight; nRow++)
	{
        int nNpcTempleteID = 0;
		AIParameterSet newSet;

		pFile->GetInteger(nRow, 1, 0, &nNpcTempleteID);

		newSet.dwID     = nNpcTempleteID;
        newSet.szName   = "NPC_AI_TEMPLATE";
		newSet.vecParameters = m_AIParameterDefine.vecParameters;
		
		for (int nCol = 3; nCol <= nWidth; nCol++)
		{
			int         nValue      = 0;
            int         nIndex      = vecRealIndex[nCol - 3];
            const char* cpszColName = NULL;

            if (nIndex == -1)   // If we can't find it, drop it!
                continue;

            pFile->GetInteger(nRow, nCol, 0, &nValue);

            newSet.vecParameters[nIndex].nValue = nValue;
		}

		m_mapNpcAITemplateParameterSet[newSet.dwID] = newSet; 
	}

	hResult = S_OK;
Exit0:
    SAFE_RELEASE(pFile);
	return hResult;
}

HRESULT KSceneEditorDocLogical::SaveAIParameters(IIniFile* pFile,LPSTR pSectionName,AIParameter& par,int i)
{
	TCHAR KeyName[MAX_PATH];
	TCHAR KeyDesc[MAX_PATH];
	TCHAR KeyDefValue[MAX_PATH];
	TCHAR KeyValue[MAX_PATH];

	wsprintf(KeyName,"Name%d",i);
	wsprintf(KeyDesc,"Desc%d",i);
	wsprintf(KeyDefValue,"DefValue%d",i);
	wsprintf(KeyValue,"Value%d",i);

	pFile->WriteString(pSectionName,KeyName,par.strName.c_str());
	pFile->WriteString(pSectionName,KeyDesc,par.strDesc.c_str());
	pFile->WriteInteger(pSectionName,KeyDefValue,par.nDefaultValue);
	pFile->WriteInteger(pSectionName,KeyValue,par.nValue);

	return S_OK;
}

HRESULT KSceneEditorDocLogical::LoadAIParameters(IIniFile* pFile,LPSTR pSectionName,AIParameter& par,int i)
{
	TCHAR KeyName[MAX_PATH];
	TCHAR KeyDesc[MAX_PATH];
	TCHAR KeyDefValue[MAX_PATH];
	TCHAR KeyValue[MAX_PATH];

	wsprintf(KeyName,"Name%d",i);
	wsprintf(KeyDesc,"Desc%d",i);
	wsprintf(KeyDefValue,"DefValue%d",i);
	wsprintf(KeyValue,"Value%d",i);

	TCHAR szParameterName[MAX_PATH];
	TCHAR szParameterDesc[MAX_PATH];

	pFile->GetString(pSectionName,KeyName,"",szParameterName,MAX_PATH);
	pFile->GetString(pSectionName,KeyDesc,"",szParameterDesc,MAX_PATH);

	par.strName = szParameterName;
	par.strDesc = szParameterDesc;

	pFile->GetInteger(pSectionName,KeyDefValue,0,&par.nDefaultValue);
	pFile->GetInteger(pSectionName,KeyValue   ,0,&par.nValue);

	return S_OK;
}

HRESULT KSceneEditorDocLogical::SaveAISetToFile(IIniFile* pFile)
{
	KGLOG_PROCESS_ERROR(pFile);

	{
		int nCount = (int)m_mapNpcAISceneParameterSet.size();
		pFile->WriteInteger("MAIN","AISetCount",nCount);

		int K = 0;
		map<DWORD,AIParameterSet>::iterator i = m_mapNpcAISceneParameterSet.begin();
		map<DWORD,AIParameterSet>::iterator iend = m_mapNpcAISceneParameterSet.end();
		while (i!=iend)
		{
			DWORD dwID = i->first;
			AIParameterSet& AISet = i->second;

			TCHAR szSectionName[MAX_PATH];
			wsprintf(szSectionName,"AISET%d",K);

			pFile->WriteInteger(szSectionName,"ID",(int)dwID);
			pFile->WriteString (szSectionName,"Name",AISet.szName.c_str());
			int nParameterCount = (int)AISet.vecParameters.size();

			pFile->WriteInteger(szSectionName,"ParameterCount",nParameterCount);

			for (size_t s=0;s<AISet.vecParameters.size();s++)
			{
				AIParameter& par = AISet.vecParameters[s];
				SaveAIParameters(pFile,szSectionName,par,(int)s);
			}
			i++;
			K++;
		}
	}

	return S_OK;
Exit0:
	return E_FAIL;
}

HRESULT KSceneEditorDocLogical::LoadAISetFromFile(IIniFile* pFile)
{
    HRESULT hResult = E_FAIL;
    int     nCount  = 0;

	KGLOG_PROCESS_ERROR(pFile);

	pFile->GetInteger("MAIN", "AISetCount", 0, &nCount);

	for (int i = 0; i < nCount; i++)
	{
		int             nID             = 0;
        int             nParameterCount = 0;
		AIParameterSet  AISet;
		TCHAR           szSectionName[MAX_PATH];
		TCHAR           szName[MAX_PATH];

		wsprintf(szSectionName,"AISET%d",i);

		pFile->GetInteger(szSectionName, "ID", 0, &nID);
		pFile->GetString (szSectionName, "Name", 0, szName, MAX_PATH);

		AISet.dwID          = nID;
		AISet.szName        = szName;
        AISet.vecParameters = m_AIParameterDefine.vecParameters;

		pFile->GetInteger(szSectionName, "ParameterCount", 0, &nParameterCount);
		for (int nParamIndex = 0; nParamIndex < nParameterCount; nParamIndex++)
		{
            int         nIndex = -1;
			AIParameter par;

			LoadAIParameters(pFile, szSectionName, par, nParamIndex);

            nIndex = AISet.FindParameterByName((LPSTR)par.strName.c_str()); // If we can't find it, drop it. 
            if (nIndex != -1)
            {
                AISet.vecParameters[nIndex] = par;
            }
		}

		m_mapNpcAISceneParameterSet[nID] = AISet;
		m_dwCurrentAIParameterSetID = max((DWORD)nID+1, m_dwCurrentAIParameterSetID);
	}

	hResult = S_OK;
Exit0:
	return hResult;
}

HRESULT KSceneEditorDocLogical::GetNpcDefauleAIParameterSetByTempleteID(int nTempleteID, AIParameterSet* pRetAIParameterSet)
{
    HRESULT hResult = E_FAIL;
	map<DWORD,AIParameterSet>::iterator iFind;
    
    KG_PROCESS_ERROR(pRetAIParameterSet);

    iFind = m_mapNpcAITemplateParameterSet.find(nTempleteID);
    KG_PROCESS_ERROR(iFind != m_mapNpcAITemplateParameterSet.end());

	*pRetAIParameterSet = iFind->second;

    hResult = S_OK;
Exit0:
	return hResult;
}

BOOL KSceneEditorDocLogical::AIParameterSet::IsEqual(AIParameterSet& set)
{
	if(set.vecParameters.size() != vecParameters.size())
		return FALSE;
	for (size_t i=0;i<vecParameters.size();i++)
	{
		if(set.vecParameters[i].nValue != vecParameters[i].nValue)
		{
			return FALSE;
		}
	}

	return TRUE;
}

DWORD KSceneEditorDocLogical::FindAIParameterSet(AIParameterSet& set)
{
	HRESULT hr = E_FAIL;
	DWORD dwNewID = 0;

    ASSERT(set.vecParameters.size() > 0);

	{
		map<DWORD,AIParameterSet>::iterator i = m_mapNpcAISceneParameterSet.begin();
		map<DWORD,AIParameterSet>::iterator iend = m_mapNpcAISceneParameterSet.end();
		while (i!=iend)
		{
			AIParameterSet& aiset = i->second;
			if(aiset.IsEqual(set))
			{
				DWORD dwID = i->first;
				return dwID;
			}
			i++;
		}
	}
	//没有找到就创建新的
	hr = NewOneAISet(dwNewID,"NewSet");
	KGLOG_COM_PROCESS_ERROR(hr);

	SetAISet(dwNewID,set);

	return dwNewID;
Exit0:
	return 0;
}


HRESULT KSceneEditorDocLogical::UpdataOneNpcAIParameters(NPCInfo* pNPC)
{
    HRESULT hResult     = E_FAIL;
    HRESULT hRetCode    = E_FAIL;

    KG_PROCESS_ERROR(pNPC);

    if (pNPC->dwNpcAISet)
	{
		hRetCode = GetAISet(pNPC->dwNpcAISet,pNPC->m_cAIParameterSet);
        KGLOG_COM_PROCESS_ERROR(hRetCode);
	}
    else
    {
        hRetCode = GetNpcDefauleAIParameterSetByTempleteID(pNPC->nTempleteID, &pNPC->m_cAIParameterSet);
        KGLOG_COM_PROCESS_ERROR(hRetCode);
    }

    hResult = S_OK;
Exit0:
	return hResult;
}

HRESULT KSceneEditorDocLogical::UpdataNpcAIParameters()
{
    DWORD dwCount = 0;
    list<WayPointSet*>::iterator itWayPoint = m_listWayPointSet.begin();
    list<WayPointSet*>::iterator itWayPointEnd = m_listWayPointSet.end();
	list<NPCInfo*>::iterator itNpc = m_listNPCInfo.begin();
	list<NPCInfo*>::iterator itNpcEnd = m_listNPCInfo.end();

    while (itWayPoint != itWayPointEnd)
    {
        WayPointSet* pPath = *itWayPoint;

        pPath->nIndex = ++dwCount;

        ++itWayPoint;
    }

	while (itNpc != itNpcEnd)
	{
		NPCInfo* pNPC = *itNpc;

		UpdataOneNpcAIParameters(pNPC);
		//ExchangeOldSettingToNpcAIParameter(pNPC);

		++itNpc;
	}

	return S_OK;
}

int  KSceneEditorDocLogical::AIParameterSet::FindParameterByName(LPSTR pName)
{
	for (size_t i=0;i<vecParameters.size();i++)
	{
		if(_strcmpi(vecParameters[i].strName.c_str(),pName)==0)
		{
			return (int)i;
		}
	}
	return -1;
}

HRESULT SetAIParam(KSceneEditorDocLogical::AIParameterSet& rAISet, char szParamName[], int nValue)
{
    HRESULT                                 hResult     = E_FAIL;
    int                                     nIndex      = 0;
    KSceneEditorDocLogical::AIParameter*    pAIParam    = NULL;

    nIndex = rAISet.FindParameterByName(szParamName);
    KGLOG_PROCESS_ERROR(nIndex >= 0 && nIndex < (int)rAISet.vecParameters.size());

    pAIParam = &(rAISet.vecParameters[nIndex]);
    pAIParam->nValue = nValue;

    hResult = S_OK;
Exit0:
	return hResult;
}

HRESULT KSceneEditorDocLogical::ExchangeOldSettingToNpcAIParameter(NPCInfo* pNpcInfo)
{
	HRESULT         hResult         = E_FAIL;
//	HRESULT         hRetCode        = E_FAIL;
//	BOOL            bUseSO3Logic    = false;
//	DWORD           dwAISetID       = 0;
//	int             nAIParamIndex   = 0;
//	AIParameterSet  AISetTemplate;
//
//	KGLOG_PROCESS_ERROR(pNpcInfo);
//
//	hRetCode = g_pEngineManager->GetUseSO3Logical(&bUseSO3Logic);
//	KGLOG_COM_PROCESS_ERROR(hRetCode);
//	KG_PROCESS_SUCCESS(!bUseSO3Logic);
//
//	if (pNpcInfo->nTempleteID == 0)
//	{
//		KGLogPrintf(KGLOG_ERR, "Abnormal NPC data! x: %d, y: %d, z: %d", pNpcInfo->nX, pNpcInfo->nY, pNpcInfo->nZ);
//		KG_PROCESS_SUCCESS(true);
//	}
//
//	ASSERT(pNpcInfo->dwNpcAISet < 8192);
//	if (pNpcInfo->dwNpcAISet)
//	{
//		KGLogPrintf(KGLOG_DEBUG, "Already have AISet, name: %s, set: %lu", pNpcInfo->szName.c_str(), pNpcInfo->dwNpcAISet);
//		KG_PROCESS_SUCCESS(true);
//	}
//
//	hRetCode = GetNpcDefauleAIParameterSetByTempleteID(pNpcInfo->nTempleteID, &AISetTemplate);
//	KGLOG_COM_PROCESS_ERROR(hRetCode);
//
//	pNpcInfo->m_cAIParameterSet = AISetTemplate;
//	pNpcInfo->m_cAIParameterSet.szName = "AUTO_CONVERT_AI_SET";
//	pNpcInfo->dwNpcAISet = 0;
//
//	// IsAutoTurn
//	if (pNpcInfo->nDisableAutoTurn == 1)
//	{
//		hRetCode = SetAIParam(pNpcInfo->m_cAIParameterSet, "IsAutoTurn", 0);
//		KGLOG_COM_PROCESS_ERROR(hRetCode);
//	}
//
//	if (pNpcInfo->nDisableAutoTurn == 0)
//	{
//		hRetCode = SetAIParam(pNpcInfo->m_cAIParameterSet, "IsAutoTurn", 1);
//		KGLOG_COM_PROCESS_ERROR(hRetCode);
//	}
//
//	if (pNpcInfo->pWayPointSet)   // Ñ²Âß
//	{
//		hRetCode = SetAIParam(pNpcInfo->m_cAIParameterSet, "PatrolPathID", pNpcInfo->pWayPointSet->nIndex);
//		KGLOG_COM_PROCESS_ERROR(hRetCode);
//
//		hRetCode = SetAIParam(pNpcInfo->m_cAIParameterSet, "PatrolOrderIndex", pNpcInfo->dwNpcTeamIndex);
//		KGLOG_COM_PROCESS_ERROR(hRetCode);
//	}
//	else if (pNpcInfo->nAIMainState == aisWander)   // ÏÐ¹ä
//	{
//		hRetCode = SetAIParam(pNpcInfo->m_cAIParameterSet, "PatrolPathID", 0);
//		KGLOG_COM_PROCESS_ERROR(hRetCode);
//
//		nAIParamIndex = pNpcInfo->m_cAIParameterSet.FindParameterByName("WanderRange");
//		KGLOG_PROCESS_ERROR(nAIParamIndex >= 0);
//
//		if (pNpcInfo->m_cAIParameterSet.vecParameters[nAIParamIndex].nValue == 0)
//		{
//			hRetCode = SetAIParam(pNpcInfo->m_cAIParameterSet, "WanderRange", 1024);
//			KGLOG_COM_PROCESS_ERROR(hRetCode);
//
//			hRetCode = SetAIParam(pNpcInfo->m_cAIParameterSet, "WanderDistance", 768);
//			KGLOG_COM_PROCESS_ERROR(hRetCode);
//
//			hRetCode = SetAIParam(pNpcInfo->m_cAIParameterSet, "WanderInterval", 80);
//			KGLOG_COM_PROCESS_ERROR(hRetCode);
//		}
//	}
//	else if (pNpcInfo->nAIMainState == aisIdle)     // ·¢´ô
//	{
//		hRetCode = SetAIParam(pNpcInfo->m_cAIParameterSet, "PatrolPathID", 0);
//		KGLOG_COM_PROCESS_ERROR(hRetCode);
//
//		hRetCode = SetAIParam(pNpcInfo->m_cAIParameterSet, "WanderRange", 0);
//		KGLOG_COM_PROCESS_ERROR(hRetCode);
//	}
//
//	if (!AISetTemplate.IsEqual(pNpcInfo->m_cAIParameterSet))
//	{
//		pNpcInfo->dwNpcAISet = FindAIParameterSet(pNpcInfo->m_cAIParameterSet);
//
//		if (pNpcInfo->pPointer)
//		{
//			IEKG3DRepresentNPC* pRepNPC = m_lpSceneSceneEditor->GetIEKG3DRepresentNPC(pNpcInfo->pPointer);
//			if (pRepNPC)
//				pRepNPC->SetAISetID(pNpcInfo->dwNpcAISet);
//		}
//	}
//
//Exit1:
//	hResult = S_OK;
//Exit0:
	return hResult;
}

HRESULT KSceneEditorDocLogical::ResortNpcAISet()
{
	m_mapNpcAISceneParameterSet.clear();
	m_dwCurrentAIParameterSetID = 1;

	list<NPCInfo*>::iterator i = m_listNPCInfo.begin();
	list<NPCInfo*>::iterator iend = m_listNPCInfo.end();
	while (i!=iend)
	{
		KSceneEditorDocLogical::NPCInfo* pNpc = *i;

		if (pNpc->nTempleteID == 0)
		{
			KGLogPrintf(KGLOG_ERR, "Abnormal NPC data! x: %d, y: %d, z: %d", pNpc->nX, pNpc->nY, pNpc->nZ);
			++i;
			continue;
		}

		if(pNpc->pWayPointSet)
		{
			pNpc->nPathIndex = pNpc->pWayPointSet->nIndex;
		}
		else
		{
			pNpc->nPathIndex = 0;
		}

		if (pNpc->dwNpcAISet)
		{
			pNpc->dwNpcAISet = FindAIParameterSet(pNpc->m_cAIParameterSet);
		}

		++i;
	}

	return S_OK;
}

int splitStr(std::list<std::string>& l, const std::string& seq, char s1, char s2, bool keeptok)
{
	typedef std::string::size_type ST;
	std::vector<int> tok_s1;
	std::vector<int> tok_s2;
	if(l.size()) l.clear();
	ST pos=0, start=0, LEN=seq.size();
	while( pos < LEN ){
		if(seq[pos] == s1){
			start = pos;
			if(s2){
				while( (pos <LEN) && (seq[pos] != s2)) ++pos;
				if(pos <LEN){
					tok_s2.push_back(pos);
					tok_s1.push_back(start);
					start = pos+1;
				}
			}
			else tok_s1.push_back(start);
		}
		++pos;
	}

	if(s2){
		if( ( tok_s1.size() != tok_s2.size() ) || (tok_s1.size() == 0) ){
			//screwed: return the original string
			l.push_back(seq);
			return 1;
		}

		if(tok_s1.size()){
			if(tok_s1[0]) l.push_back(seq.substr(0, tok_s1[0] - (keeptok ? 0: 1)) );
			for(pos = 0; pos < tok_s1.size(); pos++){
				if(pos>0){
					int c = tok_s1[pos] - tok_s2[pos-1];
					if(c > 1) l.push_back(seq.substr( tok_s2[pos-1]+1, c-1));
				}
				l.push_back(seq.substr( tok_s1[pos], tok_s2[pos]-tok_s1[pos]+1 ));
			}
		}

		if( tok_s2.back() < (LEN-1)) l.push_back(seq.substr(tok_s2.back()+1, (LEN)-(tok_s2.back()+1)));
	}

	return l.size();
}


int splitStr(std::list<std::string>& L, const std::string& seq, const std::string& _1cdelim, bool keeptoken, bool _removews )
{
	typedef std::string::size_type ST;
	std::string delims = _1cdelim;
	std::string STR;
	if(delims.empty()) delims = "\n\r";
	if(_removews) delims += " ";

	ST pos=0, LEN = seq.size();
	while(pos < LEN ){
		STR=""; // Init/clear the STR token buffer
		// remove any delimiters including optional (white)spaces
		while( (delims.find(seq[pos]) != std::string::npos) && (pos < LEN) ) ++pos;
		// leave if @eos
		if(pos==LEN) return L.size();
		// Save token data
		while( (delims.find(seq[pos]) == std::string::npos) && (pos < LEN) ) STR += seq[pos++];
		// put valid STR buffer into the supplied list
		//std::cout << "[" << STR << "]";
		if( ! STR.empty() ) L.push_back(STR);
	}

	return L.size();
}

__int64 Str2Int64(char* str)
{
	if(strstr(str,"0x") || strstr(str,"0X"))
	{
		str += 2;
		__int64 tmp = 0;
		int len = strlen(str);
		for(int i = 0;i < len ;i++)
		{
			int nDecNum;
			switch(str[i])
			{
			case 'a':
			case 'A':
				nDecNum = 10;
				break;
			case 'b':
			case 'B':
				nDecNum = 11;
				break;
			case 'c':
			case 'C':
				nDecNum = 12;
				break;
			case 'd':
			case 'D':
				nDecNum = 13;
				break;
			case 'e':
			case 'E':
				nDecNum = 14;
				break;
			case 'f':
			case 'F':
				nDecNum = 15;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				nDecNum = str[i] - '0';
				break;
			default:
				return 0;

			}
			tmp += nDecNum*pow(16.f, len-i-1);
		}
		return tmp;
	}
	else
	{
		return _atoi64(str);
	}
}

HRESULT KSceneEditorDocLogical::GetObjCount(BSTR ObjType, LONG fatherindex, LONG* ObjCount)
{
	if (_strcmpi(BSTR_TO_STRING(ObjType), "NPC") == 0)
		*ObjCount = m_listNPCInfo.size();
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "Doodad") == 0)
		*ObjCount = m_listDoodadInfo.size();
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "AIGroup") == 0)
		*ObjCount = m_listAIGroup.size();
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "AIGroupNPC") == 0)
	{
		int i = 0;
		for (list<AIGroup*>::iterator it = m_listAIGroup.begin(); it != m_listAIGroup.end(); it++)
		{
			AIGroup* pGroup = *it;
			if (i++ == fatherindex)
			{
				*ObjCount = pGroup->vecNpcInfo.size();
				break;
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "WayPointSet") == 0)
		*ObjCount = m_listWayPointSet.size();
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "WayPoint") == 0)
	{
		int i = 0;
		for (list<WayPointSet*>::iterator it = m_listWayPointSet.begin(); it != m_listWayPointSet.end(); it++)
		{
			WayPointSet* pSet = *it;
			if (i++ == fatherindex)
			{
				*ObjCount = pSet->vectorWayPoint.size();
				break;
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "LogicalPoly") == 0)
		*ObjCount = m_listLogicalPoly.size();
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "LogicalPolyVertex") == 0)
	{
		int i = 0;
		for (list<IEKG3DRepresentPoly*>::iterator it = m_listLogicalPoly.begin(); it != m_listLogicalPoly.end(); it++)
		{
			IEKG3DRepresentPoly* pPoly = *it;
			if (i++ == fatherindex)
			{
				*ObjCount = pPoly->GetVertexCount();
				break;
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "TrafficLittlePointSet") == 0)
		*ObjCount = m_listTrafficPointSet.size();
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "TrafficLittlePoint") == 0)
	{
		int i = 0;
		for (list<TrafficPointSet*>::iterator it = m_listTrafficPointSet.begin(); it != m_listTrafficPointSet.end(); it++)
		{
			TrafficPointSet* pSet = *it;
			if (i++ == fatherindex)
			{
				*ObjCount = pSet->vecTrafficPoint.size();
				break;
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "TrafficPoint") == 0)
		*ObjCount = m_mapTrafficPoint.size();
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "NpcReviveGroup") == 0)
		*ObjCount = m_mapNpcReviveGroup.size();
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "NpcReviveGroupNpc") == 0)
	{
		int i = 0;
		for (map<DWORD,ReviveGroup>::iterator it = m_mapNpcReviveGroup.begin(); it != m_mapNpcReviveGroup.end(); it++)
		{
			ReviveGroup& group = it->second;
			if (i++ == fatherindex)
			{
				*ObjCount = group.vecNpcInfo.size();
				break;
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "DoodadReviveGroup") == 0)
		*ObjCount = m_mapDoodadReviveGroup.size();
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "DoodadReviveGroupDoodad") == 0)
	{
		int i = 0;
		for (map<DWORD,ReviveGroup>::iterator it = m_mapDoodadReviveGroup.begin(); it != m_mapDoodadReviveGroup.end(); it++)
		{
			ReviveGroup& group = it->second;
			if (i++ == fatherindex)
			{
				*ObjCount = group.vecDoodadInfo.size();
				break;
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "NpcRandomGroup") == 0)
		*ObjCount = m_mapRandomGroup.size();
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "AISet") == 0)
		*ObjCount = m_mapNpcAISceneParameterSet.size();
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "AISetParam") == 0)
	{
		int i = 0;
		for (map<DWORD, AIParameterSet>::iterator it = m_mapNpcAISceneParameterSet.begin(); it != m_mapNpcAISceneParameterSet.end(); it++)
		{
			AIParameterSet& Set = it->second;
			if (i++ == fatherindex)
			{
				*ObjCount = Set.vecParameters.size();
				break;
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "NpcHatredGroup") == 0)
	{
		*ObjCount = m_npcHatredGroupMap.size();
	}

	return S_OK;
}

HRESULT KSceneEditorDocLogical::GetSelectedLogicObjPtr(LONG index, LONG* logicObjPtr, BSTR* ObjType)
{
	*logicObjPtr = 0;
	*ObjType = STRING_TO_BSTR("Unknown");

	//m_lpSceneSceneEditor->GetSelectedEntity(index, &pEntity);
	IEKG3DSceneSelectionTool* pSelectionTool = NULL;
	m_lpSceneSceneEditor->GetSelectionToolInterface(&pSelectionTool);

	IEKG3DSceneSelectionToolAccessProxy* pAccessProxy = NULL;
	pSelectionTool->GetAccessProxy(&pAccessProxy);
	_ASSERTE(NULL != pAccessProxy);

	IEKG3DSceneEntity* pEntity = pAccessProxy->Reset();
	for (int i = 0; i < index; i++)
	{
		pEntity = pAccessProxy->Next();
	}

	SCENEENTITYTYPE EntityType = SCENEENTITY_NONE;
	pEntity->GetType(&EntityType);
	if (EntityType == SCENEENTITY_OBJECT)
	{
		IEKG3DRepresentObject* pObject = NULL;
		pEntity->GetPointer((VOID**)&pObject);
		if (pObject->GetType() == REPRESENTOBJECT_NPC)
		{
			NPCInfo* pNPCinfo = NULL;
			if (SUCCEEDED(FindNPCInfoByEditorNPC(&pNPCinfo, pObject)))
			{
				*logicObjPtr = (LONG)pNPCinfo;
				*ObjType = STRING_TO_BSTR("NPC");
			}
		}
		else if (pObject->GetType() == REPRESENTOBJECT_DOODAD)
		{
			DoodadInfo* pDoodadInfo = NULL;
			if (SUCCEEDED(FindDoodadInfoByEditorDoodad(&pDoodadInfo, pObject)))
			{
				*logicObjPtr = (LONG)pDoodadInfo;
				*ObjType = STRING_TO_BSTR("Doodad");
			}
		}
		else if (pObject->GetType() == REPRESENTOBJECT_WAYPOINT)
		{
			IEKG3DRepresentWayPoint* pRepWayPoint = m_lpSceneSceneEditor->GetIEKG3DRepresentWayPoint(pObject);

			DWORD dwSetID = 0;
			dwSetID = pRepWayPoint->GetSetID();
			WayPointSet* pSet = NULL;

			if(SUCCEEDED(GetWayPointSetByID(&pSet, dwSetID)))
			{
				WayPoint* pWayPoint = NULL;
				if(SUCCEEDED(pSet->FindWayPointByEditorWayPoint(&pWayPoint, pObject)))
				{
					*logicObjPtr = (LONG)pWayPoint;
					*ObjType = STRING_TO_BSTR("WayPoint");
				}
			}
		}
		else if (pObject->GetType() == REPRESENTOBJECT_POLY)
		{
			IEKG3DRepresentPoly* pIEPoly = m_lpSceneSceneEditor->GetSelectedPoly();
			*logicObjPtr = (LONG)pIEPoly;
			*ObjType = STRING_TO_BSTR("LogicalPoly");
		}
		else if (pObject->GetType() == REPRESENTOBJECT_TRAFFICPOINT)
		{
			TrafficLittlePoint* pTrafficLittlePoint = NULL;
			TrafficPoint* pTrafficPoint = NULL;
			if (SUCCEEDED(GetTrafficPointByEditorPoint(&pTrafficLittlePoint, pObject)))
			{
				*logicObjPtr = (LONG)pTrafficLittlePoint;
				*ObjType = STRING_TO_BSTR("TrafficLittlePoint");
			}
			else if (SUCCEEDED(GetBigTrafficPointByEditorPoint(&pTrafficPoint, pObject)))
			{
				*logicObjPtr = (LONG)pTrafficPoint;
				*ObjType = STRING_TO_BSTR("TrafficPoint");
			}
		}

		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

HRESULT KSceneEditorDocLogical::GetSetObjInfo(BSTR ObjType, AtlObjInfo* AtlInfo, LONG LogicObj, LONG bGet)
{
	if (_strcmpi(BSTR_TO_STRING(ObjType), "NPC") == 0)
	{
		NPCInfo* logicInfo = (NPCInfo*)LogicObj;
		AtlInfo->strType = STRING_TO_BSTR("NPC");
		int __i = 0;
		EXCHANGE_OBJ_STR(szName);
		EXCHANGE_OBJ_STR(szNickName);
		EXCHANGE_OBJ_STR(szScriptName);
		EXCHANGE_OBJ_INT(nIndex);
		EXCHANGE_OBJ_INT(nTempleteID);
		EXCHANGE_OBJ_INT(nX);
		EXCHANGE_OBJ_INT(nY);
		EXCHANGE_OBJ_INT(nZ);
		EXCHANGE_OBJ_INT(nKind);
		EXCHANGE_OBJ_INT(nFaceDirection);
		//EXCHANGE_OBJ_INT(nDisableAutoTurn);
		//EXCHANGE_OBJ_INT(nAIType);	
		//EXCHANGE_OBJ_INT(nAIMainState);
		EXCHANGE_OBJ_INT(nLevel);
		EXCHANGE_OBJ_STRUCT(dwScriptID, 4);
		EXCHANGE_OBJ_STRUCT(dwRepresentID, 4);	
		EXCHANGE_OBJ_VEC(vPosition);
		EXCHANGE_OBJ_QUA(vRotation);
		EXCHANGE_OBJ_INT_MEM_MEM_0_TO_MINUS1(WayPointSetID, pWayPointSet, dwSetID);
		EXCHANGE_OBJ_INT_MEM(ReliveID, dwNpcReliveID);	
		EXCHANGE_OBJ_INT_MEM(RandomID, dwNpcRandomID);
		EXCHANGE_OBJ_INT_MEM(AISetID, dwNpcAISet);
		// ahpho suntao
		EXCHANGE_OBJ_INT_MEM(TeamID, dwNpcTeamID);
		// end ahpho
		AtlInfo->iFieldsCount = 20;
	}

	// ahpho suntao
	else if(_strcmpi(BSTR_TO_STRING(ObjType), "WayPointSet") == 0)
	{
		WayPointSet* logicInfo = (WayPointSet*)LogicObj;
		AtlInfo->strType = STRING_TO_BSTR("WayPointSet");
		int __i = 0;
		EXCHANGE_OBJ_STR(szName);
		EXCHANGE_OBJ_INT(dwSetID);		
		EXCHANGE_OBJ_INT(nIndex);
		
		AtlInfo->iFieldsCount = 3;	
	}
	else if(_strcmpi(BSTR_TO_STRING(ObjType), "WayPoint") == 0)
	{
		WayPoint* logicInfo = (WayPoint*)LogicObj;
		AtlInfo->strType = STRING_TO_BSTR("WayPoint");
		int __i = 0;
		EXCHANGE_OBJ_VEC(vPosition);
		EXCHANGE_OBJ_QUA(vRotation);
		EXCHANGE_OBJ_INT(nX);
		EXCHANGE_OBJ_INT(nY);
		EXCHANGE_OBJ_INT(nZ);
		EXCHANGE_OBJ_INT(nStayFrame);
		EXCHANGE_OBJ_INT(nStayDirection);
		EXCHANGE_OBJ_INT(nStayAnimation);
		EXCHANGE_OBJ_INT(bStayAniLoop);
		EXCHANGE_OBJ_INT(bIsRun);
		EXCHANGE_OBJ_STR(szScriptName);
		EXCHANGE_OBJ_STR(szInfo);

		AtlInfo->iFieldsCount = 12;	
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "TrafficLittlePointSet") == 0)
	{
		TrafficPointSet* logicInfo = (TrafficPointSet*)LogicObj;
		AtlInfo->strType = STRING_TO_BSTR("TrafficLittlePointSet");
		int __i = 0;
		EXCHANGE_OBJ_STR(szName);
		EXCHANGE_OBJ_INT(m_dwID);		
		EXCHANGE_OBJ_INT(m_dwCostMoney);
		EXCHANGE_OBJ_INT(m_dwVelocity);
		EXCHANGE_OBJ_INT(m_dwStep);
		EXCHANGE_OBJ_INT(m_dwFromID);
		EXCHANGE_OBJ_INT(m_dwToID);

		AtlInfo->iFieldsCount = 7;
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "TrafficLittlePoint") == 0)
	{
		TrafficLittlePoint* logicInfo = (TrafficLittlePoint*)LogicObj;
		AtlInfo->strType = STRING_TO_BSTR("TrafficLittlePoint");
		int __i = 0;		
		EXCHANGE_OBJ_VEC(vPosition);
		AtlInfo->iFieldsCount = 1;
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "TrafficPoint") == 0)
	{
		TrafficPoint* logicInfo = (TrafficPoint*)LogicObj;
		AtlInfo->strType = STRING_TO_BSTR("TrafficPoint");
		int __i = 0;		
		EXCHANGE_OBJ_VEC(vPosition);
		EXCHANGE_OBJ_INT(m_dwType);
		EXCHANGE_OBJ_INT(m_nLinkID);
		EXCHANGE_OBJ_INT(m_dwNodeID);
		EXCHANGE_OBJ_INT(m_dwNodeID);
		EXCHANGE_OBJ_STR(m_strDescription);
		EXCHANGE_OBJ_INT(m_dwMapID);
		EXCHANGE_OBJ_INT(m_dwCityID);
		EXCHANGE_OBJ_INT(m_dwForceID);
		EXCHANGE_OBJ_INT(m_dwTongID);
		EXCHANGE_OBJ_INT(m_bNeedOpen);
		EXCHANGE_OBJ_STR(m_strScriptFile);
		AtlInfo->iFieldsCount = 12;
	}

	else if (_strcmpi(BSTR_TO_STRING(ObjType), "NpcReviveGroup") == 0)
	{
		ReviveGroup* logicInfo = (ReviveGroup*)LogicObj;
		AtlInfo->strType = STRING_TO_BSTR("NpcReviveGroup");
		int __i = 0;		
		EXCHANGE_OBJ_INT(dwGroupID);
		EXCHANGE_OBJ_STR(szName);
		EXCHANGE_OBJ_INT(nMinNpcCount);
		EXCHANGE_OBJ_INT(nMaxNpcCount);
		EXCHANGE_OBJ_INT(nRandom);
		AtlInfo->iFieldsCount = 5;
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "DoodadReviveGroup") == 0)
	{
		ReviveGroup* logicInfo = (ReviveGroup*)LogicObj;
		AtlInfo->strType = STRING_TO_BSTR("DoodadReviveGroup");
		int __i = 0;		
		EXCHANGE_OBJ_INT(dwGroupID);
		EXCHANGE_OBJ_STR(szName);
		EXCHANGE_OBJ_INT(nMinNpcCount);
		EXCHANGE_OBJ_INT(nMaxNpcCount);
		EXCHANGE_OBJ_INT(nRandom);
		AtlInfo->iFieldsCount = 5;
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "Doodad") == 0)
	{
		DoodadInfo* logicInfo = (DoodadInfo*)LogicObj;
		AtlInfo->strType = STRING_TO_BSTR("Doodad");
		int __i = 0;
		EXCHANGE_OBJ_STR(szName);
		EXCHANGE_OBJ_STR(szNickName);
		EXCHANGE_OBJ_STR(szScriptName);
		EXCHANGE_OBJ_INT(nIndex);
		EXCHANGE_OBJ_INT(nTempleteID);
		EXCHANGE_OBJ_INT(nX);
		EXCHANGE_OBJ_INT(nY);
		EXCHANGE_OBJ_INT(nZ);
		EXCHANGE_OBJ_INT(nKind);
		EXCHANGE_OBJ_INT(nFaceDirection);
		//EXCHANGE_OBJ_INT(nAIType);	
		EXCHANGE_OBJ_INT(nLevel);
		EXCHANGE_OBJ_STRUCT(dwScriptID, 4);	
		EXCHANGE_OBJ_VEC(vPosition);
		EXCHANGE_OBJ_QUA(vRotation);
		EXCHANGE_OBJ_INT_MEM(ReliveID, dwDoodadReliveID);	
		AtlInfo->iFieldsCount = 15;
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "LogicalPoly") == 0)
	{
		IEKG3DRepresentPoly* logicInfo_ = (IEKG3DRepresentPoly*)LogicObj;
		KG3DRepresentPoly* logicInfo = dynamic_cast<KG3DRepresentPoly*>(logicInfo_);
		AtlInfo->strType = STRING_TO_BSTR("LogicalPoly");
		int __i = 0;
		//EXCHANGE_OBJ_STR_MEM(szScriptName, m_szScriptFileName);
		if (bGet)
		{
			AtlInfo->strFields[__i  ] = STRING_TO_BSTR("szScriptName");
			AtlInfo->strValues[__i++] = STRING_TO_BSTR(logicInfo->GetScriptFileName());
		}
		else
		{
			string _str(BSTR_TO_STRING(AtlInfo->strValues[__i++]));
			logicInfo->SetScriptFileName(_str.c_str());
		}
		EXCHANGE_OBJ_STR_MEM(scName, m_scName);
		EXCHANGE_OBJ_INT_MEM(LogicalSetID, m_nLogicalSetID);
		EXCHANGE_OBJ_INT_MEM(ModifyState, m_dwModifyState);
		EXCHANGE_OBJ_INT_MEM(Color, m_dwColor);
		EXCHANGE_OBJ_FLO_MEM_MEMS(Height, m_Poly, m_fHeight);
		AtlInfo->iFieldsCount = 6;
	}

	return S_OK;
}

HRESULT KSceneEditorDocLogical::GetAtlObjRepresentInfo(AtlObjInfo* atlinfo, D3DXVECTOR3* translation, D3DXQUATERNION* rotation, D3DXVECTOR3* scale)
{
	string strTrans("");
	string strRotation("");
	string strScale("");

	if (_strcmpi(BSTR_TO_STRING(atlinfo->strType), "NPC") == 0)
	{
		strTrans = BSTR_TO_STRING(atlinfo->strValues[13]);
		strRotation = BSTR_TO_STRING(atlinfo->strValues[14]);
	}
	else if (_strcmpi(BSTR_TO_STRING(atlinfo->strType), "Doodad") == 0)
	{
		strTrans = BSTR_TO_STRING(atlinfo->strValues[12]);
		strRotation = BSTR_TO_STRING(atlinfo->strValues[13]);
	} 
	else if (_strcmpi(BSTR_TO_STRING(atlinfo->strType), "WayPoint") == 0)
	{
		strTrans = BSTR_TO_STRING(atlinfo->strValues[0]);
		strRotation = BSTR_TO_STRING(atlinfo->strValues[1]);
	}
	else if (_strcmpi(BSTR_TO_STRING(atlinfo->strType), "TrafficLittlePoint") == 0)
	{
		strTrans = BSTR_TO_STRING(atlinfo->strValues[0]);

	}

	string sp(",");
	string _sub;
	list<string>::iterator _it;
	
	if (strTrans.length() > 0)
	{
		list<string> strlistTrans;
		splitStr(strlistTrans, strTrans, sp, false, true);
		_it = strlistTrans.begin();
		_sub = *_it++; 
		translation->x = atof(_sub.c_str());
		_sub = *_it++;
		translation->y = atof(_sub.c_str());
		_sub = *_it;
		translation->z = atof(_sub.c_str());
	}
	
	if (strRotation.length() > 0)
	{
		list<string> strlistRotation;
		splitStr(strlistRotation, strRotation, sp, false, true);
		_it = strlistRotation.begin();
		_sub = *_it++; 
		rotation->x = atof(_sub.c_str());
		_sub = *_it++;
		rotation->y = atof(_sub.c_str());
		_sub = *_it++;
		rotation->z = atof(_sub.c_str());
		_sub = *_it;
		rotation->w = atof(_sub.c_str());
	}	

	return S_OK;
}

HRESULT KSceneEditorDocLogical::UpdateObjRepresentByLogicObj(AtlObjInfo* objInfo, LONG logicObj)
{
	D3DXVECTOR3 translation;
	D3DXQUATERNION rotation;
	D3DXVECTOR3 scale;
	GetAtlObjRepresentInfo(objInfo, &translation, &rotation, &scale);

	IEKG3DRepresentObject* representObj = NULL;

	if (_strcmpi(BSTR_TO_STRING(objInfo->strType), "NPC") == 0)
	{
		NPCInfo* sourceinfo = (NPCInfo*)logicObj;
		representObj = (IEKG3DRepresentObject*)sourceinfo->pPointer;
		representObj->SetTranslation(&translation);
		representObj->SetRotation(&rotation);
	}
	// ahpho suntao
	else if (_strcmpi(BSTR_TO_STRING(objInfo->strType), "WayPoint") == 0) // 路径点
	{
		WayPoint* sourceinfo = (WayPoint*)logicObj;
		representObj = (IEKG3DRepresentObject*)sourceinfo->pPointer;
		representObj->SetTranslation(&translation);
		representObj->SetRotation(&rotation);
	}
	else if (_strcmpi(BSTR_TO_STRING(objInfo->strType), "TrafficLittlePoint") == 0) // 交通点
	{
		TrafficLittlePoint* sourceinfo = (TrafficLittlePoint*)logicObj;
		representObj = (IEKG3DRepresentObject*)sourceinfo->pPointer;
		representObj->SetTranslation(&translation);
	}
	// end ahpho

	/*
	if (representObj)
	{
		representObj->SetTranslation(&translation);
		representObj->SetRotation(&rotation);
		//representObj->SetScaling(&scale);
	}	
	*/

	return S_OK;
}

HRESULT KSceneEditorDocLogical::GetObjDisplayInfo(BSTR ObjType, LONG index, LONG fatherindex, BSTR* name, BSTR* nickname, LONG* hasscript, LONG* representObjPtr, LONG* logicObjPtr)
{
	*name = STRING_TO_BSTR("");
	*nickname = STRING_TO_BSTR("");
	*hasscript = 0;
	*representObjPtr = 0;
	*logicObjPtr = 0;

	if (_strcmpi(BSTR_TO_STRING(ObjType), "NPC") == 0)
	{
		for (list<NPCInfo*>::iterator it = m_listNPCInfo.begin(); it != m_listNPCInfo.end(); it++)
		{
			NPCInfo* sourceinfo = *it;
			if (sourceinfo->nIndex == index)
			{
				*representObjPtr = (LONG)sourceinfo->pPointer;
				*name = STRING_TO_BSTR(sourceinfo->szName.c_str());
				*nickname = STRING_TO_BSTR(sourceinfo->szNickName.c_str());
				*hasscript = sourceinfo->szScriptName.size() > 0 ? 1 : 0;
				*logicObjPtr = (LONG)sourceinfo;
				break;
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "Doodad") == 0)
	{
		for (list<DoodadInfo*>::iterator it = m_listDoodadInfo.begin(); it != m_listDoodadInfo.end(); it++)
		{
			DoodadInfo* sourceinfo = *it;
			if (sourceinfo->nIndex == index)
			{
				*representObjPtr = (LONG)sourceinfo->pPointer;
				*name = STRING_TO_BSTR(sourceinfo->szName.c_str());
				*nickname = STRING_TO_BSTR(sourceinfo->szNickName.c_str());
				*hasscript = sourceinfo->szScriptName.size() > 0 ? 1 : 0;
				*logicObjPtr = (LONG)sourceinfo;
				break;
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "AIGroup") == 0)
	{
		int i = 0;
		for (list<AIGroup*>::iterator it = m_listAIGroup.begin(); it != m_listAIGroup.end(); it++)
		{
			AIGroup* sourceinfo = *it;
			if (i++ == index)
			{
				*name = STRING_TO_BSTR(sourceinfo->szName.c_str());
				*logicObjPtr = (LONG)sourceinfo;
				break;
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "AIGroupNPC") == 0)
	{
		int i = 0;
		for (list<AIGroup*>::iterator it = m_listAIGroup.begin(); it != m_listAIGroup.end(); it++)
		{
			AIGroup* sourceinfo = *it;
			if (i++ == fatherindex)
			{	
				int j = 0;
				for (vector<NPCInfo*>::iterator itsub = sourceinfo->vecNpcInfo.begin(); itsub != sourceinfo->vecNpcInfo.end(); itsub++)
				{
					NPCInfo* p = *itsub;
					if (j++ == index)
					{
						*representObjPtr = (LONG)p->pPointer;
						*name = STRING_TO_BSTR(p->szName.c_str());
						*nickname = STRING_TO_BSTR(p->szNickName.c_str());
						*hasscript = p->szScriptName.size() > 0 ? 1 : 0;
						*logicObjPtr = (LONG)p;
						break;
					}
				}
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "WayPointSet") == 0)
	{
		int i = 0;
		for (list<WayPointSet*>::iterator it = m_listWayPointSet.begin(); it != m_listWayPointSet.end(); it++)
		{
			WayPointSet* sourceinfo = *it;
			if (i++ == index)
			{
				*name = STRING_TO_BSTR(sourceinfo->szName.c_str());
				*logicObjPtr = (LONG)sourceinfo;
				break;
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "WayPoint") == 0)
	{
		TCHAR szBuf[32];
		sprintf(szBuf, "WayPoint%d", index);
		*name = STRING_TO_BSTR(szBuf);

		int i = 0;
		for (list<WayPointSet*>::iterator it = m_listWayPointSet.begin(); it != m_listWayPointSet.end(); it++)
		{
			WayPointSet* sourceinfo = *it;
			if (i++ == fatherindex)
			{
				WayPoint* pwaypoint = sourceinfo->vectorWayPoint[index];
				*representObjPtr = (LONG)pwaypoint->pPointer;
				*logicObjPtr = (LONG)pwaypoint;
				break;
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "LogicalPoly") == 0)
	{
		int i = 0;
		for (list<IEKG3DRepresentPoly*>::iterator it = m_listLogicalPoly.begin(); it != m_listLogicalPoly.end(); it++)
		{
			IEKG3DRepresentPoly* sourceinfo = *it;
			KG3DRepresentPoly* pPoly = (KG3DRepresentPoly*)sourceinfo;
			IEKG3DRepresentObject* representobj = dynamic_cast<IEKG3DRepresentObject*>(pPoly);

			if (i++ == index)
			{
				*name = STRING_TO_BSTR(sourceinfo->GetPolyName());
				string script(sourceinfo->GetScriptFileName());
				*hasscript = script.size() > 0 ? 1 : 0;
				*logicObjPtr = (LONG)sourceinfo;
				*representObjPtr = (LONG)representobj;
				break;
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "LogicalPolyVertex") == 0)
	{
		TCHAR szBuf[32];
		sprintf(szBuf, "Vertex%d", index);
		*name = STRING_TO_BSTR(szBuf);
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "TrafficLittlePointSet") == 0)
	{
		int i = 0;
		for (list<TrafficPointSet*>::iterator it = m_listTrafficPointSet.begin(); it != m_listTrafficPointSet.end(); it++)
		{
			TrafficPointSet* sourceinfo = *it;
			if (i++ == index)
			{
				*name = STRING_TO_BSTR(sourceinfo->szName.c_str());
				*logicObjPtr = (LONG)sourceinfo;
				break;
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "TrafficLittlePoint") == 0)
	{
		TCHAR szBuf[32];
		sprintf(szBuf, "TrafficLittlePoint%d", index);
		*name = STRING_TO_BSTR(szBuf);

		int i = 0;
		for (list<TrafficPointSet*>::iterator it = m_listTrafficPointSet.begin(); it != m_listTrafficPointSet.end(); it++)
		{
			TrafficPointSet* sourceinfo = *it;
			if (i++ == fatherindex)
			{
				TrafficLittlePoint* plittlepoint = sourceinfo->vecTrafficPoint[index];
				*representObjPtr = (LONG)plittlepoint->pPointer;
				*logicObjPtr = (LONG)plittlepoint;
				break;
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "TrafficPoint") == 0)
	{
		int i = 0;
		for (map<DWORD,TrafficPoint>::iterator it = m_mapTrafficPoint.begin(); it != m_mapTrafficPoint.end(); it++)
		{
			TrafficPoint& sourceinfo = it->second;
			if (i++ == index)
			{
				*representObjPtr = (LONG)sourceinfo.pPointer;
				*hasscript = sourceinfo.m_strScriptFile.size() > 0 ? 1 : 0;
				*name = STRING_TO_BSTR(sourceinfo.m_strDescription.c_str());
				break;
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "NpcReviveGroup") == 0)
	{
		int i = 0;
		for (map<DWORD,ReviveGroup>::iterator it = m_mapNpcReviveGroup.begin(); it != m_mapNpcReviveGroup.end(); it++)
		{
			ReviveGroup* soureinfo = &it->second;
			if (i++ == index)
			{
				*name = STRING_TO_BSTR(soureinfo->szName.c_str());	
				*logicObjPtr = (LONG)soureinfo;
				break;
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "NpcReviveGroupNpc") == 0)
	{
		int i = 0;
		for (map<DWORD,ReviveGroup>::iterator it = m_mapNpcReviveGroup.begin(); it != m_mapNpcReviveGroup.end(); it++)
		{
			ReviveGroup& sourceinfo = it->second;
			if (i++ == fatherindex)
			{
				NPCInfo* npcinfo = sourceinfo.vecNpcInfo[index];
				*representObjPtr = (LONG)npcinfo->pPointer;
				*name = STRING_TO_BSTR(npcinfo->szName.c_str());
				*nickname = STRING_TO_BSTR(npcinfo->szNickName.c_str());
				*hasscript = npcinfo->szScriptName.size() > 0 ? 1 : 0;
				*logicObjPtr = (LONG)npcinfo;
				break;
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "DoodadReviveGroup") == 0)
	{
		int i = 0;
		for (map<DWORD,ReviveGroup>::iterator it = m_mapDoodadReviveGroup.begin(); it != m_mapDoodadReviveGroup.end(); it++)
		{
			ReviveGroup* soureinfo = &it->second;
			if (i++ == index)
			{
				*name = STRING_TO_BSTR(soureinfo->szName.c_str());
				*logicObjPtr = (LONG)soureinfo;
				break;
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "DoodadReviveGroupDoodad") == 0)
	{
		int i = 0;
		for (map<DWORD,ReviveGroup>::iterator it = m_mapDoodadReviveGroup.begin(); it != m_mapDoodadReviveGroup.end(); it++)
		{
			ReviveGroup& sourceinfo = it->second;
			if (i++ == fatherindex)
			{
				DoodadInfo* Doodadinfo = sourceinfo.vecDoodadInfo[index];
				*representObjPtr = (LONG)Doodadinfo->pPointer;
				*name = STRING_TO_BSTR(Doodadinfo->szName.c_str());
				*nickname = STRING_TO_BSTR(Doodadinfo->szNickName.c_str());
				*hasscript = Doodadinfo->szScriptName.size() > 0 ? 1 : 0;
				*logicObjPtr = (LONG)Doodadinfo;
				break;
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "NpcRandomGroup") == 0)
	{
		int i = 0; 
		for (map<DWORD,RandomGroup>::iterator it = m_mapRandomGroup.begin(); it != m_mapRandomGroup.end(); it++)
		{
			RandomGroup& sourceinfo = it->second;
			if (i++ == index)
			{
				*name = STRING_TO_BSTR(sourceinfo.szName.c_str());
				break;
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "AISet") == 0)
	{
		int i = 0;
		for (map<DWORD, AIParameterSet>::iterator it = m_mapNpcAISceneParameterSet.begin(); it != m_mapNpcAISceneParameterSet.end(); it++)
		{
			AIParameterSet& sourceinfo = it->second;
			if (i++ == index)
			{
				*name = STRING_TO_BSTR(sourceinfo.szName.c_str());
				break;
			}
		}
	}
	else if (_strcmpi(BSTR_TO_STRING(ObjType), "AISetParam") == 0)
	{
		int i = 0; 
		for (map<DWORD, AIParameterSet>::iterator it = m_mapNpcAISceneParameterSet.begin(); it != m_mapNpcAISceneParameterSet.end(); it++)
		{
			AIParameterSet& sourceinfo = it->second;
			if (i++ == fatherindex)
			{
				AIParameter& aiparam = sourceinfo.vecParameters[index];
				*name = STRING_TO_BSTR(aiparam.strName.c_str());
				break;
			}
		}
	}


	return S_OK;
}




#ifdef _DESIGN
STDMETHODIMP_(ULONG) KSceneEditorDocLogical::AddRef()
{
	return 1;
}
STDMETHODIMP_(ULONG) KSceneEditorDocLogical::Release()
{
	return 1;
}
STDMETHODIMP KSceneEditorDocLogical::QueryInterface(REFIID iid, void ** ppvObject)
{
	if (iid == IID_IAtlIEKSceneEditorDocLogical || iid == IID_IUnknown)
	{
		//AddRef();
		*ppvObject = dynamic_cast<IEKSceneEditorDocLogical*>(this);
		return S_OK;
	}
	else
	{
		return E_NOINTERFACE;
	}
}
#endif

//just a hack, give me a break
INT GetSceneBlockIndex( FLOAT In, FLOAT BlockWidth )
{
	if(In<0)
	{
		FLOAT InRevert = -In;
		InRevert /= BlockWidth;
		FLOAT fFrac = 0;
		FLOAT fIn;
		fFrac = modf(InRevert, &fIn);
		INT nRet = MT_FLOAT_TRIM_TAIL(fIn);
		if (fFrac > 0)
		{
			return -nRet - 1;
		}
		return -nRet;
	}
	FLOAT fRet = In/BlockWidth;
	return MT_FLOAT_TRIM_TAIL(fRet);
}

