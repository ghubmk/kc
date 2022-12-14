/*
 * SPDX-License-Identifier: AGPL-3.0-only
 * Copyright 2005 - 2016 Zarafa and its licensors
 */
#include <string>
#include <kopano/platform.h>
#include "kcore.hpp"
#include "ECMAPITable.h"
#include <kopano/ECDefs.h>
#include <kopano/ECGuid.h>
#include <kopano/CommonUtil.h>
#include <kopano/Util.h>
#include "ics.h"
#include <kopano/mapiext.h>
#include <kopano/memory.hpp>
#include <kopano/stringutil.h>
#include "ECABContainer.h"
#include <edkguid.h>
#include <edkmdb.h>
#include <mapi.h>
#include <mapispi.h>
#include <mapiutil.h>
#include <kopano/charset/convert.h>
#include <kopano/ECGetText.h>
#include "ClientUtil.h"
#include "ECABContainer.h"
#include "ECMailUser.h"
#include "EntryPoint.h"
#include "ProviderUtil.h"
#include "WSTransport.h"
#include "pcutil.hpp"

using namespace KC;

static const ABEID_FIXED eidRoot(MAPI_ABCONT, MUIDECSAB, 0);

ECABContainer::ECABContainer(ECABLogon *prov, unsigned int objtype, BOOL modify) :
	ECABProp(prov, objtype, modify)
{
	HrAddPropHandlers(PR_AB_PROVIDER_ID, DefaultABContainerGetProp, DefaultSetPropComputed, this);
	HrAddPropHandlers(PR_CONTAINER_FLAGS, DefaultABContainerGetProp, DefaultSetPropComputed, this);
	HrAddPropHandlers(PR_DISPLAY_TYPE, DefaultABContainerGetProp, DefaultSetPropComputed, this);
	HrAddPropHandlers(PR_EMSMDB_SECTION_UID, DefaultABContainerGetProp, DefaultSetPropComputed, this);
	HrAddPropHandlers(PR_ACCOUNT, DefaultABContainerGetProp, DefaultSetPropIgnore, this);
	HrAddPropHandlers(PR_NORMALIZED_SUBJECT, DefaultABContainerGetProp, DefaultSetPropIgnore, this);
	HrAddPropHandlers(PR_DISPLAY_NAME, DefaultABContainerGetProp, DefaultSetPropIgnore, this);
	HrAddPropHandlers(PR_TRANSMITABLE_DISPLAY_NAME, DefaultABContainerGetProp, DefaultSetPropIgnore, this);
}

HRESULT	ECABContainer::QueryInterface(REFIID refiid, void **lppInterface)
{
	REGISTER_INTERFACE2(ECABContainer, this);
	REGISTER_INTERFACE2(ECABProp, this);
	REGISTER_INTERFACE2(ECUnknown, this);
	REGISTER_INTERFACE2(IABContainer, this);
	REGISTER_INTERFACE2(IMAPIContainer, this);
	REGISTER_INTERFACE2(IMAPIProp, this);
	REGISTER_INTERFACE2(IUnknown, this);
	return MAPI_E_INTERFACE_NOT_SUPPORTED;
}

HRESULT ECABContainer::Create(ECABLogon *lpProvider, ULONG ulObjType,
    BOOL fModify, ECABContainer **lppABContainer)
{
	return alloc_wrap<ECABContainer>(lpProvider, ulObjType, fModify)
	       .put(lppABContainer);
}

HRESULT	ECABContainer::OpenProperty(ULONG ulPropTag, LPCIID lpiid, ULONG ulInterfaceOptions, ULONG ulFlags, LPUNKNOWN *lppUnk)
{
	if (lpiid == NULL)
		return MAPI_E_INVALID_PARAMETER;

	switch (ulPropTag) {
	case PR_CONTAINER_CONTENTS:
		if (*lpiid != IID_IMAPITable)
			return MAPI_E_INTERFACE_NOT_SUPPORTED;
		return GetContentsTable(ulInterfaceOptions, reinterpret_cast<IMAPITable **>(lppUnk));
	case PR_CONTAINER_HIERARCHY:
		if (*lpiid != IID_IMAPITable)
			return MAPI_E_INTERFACE_NOT_SUPPORTED;
		return GetHierarchyTable(ulInterfaceOptions, reinterpret_cast<IMAPITable **>(lppUnk));
	default:
		return ECABProp::OpenProperty(ulPropTag, lpiid, ulInterfaceOptions, ulFlags, lppUnk);
	}
}

