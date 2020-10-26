#include "stdafx.h"
#include "FileLocker.h"
#include <Windows.h>
#include <Winbase.h>
#include <Aclapi.h>
#include <conio.h>

// �������� ������ UTF �� ANSI - ���������������� �������
LPWSTR makeWide(char * str) {
	
	wchar_t * wtext = new wchar_t[strlen(str)+1];
	MultiByteToWideChar(1251,0,str,strlen(str)+1,wtext,strlen(str)+1) ;
	LPWSTR ptr = wtext;
	return ptr ;
}

// �������� ������� ���������� � ��������� ������
CFileLocker::CFileLocker(char * Afilename) {
	strcpy(filename,Afilename) ;
	pOldDACL = NULL ;
	pNewDACL = NULL ;
	pSD = NULL ;
}

// ��������� ���������� �����
bool CFileLocker::lockFile() {
	DWORD dwRes = 0;
    PSECURITY_DESCRIPTOR pSD = NULL;
    EXPLICIT_ACCESS ea;

	PSID pEveryoneSID = NULL ;
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
	
	// �������� SID ��������� ������ ���
    if(!AllocateAndInitializeSid(&SIDAuthWorld, 1,
                     SECURITY_WORLD_RID,
                     0, 0, 0, 0, 0, 0, 0,
                     &pEveryoneSID))
    return false ;

	// �������� ������������ ����� ������� ��� ����� 
	  dwRes = GetNamedSecurityInfo(
        makeWide(filename),
        SE_FILE_OBJECT,     
          DACL_SECURITY_INFORMATION,
          NULL, 
          NULL, 
          &pOldDACL, 
          NULL, 
          &pSD);
    if (ERROR_SUCCESS != dwRes) return false ;
    
	// �������������� ��������� ��� ������ ��������
    //    ������ �������� �������
    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
	ea.grfAccessPermissions = FILE_ALL_ACCESS; // ������ ������
	ea.grfAccessMode = DENY_ACCESS;
    ea.grfInheritance= CONTAINER_INHERIT_ACE;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID; // ��������� ��� �������� ptstrName
	ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea.Trustee.ptstrName = (LPTSTR) pEveryoneSID;  // ��� ���� ��������������� ������ ( ����� ����� ������� ��� ��� SID)
    //    ������ ����� ������ �������� �������, � ������� ����� ������� ����� �������
    //    � �������������� ������������.
    dwRes = SetEntriesInAcl(1, &ea, NULL, &pNewDACL);
    if (ERROR_SUCCESS != dwRes) return false ;
    
	//    ��������� ����� ������ � ������������ ������� �������
    dwRes = SetNamedSecurityInfo(makeWide(filename), SE_FILE_OBJECT, 
          DACL_SECURITY_INFORMATION,
          NULL, NULL, pNewDACL, NULL);
    if (ERROR_SUCCESS != dwRes)  return false ;
	    
	return true ;
}

// ��������� ������������� �����
bool CFileLocker::unlockFile() {
	// �������������� ����� ����������� ����� ��� �����
	DWORD dwRes = SetNamedSecurityInfo(makeWide(filename), SE_FILE_OBJECT, 
          DACL_SECURITY_INFORMATION,
          NULL, NULL, pOldDACL, NULL);
    if (ERROR_SUCCESS != dwRes)  return false ;

	// � ������� ��� �������
	if(pSD != NULL) 
        LocalFree((HLOCAL) pSD); 
    if(pNewDACL != NULL) 
        LocalFree((HLOCAL) pNewDACL); 

	return true ;
}