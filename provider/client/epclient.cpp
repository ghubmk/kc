/*
 * Copyright 2005 - 2016 Zarafa and its licensors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <kopano/platform.h>

#include <cstdint>
#include <mapi.h>
#include <mapispi.h>
#include <mapiutil.h>

#include <kopano/ECGetText.h>

#include <memory>
#include <string>
#include <cassert>

#include "kcore.hpp"
#include "Mem.h"

#include "DLLGlobal.h"
#include "ECMSProviderSwitch.h"
#include "ECXPProvider.h"
#include "ECABProviderSwitch.h"
#ifdef LINUX
#include <iostream>
#endif

#include <kopano/ecversion.h>

#include <kopano/ECDebug.h>
#include <kopano/stringutil.h>

#include <kopano/ECLogger.h>

#include <kopano/ECGuid.h>
#include <edkmdb.h>
#include <edkguid.h>

#include <kopano/mapi_ptr.h>

#include "SSLUtil.h"
#include "ClientUtil.h"
#include "SymmetricCrypt.h"

#include "EntryPoint.h"

#include <kopano/charset/convstring.h>

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW

#define DEBUG_WITH_MEMORY_DUMP 0 // Sure to dump memleaks before the dll is exit
#endif

class EPCDeleter {
	public:
	void operator()(ABEID *p) { MAPIFreeBuffer(p); }
};

static const uint32_t MAPI_S_SPECIAL_OK = MAKE_MAPI_S(0x900);

// Client wide variable
tstring		g_strCommonFilesKopano;
tstring		g_strUserLocalAppDataKopano;
tstring		g_strKopanoDirectory;

tstring		g_strManufacturer;
tstring		g_strProductName;
tstring		g_strProductNameShort;
bool		g_isOEM;
ULONG		g_ulLoadsim;

// Map of msprovider with Profilename as key
ECMapProvider	g_mapProviders;

class CKopanoApp {
public:
    CKopanoApp() {
        ssl_threading_setup();

		g_strManufacturer = _T("Kopano");
		g_strProductName = _T("Kopano Core");
		g_isOEM = false;
		g_ulLoadsim = FALSE;

		// FIXME for offline
		// - g_strUserLocalAppDataKopano = ~/kopano ?
		// - g_strKopanoDirectory = /usr/bin/ ?
    }
    ~CKopanoApp() {
        ssl_threading_cleanup();

		RemoveAllProviders(&g_mapProviders);
    }
};

CKopanoApp theApp;

///////////////////////////////////////////////////////////////////
// entrypoints
//

// Called by MAPI to return a MSProvider object when a user opens a store based on our service
extern "C" HRESULT __cdecl MSProviderInit(HINSTANCE hInstance, LPMALLOC pmalloc, LPALLOCATEBUFFER pfnAllocBuf, LPALLOCATEMORE pfnAllocMore, LPFREEBUFFER pfnFreeBuf, ULONG ulFlags, ULONG ulMAPIver, ULONG * lpulProviderVer, LPMSPROVIDER * ppmsp)
{
	TRACE_MAPI(TRACE_ENTRY, "MSProviderInit", "flags=%08X", ulFlags);

	HRESULT hr = hrSuccess;
	ECMSProviderSwitch *lpMSProvider = NULL;

	// Check the interface version is ok
	if(ulMAPIver != CURRENT_SPI_VERSION) {
		hr = MAPI_E_VERSION;
		goto exit;
	}

	*lpulProviderVer = CURRENT_SPI_VERSION;
	
	// Save the pointers for later use
	_pmalloc = pmalloc;
	_pfnAllocBuf = pfnAllocBuf;
	_pfnAllocMore = pfnAllocMore;
	_pfnFreeBuf = pfnFreeBuf;
	_hInstance = hInstance;

	// This object is created for the lifetime of the DLL and destroyed when the
	// DLL is closed (same on linux, but then for the shared library);
	hr = ECMSProviderSwitch::Create(ulFlags, &lpMSProvider);

	if(hr != hrSuccess)
		goto exit;

	hr = lpMSProvider->QueryInterface(IID_IMSProvider, (void **)ppmsp); 

exit:
	if (lpMSProvider)
		lpMSProvider->Release();

	TRACE_MAPI(TRACE_RETURN, "MSProviderInit", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

/**
 * Get the service name from the provider admin
 *
 * The service name is the string normally passed to CreateMsgService, like "ZARAFA6" or "MSEMS".
 *
 * @param lpProviderAdmin[in] The ProviderAdmin object passed to MSGServiceEntry
 * @param lpServiceName[out] The name of the message service
 * @return HRESULT Result status
 */
static HRESULT GetServiceName(IProviderAdmin *lpProviderAdmin,
    std::string *lpServiceName)
{
	lpServiceName->assign("ZARAFA6");
	return hrSuccess;
}

