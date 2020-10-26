// Task1.cpp: ���������� ����� ����� ��� ����������� ����������.
//

#include "stdafx.h"
#include <windows.h>

#include <vector> 

#include "FileLocker.h"

using namespace std ;

// ����� ������
vector<char*> * masks ;
char password[255] ;
// ������ �� ������-����������� �� ���������
HANDLE hDir;

// ���� �������
char * CONFIGFILE = "templates.tbl" ;

// ������������ �������
CFileLocker * conflocker ;

// ���������� ������ ������
vector<char*> * fixedlist ;

// ������ ��������������� ��������
vector<CFileLocker*> * locklist ;

// ������������� ������
char * decode(char * str) {
	int len = strlen(str)/2 ;
	// ��������� �� ������������������ ������������� � ������� - �� ���������� �� ������� 0-9A-F,
	// � ������� a,b,c,...
	char * res = new char[len+1] ;
	for (int i=0; i<len; i++) {
		res[i]=(str[2*i]-'a')*16+str[2*i+1]-'a' ;
	}
	res[len]=0 ;
	return res ;
}

// �������� �����
void LoadMasks() {
	masks = new vector<char*>() ;

	FILE * f = fopen(CONFIGFILE,"r") ;

	char buf[255] ;
	fgets(buf, 255, f);
	buf[strlen(buf)-1]=0 ;

	// �������� ������ �� ������ ������
	strcpy(password,decode(buf)) ;

	// ��������� ����� � ������
	while (fgets(buf, 255, f)) {
		buf[strlen(buf)-1]=0 ;
		char * str = new char[strlen(buf)+1] ;
		strcpy(str,buf) ;
		masks->push_back(str) ;
	}

	fclose(f) ;

}

// ������ ������ � ������
void loadFiles2Vector(vector<char*> * vec) {
	vec->clear() ;

	WIN32_FIND_DATAA FindFileData;
    HANDLE hf;
	
	SetCurrentDirectoryA(".") ;
	hf = FindFirstFileA("*", &FindFileData);

	if (hf != INVALID_HANDLE_VALUE)
        {
			do
                {
                        if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue ;
						char * str = new char[MAX_PATH] ;
						strcpy(str,FindFileData.cFileName) ;
 						vec->push_back(str) ;
                }
                while (FindNextFileA(hf,&FindFileData)!=0);
                FindClose(hf);
        }

}

// ��������� ������ ������
void FixList() {
	fixedlist = new vector<char*>() ;
	loadFiles2Vector(fixedlist) ;
}

// �������� ����� �� ������������
bool isMatchMask(char * filename, char * mask) {
	// �������� �� ����� � ������ �� ������ �� �����
	int pm=0 ;
	int pf=0 ; 
	// �����
	int lm = strlen(mask) ;
	int lf = strlen(filename) ;
	// ����������� ����, ��������� ������
	while (true) {
		// ���� ������� �� �������, � ��� ���� � ����� �� ����� ���� ����������� "?" - ����� �� �������������
		if ((filename[pf]!=mask[pm])&&(mask[pm]!='?')) return false ;
		pm++ ;
		pf++ ;
		// ���� ����� ����� �����������, �� ����� �� �������������
		if ((pm==lm)&&(pf<lf)) return false ;
		if ((pm<lm)&&(pf==lf)) return false ;
		// ���� ����� �� ����� ����� �����, �� ����� �������������
		if ((pm==lm)&&(pf==lf)) return true ;
	}
}

// ��������, �������� �� ����� �� ������ � �����
bool isMatchMasks(char * filename) {
	for (int i=0; i<masks->size(); i++) {
		if (isMatchMask(filename,masks->at(i))) return true ;
	}
	return false ;
}

// �������� ����� ��������������
void forcedDropFile(char * filename) {
	printf("drop %s\n",filename) ;

	while (true) {
		if (DeleteFileA(filename)!=0) {
			break ;
		}
		Sleep(500) ;
	}	
}

