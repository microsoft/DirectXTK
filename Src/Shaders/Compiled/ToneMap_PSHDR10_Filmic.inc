#if 0
//
// Generated by Microsoft (R) D3D Shader Disassembler
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_Position              0   xyzw        0      POS   float       
// TEXCOORD                 0   xy          1     NONE   float   xy  
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_Target                0   xyzw        0   TARGET   float   xyzw
// SV_Target                1   xyzw        1   TARGET   float   xyzw
//
ps_4_0
dcl_constantbuffer CB0[1], immediateIndexed
dcl_sampler s0, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_input_ps linear v1.xy
dcl_output o0.xyzw
dcl_output o1.xyzw
dcl_temps 3
sample r0.xyzw, v1.xyxx, t0.xyzw, s0
dp3 r1.x, l(0.627404, 0.329282, 0.043314, 0.000000), r0.xyzx
dp3 r1.y, l(0.069097, 0.919540, 0.011361, 0.000000), r0.xyzx
dp3 r1.z, l(0.016392, 0.088013, 0.895595, 0.000000), r0.xyzx
mul r1.xyz, r1.xyzx, cb0[0].xxxx
mul r1.xyz, r1.xyzx, l(0.000100, 0.000100, 0.000100, 0.000000)
log r1.xyz, |r1.xyzx|
mul r1.xyz, r1.xyzx, l(0.159302, 0.159302, 0.159302, 0.000000)
exp r1.xyz, r1.xyzx
mad r2.xyz, r1.xyzx, l(18.851563, 18.851563, 18.851563, 0.000000), l(0.835938, 0.835938, 0.835938, 0.000000)
mad r1.xyz, r1.xyzx, l(18.687500, 18.687500, 18.687500, 0.000000), l(1.000000, 1.000000, 1.000000, 0.000000)
div r1.xyz, r2.xyzx, r1.xyzx
log r1.xyz, r1.xyzx
mul r1.xyz, r1.xyzx, l(78.843750, 78.843750, 78.843750, 0.000000)
exp o0.xyz, r1.xyzx
add r0.xyz, r0.xyzx, l(-0.004000, -0.004000, -0.004000, 0.000000)
max r0.xyz, r0.xyzx, l(0.000000, 0.000000, 0.000000, 0.000000)
mov o0.w, r0.w
mov o1.w, r0.w
mad r1.xyz, r0.xyzx, l(6.200000, 6.200000, 6.200000, 0.000000), l(0.500000, 0.500000, 0.500000, 0.000000)
mul r1.xyz, r0.xyzx, r1.xyzx
mad r2.xyz, r0.xyzx, l(6.200000, 6.200000, 6.200000, 0.000000), l(1.700000, 1.700000, 1.700000, 0.000000)
mad r0.xyz, r0.xyzx, r2.xyzx, l(0.060000, 0.060000, 0.060000, 0.000000)
div r0.xyz, r1.xyzx, r0.xyzx
log r0.xyz, r0.xyzx
mul r0.xyz, r0.xyzx, l(2.200000, 2.200000, 2.200000, 0.000000)
exp o1.xyz, r0.xyzx
ret 
// Approximately 0 instruction slots used
#endif

