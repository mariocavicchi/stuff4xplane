/*
 * HellWorld.c
 * 
 * This plugin implements the canonical first program.  In this case, we will 
 * create a window that has the text hello-world in it.  As an added bonus
 * the  text will change to 'This is a plugin' while the mouse is held down
 * in the window.  
 * 
 * This plugin demonstrates creating a window and writing mouse and drawing
 * callbacks for that window.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "XPLMDisplay.h"
#include "XPLMDataAccess.h"
#include "XPLMGraphics.h"
#include "XPLMCamera.h"

// OpenGL libs
#include <GL/gl.h>
#include <GL/glu.h>



/*
 * Global Variables.  We will store our single window globally.  We also record
 * whether the mouse is down from our mouse handler.  The drawing handler looks
 * at this information and draws the appropriate display.
 * 
 */

XPLMWindowID		gWindow = NULL;
int			gClicked = 0;
XPLMDataRef             gPlaneX;                                                                                                                                                                                                             
XPLMDataRef             gPlaneY;
XPLMDataRef             gPlaneZ;


int     MyDrawCallback(
                                   XPLMDrawingPhase     inPhase,
                                   int                  inIsBefore,
                                   void *               inRefcon);


int MyDrawWindowCallback(
                                   XPLMWindowID         inWindowID,    
                                   void *               inRefcon);    

void MyHandleKeyCallback(
                                   XPLMWindowID         inWindowID,    
                                   char                 inKey,    
                                   XPLMKeyFlags         inFlags,    
                                   char                 inVirtualKey,    
                                   void *               inRefcon,    
                                   int                  losingFocus);    

int MyHandleMouseClickCallback(
                                   XPLMWindowID         inWindowID,    
                                   int                  x,    
                                   int                  y,    
                                   XPLMMouseStatus      inMouse,    
                                   void *               inRefcon);    

GLuint LoadTextureRAW( 		const char * 		filename, 
				int 			wrap );

/*
 * XPluginStart
 * 
 * Our start routine registers our window and does any other initialization we 
 * must do.
 * 
 */
PLUGIN_API int XPluginStart(
						char *		outName,
						char *		outSig,
						char *		outDesc)
{
	/* First we must fill in the passed in buffers to describe our
	 * plugin to the plugin-system. */


	strcpy(outName, "HelloWorld");
	strcpy(outSig, "xplanesdk.examples.helloworld");
	strcpy(outDesc, "A plugin that makes a window.");


        XPLMRegisterDrawCallback(                                                                                                                                                                                                            
                                        MyDrawCallback, 
                                        xplm_Phase_Objects,     /* Draw when sim is doing objects */
                                        0,                                              /* After objects */
                                        NULL);                                  /* No refcon needed */
                                        

 
	return 1;
}

/*
 * XPluginStop
 * 
 * Our cleanup routine deallocates our window.
 * 
 */
PLUGIN_API void	XPluginStop(void)
{
	XPLMDestroyWindow(gWindow);
}

/*
 * XPluginDisable
 * 
 * We do not need to do anything when we are disabled, but we must provide the handler.
 * 
 */
PLUGIN_API void XPluginDisable(void)
{
}

/*
 * XPluginEnable.
 * 
 * We don't do any enable-specific initialization, but we must return 1 to indicate
 * that we may be enabled at this time.
 * 
 */
PLUGIN_API int XPluginEnable(void)
{
	return 1;
}

/*
 * XPluginReceiveMessage
 * 
 * We don't have to do anything in our receive message handler, but we must provide one.
 * 
 */
PLUGIN_API void XPluginReceiveMessage(
					XPLMPluginID	inFromWho,
					long			inMessage,
					void *			inParam)
{
}

/*
 * MyDrawingWindowCallback
 * 
 * This callback does the work of drawing our window once per sim cycle each time
 * it is needed.  It dynamically changes the text depending on the saved mouse
 * status.  Note that we don't have to tell X-Plane to redraw us when our text
 * changes; we are redrawn by the sim continuously.
 * 
 */
