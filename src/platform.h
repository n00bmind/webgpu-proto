#pragma once

using OnShaderUpdatedFunc = bool( char const* );

struct ShaderUpdateListener
{
    u8 notifyBuffer[1024];
    OnShaderUpdatedFunc* callback;
    HANDLE dirHandle;
    OVERLAPPED overlapped;
};

