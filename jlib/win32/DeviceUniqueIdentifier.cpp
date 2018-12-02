#include "DeviceUniqueIdentifier.h"
#include <Windows.h>
#include <algorithm>
#include <math.h>
#include <strsafe.h>
#include <tchar.h>
#include <ntddndis.h>
#include <WbemIdl.h>
#include <comdef.h>
#include <comutil.h>
#include <atlconv.h>
#include <jlib/3rdparty/win32/WinReg.hpp>
#include <jlib/utf8.h>

#pragma comment (lib, "comsuppw.lib")
#pragma comment (lib, "wbemuuid.lib")

namespace jlib {
namespace DeviceUniqueIdentifier {
namespace detail {

struct DeviceProperty {
	enum { PROPERTY_MAX_LEN = 128 }; // �����ֶ���󳤶�
	wchar_t szProperty[PROPERTY_MAX_LEN];
};

struct WQL_QUERY
{
	const wchar_t* szSelect = nullptr;
	const wchar_t* szProperty = nullptr;
};

inline WQL_QUERY getWQLQuery(QueryType queryType) {
	switch (queryType) {
	case DeviceUniqueIdentifier::MAC_ADDR:
		return // ������ǰMAC��ַ
		{ L"SELECT * FROM Win32_NetworkAdapter WHERE (MACAddress IS NOT NULL) AND (NOT (PNPDeviceID LIKE 'ROOT%'))",
			L"MACAddress" };
	case DeviceUniqueIdentifier::MAC_ADDR_REAL:
		return // ����ԭ��MAC��ַ
		{ L"SELECT * FROM Win32_NetworkAdapter WHERE (MACAddress IS NOT NULL) AND (NOT (PNPDeviceID LIKE 'ROOT%'))",
			L"PNPDeviceID" };
	case DeviceUniqueIdentifier::HARDDISK_SERIAL:
		return // Ӳ�����к�
		{ L"SELECT * FROM Win32_DiskDrive WHERE (SerialNumber IS NOT NULL) AND (MediaType LIKE 'Fixed hard disk%')",
			L"SerialNumber" };
	case DeviceUniqueIdentifier::MOTHERBOARD_UUID:
		return // �������к�
		{ L"SELECT * FROM Win32_BaseBoard WHERE (SerialNumber IS NOT NULL)",
			L"SerialNumber" };
	case DeviceUniqueIdentifier::MOTHERBOARD_MODEL:
		return // �����ͺ�
		{ L"SELECT * FROM Win32_BaseBoard WHERE (Product IS NOT NULL)",
			L"Product" };
	case DeviceUniqueIdentifier::CPU_ID:
		return // ������ID
		{ L"SELECT * FROM Win32_Processor WHERE (ProcessorId IS NOT NULL)",
			L"ProcessorId" };
	case DeviceUniqueIdentifier::BIOS_SERIAL:
		return // BIOS���к�
		{ L"SELECT * FROM Win32_BIOS WHERE (SerialNumber IS NOT NULL)",
			L"SerialNumber" };
	case DeviceUniqueIdentifier::WINDOWS_PRODUCT_ID:
		return // Windows ��ƷID
		{ L"SELECT * FROM Win32_OperatingSystem WHERE (SerialNumber IS NOT NULL)",
			L"SerialNumber" };
	default:
		return { L"", L"" };
	}
}


static BOOL WMI_DoWithHarddiskSerialNumber(wchar_t *SerialNumber, UINT uSize)
{
	UINT	iLen;
	UINT	i;

	iLen = wcslen(SerialNumber);
	if (iLen == 40)	// InterfaceType = "IDE"
	{	// ��Ҫ��16���Ʊ��봮ת��Ϊ�ַ���
		wchar_t ch, szBuf[32];
		BYTE b;

		for (i = 0; i < 20; i++) {	// ��16�����ַ�ת��Ϊ��4λ
			ch = SerialNumber[i * 2];
			if ((ch >= '0') && (ch <= '9')) {
				b = ch - '0';
			} else if ((ch >= 'A') && (ch <= 'F')) {
				b = ch - 'A' + 10;
			} else if ((ch >= 'a') && (ch <= 'f')) {
				b = ch - 'a' + 10;
			} else {	// �Ƿ��ַ�
				break;
			}

			b <<= 4;

			// ��16�����ַ�ת��Ϊ��4λ
			ch = SerialNumber[i * 2 + 1];
			if ((ch >= '0') && (ch <= '9')) {
				b += ch - '0';
			} else if ((ch >= 'A') && (ch <= 'F')) {
				b += ch - 'A' + 10;
			} else if ((ch >= 'a') && (ch <= 'f')) {
				b += ch - 'a' + 10;
			} else {	// �Ƿ��ַ�
				break;
			}

			szBuf[i] = b;
		}

		if (i == 20) {	// ת���ɹ�
			szBuf[i] = L'\0';
			StringCchCopyW(SerialNumber, uSize, szBuf);
			iLen = wcslen(SerialNumber);
		}
	}

	// ÿ2���ַ�����λ��
	for (i = 0; i < iLen; i += 2) {
		std::swap(SerialNumber[i], SerialNumber[i + 1]);
	}

	// ȥ���ո�
	std::remove(SerialNumber, SerialNumber + wcslen(SerialNumber) + 1, L' ');
	return TRUE;
}


// ͨ����PNPDeviceID����ȡ����ԭ��MAC��ַ
static BOOL WMI_DoWithPNPDeviceID(const wchar_t *PNPDeviceID, wchar_t *MacAddress, UINT uSize)
{
	wchar_t	DevicePath[MAX_PATH];
	HANDLE	hDeviceFile;
	BOOL	isOK = FALSE;

	// �����豸·����
	StringCchCopyW(DevicePath, MAX_PATH, L"\\\\.\\");
	StringCchCatW(DevicePath, MAX_PATH, PNPDeviceID);
	// {ad498944-762f-11d0-8dcb-00c04fc3358c} is 'Device Interface Name' for 'Network Card'
	StringCchCatW(DevicePath, MAX_PATH, L"#{ad498944-762f-11d0-8dcb-00c04fc3358c}");

	// ����PNPDeviceID���еġ�/���滻�ɡ�#�����Ի���������豸·����
	std::replace(DevicePath + 4, DevicePath + 4 + wcslen(PNPDeviceID), (int)L'\\', (int)L'#');

	// ��ȡ�豸���
	hDeviceFile = CreateFileW(DevicePath,
							  0,
							  FILE_SHARE_READ | FILE_SHARE_WRITE,
							  NULL,
							  OPEN_EXISTING,
							  0,
							  NULL);

	if (hDeviceFile != INVALID_HANDLE_VALUE) {
		ULONG	dwID;
		BYTE	ucData[8];
		DWORD	dwByteRet;

		// ��ȡ����ԭ��MAC��ַ
		dwID = OID_802_3_PERMANENT_ADDRESS;
		isOK = DeviceIoControl(hDeviceFile, IOCTL_NDIS_QUERY_GLOBAL_STATS, &dwID, sizeof(dwID), ucData, sizeof(ucData), &dwByteRet, NULL);
		if (isOK) {	// ���ֽ�����ת����16�����ַ���
			for (DWORD i = 0; i < dwByteRet; i++) {
				StringCchPrintfW(MacAddress + (i << 1), uSize - (i << 1), L"%02X", ucData[i]);
			}

			MacAddress[dwByteRet << 1] = L'\0';	// д���ַ����������
		}

		CloseHandle(hDeviceFile);
	}

	return isOK;
}


static BOOL WMI_DoWithProperty(QueryType queryType, wchar_t *szProperty, UINT uSize)
{
	BOOL isOK = TRUE;
	switch (queryType) {
	case MAC_ADDR_REAL:		// ����ԭ��MAC��ַ		
		isOK = WMI_DoWithPNPDeviceID(szProperty, szProperty, uSize);
		break;
	case HARDDISK_SERIAL:	// Ӳ�����к�
		isOK = WMI_DoWithHarddiskSerialNumber(szProperty, uSize);
		break;
	case MAC_ADDR:			// ������ǰMAC��ַ
		// ȥ��ð��
		std::remove(szProperty, szProperty + wcslen(szProperty) + 1, L':');
		break;
	default:
		// ȥ���ո�
		std::remove(szProperty, szProperty + wcslen(szProperty) + 1, L' ');
	}
	return isOK;
}



std::wstring getMachineGUID()
{
	std::wstring res;
	/*using namespace winreg;
	try {
		RegKey key;
		key.Open(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography");
		res = key.GetStringValue(L"MachineGuid");
	} catch (RegException& e) {
		res = utf8::a2w(e.what());
	} catch (std::exception& e) {
		res = utf8::a2w(e.what());
	} */

	try {

		std::wstring key = L"SOFTWARE\\Microsoft\\Cryptography";
		std::wstring name = L"MachineGuid";

		HKEY hKey;

		if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &hKey) != ERROR_SUCCESS)
			throw std::runtime_error("Could not open registry key");