const BYTE ToneMap_PSHDR10_Filmic[] =
{
     68,  88,  66,  67,  26, 225, 
    198, 194, 217, 102,  97,  33, 
    171, 235,  95, 152, 119,  13, 
     45, 181,   1,   0,   0,   0, 
      0,   5,   0,   0,   3,   0, 
      0,   0,  44,   0,   0,   0, 
    132,   0,   0,   0, 208,   0, 
      0,   0,  73,  83,  71,  78, 
     80,   0,   0,   0,   2,   0, 
      0,   0,   8,   0,   0,   0, 
     56,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   0,   0,   0, 
     68,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,   3,   3,   0,   0, 
     83,  86,  95,  80, 111, 115, 
    105, 116, 105, 111, 110,   0, 
     84,  69,  88,  67,  79,  79, 
     82,  68,   0, 171, 171, 171, 
     79,  83,  71,  78,  68,   0, 
      0,   0,   2,   0,   0,   0, 
      8,   0,   0,   0,  56,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   0,   0,   0,  56,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   1,   0,   0,   0, 
     15,   0,   0,   0,  83,  86, 
     95,  84,  97, 114, 103, 101, 
    116,   0, 171, 171,  83,  72, 
     68,  82,  40,   4,   0,   0, 
     64,   0,   0,   0,  10,   1, 
      0,   0,  89,   0,   0,   4, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   0,   0,   0,   0, 
     85,  85,   0,   0,  98,  16, 
      0,   3,  50,  16,  16,   0, 
      1,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      0,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      1,   0,   0,   0, 104,   0, 
      0,   2,   3,   0,   0,   0, 
     69,   0,   0,   9, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  16,  16,   0,   1,   0, 
      0,   0,  70, 126,  16,   0, 
      0,   0,   0,   0,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     16,   0,   0,  10,  18,   0, 
     16,   0,   1,   0,   0,   0, 
      2,  64,   0,   0, 140, 157, 
     32,  63, 166, 151, 168,  62, 
    154, 105,  49,  61,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      0,   0,   0,   0,  16,   0, 
      0,  10,  34,   0,  16,   0, 
      1,   0,   0,   0,   2,  64, 
      0,   0, 186, 130, 141,  61, 
    249, 102, 107,  63,  84,  36, 
     58,  60,   0,   0,   0,   0, 
     70,   2,  16,   0,   0,   0, 
      0,   0,  16,   0,   0,  10, 
     66,   0,  16,   0,   1,   0, 
      0,   0,   2,  64,   0,   0, 
    173,  71, 134,  60,  68,  64, 
    180,  61, 183,  69, 101,  63, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     56,   0,   0,   8, 114,   0, 
     16,   0,   1,   0,   0,   0, 
     70,   2,  16,   0,   1,   0, 
      0,   0,   6, 128,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  56,   0,   0,  10, 
    114,   0,  16,   0,   1,   0, 
      0,   0,  70,   2,  16,   0, 
      1,   0,   0,   0,   2,  64, 
      0,   0,  23, 183, 209,  56, 
     23, 183, 209,  56,  23, 183, 
    209,  56,   0,   0,   0,   0, 
     47,   0,   0,   6, 114,   0, 
     16,   0,   1,   0,   0,   0, 
     70,   2,  16, 128, 129,   0, 
      0,   0,   1,   0,   0,   0, 
     56,   0,   0,  10, 114,   0, 
     16,   0,   1,   0,   0,   0, 
     70,   2,  16,   0,   1,   0, 
      0,   0,   2,  64,   0,   0, 
      0,  32,  35,  62,   0,  32, 
     35,  62,   0,  32,  35,  62, 
      0,   0,   0,   0,  25,   0, 
      0,   5, 114,   0,  16,   0, 
      1,   0,   0,   0,  70,   2, 
     16,   0,   1,   0,   0,   0, 
     50,   0,   0,  15, 114,   0, 
     16,   0,   2,   0,   0,   0, 
     70,   2,  16,   0,   1,   0, 
      0,   0,   2,  64,   0,   0, 
      0, 208, 150,  65,   0, 208, 
    150,  65,   0, 208, 150,  65, 
      0,   0,   0,   0,   2,  64, 
      0,   0,   0,   0,  86,  63, 
      0,   0,  86,  63,   0,   0, 
     86,  63,   0,   0,   0,   0, 
     50,   0,   0,  15, 114,   0, 
     16,   0,   1,   0,   0,   0, 
     70,   2,  16,   0,   1,   0, 
      0,   0,   2,  64,   0,   0, 
      0, 128, 149,  65,   0, 128, 
    149,  65,   0, 128, 149,  65, 
      0,   0,   0,   0,   2,  64, 
      0,   0,   0,   0, 128,  63, 
      0,   0, 128,  63,   0,   0, 
    128,  63,   0,   0,   0,   0, 
     14,   0,   0,   7, 114,   0, 
     16,   0,   1,   0,   0,   0, 
     70,   2,  16,   0,   2,   0, 
      0,   0,  70,   2,  16,   0, 
      1,   0,   0,   0,  47,   0, 
      0,   5, 114,   0,  16,   0, 
      1,   0,   0,   0,  70,   2, 
     16,   0,   1,   0,   0,   0, 
     56,   0,   0,  10, 114,   0, 
     16,   0,   1,   0,   0,   0, 
     70,   2,  16,   0,   1,   0, 
      0,   0,   2,  64,   0,   0, 
      0, 176, 157,  66,   0, 176, 
    157,  66,   0, 176, 157,  66, 
      0,   0,   0,   0,  25,   0, 
      0,   5, 114,  32,  16,   0, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   1,   0,   0,   0, 
      0,   0,   0,  10, 114,   0, 
     16,   0,   0,   0,   0,   0, 
     70,   2,  16,   0,   0,   0, 
      0,   0,   2,  64,   0,   0, 
    111,  18, 131, 187, 111,  18, 
    131, 187, 111,  18, 131, 187, 
      0,   0,   0,   0,  52,   0, 
      0,  10, 114,   0,  16,   0, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  54,   0,   0,   5, 
    130,  32,  16,   0,   0,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   5, 130,  32,  16,   0, 
      1,   0,   0,   0,  58,   0, 
     16,   0,   0,   0,   0,   0, 
     50,   0,   0,  15, 114,   0, 
     16,   0,   1,   0,   0,   0, 
     70,   2,  16,   0,   0,   0, 
      0,   0,   2,  64,   0,   0, 
    102, 102, 198,  64, 102, 102, 
    198,  64, 102, 102, 198,  64, 
      0,   0,   0,   0,   2,  64, 
      0,   0,   0,   0,   0,  63, 
      0,   0,   0,  63,   0,   0, 
      0,  63,   0,   0,   0,   0, 
     56,   0,   0,   7, 114,   0, 
     16,   0,   1,   0,   0,   0, 
     70,   2,  16,   0,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      1,   0,   0,   0,  50,   0, 
      0,  15, 114,   0,  16,   0, 
      2,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
      2,  64,   0,   0, 102, 102, 
    198,  64, 102, 102, 198,  64, 
    102, 102, 198,  64,   0,   0, 
      0,   0,   2,  64,   0,   0, 
    154, 153, 217,  63, 154, 153, 
    217,  63, 154, 153, 217,  63, 
      0,   0,   0,   0,  50,   0, 
      0,  12, 114,   0,  16,   0, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     70,   2,  16,   0,   2,   0, 
      0,   0,   2,  64,   0,   0, 
    143, 194, 117,  61, 143, 194, 
    117,  61, 143, 194, 117,  61, 
      0,   0,   0,   0,  14,   0, 
      0,   7, 114,   0,  16,   0, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   1,   0,   0,   0, 
     70,   2,  16,   0,   0,   0, 
      0,   0,  47,   0,   0,   5, 
    114,   0,  16,   0,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      0,   0,   0,   0,  56,   0, 
      0,  10, 114,   0,  16,   0, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
      2,  64,   0,   0, 205, 204, 
     12,  64, 205, 204,  12,  64, 
    205, 204,  12,  64,   0,   0, 
      0,   0,  25,   0,   0,   5, 
    114,  32,  16,   0,   1,   0, 
      0,   0,  70,   2,  16,   0, 
      0,   0,   0,   0,  62,   0, 
      0,   1
};