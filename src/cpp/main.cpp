#include<iostream>
#include<cstdio>
#include<string>
#include<stdint.h>
#include "windows.h"

using namespace std;
enum FileBrowserType
{
    BROWSER_OPEN,
    BROWSER_SAVE,
};

OPENFILENAME m_openFile;
TCHAR m_szPath[MAX_PATH];
TCHAR m_szBuffer[MAX_PATH];

OPENFILENAME ofn;
char szFile[100];


std::string BrowserFile(char* titleName, char* defaultName, char* fileType, char* defaultPath, FileBrowserType browserType)
{
    m_szPath[0] = '\0';
    GetCurrentDirectory(MAX_PATH - 1, m_szPath);
    //strcpy_s(m_szPath, m_szPath);
    strcat_s(m_szPath, defaultPath);
    m_szBuffer[0] = '\0';
    strcpy_s(m_szBuffer, defaultName);

    ZeroMemory(&m_openFile, sizeof(m_openFile));
    m_openFile.lStructSize = sizeof (m_openFile);
    m_openFile.hwndOwner = NULL;
    m_openFile.lpstrFile = m_szBuffer;
    m_openFile.nMaxFile = MAX_PATH;
    m_openFile.lpstrFilter = fileType;
    //m_openFile.lpstrFilter = TEXT("动作配置(*.data)\0*.buffdata\0\0");
    m_openFile.nFilterIndex = 1;
    //GlobalData::m_openFile.lpstrFileTitle = NULL;
    //GlobalData::m_openFile.nMaxFileTitle = 0;
    m_openFile.lpstrTitle = titleName;
    m_openFile.lpstrInitialDir = NULL;
    m_openFile.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;


    BOOL isOpen = false;

    if (browserType == BROWSER_OPEN)
    {
        isOpen = GetOpenFileName(&m_openFile);
    }
    else
    {
        isOpen = GetSaveFileName(&m_openFile);
    }

    if (isOpen)
    {
        return m_openFile.lpstrFile;
    }

    return "";
}

int main(){
    string file_name;
    file_name = "F:\\Users\\treertzhu\\Desktop\\workgit\\actmobileclient\\AuroraGame\\res\\editor\\config\\client_action_config.data";
    //file_name = "F:\\Users\\treertzhu/Desktop/workgit/actmobileclient/AuroraGame/res/editor/config/client_action_config.data";
    //file_name = "\\res\\editor\\config\\client_action_config.data";
    //file_name = "xx/client_action_config.data";
    file_name = BrowserFile("动作配置", LPSTR(file_name.c_str()), "动作配置(*.data)\0*.data\0动作配置2(*.data2)\0*.data2\0\0", "\\res\\eitor\\", BROWSER_SAVE);
    MessageBox(NULL, file_name.c_str(), "File Name", MB_OK);
    cout << file_name << endl;
}