		DWORD type;
		DWORD cbData;

		if (RegQueryValueExW(hKey, name.c_str(), NULL, &type, NULL, &cbData) != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			throw std::runtime_error("Could not read registry value");
		}

		if (type != REG_SZ) {
			RegCloseKey(hKey);
			throw "Incorrect registry value type";
		}

		std::wstring value(cbData / sizeof(wchar_t), L'\0');
		if (RegQueryValueExW(hKey, name.c_str(), NULL, NULL, reinterpret_cast<LPBYTE>(&value[0]), &cbData) != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			throw "Could not read registry value";
		}

		res = value;
		RegCloseKey(hKey);

	} catch (std::runtime_error& e) {
		res = utf8::a2w(e.what());
	}
	return res;
}

} // end of namespace detail


bool query(size_t queryTypes, std::vector<std::wstring>& results)
{
	std::vector<QueryType> vec;

	if (queryTypes & MAC_ADDR) {
		vec.push_back(MAC_ADDR);
	}
	if (queryTypes & MAC_ADDR_REAL) {
		vec.push_back(MAC_ADDR_REAL);
	}
	if (queryTypes & HARDDISK_SERIAL) {
		vec.push_back(HARDDISK_SERIAL);
	}
	if (queryTypes & MOTHERBOARD_UUID) {
		vec.push_back(MOTHERBOARD_UUID);
	}
	if (queryTypes & MOTHERBOARD_MODEL) {
		vec.push_back(MOTHERBOARD_MODEL);
	}
	if (queryTypes & CPU_ID) {
		vec.push_back(CPU_ID);
	}
	if (queryTypes & BIOS_SERIAL) {
		vec.push_back(BIOS_SERIAL);
	}
	if (queryTypes & WINDOWS_PRODUCT_ID) {
		vec.push_back(WINDOWS_PRODUCT_ID);
	}

	auto ok = query(vec, results);
	if (queryTypes & WINDOWS_MACHINE_GUID) {
		results.push_back(detail::getMachineGUID());
	}

	return ok;
}

