#pragma once

#if CONFIG_DEBUG

    #if _MSC_VER
        #define TRAP __debugbreak()
    #elif __GNUC__      // This includes clang
        #define TRAP ({ __asm__ ("int $3"); })
    #else               // cross platform implementation
        #define TRAP *(int*)3 = 3
    #endif

    #define ASSERT( expr, msg, ... )                                                                     \
        ((void)( !(expr)                                                                                 \
                && ( Core::AssertHandler( __FILE__, __LINE__, __FUNCTION__, #expr, msg, ##__VA_ARGS__ ) \
                    && (TRAP, 0) ) ))

#else // CONFIG_DEBUG

    #define TRAP
    #define ASSERT( expr, msg, ...)

#endif // CONFIG_DEBUG

#undef INLINE
#if _MSC_VER
    #define INLINE __forceinline
    #if __cplusplus > 201703L || _MSVC_LANG > 201703L
        #define INLINE_LAMBDA [[msvc::forceinline]]
    #else
        #define INLINE_LAMBDA 
    #endif
#else
    #define INLINE inline __attribute__((always_inline))
    #define INLINE_LAMBDA __attribute__((always_inline))
#endif


#define SIZEOF(s) ((sz)sizeof(s))
#define OFFSETOF(type, member) (sz)( (uintptr_t)&(((type *)0)->member) )
#define ARRAYCOUNT(array) (sz)( sizeof(array) / sizeof((array)[0]) )

#define COPY(source, dest) memcpy( &dest, &source, sizeof(dest) )
#define SET(dest, value) memset( &dest, value, sizeof(dest) )
#define ZERO(dest) memset( &dest, 0, sizeof(dest) )
#define EQUAL(source, dest) (memcmp( &source, &dest, sizeof(source) ) == 0)

#define COPYP(source, dest, size) memcpy( dest, source, (size_t)( size ) )
#define SETP(dest, value, size) memset( dest, value, (size_t)( size ) )
#define ZEROP(dest, size) memset( dest, 0, (size_t)( size ) )
#define EQUALP(source, dest, size) (memcmp( source, dest, (size_t)( size ) ) == 0)


#define Log( msg, ... ) \
    printf( msg "\n", ##__VA_ARGS__ );


typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef int64_t  sz;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float    f32;
typedef double   f64;


#define I8MIN INT8_MIN
#define I8MAX INT8_MAX
#define U8MAX UINT8_MAX
#define I16MAX INT16_MAX
#define I16MIN INT16_MIN
#define I32MAX INT32_MAX
#define I32MIN INT32_MIN
#define U16MAX UINT16_MAX
#define U32MAX UINT32_MAX
#define U64MAX UINT64_MAX // ULLONG_MAX?
#define I64MAX INT64_MAX
#define I64MIN INT64_MIN

#define F32MAX FLT_MAX
#define F32MIN FLT_MIN
#define F32INF INFINITY
#define F64MAX DBL_MAX
#define F64MIN DBL_MIN
#define F64INF (f64)INFINITY




namespace Core
{
    // Return true to break into the code if a debugger is attached, false to continue execution
    bool AssertHandler( const char* file, unsigned int line, const char* fn, const char* expr, const char* msg, ... );
}


/////     BUFFER VIEW    /////

template <typename T = u8>
struct Buffer
{
    T* data;
    sz length;

public:
    Buffer()
        : data( nullptr )
        , length( 0 )
    {}

    template <typename SrcT>
    explicit Buffer( SrcT* data_, i64 length_ )
        : data( data_ )
        , length( length_ )
    {}

    // FIXME enable_if T is same size
    template <typename SrcT>
    explicit Buffer( Buffer<SrcT> const& other )
        : data( (T*)other.data )
        , length( other.length )
    {}

    // Convert from any static array of a compatible type
    // NOTE The array itself must be an lvalue, since by definition Buffers only reference data
    template <size_t N>
    Buffer( T (&data_)[N] )
        : data( data_ )
        , length( (sz)N )
    {}


    explicit INLINE operator T const*() const { return data; }
    explicit INLINE operator T*() { return data; }

    INLINE T& operator []( sz offset ) { ASSERT( offset < length, "Index out of bounds" ); return *(data + offset); }
    INLINE T const& operator []( sz offset ) const { ASSERT( offset < length, "Index out of bounds" ); return *(data + offset); }

    INLINE explicit operator bool() const
    {
        return data != nullptr && length != 0;
    }

    INLINE T*          begin()         { return data; }
    INLINE const T*    begin() const   { return data; }
    INLINE T*          end()           { return data + length; }
    INLINE const T*    end() const     { return data + length; }

    INLINE sz          Size() const    { return length * SIZEOF(T); }
    INLINE bool        Valid() const   { return data && length; }


    // Return how many items were actually copied
    INLINE sz CopyTo( T* buffer, sz itemCount, sz startOffset = 0 ) const
    {
        sz itemsToCopy = Min( itemCount, length - startOffset );
        COPYP( data + startOffset, buffer, itemsToCopy * SIZEOF(T) );

        return itemsToCopy;
    }

    // Return how many items were actually copied
    INLINE sz CopyFrom( T* buffer, sz itemCount, sz startOffset )
    {
        sz itemsToCopy = Min( itemCount, length - startOffset );
        COPYP( buffer, data + startOffset, itemsToCopy * SIZEOF(T) );

        return itemsToCopy;
    }
};