static HRESULT initprov_storepub(WSTransport *tp, IProviderAdmin *provadm,
    SPropValuePtr &provuid, const sGlobalProfileProps &profprop,
    unsigned int *eid_size, EntryIdPtr &eid)
{
	/* Get the public store */
	std::string redir_srv;
	HRESULT ret;

	if (profprop.ulProfileFlags & EC_PROFILE_FLAGS_NO_PUBLIC_STORE)
		/* skip over to the DeleteProvider part */
		ret = MAPI_E_INVALID_PARAMETER;
	else
		ret = tp->HrGetPublicStore(0, eid_size, &eid, &redir_srv);

	if (ret == MAPI_E_UNABLE_TO_COMPLETE) {
		tp->HrLogOff();
		auto new_props = profprop;
		new_props.strServerPath = redir_srv;
		ret = tp->HrLogon(new_props);
		if (ret == hrSuccess)
			ret = tp->HrGetPublicStore(0, eid_size, &eid);
	}
	if (ret == hrSuccess)
		return hrSuccess;
	if (provadm != NULL && provuid.get() != NULL)
		provadm->DeleteProvider(reinterpret_cast<MAPIUID *>(provuid->Value.bin.lpb));
	/* Profile without public store */
	return MAPI_S_SPECIAL_OK;
}

static HRESULT initprov_service(WSTransport *transport,
    IProviderAdmin *provadm, const sGlobalProfileProps &profprop,
    unsigned int *eid_size, EntryIdPtr &eid)
{
	/* Get the default store for this user */
	std::string redir_srv;
	HRESULT ret = transport->HrGetStore(0, NULL, eid_size, &eid,
	              0, NULL, &redir_srv);
	if (ret == MAPI_E_NOT_FOUND) {
		ec_log_err("HrGetStore failed: No store present.");
		return ret;
	} else if (ret != MAPI_E_UNABLE_TO_COMPLETE) {
		return ret;
	}

	/* MAPI_E_UNABLE_TO_COMPLETE */
	transport->HrLogOff();
	auto new_props = profprop;
	new_props.strServerPath = redir_srv;
	ret = transport->HrLogon(new_props);
	if (ret != hrSuccess)
		return ret;
	ret = transport->HrGetStore(0, NULL, eid_size, &eid, 0, NULL);
	if (ret != hrSuccess)
		return ret;

	/* This should be a real URL */
	assert(redir_srv.compare(0, 9, "pseudo://") != 0);

	if (provadm == NULL || redir_srv.empty())
		return hrSuccess;

	/* Set/update the default store home server. */
	auto guid = reinterpret_cast<MAPIUID *>(const_cast<char *>(pbGlobalProfileSectionGuid));
	ProfSectPtr globprofsect;
	ret = provadm->OpenProfileSection(guid, NULL, MAPI_MODIFY, &globprofsect);
	if (ret != hrSuccess)
		return ret;

	SPropValue spv;
	spv.ulPropTag = PR_EC_PATH;
	spv.Value.lpszA = const_cast<char *>(redir_srv.c_str());
	return HrSetOneProp(globprofsect, &spv);
}

static HRESULT initprov_storedl(WSTransport *transport,
    IProviderAdmin *provadm, SPropValuePtr &provuid,
    const sGlobalProfileProps &profprop, IProfSect *profsect,
    unsigned int *eid_size, EntryIdPtr &eid)
{
	/* PR_EC_USERNAME is the user we want to add ... */
	SPropValuePtr name;
	HRESULT ret = HrGetOneProp(profsect, PR_EC_USERNAME_W, &name);
	if (ret != hrSuccess)
		ret = HrGetOneProp(profsect, PR_EC_USERNAME_A, &name);
	if (ret != hrSuccess) {
		/*
		 * This should probably be done in UpdateProviders. But
		 * UpdateProviders does not know the type of the provider and it
		 * should not just delete the provider for all types of
		 * providers.
		 */
		if (provadm != NULL && provuid.get() != NULL)
			provadm->DeleteProvider(reinterpret_cast<MAPIUID *>(provuid->Value.bin.lpb));
		/* Invalid or empty delegate store */
		return MAPI_S_SPECIAL_OK;
	}

	std::string redir_srv;
	ret = transport->HrResolveUserStore(convstring::from_SPropValue(name),
	      0, NULL, eid_size, &eid, &redir_srv);
	if (ret != MAPI_E_UNABLE_TO_COMPLETE)
		return ret;

	transport->HrLogOff();
	auto new_props = profprop;
	new_props.strServerPath = redir_srv;
	ret = transport->HrLogon(new_props);
	if (ret != hrSuccess)
		return ret;
	return transport->HrResolveUserStore(convstring::from_SPropValue(name),
	       0, NULL, eid_size, &eid);
}

