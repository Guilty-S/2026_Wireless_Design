#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "utilities.h"
#include "typedefs.h"
#include <time.h>
/*!
 * Redefinition of rand() and srand() standard C functions.
 * These functions are redefined in order to get the same behavior across
 * different compiler toolchains implementations.
 */
// Standard random functions redefinition start
#define RAND_LOCAL_MAX 2147483647L

static uint32_t next = 1;

int32_t rand1( void )
{

    return ( ( next = next * 1103515245L + 12345L ) % RAND_LOCAL_MAX );
}

void srand1( uint32_t seed )
{
    next = seed;
}


void memcpy1( uint8_t *dst, const uint8_t *src, uint16_t size )
{
    while( size-- )
    {
        *dst++ = *src++;
    }
}

void memcpyr( uint8_t *dst, const uint8_t *src, uint16_t size )
{
    dst = dst + ( size - 1 );
    while( size-- )
    {
        *dst-- = *src++;
    }
}


void memcat( uint8_t *dst, const uint8_t *src, uint16_t startmemory, uint16_t  size)
{
    dst = dst + startmemory;
    while( size-- )
    {
        *dst++ = *src++;
    }
}



void memset1( uint8_t *dst, uint8_t value, uint16_t size )
{
    while( size-- )
    {
        *dst++ = value;
    }
}

int8_t Nibble2HexChar( uint8_t a )
{
    if( a < 10 )
    {
        return '0' + a;
    }
    else if( a < 16 )
    {
        return 'A' + ( a - 10 );
    }
    else
    {
        return '?';
    }
}

uint16_t ntohs(uint16_t data)
{
    nt16 *p16 = (nt16 *)&data;
    nt16 result;
    result.s16.u8H = (*p16).s16.u8L;
    result.s16.u8L = (*p16).s16.u8H;
    return result.u16;
}

uint16_t htons(uint16_t data)
{
    nt16 *p16 = (nt16 *)&data;
    nt16 result;
    result.s16.u8H = (*p16).s16.u8L;
    result.s16.u8L = (*p16).s16.u8H;
    return result.u16;
}

uint32_t ntohl(uint32_t data)
{
    nt32 *p32 = (nt32 *)&data;
    nt32 result;
    result.s32.u16L = ntohs((*p32).s32.u16H);
    result.s32.u16H = ntohs((*p32).s32.u16L);
    return result.u32;
}
uint32_t htonl(uint32_t data)
{
    nt32 *p32 = (nt32 *)&data;
    nt32 result;
    result.s32.u16L = ntohs((*p32).s32.u16H);
    result.s32.u16H = ntohs((*p32).s32.u16L);
    return result.u32;
}



// 定义MT19937-32的常数
enum
{
    // 假定 W = 32 (此项省略)
    N = 624,
    M = 397,
    R = 31,
    A = 0x9908B0DF,

    F = 1812433253,

    U = 11,
    // 假定 D = 0xFFFFFFFF (此项省略)

    S = 7,
    B = 0x9D2C5680,

    T = 15,
    C = 0xEFC60000,

    L = 18,

    MASK_LOWER = (1ull << R) - 1,
    MASK_UPPER = (1ull << R)
};

static uint32_t  mt[N];
static uint16_t  index;

// 根据给定的seed初始化旋转链
void Random_Initialize(const uint32_t  seed)
{
    uint32_t  i;
    mt[0] = seed;
    for ( i = 1; i < N; i++ )
    {
        mt[i] = (F * (mt[i - 1] ^ (mt[i - 1] >> 30)) + i);
    }
    index = N;
}

static void Twist()
{
    uint32_t  i, x, xA;
    for ( i = 0; i < N; i++ )
    {
        x = (mt[i] & MASK_UPPER) + (mt[(i + 1) % N] & MASK_LOWER);
        xA = x >> 1;
        if ( x & 0x1 )
        {
            xA ^= A;
        }
        mt[i] = mt[(i + M) % N] ^ xA;
    }

    index = 0;
}

// 产生一个32位随机数
uint32_t ExtractU32()
{
    uint32_t  y;
    int       i = index;
    if ( index >= N )
    {
        Twist();
        i = index;
    }
    y = mt[i];
    index = i + 1;
    y ^= (y >> U);
    y ^= (y << S) & B;
    y ^= (y << T) & C;
    y ^= (y >> L);
    return y;
}

// Standard random functions redefinition end
uint32_t rnum;
int32_t randr( int32_t min, int32_t max )
{
    //return ( int32_t )rand1( ) % ( max - min + 1 ) + min;
    //uint32_t num = 0;
    //num = ( int32_t )rand() % (( max - min + 1 ) * 10);
    //return num /10 + min;
    int32_t result;
    //rnum = rand();
    rnum = ExtractU32();

    result = ( int32_t )((float)(rnum) * ( max - min + 1 ) / (4294967295));
    return result;
}