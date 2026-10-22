@echo off
echo ============================================================
echo   Inventory Management System - Web Server Launcher
echo ============================================================
echo.
echo Starting Python Web Server on http://localhost:8080 ...
echo Press Ctrl+C in this window to stop the server.
echo.

start "" http://localhost:8080/
python server.py 8080

pause
