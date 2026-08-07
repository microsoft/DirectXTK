@echo off
rem Copyright (c) Microsoft Corporation.
rem Licensed under the MIT License.

setlocal
set error=0

if %PROCESSOR_ARCHITECTURE%.==ARM64. (set FXCARCH=arm64) else (set FXCARCH=x64)

set FXCOPTS=/nologo /WX /Ges /Zi /Zpc /Qstrip_reflect /Qstrip_debug

if %1.==xbox. goto continuexbox
if %1.==. goto continuepc
echo usage: CompileShaders [xbox]
exit /b

:continuexbox
set XBOXOPTS=/D__XBOX_DISABLE_SHADER_NAME_EMPLACEMENT
if NOT %2.==noprecompile. goto skipnoprecompile
set XBOXOPTS=%XBOXOPTS% /D__XBOX_DISABLE_PRECOMPILE=1
:skipnoprecompile

set XBOXFXC="%XboxOneXDKLatest%\xdk\FXC\amd64\FXC.exe"
if exist %XBOXFXC% goto continue
set XBOXFXC="%XboxOneXDKLatest%xdk\FXC\amd64\FXC.exe"
if exist %XBOXFXC% goto continue
set XBOXFXC="%XboxOneXDKBuild%xdk\FXC\amd64\FXC.exe"
if exist %XBOXFXC% goto continue
set XBOXFXC="%DurangoXDK%xdk\FXC\amd64\FXC.exe"
if not exist %XBOXFXC% goto needxdk
goto continue

:continuepc

if defined LegacyShaderCompiler goto fxcviaenv
set PCFXC="%WindowsSdkVerBinPath%%FXCARCH%\fxc.exe"
if exist %PCFXC% goto continue
set PCFXC="%WindowsSdkBinPath%%WindowsSDKVersion%\%FXCARCH%\fxc.exe"
if exist %PCFXC% goto continue
set PCFXC="%WindowsSdkDir%bin\%WindowsSDKVersion%\%FXCARCH%\fxc.exe"
if exist %PCFXC% goto continue

set PCFXC=fxc.exe
goto continue

:fxcviaenv
set PCFXC="%LegacyShaderCompiler%"
if not exist %PCFXC% goto needfxc
goto continue

:continue
if not defined CompileShadersOutput set CompileShadersOutput=Compiled
set StrTrim=%CompileShadersOutput%##
set StrTrim=%StrTrim: ##=%
set CompileShadersOutput=%StrTrim:##=%
@if not exist "%CompileShadersOutput%" mkdir "%CompileShadersOutput%"
call :CompileShader%1 AlphaTestEffect vs VSAlphaTest
call :CompileShader%1 AlphaTestEffect vs VSAlphaTestNoFog
call :CompileShader%1 AlphaTestEffect vs VSAlphaTestVc
call :CompileShader%1 AlphaTestEffect vs VSAlphaTestVcNoFog

call :CompileShader%1 AlphaTestEffect ps PSAlphaTestLtGt
call :CompileShader%1 AlphaTestEffect ps PSAlphaTestLtGtNoFog
call :CompileShader%1 AlphaTestEffect ps PSAlphaTestEqNe
call :CompileShader%1 AlphaTestEffect ps PSAlphaTestEqNeNoFog

call :CompileShader%1 BasicEffect vs VSBasic
call :CompileShader%1 BasicEffect vs VSBasicNoFog
call :CompileShader%1 BasicEffect vs VSBasicVc
call :CompileShader%1 BasicEffect vs VSBasicVcNoFog
call :CompileShader%1 BasicEffect vs VSBasicTx
call :CompileShader%1 BasicEffect vs VSBasicTxNoFog
call :CompileShader%1 BasicEffect vs VSBasicTxVc
call :CompileShader%1 BasicEffect vs VSBasicTxVcNoFog

call :CompileShader%1 BasicEffect vs VSBasicVertexLighting
call :CompileShader%1 BasicEffect vs VSBasicVertexLightingBn
call :CompileShader%1 BasicEffect vs VSBasicVertexLightingVc
call :CompileShader%1 BasicEffect vs VSBasicVertexLightingVcBn
call :CompileShader%1 BasicEffect vs VSBasicVertexLightingTx
call :CompileShader%1 BasicEffect vs VSBasicVertexLightingTxBn
call :CompileShader%1 BasicEffect vs VSBasicVertexLightingTxVc
call :CompileShader%1 BasicEffect vs VSBasicVertexLightingTxVcBn