HRESULT ECABContainer::CopyTo(ULONG ciidExclude, LPCIID rgiidExclude,
    const SPropTagArray *lpExcludeProps, ULONG ulUIParam,
    IMAPIProgress *lpProgress, const IID *lpInterface, void *lpDestObj,
    ULONG ulFlags, SPropProblemArray **lppProblems)
{
	return Util::DoCopyTo(&IID_IABContainer, static_cast<IABContainer *>(this), ciidExclude, rgiidExclude, lpExcludeProps, ulUIParam, lpProgress, lpInterface, lpDestObj, ulFlags, lppProblems);
}

HRESULT ECABContainer::CopyProps(const SPropTagArray *lpIncludeProps,
    unsigned int ulUIParam, IMAPIProgress *lpProgress, const IID *lpInterface,
    void *lpDestObj, ULONG ulFlags, SPropProblemArray **lppProblems)
{
	return Util::DoCopyProps(&IID_IABContainer, static_cast<IABContainer *>(this), lpIncludeProps, ulUIParam, lpProgress, lpInterface, lpDestObj, ulFlags, lppProblems);
}

HRESULT ECABContainer::DefaultABContainerGetProp(unsigned int ulPropTag,
    void *lpProvider, unsigned int ulFlags, SPropValue *lpsPropValue,
    ECGenericProp *lpParam, void *lpBase)
{
	auto lpProp = static_cast<ECABContainer *>(lpParam);
	memory_ptr<SPropValue> lpSectionUid;
	object_ptr<IProfSect> lpProfSect;

	switch(PROP_ID(ulPropTag)) {
	case PROP_ID(PR_EMSMDB_SECTION_UID): {
		auto lpLogon = static_cast<ECABLogon *>(lpProvider);
		if (lpLogon->m_lpMAPISup == nullptr)
			return MAPI_E_NOT_FOUND;
		auto hr = lpLogon->m_lpMAPISup->OpenProfileSection(nullptr, 0, &~lpProfSect);
		if(hr != hrSuccess)
			return hr;
		hr = HrGetOneProp(lpProfSect, PR_EMSMDB_SECTION_UID, &~lpSectionUid);
		if(hr != hrSuccess)
			return hr;
		lpsPropValue->ulPropTag = PR_EMSMDB_SECTION_UID;
		hr = KAllocCopy(lpSectionUid->Value.bin.lpb, sizeof(GUID), reinterpret_cast<void **>(&lpsPropValue->Value.bin.lpb), lpBase);
		if (hr != hrSuccess)
			return hr;
		lpsPropValue->Value.bin.cb = sizeof(GUID);
		break;
		}
	case PROP_ID(PR_AB_PROVIDER_ID): {
		lpsPropValue->ulPropTag = PR_AB_PROVIDER_ID;

		lpsPropValue->Value.bin.cb = sizeof(GUID);
		auto hr = MAPIAllocateMore(sizeof(GUID), lpBase, reinterpret_cast<void **>(&lpsPropValue->Value.bin.lpb));
		if (hr != hrSuccess)
			break;
		memcpy(lpsPropValue->Value.bin.lpb, &MUIDECSAB, sizeof(GUID));
		break;
	}
	case PROP_ID(PR_ACCOUNT):
	case PROP_ID(PR_NORMALIZED_SUBJECT):
	case PROP_ID(PR_DISPLAY_NAME):
	case PROP_ID(PR_TRANSMITABLE_DISPLAY_NAME):
		{
		LPCTSTR lpszName = NULL;
		std::wstring strValue;

		auto hr = lpProp->HrGetRealProp(ulPropTag, ulFlags, lpBase, lpsPropValue);
		if(hr != hrSuccess)
			return hr;

		if (PROP_TYPE(lpsPropValue->ulPropTag) == PT_UNICODE)
			strValue = convert_to<std::wstring>(lpsPropValue->Value.lpszW);
		else if (PROP_TYPE(lpsPropValue->ulPropTag) == PT_STRING8)
			strValue = convert_to<std::wstring>(lpsPropValue->Value.lpszA);
		else
			return hrSuccess;

		if(strValue.compare( L"Global Address Book" ) == 0)
			lpszName = KC_TX("Global Address Book");
		else if(strValue.compare( L"Global Address Lists" ) == 0)
			lpszName = KC_TX("Global Address Lists");
		else if (strValue.compare( L"All Address Lists" ) == 0)
			lpszName = KC_TX("All Address Lists");

		if (lpszName == nullptr)
			break;
		if (PROP_TYPE(ulPropTag) == PT_UNICODE) {
			const auto strTmp = convert_to<std::wstring>(lpszName);
			hr = MAPIAllocateMore((strTmp.size() + 1) * sizeof(wchar_t),
			     lpBase, reinterpret_cast<void **>(&lpsPropValue->Value.lpszW));
			if (hr != hrSuccess)
				return hr;
			wcscpy(lpsPropValue->Value.lpszW, strTmp.c_str());
		} else {
			const auto strTmp = convert_to<std::string>(lpszName);
			hr = MAPIAllocateMore(strTmp.size() + 1, lpBase,
			     reinterpret_cast<void **>(&lpsPropValue->Value.lpszA));
			if (hr != hrSuccess)
				return hr;
			strcpy(lpsPropValue->Value.lpszA, strTmp.c_str());
		}
		lpsPropValue->ulPropTag = ulPropTag;
		break;
	}
	default:
		return lpProp->HrGetRealProp(ulPropTag, ulFlags, lpBase, lpsPropValue);
	}
	return hrSuccess;
}

