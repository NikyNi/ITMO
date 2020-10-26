// Task1.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <windows.h>

#include <vector> 

#include "FileLocker.h"

using namespace std ;

// Маски файлов
vector<char*> * masks ;
char password[255] ;
// Ссылка на объект-наблюдатель за каталогом
HANDLE hDir;

// Файл конфига
char * CONFIGFILE = "templates.tbl" ;

// Блокировкщик конфига
CFileLocker * conflocker ;

// Запоминаем список файлов
vector<char*> * fixedlist ;

// Список забловированных объектов
vector<CFileLocker*> * locklist ;

// Декодирование пароля
char * decode(char * str) {
	int len = strlen(str)/2 ;
	// Переводим из шестнадцатеричного представления в обычное - но используем не символы 0-9A-F,
	// а символы a,b,c,...
	char * res = new char[len+1] ;
	for (int i=0; i<len; i++) {
		res[i]=(str[2*i]-'a')*16+str[2*i+1]-'a' ;
	}
	res[len]=0 ;
	return res ;
}

// Загрузка масок
void LoadMasks() {
	masks = new vector<char*>() ;

	FILE * f = fopen(CONFIGFILE,"r") ;

	char buf[255] ;
	fgets(buf, 255, f);
	buf[strlen(buf)-1]=0 ;

	// Получаем пароль из первой строки
	strcpy(password,decode(buf)) ;

	// Считываем маски в вектор
	while (fgets(buf, 255, f)) {
		buf[strlen(buf)-1]=0 ;
		char * str = new char[strlen(buf)+1] ;
		strcpy(str,buf) ;
		masks->push_back(str) ;
	}

	fclose(f) ;

}

// Чтение файлов в вектор
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

// Фиксируем список файлов
void FixList() {
	fixedlist = new vector<char*>() ;
	loadFiles2Vector(fixedlist) ;
}

// Проверка маски на соответствие
bool isMatchMask(char * filename, char * mask) {
	// Проходим по маске и строке от начала до конца
	int pm=0 ;
	int pf=0 ; 
	// Длины
	int lm = strlen(mask) ;
	int lf = strlen(filename) ;
	// Бесконечный цикл, прерываем внутри
	while (true) {
		// Если символы не совпали, и при этом в маске не стоит знак подстановки "?" - маска не соответствует
		if ((filename[pf]!=mask[pm])&&(mask[pm]!='?')) return false ;
		pm++ ;
		pf++ ;
		// Если длины строк различаются, то маска не соответствует
		if ((pm==lm)&&(pf<lf)) return false ;
		if ((pm<lm)&&(pf==lf)) return false ;
		// Если дошли до конца обеих строк, то маска соответствует
		if ((pm==lm)&&(pf==lf)) return true ;
	}
}

// Проверка, подходит ли маска из набора к файлу
bool isMatchMasks(char * filename) {
	for (int i=0; i<masks->size(); i++) {
		if (isMatchMask(filename,masks->at(i))) return true ;
	}
	return false ;
}

// Удаление файла принудительное
void forcedDropFile(char * filename) {
	printf("drop %s\n",filename) ;

	while (true) {
		if (DeleteFileA(filename)!=0) {
			break ;
		}
		Sleep(500) ;
	}	
}

// Анализ каталога
void AnalizDir() {
	// Сначала получаем текущий список файлов
	vector<char*> * newlist = new vector<char*>() ;
	loadFiles2Vector(newlist) ;

	// Если длина списков не равна - были изменения, выполняем анализ
	if (newlist->size()>fixedlist->size()) {
		for (int i=0; i<newlist->size(); i++) {
			bool isfind=false ;
			// Ищем имя, которое есть в новом, но нет в старом списках
			for (int j=0; j<fixedlist->size(); j++) 
				if (!strcmp(fixedlist->at(j),newlist->at(i))) {
					isfind=true ;
					break ;
				}
				// Если такое нашли и оно соответствует маскам - удаляем файл принудительно
			if (!isfind) {
				if (isMatchMasks(newlist->at(i))) {
					forcedDropFile(newlist->at(i)) ;
				}				
			}
		}
	}

	delete newlist ;
	
}

// Функция потока для слежения за каталогом
DWORD WINAPI ThreadProc(LPVOID lpParameter) {

	FixList() ;
	// Бесконечный цикл потока
	while (true) {
	
		// Включаем наблюдение
	hDir=FindFirstChangeNotification(L".",
		false,FILE_NOTIFY_CHANGE_FILE_NAME);
	if (hDir!=INVALID_HANDLE_VALUE) {

		// Ждем сигнала
	while (WaitForSingleObject(hDir,-1)!=WAIT_OBJECT_0)
	{
	}
	
	// Если это изменение каталога
	if (hDir!=NULL) {
		// Обновляем список
		AnalizDir() ;
		// Закрываем слежение
		FindCloseChangeNotification(hDir);
		hDir=NULL ;
	}
	// Если было прервано вручную, то выходим из цикла
	else {
		return 0 ;
	}
	}

	}

	return 0 ;
}

// Заблокировать существующие файлы
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

// Разблокировать файлы
void UnlockLockedFiles() {
	for (int i=0; i<locklist->size(); i++)
		locklist->at(i)->unlockFile() ;
}

// Заблокировать файл конфига
void LockConfigFile() {
	conflocker = new CFileLocker(CONFIGFILE) ;
	if (!conflocker->lockFile()) {
		delete conflocker ;
		conflocker = NULL ;
	}
}

// Разблокировать файл конфига
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

	// Указатели на потоки ввода вывода
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	LoadMasks() ;

	// Начальная блокировка уже имеющихся файлов
	LockExistFiles() ;
	LockConfigFile() ;

	// Запуск отслеживания
	CreateThread(NULL,0,ThreadProc,NULL,0,0) ;

	// Бесконечный цикл ввода пароля
	while (true) {
		char msg[255]="Enter password for exit prog:" ;
		DWORD cnt ;
		char buf[255] ;
		WriteConsoleA(hStdout, msg, strlen(msg), &cnt, NULL);
		ReadConsoleA(hStdin, buf, 255, &cnt, NULL) ;
		buf[cnt-2]=0 ;
		// Если пароль верный
		if (!strcmp(buf,password)) {
			strcpy(msg,"Exit ok\n") ;
			WriteConsoleA(hStdout,msg,strlen(msg),&cnt,NULL) ;
			// Закрываем слежение
			FindCloseChangeNotification(hDir);
			hDir=NULL ;
			// Снятие блокировок	
			UnlockLockedFiles() ;
			UnlockConfigFile() ;
			return 0 ;
		}
		else {
			strcpy(msg,"Password incorrect!\n") ;
			WriteConsoleA(hStdout,msg,strlen(msg),&cnt,NULL) ;
		}
	}

	// Снятие блокировок в аварийном режиме (дублием код выше)
	UnlockLockedFiles() ;
	UnlockConfigFile() ;
	return 0;
}

