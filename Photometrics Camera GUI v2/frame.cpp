//This code interfaces with the new Photometrics SDK, to replace previous Qimaging code being used
//
//Written by: Thomas Santerre
//Adapted from existing code from Ilya
//
//Libraries:
//
//PVCam 64bit SDK used for development.
//
//IMPORTANT NOTES:
//int32 struct from master.h renamed to int32_ due to conflicts with typedef in TIFF.h
//shouldn't be an issue really as we don't use this type, only uns32 is needed.

// Order of imports is critical: Yaaseen
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include "tiffio.h"
//NEW API LIBRARIES, check order of inclusion...as well as necessities
#include "master.h"
#include "pvcam.h"
//
#define BUFFSIZE 2
// define CamList class, created due to removal from qimaging
class cameraList
{
public:
	char_ptr cam_name;
};
// define camera class

class Camera
{
private:

	//Other Config
	unsigned long frame_w;			// width and length of image
	unsigned long frame_h;

public:
	//Camera Information
	cameraList camlist[10];			// number of cameras attached (should be just one for ease)
	int16 hCamera;					// handle to camera
	int16 o_mode;					// defines method of opening camera
	uns16 *frame;					// Frame structure
	int16 status;
	uns32 buffer_size, frame_size;
	uns16 *buffer;
	void_ptr address;
	rgn_type region;
	uns32 size;
	uns32 not_needed;				// Filler Variable
	uns16 frames_total;				// total number of frames
	Camera();						// constructor
	int frames_cur;					// current number of frames grabbed
	bool InitCamera(void);			// start camera
	bool GrabFrameCont(int numberframes);
	bool ShutCamera(void);			// shut camera
	bool asyncDone[BUFFSIZE];	// flags to alert if frame grab of each frame in async is complete
	bool SaveFrame(char* filename, int frame_num);	// save the frame as TIF file to memory
};

//Variable INIT
//settings variables

//length of exposure in milliseconds
float exposure_time;
//prefix of filename (without the 0001.tif suffix)
char file_prefix[200];
//number of images to take
int num_images;
//binning setting
int bin;
//gain setting
float gain;
//offset setting
int offset;
//external trigger or free run?
bool ext_trigger;
//display captured images in preview window?
bool preview;
//display intensity histogram?
bool histogram;
//normalize image?
bool normalize;
//discards first frame taken
bool discard_first;
// do speckle
bool speckle;
//left edge of ROI
int left_edge;
//width of ROI
int width;
//top edge of ROI
int top_edge;
//height of ROI
int height;
//delay between clicking record and actual record in seconds
//allows for time to exit the room, shut lights, etc.
int delay;
//limits framerate, allows for consistent framerate
//when set below hardware capability
float max_framerate;
//EMgain for EMC2
int emGain;

// Initializes Camera Class
bool Camera::InitCamera(void)
{
	hCamera = NULL;
	unsigned long listLen = sizeof(camlist) / sizeof(camlist[0]);

	// Open Library Driver
	pl_pvcam_init();
	pl_exp_init_seq();
	// Get Camera Information
	pl_cam_get_name(0, camlist[0].cam_name);
	// left_edge, top_edge, width, height
	pl_set_param(hCamera, PARAM_GAIN_INDEX, &gain);	// Set Gain Param
	region = { 0, width, bin, 0, height, bin };
	//Setup Sequence for Camera
	pl_exp_setup_cont(hCamera, 1, &region, TIMED_MODE, 100, &frame_size, CIRC_OVERWRITE); //Still need to allocate stream space
	frame = (uns16*)malloc(size);
	return true;
}

bool Camera::GrabFrameCont(int numberframes) //Grabs continuous session of frames, saves as TIFF
{
	int frame_num = 0;
	/* set up a circular buffer of 3 frames */
	buffer_size = frame_size * 3;
	buffer = (uns16*)malloc(buffer_size);
	/* Start the acquisition */
	printf("Collecting %i Frames\n", numberframes);
	pl_exp_start_cont(hCamera, buffer, buffer_size);
	/* ACQUISITION LOOP */
	while (frame_num != numberframes) {
		/* wait for data or error */
		while (pl_exp_check_cont_status(hCamera, &status, &not_needed,
			&not_needed) &&
			(status != READOUT_COMPLETE && status != READOUT_FAILED));
		/* Check Error Codes */
		if (status == READOUT_FAILED) {
			printf("Data collection error: %i\n", pl_error_code());
			break;
		}
		if (pl_exp_get_latest_frame(hCamera, &address)) {
			/* address now points to valid data */
			SaveFrame("test", frame_num);
			printf("Remaining Frames %i\n", (numberframes-frame_num));
			frame_num++;
		}
	} /* End while */
	/* Stop the acquisition */
	pl_exp_stop_cont(hCamera, CCS_HALT);
	// Stop camera now
	ShutCamera();
	printf("Remaining Frames %i\n", numberframes);
	return true;
}

// Shutting Camera Class
bool Camera::ShutCamera(void)
{
	/*Uninit the sequence */
	pl_exp_uninit_seq();
	free(buffer);
	printf("<start close>");
	pl_cam_close(hCamera);	// stop streaming
	printf("<stopped streaming>");
	pl_pvcam_uninit();	//Close Driver
	printf("<closed driver>");
	printf("%d", "closed");
	free(frame);
	printf("endclose");
	return 0;
}
//Save Image (multi-threaded) deal with later, for now just flushing to file
bool Camera::SaveFrame(char *filename, int frame_num)
{
	long bytesPerImage = 2 * frame_w * frame_h;
	TIFF * tiff = TIFFOpen(filename, "w");
	TIFFSetDirectory(tiff, 0);
	TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, frame_w);
	TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, frame_h);
	TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 16);
	TIFFSetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
	TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
	TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 1);
	TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, frame_h);
	TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);
	TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(tiff, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
	TIFFSetField(tiff, TIFFTAG_PAGENUMBER, 0, 2);
	TIFFWriteRawStrip(tiff, 0, address, bytesPerImage);
	TIFFWriteDirectory(tiff);
	TIFFClose(tiff);
	// Ideally we want to be able to stream what we are saving, that is all this code will do in the future
	//#pragma omp parallel firstprivate (odd_frame)
	//	{
	//		//breaking code into 3 sections (image capture/save, image display, image histogram)
	//		//each section executes as independent thread
	//#pragma omp sections
	//		{
	//			//image capture section
	//#pragma omp section
	//			{
	//				
	//			}
	//
	//			//image display section
	//#pragma omp section
	//			{
	//			
	//			}
	//		}	/* end of sections */
	//	}	  /* end of parallel section */
	return true;
}

//Constructor
Camera::Camera(){};

//rec_mode is function called after user selects to record
//it reads the settings from the GUI and opens a file selection
//window for the user to select where to save the images
void rec_mode()
{


}

//prev_mode is function called after user selects to preview
//it reads the settings from the GUI and attempts to clear
//the preview buffer before grabbing the images
void prev_mode()
{

}

//roi_select_mode is function called after user chooses to select
//ROI. It sets up for a full frame single image snap, followed by
//a session allowing user to drag and select a region of interest
void roi_select_mode()
{

}

int main(){
	int numberframes = 10;
	//Call a new camera class to be used. Assuming only one PVCAM attached.
	Camera main_camera;
	main_camera.InitCamera();
	//We now need to fill remaining settings.

	//Call our main frame grab function.
	main_camera.GrabFrameCont(numberframes);
	//Shutdown the camera
	main_camera.ShutCamera();
}