HRESULT ECABContainer::TableRowGetProp(void *lpProvider,
    const struct propVal *lpsPropValSrc, SPropValue *lpsPropValDst,
    void **lpBase, ULONG ulType)
{
	ULONG size = 0;

	switch(lpsPropValSrc->ulPropTag) {
	case PR_ACCOUNT_W:
	case PR_NORMALIZED_SUBJECT_W:
	case PR_DISPLAY_NAME_W:
	case PR_TRANSMITABLE_DISPLAY_NAME_W: {
		LPWSTR lpszW = NULL;
		if (strcmp(lpsPropValSrc->Value.lpszA, "Global Address Book" ) == 0)
			lpszW = KC_W("Global Address Book");
		else if (strcmp(lpsPropValSrc->Value.lpszA, "Global Address Lists" ) == 0)
			lpszW = KC_W("Global Address Lists");
		else if (strcmp(lpsPropValSrc->Value.lpszA, "All Address Lists" ) == 0)
			lpszW = KC_W("All Address Lists");
		else
			return MAPI_E_NOT_FOUND;
		size = (wcslen(lpszW) + 1) * sizeof(wchar_t);
		lpsPropValDst->ulPropTag = lpsPropValSrc->ulPropTag;
		return KAllocCopy(lpszW, size, reinterpret_cast<void **>(&lpsPropValDst->Value.lpszW), lpBase);
	}
	case PR_ACCOUNT_A:
	case PR_NORMALIZED_SUBJECT_A:
	case PR_DISPLAY_NAME_A:
	case PR_TRANSMITABLE_DISPLAY_NAME_A: {
		LPSTR lpszA = NULL;
		if (strcmp(lpsPropValSrc->Value.lpszA, "Global Address Book" ) == 0)
			lpszA = KC_A("Global Address Book");
		else if (strcmp(lpsPropValSrc->Value.lpszA, "Global Address Lists" ) == 0)
			lpszA = KC_A("Global Address Lists");
		else if (strcmp(lpsPropValSrc->Value.lpszA, "All Address Lists" ) == 0)
			lpszA = KC_A("All Address Lists");
		else
			return MAPI_E_NOT_FOUND;
		size = (strlen(lpszA) + 1) * sizeof(CHAR);
		lpsPropValDst->ulPropTag = lpsPropValSrc->ulPropTag;
		return KAllocCopy(lpszA, size, reinterpret_cast<void **>(&lpsPropValDst->Value.lpszA), lpBase);
	}
	default:
		return MAPI_E_NOT_FOUND;
	}
	return hrSuccess;
}

// IMAPIContainer
HRESULT ECABContainer::GetContentsTable(ULONG ulFlags, LPMAPITABLE *lppTable)
{
	object_ptr<ECMAPITable> lpTable;
	object_ptr<WSTableView> lpTableOps;
	static constexpr SizedSSortOrderSet(1, sSortByDisplayName) =
		{1, 0, 0, {{PR_DISPLAY_NAME, TABLE_SORT_ASCEND}}};

	auto hr = ECMAPITable::Create("AB Contents", nullptr, 0, &~lpTable);
	if(hr != hrSuccess)
		return hr;
	hr = GetABStore()->m_lpTransport->HrOpenABTableOps(MAPI_MAILUSER,
	     ulFlags, m_cbEntryId, m_lpEntryId,
	     static_cast<ECABLogon *>(lpProvider), &~lpTableOps); // also MAPI_DISTLIST
	if(hr != hrSuccess)
		return hr;
	hr = lpTable->HrSetTableOps(lpTableOps, !(ulFlags & MAPI_DEFERRED_ERRORS));
	if(hr != hrSuccess)
		return hr;
	hr = lpTableOps->HrSortTable(sSortByDisplayName);
	if(hr != hrSuccess)
		return hr;
	hr = lpTable->QueryInterface(IID_IMAPITable, reinterpret_cast<void **>(lppTable));
	AddChild(lpTable);
	return hr;
}