call :CompileShader%1 BasicEffect vs VSBasicOneLight
call :CompileShader%1 BasicEffect vs VSBasicOneLightBn
call :CompileShader%1 BasicEffect vs VSBasicOneLightVc
call :CompileShader%1 BasicEffect vs VSBasicOneLightVcBn
call :CompileShader%1 BasicEffect vs VSBasicOneLightTx
call :CompileShader%1 BasicEffect vs VSBasicOneLightTxBn
call :CompileShader%1 BasicEffect vs VSBasicOneLightTxVc
call :CompileShader%1 BasicEffect vs VSBasicOneLightTxVcBn

call :CompileShader%1 BasicEffect vs VSBasicPixelLighting
call :CompileShader%1 BasicEffect vs VSBasicPixelLightingBn
call :CompileShader%1 BasicEffect vs VSBasicPixelLightingVc
call :CompileShader%1 BasicEffect vs VSBasicPixelLightingVcBn
call :CompileShader%1 BasicEffect vs VSBasicPixelLightingTx
call :CompileShader%1 BasicEffect vs VSBasicPixelLightingTxBn
call :CompileShader%1 BasicEffect vs VSBasicPixelLightingTxVc
call :CompileShader%1 BasicEffect vs VSBasicPixelLightingTxVcBn

call :CompileShader%1 BasicEffect ps PSBasic
call :CompileShader%1 BasicEffect ps PSBasicNoFog
call :CompileShader%1 BasicEffect ps PSBasicTx
call :CompileShader%1 BasicEffect ps PSBasicTxNoFog

call :CompileShader%1 BasicEffect ps PSBasicVertexLighting
call :CompileShader%1 BasicEffect ps PSBasicVertexLightingNoFog
call :CompileShader%1 BasicEffect ps PSBasicVertexLightingTx
call :CompileShader%1 BasicEffect ps PSBasicVertexLightingTxNoFog

call :CompileShader%1 BasicEffect ps PSBasicPixelLighting
call :CompileShader%1 BasicEffect ps PSBasicPixelLightingTx

call :CompileShader%1 DualTextureEffect vs VSDualTexture
call :CompileShader%1 DualTextureEffect vs VSDualTextureNoFog
call :CompileShader%1 DualTextureEffect vs VSDualTextureVc
call :CompileShader%1 DualTextureEffect vs VSDualTextureVcNoFog

call :CompileShader%1 DualTextureEffect ps PSDualTexture
call :CompileShader%1 DualTextureEffect ps PSDualTextureNoFog

call :CompileShader%1 EnvironmentMapEffect vs VSEnvMap
call :CompileShader%1 EnvironmentMapEffect vs VSEnvMapBn
call :CompileShader%1 EnvironmentMapEffect vs VSEnvMapFresnel
call :CompileShader%1 EnvironmentMapEffect vs VSEnvMapFresnelBn
call :CompileShader%1 EnvironmentMapEffect vs VSEnvMapOneLight
call :CompileShader%1 EnvironmentMapEffect vs VSEnvMapOneLightBn
call :CompileShader%1 EnvironmentMapEffect vs VSEnvMapOneLightFresnel
call :CompileShader%1 EnvironmentMapEffect vs VSEnvMapOneLightFresnelBn
call :CompileShader%1 EnvironmentMapEffect vs VSEnvMapPixelLighting
call :CompileShader%1 EnvironmentMapEffect vs VSEnvMapPixelLightingBn

call :CompileShader%1 EnvironmentMapEffect ps PSEnvMap
call :CompileShader%1 EnvironmentMapEffect ps PSEnvMapNoFog
call :CompileShader%1 EnvironmentMapEffect ps PSEnvMapSpecular
call :CompileShader%1 EnvironmentMapEffect ps PSEnvMapSpecularNoFog
call :CompileShader%1 EnvironmentMapEffect ps PSEnvMapPixelLighting
call :CompileShader%1 EnvironmentMapEffect ps PSEnvMapPixelLightingNoFog
call :CompileShader%1 EnvironmentMapEffect ps PSEnvMapPixelLightingFresnel
call :CompileShader%1 EnvironmentMapEffect ps PSEnvMapPixelLightingFresnelNoFog

