#include <cstdlib>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <windows.h>
#include <sys/stat.h>

#define LOGFILE "install.log"

using namespace std;

void WriteStringToLog(string strData)
{
  int Handle = 0;
  Handle = open(LOGFILE, O_APPEND);
  write(Handle, strData.c_str(), strData.size());
  write(Handle, "\r\n", 2);
  close(Handle);
}

bool GetFileVersion(char *FileName, int &Major, int &Minor)
{
  DWORD verHandle = 0;
  UINT size = 0;
  LPBYTE lpBuffer = NULL;
  DWORD verSize = GetFileVersionInfoSize( FileName, &verHandle);
    
  if(verSize != 0)
  {
    LPSTR verData = new char[verSize];    
    if(GetFileVersionInfo( FileName, verHandle, verSize, verData))
    {
      if(VerQueryValue(verData,"\\",(VOID FAR* FAR*)&lpBuffer,&size))
      {
        if(size)
        {
          VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
          if(verInfo->dwSignature == 0xfeef04bd)
          {
            int major = HIWORD(verInfo->dwFileVersionMS);
            int minor = LOWORD(verInfo->dwFileVersionMS);
            int build = verInfo->dwFileVersionLS;
            Major = major;
            Minor = minor;
            cout<<major<<" "<<minor<<" "<<build<<endl;
          }
         }
        }
      }
      delete[] verData;
    }
    return true;
}

bool PrintDirectoryList(char *Directory, string &MaxDir)
{    
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(Directory)) != NULL)
    {
      string FullPath;
      string Help;
      string RetStr;
      char buff[20];
      string MAX = "2001-12-11 12:12:12";
      cout<<"Dir: "<<Directory<<endl;
      if((ent = readdir(dir)) == NULL)
      {
        perror("");
        return false;
      }
      while((ent = readdir(dir)) != NULL)
      {
        if(strcmp(ent->d_name, ".")==0) continue;
        if(strcmp(ent->d_name, "..")==0) continue;
        if(strcmp(ent->d_name, "All Users")==0) continue;
        if(strcmp(ent->d_name, "Default User")==0) continue;
        
        FullPath = Directory;
        FullPath += ent->d_name;
        struct stat st;
        if(stat(FullPath.c_str(),&st) == -1) perror("stat\n");
        if(S_ISDIR(st.st_mode))
        {
          strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime));
          cout<<ent->d_name<<" "<<"["<<buff<<"]"<<endl;
        }
        Help = buff;
        if(MAX < Help)
        {
          MAX = Help;
          RetStr = ent->d_name;
        }
      }
      closedir(dir);
      MaxDir = RetStr;
    }
    else
    {
      perror ("");
      return EXIT_FAILURE;
    }
}

bool ChangeJavaFile(char *ProfileDir, bool IsWinXp)
{
     string JavaPath;
     char Parametr[] = "deployment.security.level=MEDIUM";
     if(IsWinXp) JavaPath = "\\Application Data\\Sun\\Java\\Deployment\\deployment.properties";
     else JavaPath = "\\AppData\\LocalLow\\Sun\\Java\\Deployment\\deployment.properties";
     JavaPath = ProfileDir + JavaPath;
     cout<<"File to config: "<<JavaPath<<endl;
     int Handle;
     Handle = open(JavaPath.c_str(), O_RDWR);
     if(Handle == -1)
     {
       cout<<"Cannot open config file"<<endl;
       return false;
     }
     
     char Buffer[2048] = {0};
     struct stat st;
     if(stat(JavaPath.c_str(), &st) == -1)
     {
       perror("");
       return false;
     }
     read(Handle, Buffer, st.st_size);
     close(Handle);
     
     string Data;
     Data = Buffer;
     cout<<"Data size: "<<Data.size()<<endl;
     if(Data.find("deployment.security.level=MEDIUM") != -1)
     {
       cout<<"No change needed. [Alredy set to MEDIUM]"<<endl;
       return true;
     }
     if(Data.find("deployment.security.level") != -1)
     {
       //string exist
       int pos;
       pos = Data.find("deployment.security.level=HIGH", 0);
       if(pos != -1)
       {  
         cout<<"HIGH"<<endl;     
         Data.erase(pos, strlen("deployment.security.level=HIGH"));
         Data.append("\r\n"); 
         Data.append(Parametr);
         open(JavaPath.c_str(), O_RDWR | O_TRUNC);
         write(Handle, Data.c_str(), Data.size());
         close(Handle);
         return true;
       }
       pos = Data.find("deployment.security.level=VERY_HIGH", 0);
       if(pos != -1)
       {
         cout<<"VERY_HIGH"<<endl;
         Data.erase(pos, strlen("deployment.security.level=VERY_HIGH"));
         Data.append("\r\n");
         Data.append(Parametr);
         open(JavaPath.c_str(), O_RDWR | O_TRUNC);
         write(Handle, Data.c_str(), Data.size());
         close(Handle);
         return true;
       }
     }
     else
     {
       //string not exist
       Data.append("deployment.security.level=MEDIUM");
       open(JavaPath.c_str(), O_RDWR | O_APPEND);
       write(Handle, "\r\n", 2);
       write(Handle, Parametr, strlen(Parametr));
       close(Handle);
     }
     
     return true;
}

