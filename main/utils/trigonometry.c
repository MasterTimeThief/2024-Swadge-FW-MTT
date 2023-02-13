//==============================================================================
// Includes
//==============================================================================

#include "trigonometry.h"

//==============================================================================
// Constant data
//==============================================================================

/// A table of 360 sine values in degrees, where the amplitude is between -1024 and +1024
const int16_t sin1024[360]
    = {0,     18,    36,    54,    71,    89,    107,   125,   143,   160,   178,   195,   213,   230,   248,   265,
       282,   299,   316,   333,   350,   367,   384,   400,   416,   433,   449,   465,   481,   496,   512,   527,
       543,   558,   573,   587,   602,   616,   630,   644,   658,   672,   685,   698,   711,   724,   737,   749,
       761,   773,   784,   796,   807,   818,   828,   839,   849,   859,   868,   878,   887,   896,   904,   912,
       920,   928,   935,   943,   949,   956,   962,   968,   974,   979,   984,   989,   994,   998,   1002,  1005,
       1008,  1011,  1014,  1016,  1018,  1020,  1022,  1023,  1023,  1024,  1024,  1024,  1023,  1023,  1022,  1020,
       1018,  1016,  1014,  1011,  1008,  1005,  1002,  998,   994,   989,   984,   979,   974,   968,   962,   956,
       949,   943,   935,   928,   920,   912,   904,   896,   887,   878,   868,   859,   849,   839,   828,   818,
       807,   796,   784,   773,   761,   749,   737,   724,   711,   698,   685,   672,   658,   644,   630,   616,
       602,   587,   573,   558,   543,   527,   512,   496,   481,   465,   449,   433,   416,   400,   384,   367,
       350,   333,   316,   299,   282,   265,   248,   230,   213,   195,   178,   160,   143,   125,   107,   89,
       71,    54,    36,    18,    0,     -18,   -36,   -54,   -71,   -89,   -107,  -125,  -143,  -160,  -178,  -195,
       -213,  -230,  -248,  -265,  -282,  -299,  -316,  -333,  -350,  -367,  -384,  -400,  -416,  -433,  -449,  -465,
       -481,  -496,  -512,  -527,  -543,  -558,  -573,  -587,  -602,  -616,  -630,  -644,  -658,  -672,  -685,  -698,
       -711,  -724,  -737,  -749,  -761,  -773,  -784,  -796,  -807,  -818,  -828,  -839,  -849,  -859,  -868,  -878,
       -887,  -896,  -904,  -912,  -920,  -928,  -935,  -943,  -949,  -956,  -962,  -968,  -974,  -979,  -984,  -989,
       -994,  -998,  -1002, -1005, -1008, -1011, -1014, -1016, -1018, -1020, -1022, -1023, -1023, -1024, -1024, -1024,
       -1023, -1023, -1022, -1020, -1018, -1016, -1014, -1011, -1008, -1005, -1002, -998,  -994,  -989,  -984,  -979,
       -974,  -968,  -962,  -956,  -949,  -943,  -935,  -928,  -920,  -912,  -904,  -896,  -887,  -878,  -868,  -859,
       -849,  -839,  -828,  -818,  -807,  -796,  -784,  -773,  -761,  -749,  -737,  -724,  -711,  -698,  -685,  -672,
       -658,  -644,  -630,  -616,  -602,  -587,  -573,  -558,  -543,  -527,  -512,  -496,  -481,  -465,  -449,  -433,
       -416,  -400,  -384,  -367,  -350,  -333,  -316,  -299,  -282,  -265,  -248,  -230,  -213,  -195,  -178,  -160,
       -143,  -125,  -107,  -89,   -71,   -54,   -36,   -18};

/// A table of the first 90 values of (1024 * tan(x)). Only 90 values are necessary because getTan1024() handles
/// symmetry
const uint16_t tan1024[90] = {
    0,    18,   36,   54,   72,   90,   108,  126,  144,  162,  181,  199,  218,  236,   255,   274,   294,   313,
    333,  353,  373,  393,  414,  435,  456,  477,  499,  522,  544,  568,  591,  615,   640,   665,   691,   717,
    744,  772,  800,  829,  859,  890,  922,  955,  989,  1024, 1060, 1098, 1137, 1178,  1220,  1265,  1311,  1359,
    1409, 1462, 1518, 1577, 1639, 1704, 1774, 1847, 1926, 2010, 2100, 2196, 2300, 2412,  2534,  2668,  2813,  2974,
    3152, 3349, 3571, 3822, 4107, 4435, 4818, 5268, 5807, 6465, 7286, 8340, 9743, 11704, 14644, 19539, 29324, 58665,
};

//==============================================================================
// Functions
//==============================================================================

/**
 * Integer sine function
 *
 * @param degree The degree, between 0 and 359
 * @return The sine of the degree, between -1024 and 1024
 */
int16_t getSin1024(int16_t degree)
{
    degree = ((degree % 360) + 360) % 360;
    return sin1024[degree];
}

/**
 * Integer cosine function
 *
 * @param degree The degree, between 0 and 359
 * @return The cosine of the degree, between -1024 and 1024
 */
int16_t getCos1024(int16_t degree)
{
    // cos is just sin offset by 90 degrees
    degree = ((degree % 360) + 450) % 360;
    return sin1024[degree];
}

/**
 * Integer tangent function
 *
 * @param degree The degree, between 0 and 359
 * @return The tangent of the degree, between -1024 and 1024
 */
int32_t getTan1024(int16_t degree)
{
    // Force always positive modulus math.
    degree = ((degree % 180) + 180) % 180;
    if (degree < 90)
    {
        return tan1024[degree];
    }
    else
    {
        return -tan1024[degree - 90];
    }
}
