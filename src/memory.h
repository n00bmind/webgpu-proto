#pragma once

// Free memory functions, passing any kind of allocator implementation as the first param
#define ALLOC(allocator, size, ...)                 Alloc( allocator, size, __FILE__, __LINE__, ##__VA_ARGS__ )
#define ALLOC_STRUCT(allocator, type, ...)          (type *)Alloc( allocator, SIZEOF(type), __FILE__, __LINE__, ##__VA_ARGS__ )
#define ALLOC_ARRAY(allocator, type, count, ...)    (type *)Alloc( allocator, (count)*SIZEOF(type), __FILE__, __LINE__, ##__VA_ARGS__ )
#define FREE(allocator, mem, ...)                   Free( allocator, (void*)(mem), ##__VA_ARGS__ )

#undef DELETE
#define NEW(allocator, type, ...)                   new ( ALLOC( allocator, sizeof(type), ##__VA_ARGS__ ) ) type
#define DELETE(allocator, p, T, ...)                { if( p ) { p->~T(); FREE( allocator, p, ##__VA_ARGS__ ); p = nullptr; } }


namespace Memory
{
    enum Tags : u8
    {
        Unknown = 0,
    };

    enum Flags : u8
    {
        None = 0,
        MF_Clear = 0x1,              // Zero memory upon allocation
    };

    struct Params
    {
        u8 flags;
        u8 tag;
        u16 alignment;

        Params( u8 flags = 0 )
            : flags( flags )
            , tag( Unknown )
            , alignment( 0 )
        {}

        bool IsSet( Flags flag ) const { return (flags & flag) == flag; }
    };

    INLINE Params Default() { return Params(); }
    INLINE Params NoClear() { return Params( MF_Clear ); }
    INLINE Params Aligned( u16 alignment )
    {
        Params result;
        result.alignment = alignment;
        return result;
    }
}

using MemoryParams = Memory::Params;


#define ALLOC_FUNC(cls) void* Alloc( cls* data, sz sizeBytes, char const* filename, int line, MemoryParams params = {} )
#define ALLOC_METHOD void* Alloc( sz sizeBytes, char const* filename, int line, MemoryParams params = {} )
typedef void* (*AllocFunc)( void* impl, sz sizeBytes, char const* filename, int line, MemoryParams params );
#define FREE_FUNC(cls)  void Free( cls* data, void* memoryBlock, MemoryParams params = {} )
#define FREE_METHOD  void Free( void* memoryBlock, MemoryParams params = {} )
typedef void (*FreeFunc)( void* impl, void* memoryBlock, MemoryParams params );


// This guy casts an opaque data pointer to the appropriate type
// and relies on overloading to call the correct pair of Alloc & Free functions accepting that as a first argument
template <typename Class>
struct AllocatorImpl
{
    static INLINE ALLOC_FUNC( void )
    {
        Class* obj = (Class*)data;
        return ::Alloc( obj, sizeBytes, filename, line, params );
    }

    static INLINE FREE_FUNC( void )
    {
        Class* obj = (Class*)data;
        ::Free( obj, memoryBlock, params );
    }
};
// This guy is just a generic non-templated wrapper to any kind of allocator whatsoever
// and two function pointers to its corresponding pair of free Alloc & Free functions
struct Allocator
{
    Allocator()
        : allocPtr( nullptr )
        , freePtr( nullptr )
        , impl( nullptr )
    {}

    template <typename Class>
    Allocator( Class* obj )
        : allocPtr( &AllocatorImpl<Class>::Alloc )
        , freePtr( &AllocatorImpl<Class>::Free )
        , impl( obj )
    {}

    // Pass-through for abstract allocators
    template <>
    Allocator( Allocator* obj )
        : allocPtr( obj->allocPtr )
        , freePtr( obj->freePtr )
        , impl( obj->impl )
    {}

    template <typename Class>
    static INLINE Allocator CreateFrom( Class* obj )
    {
        Allocator result( obj );
        return result;
    }

    INLINE ALLOC_METHOD
    {
        return allocPtr( impl, sizeBytes, filename, line, params );
    }

    INLINE FREE_METHOD
    {
        freePtr( impl, memoryBlock, params );
    }

    friend void* Alloc( Allocator* data, sz sizeBytes, char const* filename, int line, MemoryParams params );
    friend void Free( Allocator* data, void* memoryBlock, MemoryParams params );

private:
    AllocFunc allocPtr;
    FreeFunc freePtr;
    void* impl;
};

INLINE ALLOC_FUNC( Allocator )
{
    return data->allocPtr( data->impl, sizeBytes, filename, line, params );
}

INLINE FREE_FUNC( Allocator )
{
    data->freePtr( data->impl, memoryBlock, params );
}


///// DEFAULT MALLOC ALLOCATOR
struct DefaultAllocator
{
};

ALLOC_FUNC( DefaultAllocator )
{
    ASSERT( !params.alignment, "Alignment not supported" );
    
    void* result = malloc( (size_t)sizeBytes );

    if( params.IsSet( Memory::MF_Clear ) )
        ZEROP( result, sizeBytes );

    return result;
}

FREE_FUNC( DefaultAllocator )
{
    free( memoryBlock );
}

inline DefaultAllocator globalDefaultAlloc;
inline Allocator globalAlloc( &globalDefaultAlloc );

