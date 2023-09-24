@echo off
del /S /Q *.~*
del /S /Q *.map
del /S /Q *.tds
del /S /Q *.obj
del /S /Q *.db
del /S /Q *.ilc
del /S /Q *.ild
del /S /Q *.ilf
del /S /Q *.ils
del /S /Q *.dcu
del /S /Q *.dsk
rd /S /Q Debug
rd /S /Q Release
rd /S /Q ipch
::pause
@echo on