// ������ ��������
void AnalizDir() {
	// ������� �������� ������� ������ ������
	vector<char*> * newlist = new vector<char*>() ;
	loadFiles2Vector(newlist) ;

	// ���� ����� ������� �� ����� - ���� ���������, ��������� ������
	if (newlist->size()>fixedlist->size()) {
		for (int i=0; i<newlist->size(); i++) {
			bool isfind=false ;
			// ���� ���, ������� ���� � �����, �� ��� � ������ �������
			for (int j=0; j<fixedlist->size(); j++) 
				if (!strcmp(fixedlist->at(j),newlist->at(i))) {
					isfind=true ;
					break ;
				}
				// ���� ����� ����� � ��� ������������� ������ - ������� ���� �������������
			if (!isfind) {
				if (isMatchMasks(newlist->at(i))) {
					forcedDropFile(newlist->at(i)) ;
				}				
			}
		}
	}

	delete newlist ;
	
}

// ������� ������ ��� �������� �� ���������
DWORD WINAPI ThreadProc(LPVOID lpParameter) {

	FixList() ;
	// ����������� ���� ������
	while (true) {
	
		// �������� ����������
	hDir=FindFirstChangeNotification(L".",
		false,FILE_NOTIFY_CHANGE_FILE_NAME);
	if (hDir!=INVALID_HANDLE_VALUE) {

		// ���� �������
	while (WaitForSingleObject(hDir,-1)!=WAIT_OBJECT_0)
	{
	}
	
	// ���� ��� ��������� ��������
	if (hDir!=NULL) {
		// ��������� ������
		AnalizDir() ;
		// ��������� ��������
		FindCloseChangeNotification(hDir);
		hDir=NULL ;
	}
	// ���� ���� �������� �������, �� ������� �� �����
	else {
		return 0 ;
	}
	}

	}

	return 0 ;
}

// ������������� ������������ �����
void LockExistFiles() {
	locklist = new vector<CFileLocker*>() ;

	vector<char*> * filelist = new vector<char*>() ;
	loadFiles2Vector(filelist) ;

	for (int i=0; i<filelist->size(); i++)
		if (isMatchMasks(filelist->at(i))) {
			CFileLocker * locker = new CFileLocker(filelist->at(i)) ;
			if (locker->lockFile()) 
				locklist->push_back(locker) ;

		}

	delete filelist ;
	
}

// �������������� �����
void UnlockLockedFiles() {
	for (int i=0; i<locklist->size(); i++)
		locklist->at(i)->unlockFile() ;
}

// ������������� ���� �������
void LockConfigFile() {
	conflocker = new CFileLocker(CONFIGFILE) ;
	if (!conflocker->lockFile()) {
		delete conflocker ;
		conflocker = NULL ;
	}
}

// �������������� ���� �������
void UnlockConfigFile() {
	if (conflocker!=NULL) {
		conflocker->unlockFile() ;
		delete conflocker ;
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	
	if (!isMatchMask("xyz","xyz")) printf("err1\n") ;
	if ( isMatchMask("ayz","xyz")) printf("err2\n") ;
	if ( isMatchMask("ayz","xyb")) printf("err3\n") ;
	if ( isMatchMask("xy","xyz1")) printf("err4\n") ;
	if (!isMatchMask("xyz","xy?")) printf("err5\n") ;
	if ( isMatchMask("xyz","xy?1")) printf("err6\n") ;

	// ��������� �� ������ ����� ������
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	LoadMasks() ;

	// ��������� ���������� ��� ��������� ������
	LockExistFiles() ;
	LockConfigFile() ;

	// ������ ������������
	CreateThread(NULL,0,ThreadProc,NULL,0,0) ;

	// ����������� ���� ����� ������
	while (true) {
		char msg[255]="Enter password for exit prog:" ;
		DWORD cnt ;
		char buf[255] ;
		WriteConsoleA(hStdout, msg, strlen(msg), &cnt, NULL);
		ReadConsoleA(hStdin, buf, 255, &cnt, NULL) ;
		buf[cnt-2]=0 ;
		// ���� ������ ������
		if (!strcmp(buf,password)) {
			strcpy(msg,"Exit ok\n") ;
			WriteConsoleA(hStdout,msg,strlen(msg),&cnt,NULL) ;
			// ��������� ��������
			FindCloseChangeNotification(hDir);
			hDir=NULL ;
			// ������ ����������	
			UnlockLockedFiles() ;
			UnlockConfigFile() ;
			return 0 ;
		}
		else {
			strcpy(msg,"Password incorrect!\n") ;
			WriteConsoleA(hStdout,msg,strlen(msg),&cnt,NULL) ;
		}
	}

	// ������ ���������� � ��������� ������ (������� ��� ����)
	UnlockLockedFiles() ;
	UnlockConfigFile() ;
	return 0;
}

