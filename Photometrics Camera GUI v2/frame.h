
// Order of imports is critical: Yaaseen
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include "tiffio.h"

//NEW API LIBRARIES, check order of inclusion...as well as necessities
#include "master.h"
#include "pvcam.h"
struct configStruct{
	int exposure, numberFrames, rWidth, rHeight, bin, gain, offset, delay;
	char fileName[200];
};

class Camera
{
public:
	//Other Config
	configStruct config;
	//Camera Information
	int16 hCamera;					// handle to camera
	uns32 buffer_size, frame_size;
	char cam_name[CAM_NAME_LEN];
	uns16 *buffer;
	void_ptr address;
	rgn_type region;
	int16 status;
	uns32 size, not_needed;
	uns16 frames_total;				// total number of frames
	Camera();						// constructor
	bool loadSettings(char* config);
	bool InitCamera(void);			// start camera
	bool GrabFrameCont();
	bool ShutCamera(void);			// shut camera
	bool SaveFrame(char* filename, int frame_num);	// save the frame as TIF file to memory
};