HRESULT ECABContainer::GetHierarchyTable(ULONG ulFlags, LPMAPITABLE *lppTable)
{
	object_ptr<ECMAPITable> lpTable;
	object_ptr<WSTableView> lpTableOps;

	auto hr = ECMAPITable::Create("AB hierarchy", GetABStore()->m_lpNotifyClient, ulFlags, &~lpTable);
	if(hr != hrSuccess)
		return hr;
	hr = GetABStore()->m_lpTransport->HrOpenABTableOps(MAPI_ABCONT, ulFlags,
	     m_cbEntryId, m_lpEntryId, static_cast<ECABLogon *>(lpProvider),
	     &~lpTableOps);
	if(hr != hrSuccess)
		return hr;
	hr = lpTable->HrSetTableOps(lpTableOps, !(ulFlags & MAPI_DEFERRED_ERRORS));
	if(hr != hrSuccess)
		return hr;
	hr = lpTable->QueryInterface(IID_IMAPITable, reinterpret_cast<void **>(lppTable));
	AddChild(lpTable);
	return hr;
}

HRESULT ECABContainer::OpenEntry(ULONG cbEntryID, const ENTRYID *lpEntryID,
    const IID *lpInterface, ULONG ulFlags, ULONG *lpulObjType,
    IUnknown **lppUnk)
{
	return GetABStore()->OpenEntry(cbEntryID, lpEntryID, lpInterface, ulFlags, lpulObjType, lppUnk);
}

HRESULT ECABContainer::ResolveNames(const SPropTagArray *lpPropTagArray,
    ULONG ulFlags, LPADRLIST lpAdrList, LPFlagList lpFlagList)
{
	static constexpr SizedSPropTagArray(11, sptaDefault) =
		{11, {PR_ADDRTYPE_A, PR_DISPLAY_NAME_A, PR_DISPLAY_TYPE,
		PR_EMAIL_ADDRESS_A, PR_SMTP_ADDRESS_A, PR_ENTRYID,
		PR_INSTANCE_KEY, PR_OBJECT_TYPE, PR_RECORD_KEY, PR_SEARCH_KEY,
		PR_EC_SENDAS_USER_ENTRYIDS}};
	static constexpr SizedSPropTagArray(11, sptaDefaultUnicode) =
		{11, {PR_ADDRTYPE_W, PR_DISPLAY_NAME_W, PR_DISPLAY_TYPE,
		PR_EMAIL_ADDRESS_W, PR_SMTP_ADDRESS_W, PR_ENTRYID,
		PR_INSTANCE_KEY, PR_OBJECT_TYPE, PR_RECORD_KEY, PR_SEARCH_KEY,
		PR_EC_SENDAS_USER_ENTRYIDS}};
	if (lpPropTagArray == NULL)
		lpPropTagArray = (ulFlags & MAPI_UNICODE) ?
		                 sptaDefaultUnicode : sptaDefault;
	return ((ECABLogon*)lpProvider)->m_lpTransport->HrResolveNames(lpPropTagArray, ulFlags, lpAdrList, lpFlagList);
}

ECABLogon::ECABLogon(IMAPISupport *lpMAPISup, WSTransport *lpTransport,
    ULONG ulProfileFlags, const GUID *lpGUID) :
	m_lpMAPISup(lpMAPISup), m_lpTransport(lpTransport),
	/* The "legacy" guid used normally (all AB entryIDs have this GUID) */
	m_guid(MUIDECSAB),
	/* The specific GUID for *this* addressbook provider, if available */
	m_ABPGuid((lpGUID != nullptr) ? *lpGUID : GUID_NULL)
{
	if (! (ulProfileFlags & EC_PROFILE_FLAGS_NO_NOTIFICATIONS))
		ECNotifyClient::Create(MAPI_ADDRBOOK, this, ulProfileFlags, lpMAPISup, &~m_lpNotifyClient);
}

ECABLogon::~ECABLogon()
{
	if(m_lpTransport)
		m_lpTransport->HrLogOff();
	// Disable all advises
	if(m_lpNotifyClient)
		m_lpNotifyClient->ReleaseAll();
}

HRESULT ECABLogon::Create(IMAPISupport *lpMAPISup, WSTransport *lpTransport,
    ULONG ulProfileFlags, const GUID *lpGuid, ECABLogon **lppECABLogon)
{
	return alloc_wrap<ECABLogon>(lpMAPISup, lpTransport, ulProfileFlags,
	       lpGuid).put(lppECABLogon);
}

HRESULT ECABLogon::QueryInterface(REFIID refiid, void **lppInterface)
{
	REGISTER_INTERFACE2(ECABLogon, this);
	REGISTER_INTERFACE2(ECUnknown, this);
	REGISTER_INTERFACE2(IABLogon, this);
	REGISTER_INTERFACE2(IUnknown, this);
	return MAPI_E_INTERFACE_NOT_SUPPORTED;
}

HRESULT ECABLogon::GetLastError(HRESULT hResult, ULONG ulFlags, LPMAPIERROR *lppMAPIError)
{
	return MAPI_E_CALL_FAILED;
}