static HRESULT initprov_addrbook(std::unique_ptr<ABEID, EPCDeleter> &eid,
    unsigned int &count, SPropValue *prop)
{
	ABEID *eidptr;
	size_t abe_size = CbNewABEID("");
	HRESULT ret = MAPIAllocateBuffer(abe_size, reinterpret_cast<void **>(&eidptr));
	if (ret != hrSuccess)
		return ret;

	eid.reset(eidptr);
	memset(eidptr, 0, abe_size);
	memcpy(&eid->guid, &MUIDECSAB, sizeof(GUID));
	eid->ulType = MAPI_ABCONT;

	prop[count].ulPropTag = PR_ENTRYID;
	prop[count].Value.bin.cb = abe_size;
	prop[count++].Value.bin.lpb = reinterpret_cast<BYTE *>(eidptr);
	prop[count].ulPropTag = PR_RECORD_KEY;
	prop[count].Value.bin.cb = sizeof(MAPIUID);
	prop[count++].Value.bin.lpb = reinterpret_cast<BYTE *>(const_cast<GUID *>(&MUIDECSAB));
	prop[count].ulPropTag = PR_DISPLAY_NAME_A;
	prop[count++].Value.lpszA = const_cast<char *>("Kopano Addressbook");
	prop[count].ulPropTag = PR_EC_PATH;
	prop[count++].Value.lpszA = const_cast<char *>("Kopano Addressbook");
	prop[count].ulPropTag = PR_PROVIDER_DLL_NAME_A;
	prop[count++].Value.lpszA = const_cast<char *>(WCLIENT_DLL_NAME);
	return hrSuccess;
}

/**
 * Initialize one service provider
 *
 * @param lpAdminProvider[in]	Pointer to provider admin.
 * @param lpProfSect[in]		Pointer to provider profile section.
 * @param sProfileProps[in]		Global profile properties
 * @param lpcStoreID[out]		Size of lppStoreID
 * @param lppStoreID[out]		Entryid of the store of the provider
 *
 * @return MAPI error codes
 */