int MyDrawWindowCallback(
                                   XPLMWindowID         inWindowID,    
                                   void *               inRefcon)
{
	int		left, top, right, bottom;
	float		color[] = { 1.0, 1.0, 1.0 }; 	/* RGB White */
	char		tmp[255];

	double		outLatitude, outLongitude, outAltitude;    
	float 		planeX, planeY, planeZ;

	/* First we get the location of the window passed in to us. */
	XPLMGetWindowGeometry(inWindowID, &left, &top, &right, &bottom);
	
	/* We now use an XPLMGraphics routine to draw a translucent dark
	 * rectangle that is our window's shape. */
	XPLMDrawTranslucentDarkBox(left, top, right, bottom);

	/* Finally we draw the text into the window, also using XPLMGraphics
	 * routines.  The NULL indicates no word wrapping. */

        gPlaneX = XPLMFindDataRef("sim/flightmodel/position/local_x");
        gPlaneY = XPLMFindDataRef("sim/flightmodel/position/local_y");
        gPlaneZ = XPLMFindDataRef("sim/flightmodel/position/local_z");

	if (!gPlaneX || !gPlaneY || !gPlaneZ ) return 1;

        planeX = XPLMGetDataf(gPlaneX);
        planeY = XPLMGetDataf(gPlaneY);
        planeZ = XPLMGetDataf(gPlaneZ);             


	XPLMLocalToWorld(planeX, planeY, planeZ, &outLatitude, &outLongitude, &outAltitude);

	sprintf(tmp,"Lat: %f Lon: %f Alt: %f x: %f y: %f z: %f", outLatitude, outLongitude, outAltitude, planeX, planeY, planeZ);
	XPLMDrawString(color, left + 5, top - 20, tmp, NULL, xplmFont_Basic);

	XPLMSetGraphicsState(0, 0, 0, 0, 0, 0, 0);
	glColor3f(1.0, 0.0, 1.0);

        glBegin(GL_LINES);
        glVertex3f(planeX - 100, planeY, planeZ);
        glVertex3f(planeX + 100, planeY, planeZ);
        glVertex3f(planeX, planeY - 100, planeZ);
        glVertex3f(planeX, planeY + 100, planeZ);
        glVertex3f(planeX, planeY, planeZ - 100);
        glVertex3f(planeX, planeY, planeZ + 100);
        glEnd();


		
}                                   

/*
 * MyHandleKeyCallback
 * 
 * Our key handling callback does nothing in this plugin.  This is ok; 
 * we simply don't use keyboard input.
 * 
 */
void MyHandleKeyCallback(
                                   XPLMWindowID         inWindowID,    
                                   char                 inKey,    
                                   XPLMKeyFlags         inFlags,    
                                   char                 inVirtualKey,    
                                   void *               inRefcon,    
                                   int                  losingFocus)
{
}                                   

/*
 * MyHandleMouseClickCallback
 * 
 * Our mouse click callback toggles the status of our mouse variable 
 * as the mouse is clicked.  We then update our text on the next sim 
 * cycle.
 * 
 */
int MyHandleMouseClickCallback(
                                   XPLMWindowID         inWindowID,    
                                   int                  x,    
                                   int                  y,    
                                   XPLMMouseStatus      inMouse,    
                                   void *               inRefcon)
{
	/* If we get a down or up, toggle our status click.  We will
	 * never get a down without an up if we accept the down. */
	if ((inMouse == xplm_MouseDown) || (inMouse == xplm_MouseUp))
		gClicked = 1 - gClicked;
	
	/* Returning 1 tells X-Plane that we 'accepted' the click; otherwise
	 * it would be passed to the next window behind us.  If we accept
	 * the click we get mouse moved and mouse up callbacks, if we don't
	 * we do not get any more callbacks.  It is worth noting that we 
	 * will receive mouse moved and mouse up even if the mouse is dragged
	 * out of our window's box as long as the click started in our window's 
	 * box. */
	return 1;
}


GLuint LoadTextureRAW( const char * filename, int wrap ){
	GLuint		texture;
	int 		width, height;
	unsigned char 	*data;
	FILE 		*file;
	
	// open texture data
	file = fopen( filename, "rb" );
	if ( file == NULL ) return 0;
	
	// allocate buffer
	width = 256;
	height = 256;
	data = ( unsigned char   *) malloc( width * height * 3 );
	
	// read texture data
	fread( data, width * height * 3, 1, file );
	fclose( file );
	
	// allocate a texture name
	glGenTextures( 1, &texture );
	
	// select our current texture
	glBindTexture( GL_TEXTURE_2D, texture );
	
	// select modulate to mix texture with color for shading
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	
	// when texture area is small, bilinear filter the closest MIP map
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
	                 GL_LINEAR_MIPMAP_NEAREST );
	// when texture area is large, bilinear filter the first MIP map
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	
	// if wrap is true, the texture wraps over at the edges (repeat)
	//       ... false, the texture ends at the edges (clamp)
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
	                 wrap ? GL_REPEAT : GL_CLAMP );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
	                 wrap ? GL_REPEAT : GL_CLAMP );
	
	// build our texture MIP maps
	gluBuild2DMipmaps( GL_TEXTURE_2D, 3, width,
	  height, GL_RGB, GL_UNSIGNED_BYTE, data );
	
	// free buffer
	free( data );
	
	return texture;

}

