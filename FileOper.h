// FileOper.h: interface for the CFileOper class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEOPER_H__25D4B209_DAC6_4D93_98B2_692621E5508F__INCLUDED_)
#define AFX_FILEOPER_H__25D4B209_DAC6_4D93_98B2_692621E5508F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdio.h>
#include <Shlobj.h>


static int CALLBACK BrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData)
{
	switch(uMsg)
	{
	case BFFM_INITIALIZED:    //��ʼ����Ϣ
		::SendMessage(hwnd,BFFM_SETSELECTION,TRUE,lpData);
		break;
	case BFFM_SELCHANGED:    //ѡ��·���仯��
		{
			TCHAR curr[MAX_PATH];   
			SHGetPathFromIDList((LPCITEMIDLIST)lParam,curr);   
			::SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)curr);   
		}
		break;
	default:
		break;
	}
	return 0;   
}

class CFileOper  
{
public:
	// �����ļ���ѡ��Ի��򣬲�����ѡ�����ļ���·��
	static BOOL BrowseGetPath(CString &path, HWND hWnd){
		TCHAR szPath[MAX_PATH] = {0};
		BROWSEINFO bi;
		memset(&bi, 0, sizeof(bi));

		bi.hwndOwner = hWnd;
		bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT;
		bi.lpszTitle = _T("select folder");
		bi.lpfn = BrowseCallbackProc;
		bi.lParam = (LONG)(LPCTSTR)path;

		LPITEMIDLIST idl = SHBrowseForFolder(&bi);
		if(idl == NULL)
			return FALSE;

		SHGetPathFromIDList(idl, szPath);
		path.Format(_T("%s"), szPath);
		return TRUE;
	}

	// ɾ����·���������ļ����ļ���
	static BOOL DeleteFolderAndAllSubFolder_Files(LPCTSTR folderPath){
		system("echo delete old folder and sub files...\n");
		//CString cmd = _T("");
		char cmd[64] = {0};

#if defined(_UNICODE) || defined(UNICODE)
		char *path = Utf16ToAnsi(folderPath);
		sprintf_s(cmd, "rd /s /q \"%s\"", path);
		SAFEDELETEARR(path);
#else
		sprintf_s(cmd, "rd /s /q \"%s\"", folderPath);
#endif
		BOOL ret = system(cmd) == 0;
		sprintf_s(cmd, "echo delete %s\n", ret ? "success" : "failed");
		system(cmd);
		return ret;
	}

	// ��ȡ�ļ���С
	static long GetFileSize(const CString& path){
		FILE *p = NULL;
#if defined(_UNICODE) || defined(UNICODE)
		char *cpath = Utf16ToAnsi(path);
		fopen_s(&p, cpath, "r");
		if(p == NULL)
		{
			SAFEDELETEARR(cpath);
			return 0;
		}
		SAFEDELETEARR(cpath);
#else
		fopen_s(&p, path, "r");
		if(p == NULL)
			return 0;
#endif
		fseek(p, 0, SEEK_END);
		long len = ftell(p);
		fclose(p);
		return len;
	}

	// ��ȡ·�������ҵ��ļ������ƣ���D:\A\B,����B
	static CString GetCurFolderFromFullPath(CString strpath){
		int pos = -1;
		pos = strpath.ReverseFind(_T('\\'));
		if(pos != -1)
			return strpath.Right(strpath.GetLength() - pos - 1);
		return _T("");
	}

	// �����ļ��е�Ŀ�ģ��������ļ��и��ƶԻ���
	static BOOL CopyFolder(CString dstFolder, CString srcFolder, HWND hWnd){
		TCHAR szDst[MAX_PATH], szSrc[MAX_PATH];
		ZeroMemory(szDst, MAX_PATH);
		ZeroMemory(szSrc, MAX_PATH);
		lstrcpy(szDst, dstFolder);
		lstrcpy(szSrc, srcFolder);

		SHFILEOPSTRUCT lpFile;
		lpFile.hwnd = hWnd;
		lpFile.wFunc = FO_COPY;
		lpFile.pFrom = szSrc;
		lpFile.pTo = szDst;
		lpFile.fFlags = FOF_ALLOWUNDO;
		lpFile.fAnyOperationsAborted = FALSE;
		lpFile.hNameMappings = NULL;
		lpFile.lpszProgressTitle = NULL;

		int ret = SHFileOperation(&lpFile);
		return ret == 0 && lpFile.fAnyOperationsAborted == FALSE;
	}
	
	// �ж�ָ��·���Ƿ���ڣ��ļ��У�
	static BOOL IsFolderExists(LPCTSTR lpszFolderPath){
		return FindFirstFileExists(lpszFolderPath, FILE_ATTRIBUTE_DIRECTORY);
	}

	// �ж�ָ��·���Ƿ���ڣ��ļ����ļ��У�
	static BOOL PathExists(LPCTSTR lpszPath){
		return FindFirstFileExists(lpszPath, FALSE);
	}

	// �����ļ�����׺��
	static CString ChangeFileExt(const CString& srcFilePath, const CString& dstExt){
		CString dstFilePath = srcFilePath;
		int pos = srcFilePath.ReverseFind(_T('.'));
		if(pos != -1){
			dstFilePath = srcFilePath.Left(pos + 1);
			dstFilePath += dstExt;
		}
		return dstFilePath;
	}

	// ���ļ�·���л�ȡ�ļ����⣬��D:\AAA.TXT����AAA.TXT������AAA
	static CString GetFileTitle(const CString& fileName){
		CString title = _T("");
		int pos = -1;
		pos = fileName.FindOneOf(_T("\\"));
		CString name = fileName;
		if(pos > 0)
			name = GetFileNameFromPathName(fileName);
		pos = name.ReverseFind(_T('.'));
		if(pos != -1)
			title = name.Left(pos);
		return title;
	}

