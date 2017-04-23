set DEPENDENCY_DIR=..\..\..\Dependency
set SOURCE_DIR=%~dp0..\Source

xcopy %SOURCE_DIR%\Model\*.* %OUT_DIR%Model\ /s /y
xcopy %SOURCE_DIR%\Shader\*.* %OUT_DIR%Shader\ /s /y

xcopy %DEPENDENCY_DIR%\Assimp\lib\%PLATFORM%\%CONFIGURATION%\assimp-vc140-mt.dll %OUT_DIR% /s /y

if %PLATFORM%==Win32 (
xcopy %DEPENDENCY_DIR%\UniversalDXSDK\Redist\x86\d3dcompiler_47.dll %OUT_DIR% /y
)
if %PLATFORM%==x64 (
xcopy %DEPENDENCY_DIR%\UniversalDXSDK\Redist\x64\d3dcompiler_47.dll %OUT_DIR% /y
)