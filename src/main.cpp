#include <iostream>
#include <map>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <zconf.h>
#endif

#include "stdlib.h"
#include "stdio.h"
#include "math.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "scene.hpp"
#include "arguments.hpp"

static void printHelp(const Arguments& args)
{
	printf("Usage: voxpng [OPTIONS]\n");
	printf("converts .vox to png file(s)\n");
	#ifdef _WIN32
		printf("example: voxpng -i C:/guy.vox -o C:/guy.png\n");
	#else		
		printf("example: voxpng -i ~/guy.vox -o ~/guy.png\n");
	#endif
	printf("\n");
	args.printOptions();
	printf("\n");
	printf("if any error occurs, the exit status is 2.\n");
}

int main(int argc, char **argv)
{
	bool byDragAndDrop = false;
	Arguments args(argc, argv);	
	if(argc == 2)
	{
		char outputFilePath[512] = "";
		strcpy(outputFilePath, argv[1]);
		size_t len = strlen(outputFilePath);
		if(len > 3)
		{
			outputFilePath[len-3] = 'p';
			outputFilePath[len-2] = 'n';
			outputFilePath[len-1] = 'g';
		}
		args.clear();
		args.addArgument("i", argv[1]);
		args.addArgument("o", outputFilePath);
		byDragAndDrop = true;
	}
	
	#ifdef _WIN32
		if(argc == 1) 
		{
			printf("\n");
			printf(" =======  .VOX to .PNG converter 5000! ======= \n");
			printf("Usage: Drag & Drop the .vox file in the .exe file\n");
			printf("or call this from the command line with the '-h' option for more options\n");
			printf("Application will close in 5 seconds\n");
			printf("\n");
			SLEEP(5000);
		}
	#endif

	if(!byDragAndDrop && (argc < 3 || args.getArgument_ShouldShowHelp()))
	{
		printHelp(args);
		return 0;
	}
	else
	{
		auto voxPath = args.getArgument_InputPath();
		auto outputPath = args.getArgument_OutputPath();
		auto setting = args.getArgument_Setting();
		if(voxPath.handleError() || outputPath.handleError() || setting.handleError())
		{
			if(byDragAndDrop)
			{
				SLEEP(5000);
			}
			return 2;
		}
		
		Scene scene(args.getArgument_Verbose());
		error err = scene.load(voxPath.result);
		if(!err.empty())
		{
			printf("%s\n", err.c_str());
			if(byDragAndDrop)
			{
				SLEEP(5000);
			}
			return 2;
		}

		switch(setting.result)
		{
			case Setting_SeperatePngs:
				err = scene.saveAsPngArray(outputPath.result);
				break;

			case Setting_Merged:
				err = scene.saveAsMergedPng(outputPath.result, args.getArgument_BorderColor());
				break;
		}
		if(!err.empty())
		{
			printf("%s\n", err.c_str());
			if(byDragAndDrop)
			{
				SLEEP(5000);
			}
			return 2;
		}

		if(args.getArgument_Verbose())
		{
			printf("done");
		}
		return 0;
	}
}