HRESULT ECABLogon::Logoff(ULONG ulFlags)
{
	//FIXME: Release all Other open objects ?
	//Releases all open objects, such as any subobjects or the status object.
	//Releases the provider's support object.
	m_lpMAPISup.reset();
	return hrSuccess;
}

HRESULT ECABLogon::OpenEntry(ULONG cbEntryID, const ENTRYID *lpEntryID,
    const IID *lpInterface, ULONG ulFlags, ULONG *lpulObjType,
    IUnknown **lppUnk)
{
	if (lppUnk == nullptr)
		return MAPI_E_INVALID_PARAMETER;

	object_ptr<ECABContainer> lpABContainer;
	bool fModifyObject = false;
	ABEID_FIXED lpABeid;
	object_ptr<IECPropStorage> lpPropStorage;
	object_ptr<ECMailUser> lpMailUser;
	object_ptr<ECDistList> 	lpDistList;
	memory_ptr<ENTRYID> lpEntryIDServer;

	/*if(ulFlags & MAPI_MODIFY) {
		if (!fModify)
			return MAPI_E_NO_ACCESS;
		else
			fModifyObject = true;
	}
	if(ulFlags & MAPI_BEST_ACCESS)
		fModifyObject = fModify;
	*/

	if(cbEntryID == 0 && lpEntryID == NULL) {
		memcpy(&lpABeid, &eidRoot, sizeof(lpABeid));
		cbEntryID = sizeof(lpABeid);
		lpEntryID = reinterpret_cast<ENTRYID *>(&lpABeid);
	} else {
		if (cbEntryID == 0 || lpEntryID == nullptr || cbEntryID < sizeof(ABEID))
			return MAPI_E_UNKNOWN_ENTRYID;
		auto hr = KAllocCopy(lpEntryID, cbEntryID, &~lpEntryIDServer);
		if(hr != hrSuccess)
			return hr;
		lpEntryID = lpEntryIDServer;
		memcpy(&lpABeid, lpEntryID, sizeof(ABEID));

		// Check sane entryid
		if (lpABeid.ulType != MAPI_ABCONT &&
		    lpABeid.ulType != MAPI_MAILUSER &&
		    lpABeid.ulType != MAPI_DISTLIST)
			return MAPI_E_UNKNOWN_ENTRYID;

		// Check entryid GUID, must be either MUIDECSAB or m_ABPGuid
		if (lpABeid.guid != MUIDECSAB && lpABeid.guid != m_ABPGuid)
			return MAPI_E_UNKNOWN_ENTRYID;
		memcpy(&lpABeid.guid, &MUIDECSAB, sizeof(MAPIUID));
	}

	//TODO: check entryid serverside?
	switch (lpABeid.ulType) {
	case MAPI_ABCONT: {
		auto hr = ECABContainer::Create(this, MAPI_ABCONT, fModifyObject, &~lpABContainer);
		if (hr != hrSuccess)
			return hr;
		hr = lpABContainer->SetEntryId(cbEntryID, lpEntryID);
		if (hr != hrSuccess)
			return hr;
		AddChild(lpABContainer);
		hr = m_lpTransport->HrOpenABPropStorage(cbEntryID, lpEntryID, &~lpPropStorage);
		if (hr != hrSuccess)
			return hr;
		hr = lpABContainer->HrSetPropStorage(lpPropStorage, true);
		if (hr != hrSuccess)
			return hr;
		hr = lpABContainer->QueryInterface(lpInterface != nullptr ? *lpInterface : IID_IABContainer, reinterpret_cast<void **>(lppUnk));
		if (hr != hrSuccess)
			return hr;
		break;
	}
	case MAPI_MAILUSER: {
		auto hr = ECMailUser::Create(this, fModifyObject, &~lpMailUser);
		if (hr != hrSuccess)
			return hr;
		hr = lpMailUser->SetEntryId(cbEntryID, lpEntryID);
		if (hr != hrSuccess)
			return hr;
		AddChild(lpMailUser);
		hr = m_lpTransport->HrOpenABPropStorage(cbEntryID, lpEntryID, &~lpPropStorage);
		if (hr != hrSuccess)
			return hr;
		hr = lpMailUser->HrSetPropStorage(lpPropStorage, true);
		if (hr != hrSuccess)
			return hr;
		hr = lpMailUser->QueryInterface(lpInterface != nullptr ? *lpInterface : IID_IMailUser, reinterpret_cast<void **>(lppUnk));
		if (hr != hrSuccess)
			return hr;
		break;
	}
	case MAPI_DISTLIST: {
		auto hr = ECDistList::Create(this, fModifyObject, &~lpDistList);
		if (hr != hrSuccess)
			return hr;
		hr = lpDistList->SetEntryId(cbEntryID, lpEntryID);
		if (hr != hrSuccess)
			return hr;
		AddChild(lpDistList);
		hr = m_lpTransport->HrOpenABPropStorage(cbEntryID, lpEntryID, &~lpPropStorage);
		if (hr != hrSuccess)
			return hr;
		hr = lpDistList->HrSetPropStorage(lpPropStorage, true);
		if (hr != hrSuccess)
			return hr;
		hr = lpDistList->QueryInterface(lpInterface != nullptr ? *lpInterface : IID_IDistList, reinterpret_cast<void **>(lppUnk));
		if (hr != hrSuccess)
			return hr;
		break;
	}
	default:
		return MAPI_E_NOT_FOUND;
	}

	if(lpulObjType)
		*lpulObjType = lpABeid.ulType;
	return hrSuccess;
}