// ����Windows Management Instrumentation��Windows�����淶��
bool query(const std::vector<QueryType>& queryTypes, std::vector<std::wstring>& results)
{
	bool ok = false;

	// ��ʼ��COM
	HRESULT hres = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hres)) {
		return false;
	}

	// ����COM�İ�ȫ��֤����
	hres = CoInitializeSecurity(
		NULL,
		-1,
		NULL,
		NULL,
		RPC_C_AUTHN_LEVEL_DEFAULT,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE,
		NULL
	);
	if (FAILED(hres)) {
		CoUninitialize();
		return false;
	}

	// ���WMI����COM�ӿ�
	IWbemLocator *pLoc = NULL;
	hres = CoCreateInstance(
		CLSID_WbemLocator,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator,
		reinterpret_cast<LPVOID*>(&pLoc)
	);
	if (FAILED(hres)) {
		CoUninitialize();
		return false;
	}

	// ͨ�����ӽӿ�����WMI���ں˶�����"ROOT//CIMV2"
	IWbemServices *pSvc = NULL;
	hres = pLoc->ConnectServer(
		_bstr_t(L"ROOT\\CIMV2"),
		NULL,
		NULL,
		NULL,
		0,
		NULL,
		NULL,
		&pSvc
	);
	if (FAILED(hres)) {
		_com_error e(hres);
		e.ErrorMessage();
		pLoc->Release();
		CoUninitialize();
		return false;
	}

	// ������������İ�ȫ����
	hres = CoSetProxyBlanket(
		pSvc,
		RPC_C_AUTHN_WINNT,
		RPC_C_AUTHZ_NONE,
		NULL,
		RPC_C_AUTHN_LEVEL_CALL,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE
	);
	if (FAILED(hres)) {
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return false;
	}

	for (auto queryType : queryTypes) {
		auto query = detail::getWQLQuery(queryType);

		// ͨ�������������WMI��������
		IEnumWbemClassObject *pEnumerator = NULL;
		hres = pSvc->ExecQuery(
			bstr_t("WQL"),
			bstr_t(query.szSelect),
			WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
			NULL,
			&pEnumerator
		);
		if (FAILED(hres)) {
			pSvc->Release();
			pLoc->Release();
			CoUninitialize();
			continue;
		}

		// ѭ��ö�����еĽ������  
		while (pEnumerator) {
			IWbemClassObject *pclsObj = NULL;
			ULONG uReturn = 0;

			pEnumerator->Next(
				WBEM_INFINITE,
				1,
				&pclsObj,
				&uReturn
			);

			if (uReturn == 0) {
				break;
			}

			// ��ȡ����ֵ
			{
				VARIANT vtProperty;
				VariantInit(&vtProperty);
				pclsObj->Get(query.szProperty, 0, &vtProperty, NULL, NULL);
				detail::DeviceProperty deviceProperty{};
				StringCchCopyW(deviceProperty.szProperty, detail::DeviceProperty::PROPERTY_MAX_LEN, vtProperty.bstrVal);
				VariantClear(&vtProperty);
				// ������ֵ����һ���Ĵ���
				if (detail::WMI_DoWithProperty(queryType, deviceProperty.szProperty, detail::DeviceProperty::PROPERTY_MAX_LEN)) {
					results.push_back(deviceProperty.szProperty);
					ok = true;
					pclsObj->Release();
					break;
				}
			}

			pclsObj->Release();
		} // End While
		  // �ͷ���Դ
		pEnumerator->Release();

	}

	pSvc->Release();
	pLoc->Release();
	CoUninitialize();
	return ok;
}

std::wstring join_result(const std::vector<std::wstring>& results, const std::wstring & conjunction)
{
	std::wstring result;
	auto itBegin = results.cbegin();
	auto itEnd = results.cend();
	if (itBegin != itEnd) {
		result = *itBegin++;
	}
	for (; itBegin != itEnd; itBegin++) {
		result += conjunction;
		result += *itBegin;
	}
	return result;
}

}
}