#include <iostream>
#include <map>

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
	printf("example: voxpng -i ~/guy.vox -o ~/guy.png\n");
	printf(" ");
	args.printOptions();
	printf(" ");
	printf("if any error occurs, the exit status is 2.");
}

int main(int argc, char **argv)
{
	Arguments args(argc, argv);

	if(argc < 3 || args.getArgument_ShouldShowHelp())
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
			return 2;
		}

		Scene scene;
		error err = scene.load(voxPath.result);
		if(!err.empty())
		{
			printf("%s\n", err.c_str());
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
			return 2;
		}

		return 0;
	}
}