HRESULT InitializeProvider(LPPROVIDERADMIN lpAdminProvider,
    IProfSect *lpProfSect, const sGlobalProfileProps &sProfileProps,
    ULONG *lpcStoreID, LPENTRYID *lppStoreID, WSTransport *transport)
{
	HRESULT hr = hrSuccess;

	WSTransport		*lpTransport = NULL;
	WSTransport		*lpAltTransport = NULL;
	std::unique_ptr<ABEID, EPCDeleter> abe_id;
	ULONG			cbEntryId = 0;
	ULONG			cbWrappedEntryId = 0;
	EntryIdPtr		ptrEntryId;
	EntryIdPtr		ptrWrappedEntryId;
	ProfSectPtr		ptrGlobalProfSect;
	SPropValuePtr	ptrPropValueName;
	SPropValuePtr	ptrPropValueMDB;
	SPropValuePtr	ptrPropValueResourceType;
	SPropValuePtr	ptrPropValueServiceName;
	SPropValuePtr	ptrPropValueProviderUid;
	SPropValuePtr	ptrPropValueServerName;
	WStringPtr		ptrStoreName;
	std::string		strDefStoreServer;
	std::string		strServiceName;
	SPropValue		sPropVals[6]; 
	ULONG			cPropValue = 0;
	ULONG			ulResourceType=0;

	if (lpAdminProvider) {
		hr = GetServiceName(lpAdminProvider, &strServiceName);
		if (hr != hrSuccess)
			goto exit;
	} else {

		hr = HrGetOneProp(lpProfSect, PR_SERVICE_NAME_A, &ptrPropValueServiceName);
		if(hr == hrSuccess)
			strServiceName = ptrPropValueServiceName->Value.lpszA;
		
		hr = hrSuccess;
	}
	
	hr = HrGetOneProp(lpProfSect, PR_RESOURCE_TYPE, &ptrPropValueResourceType);
	if(hr != hrSuccess) {
		// Ignore this provider; apparently it has no resource type, so just skip it
		hr = hrSuccess;
		goto exit;
	}
	
	HrGetOneProp(lpProfSect, PR_PROVIDER_UID, &ptrPropValueProviderUid);

	ulResourceType = ptrPropValueResourceType->Value.l;

	TRACE_MAPI(TRACE_INFO, "InitializeProvider", "Resource type=%s", ResourceTypeToString(ulResourceType) );

	if (transport != NULL) {
		lpTransport = transport;
	} else {
		hr = WSTransport::Create(0, &lpTransport);
		if (hr != hrSuccess)
			goto exit;
	}

	hr = lpTransport->HrLogon(sProfileProps);
	if(hr != hrSuccess)
		goto exit;

	if(ulResourceType == MAPI_STORE_PROVIDER)
	{
		hr = HrGetOneProp(lpProfSect, PR_MDB_PROVIDER, &ptrPropValueMDB);
		if(hr != hrSuccess)
			goto exit;

		if(CompareMDBProvider(ptrPropValueMDB->Value.bin.lpb, &KOPANO_STORE_PUBLIC_GUID)) {
			hr = initprov_storepub(lpTransport, lpAdminProvider, ptrPropValueProviderUid, sProfileProps, &cbEntryId, ptrEntryId);
			if (hr != hrSuccess)
				goto exit;
		} else if(CompareMDBProvider(ptrPropValueMDB->Value.bin.lpb, &KOPANO_SERVICE_GUID)) {
			hr = initprov_service(lpTransport, lpAdminProvider, sProfileProps, &cbEntryId, ptrEntryId);
			if (hr != hrSuccess) 
				goto exit;
		} else if(CompareMDBProvider(ptrPropValueMDB->Value.bin.lpb, &KOPANO_STORE_DELEGATE_GUID)) {
			hr = initprov_storedl(lpTransport, lpAdminProvider, ptrPropValueProviderUid, sProfileProps, lpProfSect, &cbEntryId, ptrEntryId);
			if (hr != hrSuccess)
				goto exit;
		} else if(CompareMDBProvider(ptrPropValueMDB->Value.bin.lpb, &KOPANO_STORE_ARCHIVE_GUID)) {
			// We need to get the username and the server name or url from the profsect.
			// That's enough information to get the entryid from the correct server. There's no redirect
			// available when resolving archive stores.
			hr = HrGetOneProp(lpProfSect, PR_EC_USERNAME_W, &ptrPropValueName);
			if (hr != hrSuccess)
				hr = HrGetOneProp(lpProfSect, PR_EC_USERNAME_A, &ptrPropValueName);
			if (hr == hrSuccess) {
				hr = HrGetOneProp(lpProfSect, PR_EC_SERVERNAME_W, &ptrPropValueServerName);
				if (hr != hrSuccess)
					hr = HrGetOneProp(lpProfSect, PR_EC_SERVERNAME_A, &ptrPropValueServerName);
				if (hr != hrSuccess) {
					hr = MAPI_E_UNCONFIGURED;
					goto exit;
				}
			}
			if (hr != hrSuccess) {
				// This should probably be done in UpdateProviders. But UpdateProviders doesn't
				// know the type of the provider and it shouldn't just delete the provider for
				// all types of providers.
				if(lpAdminProvider && ptrPropValueProviderUid.get())
					lpAdminProvider->DeleteProvider((MAPIUID *)ptrPropValueProviderUid->Value.bin.lpb);

				// Invalid or empty archive store
				hr = hrSuccess;
				goto exit;
			}

			hr = GetTransportToNamedServer(lpTransport, ptrPropValueServerName->Value.LPSZ, (PROP_TYPE(ptrPropValueServerName->ulPropTag) == PT_STRING8 ? 0 : MAPI_UNICODE), &lpAltTransport);
			if (hr != hrSuccess)
				goto exit;

			std::swap(lpTransport, lpAltTransport);
			lpAltTransport->Release();
			lpAltTransport = NULL;

			hr = lpTransport->HrResolveTypedStore(convstring::from_SPropValue(ptrPropValueName), ECSTORE_TYPE_ARCHIVE, &cbEntryId, &ptrEntryId);
			if (hr != hrSuccess)
				goto exit;
		} else {
			ASSERT(FALSE); // unknown GUID?
			goto exit;
		}

		hr = lpTransport->HrGetStoreName(cbEntryId, ptrEntryId, MAPI_UNICODE, (LPTSTR*)&ptrStoreName);
		if(hr != hrSuccess) 
			goto exit;

		hr = WrapStoreEntryID(0, (LPTSTR)WCLIENT_DLL_NAME, cbEntryId, ptrEntryId, &cbWrappedEntryId, &ptrWrappedEntryId);
		if(hr != hrSuccess) 
			goto exit;

		sPropVals[cPropValue].ulPropTag = PR_ENTRYID;
		sPropVals[cPropValue].Value.bin.cb = cbWrappedEntryId;
		sPropVals[cPropValue++].Value.bin.lpb = (LPBYTE) ptrWrappedEntryId.get();

		sPropVals[cPropValue].ulPropTag = PR_RECORD_KEY;
		sPropVals[cPropValue].Value.bin.cb = sizeof(MAPIUID);
		sPropVals[cPropValue++].Value.bin.lpb = (LPBYTE) &((PEID)ptrEntryId.get())->guid; //@FIXME validate guid

			sPropVals[cPropValue].ulPropTag = PR_DISPLAY_NAME_W;
			sPropVals[cPropValue++].Value.lpszW = ptrStoreName.get();

		sPropVals[cPropValue].ulPropTag = PR_EC_PATH;
		sPropVals[cPropValue++].Value.lpszA = const_cast<char *>("Server");

		sPropVals[cPropValue].ulPropTag = PR_PROVIDER_DLL_NAME_A;
		sPropVals[cPropValue++].Value.lpszA = const_cast<char *>(WCLIENT_DLL_NAME);
						
	} else if(ulResourceType == MAPI_AB_PROVIDER) {
		hr = initprov_addrbook(abe_id, cPropValue, sPropVals);
		if (hr != hrSuccess)
			goto exit;
	} else {
		if(ulResourceType != MAPI_TRANSPORT_PROVIDER) {
			ASSERT(FALSE);
		}
		goto exit;
	}

	hr = lpProfSect->SetProps(cPropValue, sPropVals, NULL);
	if(hr != hrSuccess)
		goto exit;

	hr = lpProfSect->SaveChanges(0);
	if(hr != hrSuccess)
		goto exit;

	if (lpcStoreID && lppStoreID) {
		*lpcStoreID = cbEntryId;

		hr = MAPIAllocateBuffer(cbEntryId, (void**)lppStoreID);
		if(hr != hrSuccess)
			goto exit;
		
		memcpy(*lppStoreID, ptrEntryId, cbEntryId);
	}
exit:
	//Free allocated memory
	if (lpTransport != NULL && lpTransport != transport)
		lpTransport->Release(); /* implies logoff */
	else
		lpTransport->logoff_nd();
	if (hr == MAPI_S_SPECIAL_OK)
		return hrSuccess;
	return hr;
}

