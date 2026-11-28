#pragma once
// Small OpenGL 1.x ABI subset for the standalone Linux capture tool.
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#if __has_include(<GL/gl.h>) && __has_include(<GL/glx.h>)
#include <GL/gl.h>
#include <GL/glx.h>
#else
using GLenum=unsigned int; using GLuint=unsigned int; using GLsizei=int; using GLint=int;
using GLbitfield=unsigned int; using GLfloat=float; using GLdouble=double; using GLubyte=unsigned char;
using GLXContext=struct __GLXcontextRec*;
extern "C" {
XVisualInfo* glXChooseVisual(Display*,int,int*);
GLXContext glXCreateContext(Display*,XVisualInfo*,GLXContext,Bool);
Bool glXMakeCurrent(Display*,XID,GLXContext); void glXDestroyContext(Display*,GLXContext);
const GLubyte* glGetString(GLenum); GLenum glGetError();
void glViewport(GLint,GLint,GLsizei,GLsizei); void glMatrixMode(GLenum); void glLoadIdentity();
void glOrtho(GLdouble,GLdouble,GLdouble,GLdouble,GLdouble,GLdouble);
void glClearColor(GLfloat,GLfloat,GLfloat,GLfloat); void glClear(GLbitfield);
void glEnable(GLenum); void glDisable(GLenum); void glBlendFunc(GLenum,GLenum); void glScissor(GLint,GLint,GLsizei,GLsizei);
void glGenTextures(GLsizei,GLuint*); void glBindTexture(GLenum,GLuint); void glTexParameteri(GLenum,GLenum,GLint);
void glTexImage2D(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*);
void glPixelStorei(GLenum,GLint); void glBegin(GLenum); void glEnd();
void glColor4ub(GLubyte,GLubyte,GLubyte,GLubyte); void glTexCoord2f(GLfloat,GLfloat); void glVertex2f(GLfloat,GLfloat);
void glDrawBuffer(GLenum); void glReadBuffer(GLenum); void glFinish();
void glReadPixels(GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,void*);
}
constexpr int GLX_RGBA=4,GLX_RED_SIZE=8,GLX_GREEN_SIZE=9,GLX_BLUE_SIZE=10;
constexpr GLenum GL_RENDERER=0x1F01,GL_VERSION=0x1F02,GL_PROJECTION=0x1701,GL_MODELVIEW=0x1700,GL_COLOR_BUFFER_BIT=0x4000;
constexpr GLenum GL_TEXTURE_2D=0x0DE1,GL_BLEND=0x0BE2,GL_ONE=1,GL_ONE_MINUS_SRC_ALPHA=0x0303;
constexpr GLenum GL_SCISSOR_TEST=0x0C11,GL_DITHER=0x0BD0,GL_DEPTH_TEST=0x0B71;
constexpr GLenum GL_TEXTURE_MIN_FILTER=0x2801,GL_TEXTURE_MAG_FILTER=0x2800,GL_TEXTURE_WRAP_S=0x2802,GL_TEXTURE_WRAP_T=0x2803;
constexpr GLenum GL_NEAREST=0x2600,GL_CLAMP_TO_EDGE=0x812F,GL_RGBA=0x1908,GL_RGBA8=0x8058,GL_RGB=0x1907,GL_UNSIGNED_BYTE=0x1401;
constexpr GLenum GL_TRIANGLES=4,GL_UNPACK_ALIGNMENT=0x0CF5,GL_PACK_ALIGNMENT=0x0D05,GL_FRONT=0x0404;
#endif
