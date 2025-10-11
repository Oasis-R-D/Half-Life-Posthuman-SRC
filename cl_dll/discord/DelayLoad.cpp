#include "PlatformHeaders.h"
#include <delayimp.h>
#include "filesystem_utils.h"

FARPROC LoadGameLibrary(const char *szDll)
{
    std::string strPath = FileSystem_GetModDirectoryName();
    strPath += "/";
    strPath += szDll;
    return reinterpret_cast<FARPROC>(LoadLibraryA(strPath.c_str()));
}

FARPROC WINAPI DelayHook(unsigned dliNotify, PDelayLoadInfo pdli)
{
    if (dliNotify == dliNotePreLoadLibrary)
    {
        if (!strcmp(pdli->szDll, "discord-rpc.dll"))
            return LoadGameLibrary("cl_dlls/discord-rpc.dll");
    }

    return nullptr;
}

extern "C"
{
const PfnDliHook __pfnDliNotifyHook2 = DelayHook;
const PfnDliHook __pfnDliFailureHook2 = nullptr;
}