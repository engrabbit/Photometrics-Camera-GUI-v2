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


#include "frame.h"
// Initializes Camera Class
bool Camera::InitCamera(void)
{
	hCamera = NULL;
	rs_bool avail_flag;
	// Open Library Driver
	pl_pvcam_init();
	// Get Camera Information
	pl_cam_get_name(0, cam_name);
	// Open Camera
	pl_cam_open(cam_name, &hCamera, OPEN_EXCLUSIVE);
	//Check Circular Buffer
	if (pl_get_param(hCamera, PARAM_CIRC_BUFFER, ATTR_AVAIL, &avail_flag)
		&&
		avail_flag)
		printf("Circular Buffer Supported.\n");
	else{
		printf("circular buffers not supported\n");
		ShutCamera();
	}
	region = { 0, 511, 1, 0, 511, 1 };
	//Setup Sequence for Camera
	pl_exp_init_seq();
	pl_exp_setup_cont(hCamera, 1, &region, TIMED_MODE, EXP_RES_ONE_MILLISEC, &frame_size, CIRC_OVERWRITE); 
	return true;
}

bool Camera::GrabFrameCont() //Grabs continuous session of frames, saves as TIFF
{
	int frame_num = 0;
	/* set up a circular buffer of 3 frames */
	buffer_size = frame_size * 3;
	buffer = (uns16*)malloc(buffer_size);
	/* Start the acquisition */
	printf("Collecting %i Frames\n", config.numberFrames);
	pl_exp_start_cont(hCamera, buffer, buffer_size);
	/* ACQUISITION LOOP */
	while (frame_num != config.numberFrames) {
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
			if (!(SaveFrame("test", frame_num)))
				break;
			printf("Remaining Frames %i\n", (config.numberFrames-frame_num));
			frame_num++;
		}
	}
	/* Stop the acquisition */
	printf("Capture Complete\n");
	pl_exp_stop_cont(hCamera, CCS_HALT);
	pl_exp_finish_seq(hCamera, buffer, 0);
	pl_exp_uninit_seq();
	free(buffer);
	return true;
}

// Shutting Camera Class
bool Camera::ShutCamera(void)
{
	pl_cam_close(hCamera);	// stop streaming
	printf("<Stopped Camera>\n");
	pl_pvcam_uninit();	//Close Driver
	printf("<Closed Driver>\n");
	return true;
}
//Save Image (multi-threaded) deal with later, for now just flushing to file
bool Camera::SaveFrame(char *filename, int frame_num)
{
	printf("SaveFrame %d\n", frame_num);
	long bytesPerImage = 2 * 511 * 511; // Needs to change to adapt to ROI
	char integer_char[32];
	sprintf(integer_char, "%d", frame_num);
	char filename_added[100] = "TIFF_";
	strcat(filename_added, integer_char);
	strcat(filename_added, ".tiff");
	TIFF * tiff = TIFFOpen(filename_added, "w");
	TIFFSetDirectory(tiff, 0);
	TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, 511);
	TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, 511);
	TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 16);
	TIFFSetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
	TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
	TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 1);
	TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, 511);
	TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);
	TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(tiff, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
	TIFFSetField(tiff, TIFFTAG_PAGENUMBER, 0, 2);
	TIFFWriteRawStrip(tiff, 0, address, bytesPerImage);
	TIFFClose(tiff);
		return true;
}

//Constructor
Camera::Camera(){};

//Load settings from config file, to be replaced with GUI Widget in future.
bool Camera::loadSettings(char* config_name){
	FILE* file = fopen(config_name, "r");
	char *importedFile;
	char temp[100];
	long lSize = ftell(file);
	rewind(file);
	fread(importedFile, lSize, 1, file);
	std::getline()

}

int main(int argc, char** argv){
	//Call a new camera class to be used. Assuming only one PVCAM attached.
	Camera main_camera;
	main_camera.loadSettings(argv[0]);
	main_camera.InitCamera();
	//Call our main frame grab function.
	main_camera.GrabFrameCont();
	//Shutdown the camera
	main_camera.ShutCamera();
	return 0;
}