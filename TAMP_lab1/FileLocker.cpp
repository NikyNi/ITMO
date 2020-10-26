#include "stdafx.h"
#include "FileLocker.h"
#include <Windows.h>
#include <Winbase.h>
#include <Aclapi.h>
#include <conio.h>

// Создание строки UTF из ANSI - воспомогательная функция
LPWSTR makeWide(char * str) {
	
	wchar_t * wtext = new wchar_t[strlen(str)+1];
	MultiByteToWideChar(1251,0,str,strlen(str)+1,wtext,strlen(str)+1) ;
	LPWSTR ptr = wtext;
	return ptr ;
}

// Создание объекта блокировки с указанным файлом
CFileLocker::CFileLocker(char * Afilename) {
	strcpy(filename,Afilename) ;
	pOldDACL = NULL ;
	pNewDACL = NULL ;
	pSD = NULL ;
}

// Процедура блокировки файла
bool CFileLocker::lockFile() {
	DWORD dwRes = 0;
    PSECURITY_DESCRIPTOR pSD = NULL;
    EXPLICIT_ACCESS ea;

	PSID pEveryoneSID = NULL ;
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
	
	// Получаем SID системной группы Все
    if(!AllocateAndInitializeSid(&SIDAuthWorld, 1,
                     SECURITY_WORLD_RID,
                     0, 0, 0, 0, 0, 0, 0,
                     &pEveryoneSID))
    return false ;

	// Получаем существующие права доступа для файла 
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
    
	// Инициализируем структуру для нового элемента
    //    списка контроля доступа
    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
	ea.grfAccessPermissions = FILE_ALL_ACCESS; // только чтение
	ea.grfAccessMode = DENY_ACCESS;
    ea.grfInheritance= CONTAINER_INHERIT_ACE;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID; // указывает тип элемента ptstrName
	ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea.Trustee.ptstrName = (LPTSTR) pEveryoneSID;  // для кого устанавливается доступ ( здесь можно указать имя или SID)
    //    создаём новый список контроля доступа, в который будет входить новый элемент
    //    с установленными разрешениями.
    dwRes = SetEntriesInAcl(1, &ea, NULL, &pNewDACL);
    if (ERROR_SUCCESS != dwRes) return false ;
    
	//    Соединяем новый список с существующим списком объекта
    dwRes = SetNamedSecurityInfo(makeWide(filename), SE_FILE_OBJECT, 
          DACL_SECURITY_INFORMATION,
          NULL, NULL, pNewDACL, NULL);
    if (ERROR_SUCCESS != dwRes)  return false ;
	    
	return true ;
}

// Процедура разблокировки файла
bool CFileLocker::unlockFile() {
	// Устаанавливаем ранее сохраненные права для файла
	DWORD dwRes = SetNamedSecurityInfo(makeWide(filename), SE_FILE_OBJECT, 
          DACL_SECURITY_INFORMATION,
          NULL, NULL, pOldDACL, NULL);
    if (ERROR_SUCCESS != dwRes)  return false ;

	// И удаляем все объекты
	if(pSD != NULL) 
        LocalFree((HLOCAL) pSD); 
    if(pNewDACL != NULL) 
        LocalFree((HLOCAL) pNewDACL); 

	return true ;
}