call :CompileShader%1 EnvironmentMapEffect ps PSEnvMapSpherePixelLighting
call :CompileShader%1 EnvironmentMapEffect ps PSEnvMapSpherePixelLightingNoFog
call :CompileShader%1 EnvironmentMapEffect ps PSEnvMapSpherePixelLightingFresnel
call :CompileShader%1 EnvironmentMapEffect ps PSEnvMapSpherePixelLightingFresnelNoFog

call :CompileShader%1 EnvironmentMapEffect ps PSEnvMapDualParabolaPixelLighting
call :CompileShader%1 EnvironmentMapEffect ps PSEnvMapDualParabolaPixelLightingNoFog
call :CompileShader%1 EnvironmentMapEffect ps PSEnvMapDualParabolaPixelLightingFresnel
call :CompileShader%1 EnvironmentMapEffect ps PSEnvMapDualParabolaPixelLightingFresnelNoFog

call :CompileShader%1 SkinnedEffect vs VSSkinnedVertexLightingOneBone
call :CompileShader%1 SkinnedEffect vs VSSkinnedVertexLightingOneBoneBn
call :CompileShader%1 SkinnedEffect vs VSSkinnedVertexLightingTwoBones
call :CompileShader%1 SkinnedEffect vs VSSkinnedVertexLightingTwoBonesBn
call :CompileShader%1 SkinnedEffect vs VSSkinnedVertexLightingFourBones
call :CompileShader%1 SkinnedEffect vs VSSkinnedVertexLightingFourBonesBn

call :CompileShader%1 SkinnedEffect vs VSSkinnedOneLightOneBone
call :CompileShader%1 SkinnedEffect vs VSSkinnedOneLightOneBoneBn
call :CompileShader%1 SkinnedEffect vs VSSkinnedOneLightTwoBones
call :CompileShader%1 SkinnedEffect vs VSSkinnedOneLightTwoBonesBn
call :CompileShader%1 SkinnedEffect vs VSSkinnedOneLightFourBones
call :CompileShader%1 SkinnedEffect vs VSSkinnedOneLightFourBonesBn

call :CompileShader%1 SkinnedEffect vs VSSkinnedPixelLightingOneBone
call :CompileShader%1 SkinnedEffect vs VSSkinnedPixelLightingOneBoneBn
call :CompileShader%1 SkinnedEffect vs VSSkinnedPixelLightingTwoBones
call :CompileShader%1 SkinnedEffect vs VSSkinnedPixelLightingTwoBonesBn
call :CompileShader%1 SkinnedEffect vs VSSkinnedPixelLightingFourBones
call :CompileShader%1 SkinnedEffect vs VSSkinnedPixelLightingFourBonesBn

call :CompileShader%1 SkinnedEffect ps PSSkinnedVertexLighting
call :CompileShader%1 SkinnedEffect ps PSSkinnedVertexLightingNoFog
call :CompileShader%1 SkinnedEffect ps PSSkinnedPixelLighting

call :CompileShader%1 NormalMapEffect vs VSNormalPixelLightingTx
call :CompileShader%1 NormalMapEffect vs VSNormalPixelLightingTxBn
call :CompileShader%1 NormalMapEffect vs VSNormalPixelLightingTxVc
call :CompileShader%1 NormalMapEffect vs VSNormalPixelLightingTxVcBn

call :CompileShader%1 NormalMapEffect vs VSNormalPixelLightingTxInst
call :CompileShader%1 NormalMapEffect vs VSNormalPixelLightingTxBnInst
call :CompileShader%1 NormalMapEffect vs VSNormalPixelLightingTxVcInst
call :CompileShader%1 NormalMapEffect vs VSNormalPixelLightingTxVcBnInst

call :CompileShader%1 NormalMapEffect vs VSSkinnedPixelLightingTx
call :CompileShader%1 NormalMapEffect vs VSSkinnedPixelLightingTxBn

call :CompileShader%1 NormalMapEffect ps PSNormalPixelLightingTx
call :CompileShader%1 NormalMapEffect ps PSNormalPixelLightingTxNoFog
call :CompileShader%1 NormalMapEffect ps PSNormalPixelLightingTxNoSpec
call :CompileShader%1 NormalMapEffect ps PSNormalPixelLightingTxNoFogSpec