HRESULT ECABLogon::CompareEntryIDs(ULONG cbEntryID1, const ENTRYID *lpEntryID1,
    ULONG cbEntryID2, const ENTRYID *lpEntryID2, ULONG ulFlags,
    ULONG *lpulResult)
{
	if(lpulResult)
		*lpulResult = CompareABEID(cbEntryID1, lpEntryID1, cbEntryID2, lpEntryID2);
	return hrSuccess;
}

HRESULT ECABLogon::Advise(ULONG cbEntryID, const ENTRYID *lpEntryID,
    ULONG ulEventMask, IMAPIAdviseSink *lpAdviseSink, ULONG *lpulConnection)
{
	if (lpAdviseSink == NULL || lpulConnection == NULL)
		return MAPI_E_INVALID_PARAMETER;
	if (lpEntryID == NULL)
		//NOTE: Normal you must give the entryid of the addressbook toplevel
		return MAPI_E_INVALID_PARAMETER;
	assert(m_lpNotifyClient != NULL);
	if(m_lpNotifyClient->Advise(cbEntryID, (LPBYTE)lpEntryID, ulEventMask, lpAdviseSink, lpulConnection) != S_OK)
		return MAPI_E_NO_SUPPORT;
	return hrSuccess;
}

HRESULT ECABLogon::Unadvise(ULONG ulConnection)
{
	assert(m_lpNotifyClient != NULL);
	m_lpNotifyClient->Unadvise(ulConnection);
	return hrSuccess;
}

HRESULT ECABLogon::PrepareRecips(ULONG ulFlags,
    const SPropTagArray *lpPropTagArray, LPADRLIST lpRecipList)
{
	if (lpPropTagArray == nullptr || lpPropTagArray->cValues == 0)
		return hrSuccess;

	ULONG cValues, ulObjType;
	for (unsigned int i = 0; i < lpRecipList->cEntries; ++i) {
		auto rgpropvalsRecip = lpRecipList->aEntries[i].rgPropVals;
		unsigned int cPropsRecip = lpRecipList->aEntries[i].cValues;

		// For each recipient, find its entryid
		auto lpPropVal = PCpropFindProp(rgpropvalsRecip, cPropsRecip, PR_ENTRYID);
		if(!lpPropVal)
			continue; // no

		auto lpABeid = reinterpret_cast<ABEID *>(lpPropVal->Value.bin.lpb);
		auto cbABeid = lpPropVal->Value.bin.cb;
		/* Is it one of ours? */
		if ( cbABeid  < CbNewABEID("") || lpABeid == NULL)
			continue;	// no
		if (lpABeid->guid != m_guid)
			continue;	// no

		object_ptr<IMailUser> lpIMailUser;
		auto hr = OpenEntry(cbABeid, reinterpret_cast<ENTRYID *>(lpABeid), nullptr, 0, &ulObjType, &~lpIMailUser);
		if(hr != hrSuccess)
			continue;	// no
		memory_ptr<SPropValue> lpPropArray, lpNewPropArray;
		hr = lpIMailUser->GetProps(lpPropTagArray, 0, &cValues, &~lpPropArray);
		if(FAILED(hr) != hrSuccess)
			continue;	// no
		// merge the properties
		hr = MAPIAllocateBuffer((cValues + cPropsRecip) * sizeof(SPropValue), &~lpNewPropArray);
		if (hr != hrSuccess)
			return hr;

		for (unsigned int j = 0; j < cValues; ++j) {
			lpPropVal = NULL;

			if(PROP_TYPE(lpPropArray[j].ulPropTag) == PT_ERROR)
				lpPropVal = PCpropFindProp(rgpropvalsRecip, cPropsRecip, lpPropTagArray->aulPropTag[j]);
			if(lpPropVal == NULL)
				lpPropVal = &lpPropArray[j];
			hr = Util::HrCopyProperty(lpNewPropArray + j, lpPropVal, lpNewPropArray);
			if(hr != hrSuccess)
				return hr;
		}

		for (unsigned int j = 0; j < cPropsRecip; ++j) {
			if (PCpropFindProp(lpNewPropArray, cValues, rgpropvalsRecip[j].ulPropTag) ||
				PROP_TYPE( rgpropvalsRecip[j].ulPropTag ) == PT_ERROR )
				continue;
			hr = Util::HrCopyProperty(lpNewPropArray + cValues, &rgpropvalsRecip[j], lpNewPropArray);
			if(hr != hrSuccess)
				return hr;
			++cValues;
		}

		lpRecipList->aEntries[i].rgPropVals	= lpNewPropArray.release();
		lpRecipList->aEntries[i].cValues	= cValues;
		if(rgpropvalsRecip) {
			MAPIFreeBuffer(rgpropvalsRecip);
			rgpropvalsRecip = NULL;
		}
	}

	// Always succeeded on this point
	return hrSuccess;
}

