@echo off

SETLOCAL

WHERE g++
echo       - ignore this info line above me -
IF (%ERRORLEVEL% NEQ 0) (

	IF EXIST mingw (
		echo portable version of MinGW found.
	) ELSE (
		echo MinGW not found. checking if portable version has been downloaded.
		@rem echo downloading portable version to ./mingw from 'https://github.com/jonasstrandstedt/MinGW'
		echo downloading portable version to ./mingw from 'https://github.com/pedrofernandesfilho/MinGWMSYSPortable'
		echo MinGW is needed to compile the C++ source code, as it uses the 'g++' command
		echo "make sure 'git' is installed ( or simply install mingw )"
		mkdir mingw
		cd mingw
		
	
		git clone https://github.com/pedrofernandesfilho/MinGWMSYSPortable
		@rem git clone https://github.com/jonasstrandstedt/MinGW
		cd ../
	)
	
	cd mingw/MinGW
	set MINGW_PATH=%CD%\mingw\MinGW\MinGW
	set PORTABLE=y
	cd ../../
)

IF "y" == "%PORTABLE%" set PATH=%PATH%;%MINGW_PATH%
IF "y" == "%PORTABLE%" set PATH=%PATH%;%MINGW_PATH%/bin
IF "y" == "%PORTABLE%" set PATH=%PATH%;%MINGW_PATH%/msys/1.0/bin
IF "y" == "%PORTABLE%" set PATH=%PATH%;%MINGW_PATH%/dll

rmdir /s/q bin
mkdir bin
echo building...
cd src
g++ -O2 -std=c++11 -fpermissive main.cpp arguments.cpp dictionary.cpp loader.cpp model.cpp node.cpp node_group.cpp node_shape.cpp node_transform.cpp palette.cpp scene.cpp -o ../bin/voxToPng
cd ../
copy "%CD%\mingw\MinGW\MinGW\bin\libgcc_s_dw2-1.dll" "%CD%\bin\libgcc_s_dw2-1.dll"
copy "%CD%\mingw\MinGW\MinGW\bin\libstdc++-6.dll" "%CD%\bin\libstdc++-6.dll"
echo done
cd ../

ENDLOCAL 