call :CompileShader%1 PBREffect vs VSConstant
call :CompileShader%1 PBREffect vs VSConstantInst
call :CompileShader%1 PBREffect vs VSConstantVelocity
call :CompileShader%1 PBREffect vs VSConstantBn
call :CompileShader%1 PBREffect vs VSConstantBnInst
call :CompileShader%1 PBREffect vs VSConstantVelocityBn
call :CompileShader%1 PBREffect vs VSSkinned
call :CompileShader%1 PBREffect vs VSSkinnedBn

call :CompileShader%1 PBREffect ps PSConstant
call :CompileShader%1 PBREffect ps PSTextured
call :CompileShader%1 PBREffect ps PSTexturedEmissive
call :CompileShader%1 PBREffect ps PSTexturedVelocity
call :CompileShader%1 PBREffect ps PSTexturedEmissiveVelocity

call :CompileShader%1 DebugEffect vs VSDebug
call :CompileShader%1 DebugEffect vs VSDebugBn
call :CompileShader%1 DebugEffect vs VSDebugVc
call :CompileShader%1 DebugEffect vs VSDebugVcBn

call :CompileShader%1 DebugEffect vs VSDebugInst
call :CompileShader%1 DebugEffect vs VSDebugBnInst
call :CompileShader%1 DebugEffect vs VSDebugVcInst
call :CompileShader%1 DebugEffect vs VSDebugVcBnInst

call :CompileShader%1 DebugEffect ps PSHemiAmbient
call :CompileShader%1 DebugEffect ps PSRGBNormals
call :CompileShader%1 DebugEffect ps PSRGBTangents
call :CompileShader%1 DebugEffect ps PSRGBBiTangents

call :CompileShader%1 NPREffect vs VSNPREffect
call :CompileShader%1 NPREffect vs VSNPREffectBn
call :CompileShader%1 NPREffect vs VSNPREffectVc
call :CompileShader%1 NPREffect vs VSNPREffectVcBn

call :CompileShader%1 NPREffect vs VSNPREffectInst
call :CompileShader%1 NPREffect vs VSNPREffectBnInst
call :CompileShader%1 NPREffect vs VSNPREffectVcInst
call :CompileShader%1 NPREffect vs VSNPREffectVcBnInst

call :CompileShader%1 NPREffect vs VSNPREffectTx
call :CompileShader%1 NPREffect vs VSNPREffectBnTx
call :CompileShader%1 NPREffect vs VSNPREffectVcTx
call :CompileShader%1 NPREffect vs VSNPREffectVcBnTx

call :CompileShader%1 NPREffect vs VSNPREffectInstTx
call :CompileShader%1 NPREffect vs VSNPREffectBnInstTx
call :CompileShader%1 NPREffect vs VSNPREffectVcInstTx
call :CompileShader%1 NPREffect vs VSNPREffectVcBnInstTx

call :CompileShader%1 NPREffect vs VSSkinnedNPREffectTx
call :CompileShader%1 NPREffect vs VSSkinnedNPREffectTxBn

call :CompileShader%1 NPREffect ps PSCelShading
call :CompileShader%1 NPREffect ps PSCelShadingTx

call :CompileShader%1 NPREffect ps PSGoochShading
call :CompileShader%1 NPREffect ps PSGoochShadingTx

call :CompileShader%1 NPREffect ps PSMatCapShading
call :CompileShader%1 NPREffect ps PSMatCapShadingTx

call :CompileShader%1 SpriteEffect vs SpriteVertexShader
call :CompileShader%1 SpriteEffect ps SpritePixelShader

call :CompileShader%1 DGSLEffect vs main
call :CompileShader%1 DGSLEffect vs mainVc
call :CompileShader%1 DGSLEffect vs main1Bones
call :CompileShader%1 DGSLEffect vs main1BonesVc
call :CompileShader%1 DGSLEffect vs main2Bones
call :CompileShader%1 DGSLEffect vs main2BonesVc
call :CompileShader%1 DGSLEffect vs main4Bones
call :CompileShader%1 DGSLEffect vs main4BonesVc

call :CompileShaderHLSL%1 DGSLUnlit ps main
call :CompileShaderHLSL%1 DGSLLambert ps main
call :CompileShaderHLSL%1 DGSLPhong ps main

call :CompileShaderHLSL%1 DGSLUnlit ps mainTk
call :CompileShaderHLSL%1 DGSLLambert ps mainTk
call :CompileShaderHLSL%1 DGSLPhong ps mainTk

