/*
 * Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "captureWindow.h"

#include "gstCamera.h"
#include "glDisplay.h"
#include "imageIO.h"

#include <X11/cursorfont.h>


// constructor
CaptureWindow::CaptureWindow()
{
	mode    = Live;
	camera  = NULL;
	display = NULL;
	imgRGBA = NULL;
}


// destructor
CaptureWindow::~CaptureWindow()
{
	SAFE_DELETE(camera);
	SAFE_DELETE(display);
}


// Create
CaptureWindow* CaptureWindow::Create( commandLine& cmdLine )
{
	CaptureWindow* window = new CaptureWindow();

	if( !window || !window->init(cmdLine) )
	{
		printf("camera-capture:  CaptureWindow::Create() failed\n");
		return NULL;
	}

	return window;
}


// init
bool CaptureWindow::init( commandLine& cmdLine )
{
	/*
	 * create the camera device
	 */
	camera = gstCamera::Create(cmdLine.GetInt("width", gstCamera::DefaultWidth),
						  cmdLine.GetInt("height", gstCamera::DefaultHeight),
						  cmdLine.GetString("camera"));

	if( !camera )
	{
		printf("\ncamera-capture:  failed to initialize camera device\n");
		return false;
	}
	
	printf("\ncamera-capture:  successfully initialized camera device\n");
	printf("    width:  %u\n", camera->GetWidth());
	printf("   height:  %u\n", camera->GetHeight());
	printf("    depth:  %u (bpp)\n\n", camera->GetPixelDepth());
	

	/*
	 * create openGL window
	 */
	display = glDisplay::Create("Data Collection Tool",
						   camera->GetWidth() + cameraOffsetX + 5,
						   camera->GetHeight() + cameraOffsetY + 5);

	if( !display ) 
	{
		printf("camera-capture:  failed to create openGL display\n");
		return false;
	}

	/*glWidget* widget = display->AddWidget(new glWidget(50, 50, 200, 500));
	
	widget->SetMoveable(true);
	widget->SetResizeable(true);*/

	//display->SetCursor(XC_tcross);
	//display->ResetCursor();

	return true;
}


// Render
void CaptureWindow::Render()
{
	// capture RGBA image
	if( mode == Live )
	{
		if( !camera->CaptureRGBA(&imgRGBA, 1000, true) )
			printf("camera-capture:  failed to capture RGBA image from camera\n");
	}

	// update display
	if( display != NULL )
	{
		// render the image
		if( imgRGBA != NULL )
			display->RenderOnce(imgRGBA, camera->GetWidth(), camera->GetHeight(), cameraOffsetX, cameraOffsetY);

		// update the status bar
		char str[256];
		sprintf(str, "Data Collection Tool | %.0f FPS", display->GetFPS());
		display->SetTitle(str);
	}
}


// Save
bool CaptureWindow::Save( const char* filename, int quality )
{
	if( !filename || !imgRGBA )
		return false;

	CUDA(cudaDeviceSynchronize());

	if( !saveImageRGBA(filename, (float4*)imgRGBA, camera->GetWidth(), camera->GetHeight(), 255.0f, quality) )
	{
		printf("camera-capture:  failed to save %s\n", filename);
		return false;
	}

	printf("camera-capture:  saved %s\n", filename);
	return true;
}


// SetMode
void CaptureWindow::SetMode( CaptureMode _mode )
{
	mode = _mode;

	if( mode == Edit )
	{
		display->SetDefaultCursor(XC_tcross);
		display->SetDragMode(glDisplay::DragCreate);
	}
	else if( mode == Live )
	{
		display->ResetDefaultCursor();
		display->SetDragMode(glDisplay::DragDefault);
	}
}


// IsOpen
bool CaptureWindow::IsOpen() const
{
	return display->IsOpen();
}


// IsClosed
bool CaptureWindow::IsClosed() const
{
	return display->IsClosed();
}


// IsStreaming
bool CaptureWindow::IsStreaming() const
{
	return camera->IsStreaming();
}


// GetCameraWidth
int CaptureWindow::GetCameraWidth() const
{
	return camera->GetWidth();
}


// GetCameraHeight
int CaptureWindow::GetCameraHeight() const
{
	return camera->GetHeight();
}


// GetWindowWidth
int CaptureWindow::GetWindowWidth() const
{
	return display->GetWidth();
}


// GetWindowHeight
int CaptureWindow::GetWindowHeight() const
{
	return display->GetHeight();
}