static HRESULT UpdateProviders(LPPROVIDERADMIN lpAdminProviders,
    const sGlobalProfileProps &sProfileProps, WSTransport *transport)
{
	HRESULT hr;

	ProfSectPtr		ptrProfSect;
	MAPITablePtr	ptrTable;
	SRowSetPtr		ptrRows;
	LPSPropValue	lpsProviderUID;

	// Get the provider table
	hr = lpAdminProviders->GetProviderTable(0, &ptrTable);
	if(hr != hrSuccess)
		return hr;

	// Get the rows
	hr = ptrTable->QueryRows(0xFF, 0, &ptrRows);
	if(hr != hrSuccess)
		return hr;

	//Check if exist one or more rows
	if (ptrRows.size() == 0)
		return MAPI_E_NOT_FOUND;

	// Scan the rows for message stores
	for (ULONG curRow = 0; curRow < ptrRows.size(); ++curRow) {
		//Get de UID of the provider to open the profile section
		lpsProviderUID = PpropFindProp(ptrRows[curRow].lpProps, ptrRows[curRow].cValues, PR_PROVIDER_UID);
		if(lpsProviderUID == NULL || lpsProviderUID->Value.bin.cb == 0) {
			// Provider without a provider uid,  just move to the next
			ASSERT(FALSE);
			continue;
		}

		hr = lpAdminProviders->OpenProfileSection((MAPIUID *)lpsProviderUID->Value.bin.lpb, NULL, MAPI_MODIFY, &ptrProfSect);
		if(hr != hrSuccess)
			return hr;

		// Set already PR_PROVIDER_UID, ignore error
		HrSetOneProp(ptrProfSect, lpsProviderUID);

		hr = InitializeProvider(lpAdminProviders, ptrProfSect, sProfileProps, NULL, NULL, transport);
		if (hr != hrSuccess)
			return hr;
	}
	return hrSuccess;
}

static std::string GetServerTypeFromPath(const char *szPath)
{
	std::string path = szPath;
	size_t pos;

	pos = path.find("://");
	if (pos != std::string::npos)
		return path.substr(0, pos);
	return std::string();
}

