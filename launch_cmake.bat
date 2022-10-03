:: call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin\amd64\vcvars64.bat"

cd build
cmake -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles" ..
pause