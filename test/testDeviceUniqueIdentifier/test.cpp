#include <jlib/win32/DeviceUniqueIdentifier.h>
#include <stdio.h>
#include <algorithm>
#include <locale>

int main()
{
	std::locale::global(std::locale(""));
	using namespace jlib::DeviceUniqueIdentifier;
	std::vector<std::wstring> results;
	/*query(RecommendedQueryTypes, results);
	auto str = join_result(results, L"\n");
	std::wcout << str << std::endl;*/

	results.clear();
	query(AllQueryTypes, results);
	std::wstring str;
	std::vector<std::wstring> types = {
		L"MAC_ADDR ����MAC��ַ",
		L"MAC_ADDR_REAL ����ԭ��MAC��ַ",
		L"HARDDISK_SERIAL ��Ӳ�����к�",
		L"MOTHERBOARD_UUID �������к�",
		L"MOTHERBOARD_MODEL �����ͺ�",
		L"CPU_ID ���������к�",
		L"BIOS_SERIAL BIOS���к�",
		L"WINDOWS_PRODUCT_ID Windows��ƷID",
		L"WINDOWS_MACHINE_GUID Windows������",

	};
	for (size_t i = 0; i < std::min(types.size(), results.size()); i++) {
		printf("%ls:\n%ls\n\n", types[i].c_str(), results[i].c_str());
	}
	//auto str = join_result(results, L"\n");
	//std::wcout << std::endl << str << std::endl;

	system("pause");
}