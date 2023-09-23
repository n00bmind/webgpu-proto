
namespace Core
{
    bool AssertHandler( const char* file, unsigned int line, const char* fn, const char* expr, const char* msg, ... )
    {
        char msgbuf[1024];

        va_list args;
        va_start( args, msg );
        vsnprintf( msgbuf, sizeof(msgbuf), msg, args );
        va_end( args );

        // TODO 
        //Platform::CallstackStorage<64> callstack;
        //globalPlatform.CaptureCallstack( &callstack, 3 );

        Log( "### ASSERT FAILED! ###\n%s", msgbuf );

        //if( !s_disablePrompts && globalPlatform.ErrorHandler )
            //globalPlatform.ErrorHandler( Platform::Assert, file, line, fn, expr, msgbuf, callstack );

        return true;
    }
}