	// ���ļ�·���л�ȡ�ļ���׺������D:\AAA.TXT������TXT
	static CString GetFileExt(const CString& filePath){
		CString ext = _T("");
		int pos = filePath.ReverseFind(_T('.'));
		if(pos != -1){
			ext = filePath.Right(filePath.GetLength() - pos - 1);
			ext.MakeUpper();
		}
		return ext;
	}

	// ���ļ�·���л�ȡ�ļ�������D:\AAA.TXT������AAA.TXT
	static CString GetFileNameFromPathName(const CString& filePathName){
		CString fileName = _T("");
		int pos = filePathName.ReverseFind(_T('\\'));
		if(pos != -1)		{
			fileName = filePathName.Right(filePathName.GetLength() - pos - 1);
		}
		return fileName;
	}

	// Ѱ��Ŀ¼(��·�������� \ ��β)������ָ����׺��(��֧��*.*)���ļ����������������CStringList�У�
	static int FindFilesInFolder(LPCTSTR lpszFolder, LPCTSTR lpszFilter, CStringList &dstList){
		if(lstrlen(lpszFolder) == 0)
			return 0;
		int nNums = 0;
		CFileFind filefind;
		CString path, filter;
		filter.Format(_T("%s\\%s"), lpszFolder, lpszFilter);
		BOOL bFound = filefind.FindFile(filter); 
		while(bFound){
			bFound = filefind.FindNextFile(); 
			if(filefind.IsDots())
				continue;
			if(!filefind.IsDirectory()){			
				path = filefind.GetFilePath();
				dstList.AddTail(path);
				nNums++;
			}
		}
		filefind.Close();
		return nNums;
	}

	// ��Ŀ¼�·��� *n.* ���ļ�(n��������)������Ϊ *00n.*����ʽ����n��ʽ��Ϊ3λ��
	static BOOL RenameFile03d(LPCTSTR lpszFilePath){
		CString fileName = lpszFilePath;
		CString tittle, num, ext, newFileName;
		int pos;
		pos = fileName.ReverseFind(_T('.'));
		if(pos == -1) return FALSE;

		tittle = fileName.Left(pos);
		ext = fileName.Right(fileName.GetLength() - pos);

		int i = 0, posNum = 0, numnums = 0;
		TCHAR ch;
		bool bNumCharFound = false;
		for(i = tittle.GetLength()-1, posNum = i+1; i > 0; i--){
			ch = tittle.GetAt(i);
			if(ch > _T('0') && ch < _T('9')){
				if(posNum != i+1)
					break;
				posNum = i;
				numnums++;
				bNumCharFound = true;
			}
		}
		if(!bNumCharFound) return FALSE;
		num = tittle.Right(numnums);
		tittle = tittle.Left(posNum);
		int nums = _ttoi(num.GetBuffer(num.GetLength()));
		num.ReleaseBuffer();
		num.Format(_T("%03d"), nums);
		tittle += num;
		newFileName.Format(_T("%s%s"), tittle, ext);
		LPCTSTR szOldName = fileName.GetBuffer(fileName.GetLength());
		fileName.ReleaseBuffer();
		LPCTSTR szNewName = newFileName.GetBuffer(newFileName.GetLength());
		newFileName.ReleaseBuffer();
		MoveFile(szOldName, szNewName);
		TRACE(_T("movefile %s to %s \n"), fileName, newFileName);
		return TRUE;
	}

	// ɾ�����и�Ŀ¼·����ָ����׺�����ļ�
	static DWORD DeleteFilesInFolder(LPCTSTR lpszFolder, LPCTSTR lpszFilter){
		DWORD dwDeletedFileNums = 0;
		CFileFind find;
		BOOL bfoundone = FALSE;
		CString filter = _T("");
		filter.Format(_T("%s\\%s"), lpszFolder, lpszFilter);
		BOOL bFound = find.FindFile(filter); 
		TRACE(_T("CFileOper::DeleteFilesInFolder find path %s\n"), filter);
		CString filepath = _T("");
		while(bFound){
			bFound = find.FindNextFile(); 
			if(find.IsDots())
				continue;
			if(!find.IsDirectory()){			
				filepath = find.GetFilePath();
				if(DeleteFile(filepath.GetBuffer(filepath.GetLength()))){
					CString trace;trace.Format(_T("CFileOper::DeleteFilesInFolder delete file %s \n"), 
						find.GetFilePath());
					TRACE(trace);
					bfoundone = TRUE;
					dwDeletedFileNums++;
				}
			}
		}
		if(!bfoundone)
			TRACE(_T("CFileOper::DeleteFilesInFolder no file deleted\n"));
		find.Close();
		return dwDeletedFileNums;
	}
private:
	CFileOper();
	virtual ~CFileOper();
protected:
	static BOOL FindFirstFileExists(LPCTSTR lspzPath, DWORD dwFilter){
		TCHAR realPath[MAX_PATH] = {0};
		if(_tcsclen(lspzPath) > MAX_PATH)
			GetShortPathName(lspzPath, realPath, MAX_PATH);
		else
			_tcscpy_s(realPath, lspzPath);

		WIN32_FIND_DATA fd;  
		HANDLE hFind = FindFirstFile(realPath, &fd);  
		BOOL bFilter = (FALSE == dwFilter) ? TRUE : fd.dwFileAttributes & dwFilter;  
		BOOL RetValue = ((hFind != INVALID_HANDLE_VALUE) && bFilter) ? TRUE : FALSE;  
		FindClose(hFind);  
		return RetValue;  
	}
};

#endif // !defined(AFX_FILEOPER_H__25D4B209_DAC6_4D93_98B2_692621E5508F__INCLUDED_)