ECABProp::ECABProp(ECABLogon *prov, unsigned int objtype, BOOL modify) :
	ECGenericProp(prov, objtype, modify)
{
	HrAddPropHandlers(PR_RECORD_KEY, DefaultABGetProp, DefaultSetPropComputed, this);
	HrAddPropHandlers(PR_STORE_SUPPORT_MASK, DefaultABGetProp, DefaultSetPropComputed, this);
}

HRESULT ECABProp::QueryInterface(REFIID refiid, void **lppInterface)
{
	REGISTER_INTERFACE2(ECABProp, this);
	return ECGenericProp::QueryInterface(refiid, lppInterface);
}

HRESULT ECABProp::DefaultABGetProp(unsigned int ulPropTag, void *lpProvider,
    unsigned int ulFlags, SPropValue *lpsPropValue, ECGenericProp *lpParam,
    void *lpBase)
{
	auto lpProp = static_cast<ECABProp *>(lpParam);

	switch(PROP_ID(ulPropTag)) {
	case PROP_ID(PR_RECORD_KEY):
		lpsPropValue->ulPropTag = PR_RECORD_KEY;

		if(lpProp->m_lpEntryId && lpProp->m_cbEntryId > 0) {
			lpsPropValue->Value.bin.cb = lpProp->m_cbEntryId;
			auto hr = MAPIAllocateMore(lpsPropValue->Value.bin.cb, lpBase, reinterpret_cast<void **>(&lpsPropValue->Value.bin.lpb));
			if (hr != hrSuccess)
				return hr;
			memcpy(lpsPropValue->Value.bin.lpb, lpProp->m_lpEntryId, lpsPropValue->Value.bin.cb);
		} else {
			return MAPI_E_NOT_FOUND;
		}
		break;
	case PROP_ID(PR_STORE_SUPPORT_MASK): {
		unsigned int ulClientVersion = -1;
		GetClientVersion(&ulClientVersion);

		// No real unicode support in outlook 2000 and xp
		if (ulClientVersion > CLIENT_VERSION_OLK2002) {
			lpsPropValue->Value.l = STORE_UNICODE_OK;
			lpsPropValue->ulPropTag = PR_STORE_SUPPORT_MASK;
		} else {
			return MAPI_E_NOT_FOUND;
		}
		break;
	}
	default:
		return lpProp->HrGetRealProp(ulPropTag, ulFlags, lpBase, lpsPropValue);
	}

	return hrSuccess;
}

HRESULT ECABProp::TableRowGetProp(void *lpProvider,
    const struct propVal *lpsPropValSrc, SPropValue *lpsPropValDst,
    void **lpBase, ULONG ulType)
{
	if (lpsPropValSrc->ulPropTag != CHANGE_PROP_TYPE(PR_AB_PROVIDER_ID, PT_ERROR))
		return MAPI_E_NOT_FOUND;
	lpsPropValDst->ulPropTag = PR_AB_PROVIDER_ID;
	lpsPropValDst->Value.bin.cb = sizeof(GUID);
	auto hr = MAPIAllocateMore(sizeof(GUID), lpBase,
	          reinterpret_cast<void **>(&lpsPropValDst->Value.bin.lpb));
	if (hr != hrSuccess)
		return hr;
	memcpy(lpsPropValDst->Value.bin.lpb, &MUIDECSAB, sizeof(GUID));
	return hrSuccess;
}

HRESULT ECABProvider::Create(ECABProvider **lppECABProvider)
{
	return alloc_wrap<ECABProvider>().put(lppECABProvider);
}

HRESULT ECABProvider::QueryInterface(REFIID refiid, void **lppInterface)
{
	REGISTER_INTERFACE2(ECUnknown, this);
	REGISTER_INTERFACE2(IABProvider, this);
	REGISTER_INTERFACE2(IUnknown, this);
	return MAPI_E_INTERFACE_NOT_SUPPORTED;
}

