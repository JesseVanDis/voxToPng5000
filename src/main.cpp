#include <iostream>

#include "stdlib.h"
#include "stdio.h"
#include "math.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "scene.hpp"


int main(int argc, char **argv)
{
	if(argc < 3)
	{
		printf("Usage: \"input.vox\" \"outputfolder\"\n");
		return 1;
	}

	const char* pVoxPath = argv[1];
	const char* pPngPath = argv[2];

	Scene scene;
	error err = scene.load(pVoxPath);
	if(!err.empty())
	{
		printf("%s\n", err.c_str());
		return 1;
	}

	err = scene.saveAsPngArray(pPngPath);
	if(!err.empty())
	{
		printf("%s\n", err.c_str());
		return 1;
	}

	return 0;
}