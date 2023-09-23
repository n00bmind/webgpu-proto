
inline char const* StringFindSuffix( const char* str, const char* find, int len = 0 )
{
    if( !str || !find )
        return nullptr;

    size_t strLen = len ? len : strlen( str );
    size_t sufLen = strlen( find );
    if( sufLen >  strLen )
        return nullptr;

    if( strncmp( str + strLen - sufLen, find, sufLen ) == 0 )
    {
        // Found it
        return str + strLen - sufLen;
    }

    // Fail
    return nullptr;
}

INLINE bool StringEndsWith( char const* str, char const* find, int len = 0 )
{
    return StringFindSuffix( str, find, len ) != nullptr;
}

const u32	PRNG_A = 48828125, 	 //5^11
      PRNG_B = 244355009,
      PRNG_M = 1073741824, //2^30
      PRNG_M_MASK = 1073741823; //2^30 - 1
const float	PRNG_INV_DENOM= 1.0 / (double)(PRNG_M_MASK >> 16);

class RandomStream
//linear congruence random number generator
{
    private:
        mutable u32 r0;
        static constexpr u32 s_maxInt = (1u << 14) - 1u;

    public:
        RandomStream( u32 seed = 42 /* HDD TODO this encourages bad behaviour, seeds should be conciously chosen and passed */ ) { SetSeed( seed ); }

        // seed this RNG by hashing the given string stream
        explicit RandomStream( const char* string_seed );

        void SetSeed( u32 seed ) { r0 = seed; }

        u32& GetSeed() {return r0;}

    private:
        // for internal use only, so all calls are bounds checked

        //return an int in range 0..16383 drops 16 lowest bits for better randomness
        inline u32 GetIntInternal() const 			{ r0 = (PRNG_A * r0 + PRNG_B) & PRNG_M_MASK; return r0 >> 16; }

    public:
        // return an int in range 0..2^30
        inline u64 GetBigInt() const		{ r0 = (PRNG_A * r0 + PRNG_B) & PRNG_M_MASK; return r0 ; }

        //return a float in 0..1
        inline float 	GetUnitFloat() const	{ return (float)GetIntInternal() * (float)PRNG_INV_DENOM; }
        inline double 	GetUnitDouble() const 	{ return (double)GetIntInternal() * PRNG_INV_DENOM; }
        inline bool 	GetBool() const			{ return GetUnitFloat() >= 0.5f; }

        //return a ranged integer in [min, max) i.e. not inclusive of max
        // this is useful for e.g. random selection from a list
        inline int GetInt( int min = 0, int max = s_maxInt ) const
        {
            ASSERT( max - min <= (int)s_maxInt, "Range too big, use GetBigInt()" );
            return (((int)GetIntInternal() * (max - min)) >> 14) + min;
        }

        // ranged integer in [min, max_inclusive] 
        inline int GetIntInclusive( int min, int max_inclusive ) const
        {
            return GetInt( min, max_inclusive + 1 );
        }

        inline u32 GetUintInclusive( u32 min, u32 max_inclusive) const
        {
            return static_cast<u32>( GetInt( min, max_inclusive + 1 ) );
        }

        //return a ranged float
        inline float GetFloat( float min, float max ) const { return GetUnitFloat() * (max - min) + min; }
};
