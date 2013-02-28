#include <stdio.h>
#include "FbxNodesToKScene.h"
#include <KRTCore/entry/Entry.h>


KRayTracer::KRayTracer_Root* gRoot = NULL;

int main(int arg_cnt, const char* args[])
{
	if (arg_cnt < 2) {
		printf("Missing argument - please specify the FBX file that need to be converted.\n");
		printf("For example: \"KFbxTranslator MyScene.fbx MyConvertedScene.scn\", the second argument is optional.\n");
		return -1;
	}
	std::string input_file = args[1];
	std::string output_file;
	if (arg_cnt == 2) {
		std::string pathname(input_file);
		size_t dot_pos = pathname.rfind('.');
		if (dot_pos != std::string::npos) 
			pathname.resize(dot_pos);
		
		pathname += ".scn";
		output_file = pathname;
	}
	else
		output_file = args[2];

	// Initialize the KRTCore
	gRoot = KRayTracer::InitializeKRayTracer();

	int ret = -1;
	{
		// Using FBX SDK to convert the scene
		if (1 == InitFbxSDK(input_file.c_str())) {

			printf("Starting to Convert FBX file\n");
			printf("-----------------------------------\n");
			ret = ProccessFBXFile(output_file.c_str());
			if (ret != -1) {
				printf("Conversion succeeded!\n");
			}
			else {
				printf("Conversion failed.\n");
			}
		}
	}

	DestroyFbxSDK();
	// The KTRCore can be closed now
	KRayTracer::DestroyKRayTracer();
	return ret;
}
