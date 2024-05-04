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

set PCNUGET=%1\Microsoft.GDK.PC.%GXDKEDITION%\
if NOT EXIST %PCNUGET% goto missingpcnuget

set GRDKLatest=%PCNUGET%native\%GXDKEDITION%\GRDK\
echo GRDKLatest: %GRDKLatest%

if %3.==PC. goto grdkonly

set XBOXNUGET=%1\Microsoft.gdk.xbox.%GXDKEDITION%\
if NOT EXIST %XBOXNUGET% goto missingxboxnuget

set GXDKLatest=%XBOXNUGET%native\%GXDKEDITION%\GXDK\
echo GXDKLatest: %GXDKLatest%

set GameDK=%XBOXNUGET%native\
set GameDKLatest=%XBOXNUGET%native\%GXDKEDITION%\

set ADDBIN=%GXDKLatest%bin\%3;%PCNUGET%native\bin;%XBOXNUGET%native\bin
set ADDINCLUDE=%GXDKLatest%gamekit\include\%3;%GXDKLatest%gamekit\include;%GRDKLatest%gamekit\include
set ADDLIB=%GXDKLatest%gamekit\lib\amd64\%3;%GXDKLatest%gamekit\lib\amd64;%GRDKLatest%gamekit\lib\amd64
goto continue

:grdkonly
set GameDK=%PCNUGET%native\
set GameDKLatest=%PCNUGET%native\%GXDKEDITION%\

set ADDBIN=%PCNUGET%native\bin
set ADDINCLUDE=%GRDKLatest%gamekit\include
set ADDLIB=%GRDKLatest%gamekit\lib\amd64

:continue
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
echo ERROR - Cannot find Microsoft.GDK.PC.<edition> installed at '%1'
exit /b 1

:missingxboxnuget
echo ERROR - Cannot find Microsoft.GDK.Xbox.<edition> installed at '%1'
exit /b 1
