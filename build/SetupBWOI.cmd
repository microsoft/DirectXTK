@echo off
rem Copyright (c) Microsoft Corporation.
rem Licensed under the MIT License.

if %1.==. goto needpackage
if %2.==. goto needgdk
if %3.==PC. goto haveparams
if %3.==XboxOne. goto haveparams
if %3.==Scarlett. goto haveparams
goto needconsole

:haveparams
set GXDKEDITION=%2
echo GXDKEDITION: %GXDKEDITION%

set CORENUGET=%1\Microsoft.GDK.Core\
if EXIST %CORENUGET% goto newlayout

set PCNUGET=%1\Microsoft.GDK.PC\
if EXIST %PCNUGET% goto oldlayout

goto missingpcnuget

REM Use new layouts (October 2025 GDK and later)
:newlayout

set GameDK=%CORENUGET%native\bin\

if %3.==PC. goto newlayoutpc
if %3.==XboxOne. goto newlayoutxboxone
if %3.==Scarlett. goto newlayoutscarlett
goto needconsole

:newlayoutpc
set WINDOWSNUGET=%1\Microsoft.GDK.Windows\
if NOT EXIST %WINDOWSNUGET% goto missingpcnuget

set GameDKCoreLatest=%WINDOWSNUGET%native\%GXDKEDITION%\

set ADDINCLUDE=%GameDKCoreLatest%windows\include
REM arm64?
set ADDBIN=%GameDKCoreLatest%windows\bin\x64;%CORENUGET%native\bin
set ADDLIB=%GameDKCoreLatest%windows\lib\x64
goto continuenew

:newlayoutxboxone
set XBOXNUGET=%1\Microsoft.GDK.Xbox.XboxOne\
if NOT EXIST %XBOXNUGET% goto missingxboxnuget

set GameDKXboxLatest=%XBOXNUGET%native\%GXDKEDITION%\

set ADDINCLUDE=%GameDKXboxLatest%xbox\include\gen8;%GameDKXboxLatest%xbox\include
set ADDBIN=%GameDKXboxLatest%xbox\bin\gen8;%GameDKXboxLatest%xbox\bin\x64;%CORENUGET%native\bin
set ADDLIB=%GameDKXboxLatest%xbox\lib\gen8;%GameDKXboxLatest%xbox\lib\x64
goto continuenew

:newlayoutscarlett
set XBOXNUGET=%1\Microsoft.GDK.Xbox.XboxSeriesX_S\
if NOT EXIST %XBOXNUGET% goto missingxboxnuget

set GameDKXboxLatest=%XBOXNUGET%native\%GXDKEDITION%\

set ADDINCLUDE=%GameDKXboxLatest%xbox\include\gen9;%GameDKXboxLatest%xbox\include
set ADDBIN=%GameDKXboxLatest%xbox\bin\gen9;%GameDKXboxLatest%xbox\bin\x64;%CORENUGET%native\bin
set ADDLIB=%GameDKXboxLatest%xbox\lib\gen9;%GameDKXboxLatest%xbox\lib\x64
goto continuenew

:continuenew
echo GameDK: %GameDK%
echo GameDKCoreLatest: %GameDKCoreLatest%
echo GameDKXboxLatest: %GameDKXboxLatest%
echo ADDBIN: %ADDBIN%
echo ADDINCLUDE: %ADDINCLUDE%
echo ADDLIB: %ADDLIB%

set PATH=%ADDBIN%;%PATH%
set INCLUDE=%INCLUDE%;%ADDINCLUDE%
set LIB=%LIB%;%ADDLIB%
exit /b 0

REM Use old layouts (pre-October 2025 GDK)
:oldlayout
set GRDKLatest=%PCNUGET%native\%GXDKEDITION%\GRDK\
echo GRDKLatest: %GRDKLatest%

if %3.==PC. goto grdkonly

set XBOXNUGET=%1\Microsoft.gdk.xbox\
if NOT EXIST %XBOXNUGET% goto missingxboxnuget

set GXDKLatest=%XBOXNUGET%native\%GXDKEDITION%\GXDK\
echo GXDKLatest: %GXDKLatest%

set GameDK=%XBOXNUGET%native\
set GameDKLatest=%XBOXNUGET%native\%GXDKEDITION%\

set ADDBIN=%GXDKLatest%bin\%3;%PCNUGET%native\bin;%XBOXNUGET%native\bin
set ADDINCLUDE=%GXDKLatest%gamekit\include\%3;%GXDKLatest%gamekit\include;%GRDKLatest%gamekit\include
set ADDLIB=%GXDKLatest%gamekit\lib\amd64\%3;%GXDKLatest%gamekit\lib\amd64;%GRDKLatest%gamekit\lib\amd64
goto continueold

:grdkonly
set GameDK=%PCNUGET%native\
set GameDKLatest=%PCNUGET%native\%GXDKEDITION%\

set ADDBIN=%PCNUGET%native\bin
set ADDINCLUDE=%GRDKLatest%gamekit\include
set ADDLIB=%GRDKLatest%gamekit\lib\amd64

:continueold
echo GameDK: %GameDK%
echo GameDKLatest: %GameDKLatest%
echo ADDBIN: %ADDBIN%
echo ADDINCLUDE: %ADDINCLUDE%
echo ADDLIB: %ADDLIB%

set PATH=%ADDBIN%;%PATH%
set INCLUDE=%INCLUDE%;%ADDINCLUDE%
set LIB=%LIB%;%ADDLIB%
exit /b 0

:needpackage
echo Usage: This script requires the path to the installed NuGet packages as the first parameter.
exit /b 1

:needgdk
echo Usage: This script requires the GDK edition number in YYMMQQ format as the second parameter
exit /b 1

:needconsole
echo Usage: This script requires the target type of PC, Scarlett, or XboxOne in the third parameter
exit /b 1

:missingpcnuget
echo ERROR - Cannot find Microsoft.GDK.Core/Windows/PC installed at '%1'
exit /b 1

:missingxboxnuget
echo ERROR - Cannot find Microsoft.GDK.Xbox/.XboxOne/.XboxSeriesX_S installed at '%1'
exit /b 1
