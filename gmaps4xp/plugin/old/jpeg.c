/*
 * jpeg.c -- jpeg texture loader
 * last modification: aug. 14, 2007
 *
 * Copyright (c) 2005-2007 David HENRY
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * gcc -Wall -ansi -lGL -lGLU -lglut -ljpeg jpeg.c -o jpeg
 */

#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <jpeglib.h>

/* Microsoft Visual C++ */
#ifdef _MSC_VER
#pragma comment (lib, "libjpeg.lib")
#endif	/* _MSC_VER */

/* OpenGL texture info */
struct gl_texture_t
{
  GLsizei width;
  GLsizei height;

  GLenum format;
  GLint internalFormat;
  GLuint id;

  GLubyte *texels;
};

/* Texture id for the demo */
GLuint texId;


static struct gl_texture_t *
ReadJPEGFromFile (const char *filename)
{
  struct gl_texture_t *texinfo = NULL;
  FILE *fp = NULL;
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPROW j;
  int i;

  /* Open image file */
  fp = fopen (filename, "rb");
  if (!fp)
    {
      fprintf (stderr, "error: couldn't open \"%s\"!\n", filename);
      return NULL;
    }

  /* Create and configure decompressor */
  jpeg_create_decompress (&cinfo);
  cinfo.err = jpeg_std_error (&jerr);
  jpeg_stdio_src (&cinfo, fp);

  /*
   * NOTE: this is the simplest "readJpegFile" function. There
   * is no advanced error handling.  It would be a good idea to
   * setup an error manager with a setjmp/longjmp mechanism.
   * In this function, if an error occurs during reading the JPEG
   * file, the libjpeg abords the program.
   * See jpeg_mem.c (or RTFM) for an advanced error handling which
   * prevent this kind of behavior (http://tfc.duke.free.fr)
   */

  /* Read header and prepare for decompression */
  jpeg_read_header (&cinfo, TRUE);
  jpeg_start_decompress (&cinfo);

  /* Initialize image's member variables */
  texinfo = (struct gl_texture_t *)
    malloc (sizeof (struct gl_texture_t));
  texinfo->width = cinfo.image_width;
  texinfo->height = cinfo.image_height;
  texinfo->internalFormat = cinfo.num_components;

  if (cinfo.num_components == 1)
    texinfo->format = GL_LUMINANCE;
  else
    texinfo->format = GL_RGB;

  texinfo->texels = (GLubyte *)malloc (sizeof (GLubyte) * texinfo->width
			       * texinfo->height * texinfo->internalFormat);

  /* Extract each scanline of the image */
  for (i = 0; i < texinfo->height; ++i)
    {
      j = (texinfo->texels +
	((texinfo->height - (i + 1)) * texinfo->width * texinfo->internalFormat));
      jpeg_read_scanlines (&cinfo, &j, 1);
    }

  /* Finish decompression and release memory */
  jpeg_finish_decompress (&cinfo);
  jpeg_destroy_decompress (&cinfo);

  fclose (fp);
  return texinfo;
}

GLuint
loadJPEGTexture (const char *filename)
{
  struct gl_texture_t *jpeg_tex = NULL;
  GLuint tex_id = 0;

  jpeg_tex = ReadJPEGFromFile (filename);

  if (jpeg_tex && jpeg_tex->texels)
    {
      /* Generate texture */
      glGenTextures (1, &jpeg_tex->id);
      glBindTexture (GL_TEXTURE_2D, jpeg_tex->id);

      /* Setup some parameters for texture filters and mipmapping */
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#if 0
      glTexImage2D (GL_TEXTURE_2D, 0, jpeg_tex->internalFormat,
		    jpeg_tex->width, jpeg_tex->height, 0, jpeg_tex->format,
		    GL_UNSIGNED_BYTE, jpeg_tex->texels);
#else
      gluBuild2DMipmaps (GL_TEXTURE_2D, jpeg_tex->internalFormat,
			 jpeg_tex->width, jpeg_tex->height,
			 jpeg_tex->format, GL_UNSIGNED_BYTE, jpeg_tex->texels);
#endif

      tex_id = jpeg_tex->id;

      /* OpenGL has its own copy of texture data */
      free (jpeg_tex->texels);
      free (jpeg_tex);
    }

  return tex_id;
}

static void
cleanup ()
{
  glDeleteTextures (1, &texId);
}

static void
init (const char *filename)
{
  /* Initialize OpenGL */
  glClearColor (0.5f, 0.5f, 0.5f, 1.0f);
  glShadeModel (GL_SMOOTH);

  glEnable (GL_DEPTH_TEST);

  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  /* Load JPEG texture from file */
  texId = loadJPEGTexture (filename);
  if (!texId)
    exit (EXIT_FAILURE);
}

static void
reshape (int w, int h)
{
  if (h == 0)
    h = 1;

  glViewport (0, 0, (GLsizei)w, (GLsizei)h);

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  gluPerspective (45.0, w/(GLdouble)h, 0.1, 1000.0);

  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();

  glutPostRedisplay ();
}

static void
display ()
{
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity ();

  glEnable (GL_TEXTURE_2D);
  glBindTexture (GL_TEXTURE_2D, texId);

  /* Draw textured quad */
  glTranslatef (0.0, 0.0, -5.0);
  glBegin (GL_QUADS);
    glTexCoord2f (0.0f, 0.0f);
    glVertex3f (-1.0f, -1.0f, 0.0f);

    glTexCoord2f (1.0f, 0.0f);
    glVertex3f (1.0f, -1.0f, 0.0f);

    glTexCoord2f (1.0f, 1.0f);
    glVertex3f (1.0f, 1.0f, 0.0f);

    glTexCoord2f (0.0f, 1.0f);
    glVertex3f (-1.0f, 1.0f, 0.0f);
  glEnd  ();

  glDisable (GL_TEXTURE_2D);

  glutSwapBuffers ();
}

static void
keyboard (unsigned char key, int x, int y)
{
  /* Escape */
  if (key == 27)
    exit (0);
}

int
main (int argc, char *argv[])
{
  if (argc < 2)
    {
      fprintf (stderr, "usage: %s <filename.jpg>\n", argv[0]);
      return -1;
    }

  glutInit (&argc, argv);
  glutInitDisplayMode (GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowSize (640, 480);
  glutCreateWindow ("JPEG Texture Demo");

  atexit (cleanup);
  init (argv[1]);

  glutReshapeFunc (reshape);
  glutDisplayFunc (display);
  glutKeyboardFunc (keyboard);

  glutMainLoop ();

  return 0;
}