HRESULT ECABProvider::Shutdown(ULONG * lpulFlags)
{
	return hrSuccess;
}

HRESULT ECABProvider::Logon(IMAPISupport *lpMAPISup, ULONG_PTR ulUIParam,
    const TCHAR *lpszProfileName, ULONG ulFlags, ULONG *lpulcbSecurity,
    BYTE **lppbSecurity, MAPIERROR **lppMAPIError, IABLogon **lppABLogon)
{
	if (lpMAPISup == nullptr || lppABLogon == nullptr)
		return MAPI_E_INVALID_PARAMETER;

	object_ptr<ECABLogon> lpABLogon;
	sGlobalProfileProps	sProfileProps;
	object_ptr<WSTransport> lpTransport;

	// Get the username and password from the profile settings
	auto hr = ClientUtil::GetGlobalProfileProperties(lpMAPISup, &sProfileProps);
	if(hr != hrSuccess)
		return hr;
	// Create a transport for this provider
	hr = WSTransport::Create(&~lpTransport);
	if(hr != hrSuccess)
		return hr;
	// Log on the transport to the server
	hr = lpTransport->HrLogon(sProfileProps);
	if(hr != hrSuccess)
		return hr;
	hr = ECABLogon::Create(lpMAPISup, lpTransport, sProfileProps.ulProfileFlags, nullptr, &~lpABLogon);
	if(hr != hrSuccess)
		return hr;
	AddChild(lpABLogon);
	hr = lpABLogon->QueryInterface(IID_IABLogon, reinterpret_cast<void **>(lppABLogon));
	if(hr != hrSuccess)
		return hr;
	if (lpulcbSecurity)
		*lpulcbSecurity = 0;
	if (lppbSecurity)
		*lppbSecurity = NULL;
	if (lppMAPIError)
		*lppMAPIError = NULL;
	return hrSuccess;
}

HRESULT ECABProviderSwitch::Create(ECABProviderSwitch **lppECABProvider)
{
	return alloc_wrap<ECABProviderSwitch>().put(lppECABProvider);
}

HRESULT ECABProviderSwitch::QueryInterface(REFIID refiid, void **lppInterface)
{
	REGISTER_INTERFACE2(ECUnknown, this);
	REGISTER_INTERFACE2(IABProvider, this);
	REGISTER_INTERFACE2(IUnknown, this);
	return MAPI_E_INTERFACE_NOT_SUPPORTED;
}

HRESULT ECABProviderSwitch::Shutdown(ULONG * lpulFlags)
{
	return hrSuccess;
}

HRESULT ECABProviderSwitch::Logon(IMAPISupport *lpMAPISup, ULONG_PTR ulUIParam,
    const TCHAR *lpszProfileName, ULONG ulFlags, ULONG *lpulcbSecurity,
    BYTE **lppbSecurity, MAPIERROR **lppMAPIError, IABLogon **lppABLogon)
{
	PROVIDER_INFO sProviderInfo;
	object_ptr<IABLogon> lpABLogon;
	object_ptr<IABProvider> lpOnline;

	auto hr = GetProviders(&g_mapProviders, lpMAPISup,
	          lpszProfileName == nullptr ? nullptr : tfstring_to_lcl(lpszProfileName, ulFlags).c_str(),
	          &sProviderInfo);
	if (hr != hrSuccess)
		return hr;
	hr = sProviderInfo.lpABProviderOnline->QueryInterface(IID_IABProvider, &~lpOnline);
	if (hr != hrSuccess)
		return hr;

	// Online
	hr = lpOnline->Logon(lpMAPISup, ulUIParam, lpszProfileName, ulFlags, nullptr, nullptr, nullptr, &~lpABLogon);
	if(hr != hrSuccess) {
		if (hr == MAPI_E_NETWORK_ERROR)
			/* for disable public folders, so you can work offline */
			return MAPI_E_FAILONEPROVIDER;
		else if (hr == MAPI_E_LOGON_FAILED)
			return MAPI_E_UNCONFIGURED; /* Linux error ?? */
			//hr = MAPI_E_LOGON_FAILED;
		else
			return MAPI_E_LOGON_FAILED;
	}

	hr = lpMAPISup->SetProviderUID((LPMAPIUID)&MUIDECSAB, 0);
	if(hr != hrSuccess)
		return hr;
	hr = lpABLogon->QueryInterface(IID_IABLogon, reinterpret_cast<void **>(lppABLogon));
	if(hr != hrSuccess)
		return hr;
	if(lpulcbSecurity)
		*lpulcbSecurity = 0;
	if(lppbSecurity)
		*lppbSecurity = NULL;
	if (lppMAPIError)
		*lppMAPIError = NULL;
	return hrSuccess;
}
