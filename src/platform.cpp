namespace Platform
{
    const char* GetWorkingDirectory( char* buf, int bufLen )
    {
        GetCurrentDirectory( bufLen, buf );
        return buf;
    }

    Buffer<u8> ReadEntireFile( char const* filename, Allocator* allocator, bool nullTerminate = false )
    {
        u8* resultData = nullptr;
        sz resultLength = 0;

        HANDLE fileHandle = CreateFile( filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0 );
        if( fileHandle != INVALID_HANDLE_VALUE )
        {
            sz fileSize;
            if( GetFileSizeEx( fileHandle, (PLARGE_INTEGER)&fileSize ) )
            {
                resultLength = nullTerminate ? fileSize + 1 : fileSize;
                resultData = (u8*)ALLOC( allocator, resultLength );

                if( resultData )
                {
                    DWORD bytesRead;
                    if( ReadFile( fileHandle, resultData, (u32)fileSize, &bytesRead, 0 )
                        && (fileSize == bytesRead) )
                    {
                        // Null-terminate to help when handling text files
                        if( nullTerminate )
                            *(resultData + fileSize) = '\0';
                    }
                    else
                    {
                        Log( "Platform", "ReadFile failed for '%s'", filename );
                        resultData = nullptr;
                        resultLength = 0;
                    }
                }
                else
                {
                    Log( "Platform", "Couldn't allocate buffer for file contents" );
                }
            }
            else
            {
                Log( "Platform", "Failed querying file size for '%s'", filename );
            }

            CloseHandle( fileHandle );
        }
        else
        {
            Log( "Platform", "Failed opening file '%s' for reading", filename );
        }

        return Buffer<u8>( resultData, resultLength );
    }

    bool SetupShaderUpdateListener( char const* relDirPath, OnShaderUpdatedFunc* callback, ShaderUpdateListener* listener )
    {
        listener->dirHandle
            = CreateFile( relDirPath, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                          OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL );

        if( listener->dirHandle == INVALID_HANDLE_VALUE )
        {
            Log( "ERROR :: Could not open directory for reading (%s)", relDirPath );
            return false;
        }

        listener->overlapped = {0};
        listener->overlapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

        BOOL read = ReadDirectoryChangesW( listener->dirHandle,
                                           listener->notifyBuffer,
                                           sizeof(listener->notifyBuffer),
                                           TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE,
                                           NULL, &listener->overlapped, NULL );
        if( !read )
        {
            Log( "ERROR :: Could not start reading directory changes (%s)", relDirPath );
            return false;
        }

        listener->callback = callback;
        return true;
    }

    bool CheckShaderUpdates( ShaderUpdateListener* listener, char const* extension = ".wgsl" )
    {
        bool result = false;
        DWORD bytesReturned;

        // Return immediately if no info ready
        if( GetOverlappedResult( listener->dirHandle, &listener->overlapped, &bytesReturned, FALSE ) )
        {
            ASSERT( bytesReturned < sizeof(listener->notifyBuffer), "Notify buffer overflow" );

            u32 entryOffset = 0;
            do
            {
                FILE_NOTIFY_INFORMATION *notifyInfo
                    = (FILE_NOTIFY_INFORMATION *)(listener->notifyBuffer + entryOffset);
                if( notifyInfo->Action == FILE_ACTION_MODIFIED )
                {
                    char filename[MAX_PATH];
                    WideCharToMultiByte( CP_ACP, WC_COMPOSITECHECK,
                                         notifyInfo->FileName, int( notifyInfo->FileNameLength ),
                                         filename, sizeof(filename), NULL, NULL );
                    filename[notifyInfo->FileNameLength/2] = 0;

                    if( StringEndsWith( filename, extension ) )
                    {
                        Log( "Shader file '%s' was modified. Reloading..", filename );
                        result = listener->callback( filename );
                    }
                }

                entryOffset = notifyInfo->NextEntryOffset;
            } while( entryOffset );

            // Restart monitorization
            BOOL read = ReadDirectoryChangesW( listener->dirHandle,
                                               listener->notifyBuffer,
                                               sizeof(listener->notifyBuffer),
                                               TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE,
                                               NULL, &listener->overlapped, NULL );
            if( !read )
            {
                Log( "ERROR :: Could not re-start reading directory changes" );
            }
        }
        else
        {
            ASSERT( GetLastError() == ERROR_IO_INCOMPLETE, "GetOverlappedResult failed" );
        }

        return result;
    }


    f64 CurrentTimeMillis()
    {
        static f64 perfCounterFrequency = 0;
        if( !perfCounterFrequency )
        {
            LARGE_INTEGER perfCounterFreqMeasure;
            QueryPerformanceFrequency( &perfCounterFreqMeasure );
            perfCounterFrequency = (f64)perfCounterFreqMeasure.QuadPart;
        }

        LARGE_INTEGER counter;
        QueryPerformanceCounter( &counter );
        f64 result = (f64)counter.QuadPart / perfCounterFrequency * 1000;
        
        return result;
    }
    static f64 appStartTimeMillis = CurrentTimeMillis();

    f32 AppTimeMillis()
    {
        return (f32)(CurrentTimeMillis() - appStartTimeMillis);
    }

    f32 AppTimeSeconds()
    {
        return (f32)((CurrentTimeMillis() - appStartTimeMillis) * 0.001);
    }
}