bool ChangeOperation(char *IPorName, bool Auto=false)
{
    if(!IPorName) return false;
  
    //test installed java
    string MaxDir;
    int Major=0, Minor=0;
    int handle = 0;
    string FullPath;
    FullPath = "\\\\";
    FullPath += IPorName;
    FullPath += "\\c$\\Program Files\\Java\\jre7\\release";
    handle = open(FullPath.c_str(), O_RDONLY);
    if(handle == -1)
    {
      cout<<"Java path: "<<FullPath<<endl;
      perror("");
      cout<<"Java not installed!"<<endl;
      return false;
    }
    else cout<<"Java installed"<<endl;
    close(handle);
    
    char SysPath[] = "\\c$\\Windows\\System32\\ntoskrnl.exe";
    char Win2000XPProfile[] = "\\c$\\Documents and Settings\\";
    char WinVista7Priofile[] = "\\c$\\Users\\";
    
    FullPath = "\\\\";
    FullPath += IPorName;
    FullPath += SysPath;
    cout<<"Trying to localte file ntskrln.exe. Please wait..."<<endl;
    handle = open(FullPath.c_str(), O_RDONLY);
    if(handle == -1)
    {
      cout<<"Cannot find ntskrln.exe"<<endl;
      cout<<"Teriminating ..."<<endl;
      return true;
    }
    else cout<<"Found ntskrln.exe"<<endl;
    close(handle);
    cout<<"Determinating OS version ..."<<endl;
    GetFileVersion((char*)FullPath.c_str(), Major, Minor );
    if(Major == 5)
    {
        //windows xp or windows 2000 
        if(Minor == 0) cout<<"OS: Windows 2000"<<endl;
        if(Minor == 1) cout<<"OS: Windows XP"<<endl;
        
        FullPath = "\\\\";
        FullPath += IPorName;
        FullPath += Win2000XPProfile;
        if(!PrintDirectoryList((char*)FullPath.c_str(), MaxDir)) 
        {
          return true;
        }
    }
    if(Major == 6)
    {
         //windows vista or windows 7
         if(Minor == 0) cout<<"OS: Windows Vista"<<endl;
         if(Minor == 1) cout<<"OS: Windows Seven"<<endl;
         
         FullPath = "\\\\";
         FullPath += IPorName;
         FullPath += WinVista7Priofile;
         if(!PrintDirectoryList((char*)FullPath.c_str(), MaxDir))
         {
          return true;
         }
    }
    MaxDir = FullPath + MaxDir;
    cout<<"Last modified directory is: "<<MaxDir.c_str()<<endl;
    if(!Auto)
    {
      char Press;
      cout<<"You agreed(y/n)? ";
      cin>>Press;
      if(Press == 'y') ChangeJavaFile((char*)MaxDir.c_str(), (Major==6)?0:1);
    }
    else ChangeJavaFile((char*)MaxDir.c_str(), (Major==6)?0:1);
    
    return true;
}

bool ChaneOperationByFile(char *FileName)
{
    if(!FileName) return false;
    char Line[32] = {0};
    fstream ifs(FileName);
    if(ifs.is_open())
    {
        while(ifs.good())
        {
            memset(Line, 0, 32);
            ifs.getline(Line, 32);
            ChangeOperation(Line, true);
            cout<<"=========================================="<<endl;
        }
    }
    else cout<<"Cannot open file"<<endl;
    return true;
}

int main(int argc, char *argv[])
{
    if(argc <= 1)
    {
        char IPorName[14] = {0};
        cout<<"Enter PC name: ";
        cin>>IPorName;
        if(ChangeOperation(IPorName)) cout<<"Complited."<<endl;
    }
    else
    {
        if(!strcmp(argv[1],"-f") && argv[2])
        {
            cout<<"Work with file: "<<argv[2]<<endl;
            if(ChaneOperationByFile(argv[2])) cout<<"Complited."<<endl;
        }
        else
        {
            cout<<"Worng parametr"<<endl;
            cout<<argv[1]<<" "<<argv[2]<<endl;
        }
    }
    
    system("PAUSE");
    return EXIT_SUCCESS;
}
