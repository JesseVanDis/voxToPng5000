# voxToPng5000

Converts .vox -> .png  ( MagicaVoxel to PNG image )

A tool that merges all layers/nodes of a .vox file and converts it to an .png image
The main motivation was to create an easyer way of reading .vox files in general, and to save disk space. 
( a .vox file of 30kb will change into an 500b .png file )

// TODO Add some screenshots

## Building

### Windows
If you have [Git](https://git-scm.com) you don't need anything else to build the application.
When you do **not** have *Git*, you need to install [MinGW](http://www.mingw.org)
( if *MinGW* is not on your machine, *Git* will clone a rep that contains an portable *MinGW* and uses that. )

To build, just run the:
```
build_win.bat
```

Once the build is done, the .exe can be found in the generated 'bin' folder.

### Linux

The *gcc* compiler is needed, but should be already pre-installed in your linux system.
So just run the following to build the program:
```
build_linux.sh
```

## Running

### Windows
The easiest way of using the converter, is to drag and drop the *.vox* file into the *voxToPng.exe*.

The *.png* will appear where the *.vox* is located

Keep in mind that there is no feedback when the conversion was successfull. The program will run for as long as it takes, and quit automatically when the .png has been generated

You can also run this with options. ( in case you want borders, or the output name to be different. ) run it with '-h' to see what options there are
It should be similar to the 'Linux' section below

### Linux
Run the following to convert the .vox file:

```
voxToPng -i path\towards\the\voxelfile.vox -o path\towards\the\pngoutputfile.png
```

Small overview of the options:
```
-i = INPUTFILE     the filepath towards the .vox file.
-o = OUTPUT        the target directory/filepath (supports format. more details at -h)
-b = COLORHEX      the color for the borders. (default: no borders)
-h                 Show help ( and see more details )
```
Example: 
```
voxToPng -b ffff00ff -i voxelfile.vox -o outputfile_{SIZE_X}_{SIZE_Y}.png
```

## License

This project is licensed under the GNU General Public License v3.0 License - see the [LICENSE](LICENSE) file for details
