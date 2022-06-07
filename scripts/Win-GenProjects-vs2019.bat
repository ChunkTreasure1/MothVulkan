@echo off

pushd ..\
call scripts\data\premake5.exe vs2019
popd

PAUSE