// Called by MAPI to configure, or create a service
extern "C" HRESULT __stdcall MSGServiceEntry(HINSTANCE hInst,
    LPMALLOC lpMalloc, LPMAPISUP psup, ULONG ulUIParam, ULONG ulFlags,
    ULONG ulContext, ULONG cvals, LPSPropValue pvals,
    LPPROVIDERADMIN lpAdminProviders, MAPIERROR **lppMapiError)
{
	TRACE_MAPI(TRACE_ENTRY, "MSGServiceEntry", "flags=0x%08X, context=%s", ulFlags, MsgServiceContextToString(ulContext));

	HRESULT			hr = erSuccess;
	std::string		strServerName;
	std::wstring	strUserName;
	std::wstring	strUserPassword;
	std::string		strServerPort;
	std::string		strDefaultOfflinePath;
	std::string		strType;
	std::string		strDefStoreServer;
	sGlobalProfileProps	sProfileProps;
	std::basic_string<TCHAR> strError;

	ProfSectPtr		ptrGlobalProfSect;
	ProfSectPtr		ptrProfSect;
	MAPISessionPtr	ptrSession;

	WSTransport		*lpTransport = NULL;
	LPSPropValue	lpsPropValue = NULL;
	ULONG			cValues = 0;
	bool			bShowDialog = false;

	MAPIERROR		*lpMapiError = NULL;
	LPBYTE			lpDelegateStores = NULL;
	ULONG			cDelegateStores = 0;
	LPSPropValue	lpsPropValueFind = NULL;
	ULONG 			cValueIndex = 0;
	convert_context	converter;

	bool bGlobalProfileUpdate = false;
	bool bUpdatedPageConnection = false;
	bool bInitStores = true;

	_hInstance = hInst;

	if (psup) {
		hr = psup->GetMemAllocRoutines(&_pfnAllocBuf, &_pfnAllocMore, &_pfnFreeBuf);
		if(hr != hrSuccess) {
			ASSERT(FALSE);
		}
	} else {
		// Support object not available on linux at this time... TODO: fix mapi4linux?
		_pfnAllocBuf = MAPIAllocateBuffer;
		_pfnAllocMore = MAPIAllocateMore;
		_pfnFreeBuf = MAPIFreeBuffer;
	}

	// Logon defaults
	strType = "http";
	strServerName = "";
	strServerPort ="236";

	switch(ulContext) {
	case MSG_SERVICE_INSTALL:
		hr = hrSuccess;
		break;
	case MSG_SERVICE_UNINSTALL:
		hr = hrSuccess;
		break;
	case MSG_SERVICE_DELETE:
		hr = hrSuccess;
		break;
	case MSG_SERVICE_PROVIDER_CREATE:
		if(cvals && pvals) {

			LPSPropValue lpsPropName = NULL;
			
			lpsPropValueFind = PpropFindProp(pvals, cvals, PR_PROVIDER_UID);
			if(lpsPropValueFind == NULL || lpsPropValueFind->Value.bin.cb == 0)
			{
				//FIXME: give the right error?
				hr = MAPI_E_UNCONFIGURED;
				goto exit;
			}

			// PR_EC_USERNAME is the user we're adding ...
			lpsPropName = PpropFindProp(pvals, cvals, CHANGE_PROP_TYPE(PR_EC_USERNAME_A, PT_UNSPECIFIED));
			if(lpsPropName == NULL || lpsPropName->Value.bin.cb == 0)
			{
				hr = MAPI_E_UNCONFIGURED;
				goto exit;
			}

			//Open profile section
			hr = lpAdminProviders->OpenProfileSection((MAPIUID *)lpsPropValueFind->Value.bin.lpb, NULL, MAPI_MODIFY, &ptrProfSect);
			if(hr != hrSuccess)
				goto exit;

			hr = HrSetOneProp(ptrProfSect, lpsPropName);
			if(hr != hrSuccess)
				goto exit;
		
			hr = lpAdminProviders->OpenProfileSection((LPMAPIUID)pbGlobalProfileSectionGuid, NULL, MAPI_MODIFY , &ptrGlobalProfSect);
			if(hr != hrSuccess)
				goto exit;

			// Get username/pass settings
			hr = ClientUtil::GetGlobalProfileProperties(ptrGlobalProfSect, &sProfileProps);
			if(hr != hrSuccess)
				goto exit;

			if(sProfileProps.strUserName.empty() || sProfileProps.strServerPath.empty()) {
				hr = MAPI_E_UNCONFIGURED; // @todo: check if this is the right error
				goto exit;
			}

			hr = InitializeProvider(lpAdminProviders, ptrProfSect, sProfileProps, NULL, NULL, NULL);
			if (hr != hrSuccess)
				goto exit;
		
		}

		break;
	case MSG_SERVICE_PROVIDER_DELETE:
		hr = hrSuccess;

		//FIXME: delete Offline database

		break;
	case MSG_SERVICE_CONFIGURE:
		//bShowAllSettingsPages = true;
		// Do not break here
	case MSG_SERVICE_CREATE:
		
		//Open global profile, add the store.(for show list, delete etc)
		hr = lpAdminProviders->OpenProfileSection((LPMAPIUID)pbGlobalProfileSectionGuid, NULL, MAPI_MODIFY , &ptrGlobalProfSect);
		if(hr != hrSuccess)
			goto exit;

		if(cvals) {
			hr = ptrGlobalProfSect->SetProps(cvals, pvals, NULL);

			if(hr != hrSuccess)
				goto exit;
		}

		hr = ClientUtil::GetGlobalProfileProperties(ptrGlobalProfSect, &sProfileProps);

		if(sProfileProps.strServerPath.empty() || sProfileProps.strUserName.empty() || (sProfileProps.strPassword.empty() && sProfileProps.strSSLKeyFile.empty())) {
			bShowDialog = true;
		}
		//FIXME: check here offline path with the flags
		if(!sProfileProps.strServerPath.empty()) {
			strServerName = GetServerNameFromPath(sProfileProps.strServerPath.c_str());
			strServerPort = GetServerPortFromPath(sProfileProps.strServerPath.c_str());
			strType = GetServerTypeFromPath(sProfileProps.strServerPath.c_str());
		}

		// Get deligate stores, Ignore error
		ClientUtil::GetGlobalProfileDelegateStoresProp(ptrGlobalProfSect, &cDelegateStores, &lpDelegateStores);

		// init defaults
		hr = WSTransport::Create(ulFlags & SERVICE_UI_ALLOWED ? 0 : MDB_NO_DIALOG, &lpTransport);
		if(hr != hrSuccess)
			goto exit;

		// Check the path, username and password
		while(1)
		{
			bGlobalProfileUpdate = false;
			bUpdatedPageConnection = false;

			if((bShowDialog && ulFlags & SERVICE_UI_ALLOWED) || ulFlags & SERVICE_UI_ALWAYS )
			{
				hr = MAPI_E_USER_CANCEL;
			}// if(bShowDialog...)

						
			if(!(ulFlags & SERVICE_UI_ALLOWED || ulFlags & SERVICE_UI_ALWAYS) && (strServerName.empty() || sProfileProps.strUserName.empty())){
				hr = MAPI_E_UNCONFIGURED;
				goto exit;
			}else if(!strServerName.empty() && !sProfileProps.strUserName.empty()) {
				//Logon the server
				hr = lpTransport->HrLogon(sProfileProps);
			}else{
				hr = MAPI_E_LOGON_FAILED;
			}

			if(hr == MAPI_E_LOGON_FAILED || hr == MAPI_E_NETWORK_ERROR || hr == MAPI_E_VERSION || hr == MAPI_E_INVALID_PARAMETER) {
				bShowDialog = true;
			} else if(hr != erSuccess){ // Big error?
				bShowDialog = true;
				ASSERT(FALSE);
			}else {
				//Update global profile
				if( bGlobalProfileUpdate == true) {

					cValues = 12;
					cValueIndex = 0;
					hr = MAPIAllocateBuffer(sizeof(SPropValue) * cValues, (void**)&lpsPropValue);
					if(hr != hrSuccess)
						goto exit;

					lpsPropValue[cValueIndex].ulPropTag	= PR_EC_PATH;
					lpsPropValue[cValueIndex++].Value.lpszA = (char *)sProfileProps.strServerPath.c_str();

					lpsPropValue[cValueIndex].ulPropTag	= PR_EC_USERNAME_W;
					lpsPropValue[cValueIndex++].Value.lpszW = (wchar_t *)sProfileProps.strUserName.c_str();

					lpsPropValue[cValueIndex].ulPropTag = PR_EC_USERPASSWORD_W;
					lpsPropValue[cValueIndex++].Value.lpszW = (wchar_t *)sProfileProps.strPassword.c_str();

					lpsPropValue[cValueIndex].ulPropTag = PR_EC_FLAGS;
					lpsPropValue[cValueIndex++].Value.ul = sProfileProps.ulProfileFlags;

					lpsPropValue[cValueIndex].ulPropTag = PR_EC_STATS_SESSION_CLIENT_APPLICATION_VERSION;
					lpsPropValue[cValueIndex++].Value.lpszA = (char *)sProfileProps.strClientAppVersion.c_str();

					lpsPropValue[cValueIndex].ulPropTag = PR_EC_STATS_SESSION_CLIENT_APPLICATION_MISC;
					lpsPropValue[cValueIndex++].Value.lpszA = (char *)sProfileProps.strClientAppMisc.c_str();

					if (bUpdatedPageConnection == true)
					{
						lpsPropValue[cValueIndex].ulPropTag = PR_EC_CONNECTION_TIMEOUT;
						lpsPropValue[cValueIndex++].Value.ul = sProfileProps.ulConnectionTimeOut;

						// Proxy settings
						lpsPropValue[cValueIndex].ulPropTag = PR_EC_PROXY_FLAGS;
						lpsPropValue[cValueIndex++].Value.ul = sProfileProps.ulProxyFlags;

						lpsPropValue[cValueIndex].ulPropTag = PR_EC_PROXY_PORT;
						lpsPropValue[cValueIndex++].Value.ul = sProfileProps.ulProxyPort;

						lpsPropValue[cValueIndex].ulPropTag = PR_EC_PROXY_HOST;
						lpsPropValue[cValueIndex++].Value.lpszA = (char *)sProfileProps.strProxyHost.c_str();
							
						lpsPropValue[cValueIndex].ulPropTag = PR_EC_PROXY_USERNAME;
						lpsPropValue[cValueIndex++].Value.lpszA = (char *)sProfileProps.strProxyUserName.c_str();

						lpsPropValue[cValueIndex].ulPropTag = PR_EC_PROXY_PASSWORD;
						lpsPropValue[cValueIndex++].Value.lpszA = (char *)sProfileProps.strProxyPassword.c_str();
					}

					hr = ptrGlobalProfSect->SetProps(cValueIndex, lpsPropValue, NULL);
					if(hr != hrSuccess)
						goto exit;
					
					//Free allocated memory
					MAPIFreeBuffer(lpsPropValue);
					lpsPropValue = NULL;

				}
				break; // Everything is oke
			}
			

			// On incorrect password, and UI allowed, show incorrect password error
			if((ulFlags & SERVICE_UI_ALLOWED || ulFlags & SERVICE_UI_ALWAYS)) {
				// what do we do on linux?
				cout << "Access Denied: Incorrect username and/or password." << endl;
				hr = MAPI_E_UNCONFIGURED;
				goto exit;
			}else if(!(ulFlags & SERVICE_UI_ALLOWED || ulFlags & SERVICE_UI_ALWAYS)){
				// Do not reset the logon error from HrLogon()
				// The DAgent uses this value to determain if the delivery is fatal or not
				// 
				// Although this error is not in the online spec from MS, it should not really matter .... right?
				// hr = MAPI_E_UNCONFIGURED;
				goto exit;
			}

		}// while(1)

		if(bInitStores) {
			hr = UpdateProviders(lpAdminProviders, sProfileProps, lpTransport);
			if(hr != hrSuccess)
				goto exit;
		}
		break;
	} // switch(ulContext)


exit:
	if (lppMapiError) {
		
		*lppMapiError = NULL;

		if(hr != hrSuccess) {
			LPTSTR lpszErrorMsg;

			if (Util::HrMAPIErrorToText(hr, &lpszErrorMsg) == hrSuccess) {
				// Set Error
				strError = _T("EntryPoint: ");
				strError += lpszErrorMsg;
				MAPIFreeBuffer(lpszErrorMsg);

				// Some outlook 2007 clients can't allocate memory so check it
				if(MAPIAllocateBuffer(sizeof(MAPIERROR), (void**)&lpMapiError) == hrSuccess) { 

					memset(lpMapiError, 0, sizeof(MAPIERROR));				

					if ((ulFlags & MAPI_UNICODE) == MAPI_UNICODE) {
						std::wstring wstrErrorMsg = convert_to<std::wstring>(strError);
						std::wstring wstrCompName = convert_to<std::wstring>(g_strProductName.c_str());
							
						if ((hr = MAPIAllocateMore(sizeof(std::wstring::value_type) * (wstrErrorMsg.size() + 1), lpMapiError, (void**)&lpMapiError->lpszError)) != hrSuccess)
							goto exit;
						wcscpy((wchar_t*)lpMapiError->lpszError, wstrErrorMsg.c_str());
						
						if ((hr = MAPIAllocateMore(sizeof(std::wstring::value_type) * (wstrCompName.size() + 1), lpMapiError, (void**)&lpMapiError->lpszComponent)) != hrSuccess)
							goto exit;
						wcscpy((wchar_t*)lpMapiError->lpszComponent, wstrCompName.c_str()); 
					} else {
						std::string strErrorMsg = convert_to<std::string>(strError);
						std::string strCompName = convert_to<std::string>(g_strProductName.c_str());

						if ((hr = MAPIAllocateMore(strErrorMsg.size() + 1, lpMapiError, (void**)&lpMapiError->lpszError)) != hrSuccess)
							goto exit;
						strcpy((char*)lpMapiError->lpszError, strErrorMsg.c_str());
						
						if ((hr = MAPIAllocateMore(strCompName.size() + 1, lpMapiError, (void**)&lpMapiError->lpszComponent)) != hrSuccess)
							goto exit;
						strcpy((char*)lpMapiError->lpszComponent, strCompName.c_str());  
					}
				
					lpMapiError->ulVersion = 0;
					lpMapiError->ulLowLevelError = 0;
					lpMapiError->ulContext = 0;

					*lppMapiError = lpMapiError;
				}
			}
		}
	}

	MAPIFreeBuffer(lpDelegateStores);
	if(lpTransport)
		lpTransport->Release();
	MAPIFreeBuffer(lpsPropValue);
	TRACE_MAPI(TRACE_RETURN, "MSGServiceEntry", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}


extern "C" HRESULT __cdecl XPProviderInit(HINSTANCE hInstance, LPMALLOC lpMalloc, LPALLOCATEBUFFER lpAllocateBuffer, LPALLOCATEMORE lpAllocateMore, LPFREEBUFFER lpFreeBuffer, ULONG ulFlags, ULONG ulMAPIVer, ULONG * lpulProviderVer, LPXPPROVIDER * lppXPProvider)
{
	TRACE_MAPI(TRACE_ENTRY, "XPProviderInit", "");

	HRESULT hr = hrSuccess;
	ECXPProvider	*pXPProvider = NULL;

    if (ulMAPIVer < CURRENT_SPI_VERSION)
    {
        hr = MAPI_E_VERSION;
		goto exit;
    }

	*lpulProviderVer = CURRENT_SPI_VERSION;

	// Save the pointer to the allocation routines in global variables
	_pmalloc = lpMalloc;
	_pfnAllocBuf = lpAllocateBuffer;
	_pfnAllocMore = lpAllocateMore;
	_pfnFreeBuf = lpFreeBuffer;
	_hInstance = hInstance;

	hr = ECXPProvider::Create(&pXPProvider);
	if(hr != hrSuccess)
		goto exit;

	hr = pXPProvider->QueryInterface(IID_IXPProvider, (void **)lppXPProvider);

exit:
	if(pXPProvider)
		pXPProvider->Release();


	return hr;
}


extern "C" HRESULT  __cdecl ABProviderInit(HINSTANCE hInstance, LPMALLOC lpMalloc, LPALLOCATEBUFFER lpAllocateBuffer, LPALLOCATEMORE lpAllocateMore, LPFREEBUFFER lpFreeBuffer, ULONG ulFlags, ULONG ulMAPIVer, ULONG * lpulProviderVer, LPABPROVIDER * lppABProvider)
{
	TRACE_MAPI(TRACE_ENTRY, "ABProviderInit", "");

	HRESULT hr = hrSuccess;
	ECABProviderSwitch	*lpABProvider = NULL;

	if (ulMAPIVer < CURRENT_SPI_VERSION)
	{
		hr = MAPI_E_VERSION;
		goto exit;
	}

	*lpulProviderVer = CURRENT_SPI_VERSION;
	// Save the pointer to the allocation routines in global variables
	_pmalloc = lpMalloc;
	_pfnAllocBuf = lpAllocateBuffer;
	_pfnAllocMore = lpAllocateMore;
	_pfnFreeBuf = lpFreeBuffer;
	_hInstance = hInstance;

	hr = ECABProviderSwitch::Create(&lpABProvider);
	if(hr != hrSuccess)
		goto exit;

	hr = lpABProvider->QueryInterface(IID_IABProvider, (void **)lppABProvider);

exit:
	if(lpABProvider)
		lpABProvider->Release();


	return hr;
}
