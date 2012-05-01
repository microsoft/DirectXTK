@echo off
rem THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
rem ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
rem THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
rem PARTICULAR PURPOSE.
rem
rem Copyright (c) Microsoft Corporation. All rights reserved.

setlocal
set error=0

call :CompileShader AlphaTestEffect vs VSAlphaTest
call :CompileShader AlphaTestEffect vs VSAlphaTestNoFog
call :CompileShader AlphaTestEffect vs VSAlphaTestVc
call :CompileShader AlphaTestEffect vs VSAlphaTestVcNoFog

call :CompileShader AlphaTestEffect ps PSAlphaTestLtGt
call :CompileShader AlphaTestEffect ps PSAlphaTestLtGtNoFog
call :CompileShader AlphaTestEffect ps PSAlphaTestEqNe
call :CompileShader AlphaTestEffect ps PSAlphaTestEqNeNoFog

call :CompileShader BasicEffect vs VSBasic
call :CompileShader BasicEffect vs VSBasicNoFog
call :CompileShader BasicEffect vs VSBasicVc
call :CompileShader BasicEffect vs VSBasicVcNoFog
call :CompileShader BasicEffect vs VSBasicTx
call :CompileShader BasicEffect vs VSBasicTxNoFog
call :CompileShader BasicEffect vs VSBasicTxVc
call :CompileShader BasicEffect vs VSBasicTxVcNoFog

call :CompileShader BasicEffect vs VSBasicVertexLighting
call :CompileShader BasicEffect vs VSBasicVertexLightingVc
call :CompileShader BasicEffect vs VSBasicVertexLightingTx
call :CompileShader BasicEffect vs VSBasicVertexLightingTxVc

call :CompileShader BasicEffect vs VSBasicOneLight
call :CompileShader BasicEffect vs VSBasicOneLightVc
call :CompileShader BasicEffect vs VSBasicOneLightTx
call :CompileShader BasicEffect vs VSBasicOneLightTxVc

call :CompileShader BasicEffect vs VSBasicPixelLighting
call :CompileShader BasicEffect vs VSBasicPixelLightingVc
call :CompileShader BasicEffect vs VSBasicPixelLightingTx
call :CompileShader BasicEffect vs VSBasicPixelLightingTxVc

call :CompileShader BasicEffect ps PSBasic
call :CompileShader BasicEffect ps PSBasicNoFog
call :CompileShader BasicEffect ps PSBasicTx
call :CompileShader BasicEffect ps PSBasicTxNoFog

call :CompileShader BasicEffect ps PSBasicVertexLighting
call :CompileShader BasicEffect ps PSBasicVertexLightingNoFog
call :CompileShader BasicEffect ps PSBasicVertexLightingTx
call :CompileShader BasicEffect ps PSBasicVertexLightingTxNoFog

call :CompileShader BasicEffect ps PSBasicPixelLighting
call :CompileShader BasicEffect ps PSBasicPixelLightingTx

call :CompileShader DualTextureEffect vs VSDualTexture
call :CompileShader DualTextureEffect vs VSDualTextureNoFog
call :CompileShader DualTextureEffect vs VSDualTextureVc
call :CompileShader DualTextureEffect vs VSDualTextureVcNoFog

call :CompileShader DualTextureEffect ps PSDualTexture
call :CompileShader DualTextureEffect ps PSDualTextureNoFog

call :CompileShader EnvironmentMapEffect vs VSEnvMap
call :CompileShader EnvironmentMapEffect vs VSEnvMapFresnel
call :CompileShader EnvironmentMapEffect vs VSEnvMapOneLight
call :CompileShader EnvironmentMapEffect vs VSEnvMapOneLightFresnel

call :CompileShader EnvironmentMapEffect ps PSEnvMap
call :CompileShader EnvironmentMapEffect ps PSEnvMapNoFog
call :CompileShader EnvironmentMapEffect ps PSEnvMapSpecular
call :CompileShader EnvironmentMapEffect ps PSEnvMapSpecularNoFog

call :CompileShader SkinnedEffect vs VSSkinnedVertexLightingOneBone
call :CompileShader SkinnedEffect vs VSSkinnedVertexLightingTwoBones
call :CompileShader SkinnedEffect vs VSSkinnedVertexLightingFourBones

call :CompileShader SkinnedEffect vs VSSkinnedOneLightOneBone
call :CompileShader SkinnedEffect vs VSSkinnedOneLightTwoBones
call :CompileShader SkinnedEffect vs VSSkinnedOneLightFourBones

call :CompileShader SkinnedEffect vs VSSkinnedPixelLightingOneBone
call :CompileShader SkinnedEffect vs VSSkinnedPixelLightingTwoBones
call :CompileShader SkinnedEffect vs VSSkinnedPixelLightingFourBones

call :CompileShader SkinnedEffect ps PSSkinnedVertexLighting
call :CompileShader SkinnedEffect ps PSSkinnedVertexLightingNoFog
call :CompileShader SkinnedEffect ps PSSkinnedPixelLighting

call :CompileShader SpriteEffect vs SpriteVertexShader
call :CompileShader SpriteEffect ps SpritePixelShader

echo.

if %error% == 0 (
    echo Shaders compiled ok
) else (
    echo There were shader compilation errors!
)

endlocal
exit /b

:CompileShader
set fxc=fxc /nologo %1.fx /T%2_4_0_level_9_1 /Zpc /Qstrip_reflect /Qstrip_debug /E%3 /FhCompiled\%1_%3.inc /Vn%1_%3
echo.
echo %fxc%
%fxc% || set error=1
exit /b