call :CompileShaderHLSL%1 DGSLUnlit ps mainTx
call :CompileShaderHLSL%1 DGSLLambert ps mainTx
call :CompileShaderHLSL%1 DGSLPhong ps mainTx

call :CompileShaderHLSL%1 DGSLUnlit ps mainTxTk
call :CompileShaderHLSL%1 DGSLLambert ps mainTxTk
call :CompileShaderHLSL%1 DGSLPhong ps mainTxTk

call :CompileShader%1 PostProcess vs VSQuad
call :CompileShader%1 PostProcess ps PSCopy
call :CompileShader%1 PostProcess ps PSMonochrome
call :CompileShader%1 PostProcess ps PSSepia
call :CompileShader%1 PostProcess ps PSDownScale2x2
call :CompileShader%1 PostProcess ps PSDownScale4x4
call :CompileShader%1 PostProcess ps PSGaussianBlur5x5
call :CompileShader%1 PostProcess ps PSBloomExtract
call :CompileShader%1 PostProcess ps PSBloomBlur
call :CompileShader%1 PostProcess ps PSMerge
call :CompileShader%1 PostProcess ps PSBloomCombine

call :CompileShader%1 ToneMap vs VSQuad
call :CompileShader%1 ToneMap ps PSCopy
call :CompileShader%1 ToneMap ps PSSaturate
call :CompileShader%1 ToneMap ps PSReinhard
call :CompileShader%1 ToneMap ps PSACESFilmic
call :CompileShader%1 ToneMap ps PS_SRGB
call :CompileShader%1 ToneMap ps PSSaturate_SRGB
call :CompileShader%1 ToneMap ps PSReinhard_SRGB
call :CompileShader%1 ToneMap ps PSACESFilmic_SRGB
call :CompileShader%1 ToneMap ps PSHDR10

if NOT %1.==xbox. goto skipxboxonly

call :CompileShaderxbox ToneMap ps PSHDR10_Saturate
call :CompileShaderxbox ToneMap ps PSHDR10_Reinhard
call :CompileShaderxbox ToneMap ps PSHDR10_ACESFilmic
call :CompileShaderxbox ToneMap ps PSHDR10_Saturate_SRGB
call :CompileShaderxbox ToneMap ps PSHDR10_Reinhard_SRGB
call :CompileShaderxbox ToneMap ps PSHDR10_ACESFilmic_SRGB

:skipxboxonly

echo.

if %error% == 0 (
    echo Shaders compiled ok
) else (
    echo There were shader compilation errors!
    exit /b 1
)

endlocal
exit /b 0

:CompileShader
set fxc=%PCFXC% "%1.fx" %FXCOPTS% /T%2_4_0 /E%3 "/Fh%CompileShadersOutput%\%1_%3.inc" "/Fd%CompileShadersOutput%\%1_%3.pdb" /Vn%1_%3
echo.
echo %fxc%
%fxc% || set error=1
exit /b

:CompileShaderHLSL
set fxc=%PCFXC% "%1.hlsl" %FXCOPTS% /T%2_4_0 /E%3 "/Fh%CompileShadersOutput%\%1_%3.inc" "/Fd%CompileShadersOutput%\%1_%3.pdb" /Vn%1_%3
echo.
echo %fxc%
%fxc% || set error=1
exit /b

:CompileShaderxbox
set fxc=%XBOXFXC% "%1.fx" %FXCOPTS% /T%2_5_0 %XBOXOPTS% /E%3 "/Fh%CompileShadersOutput%\XboxOne%1_%3.inc" "/Fd%CompileShadersOutput%\XboxOne%1_%3.pdb" /Vn%1_%3
echo.
echo %fxc%
%fxc% || set error=1
exit /b

:CompileShaderHLSLxbox
set fxc=%XBOXFXC% "%1.hlsl" %FXCOPTS% /T%2_5_0 %XBOXOPTS% /E%3 "/Fh%CompileShadersOutput%\XboxOne%1_%3.inc" "/Fd%CompileShadersOutput%\XboxOne%1_%3.pdb" /Vn%1_%3
echo.
echo %fxc%
%fxc% || set error=1
exit /b

:needfxc
echo ERROR: CompileShaders requires FXC.EXE
exit /b 1

:needxdk
echo ERROR: CompileShaders xbox requires the Microsoft Xbox One XDK
echo        (try re-running from the XDK Command Prompt)
exit /b 1
