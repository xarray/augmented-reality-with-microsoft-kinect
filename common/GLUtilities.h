#ifndef GL_UTILITIES_H
#define GL_UTILITIES_H

#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>

enum VertexFormat
{
    WITH_POSITION = 0x1,
    WITH_NORMAL = 0x2,
    WITH_COLOR = 0x4,
    WITH_TEXCOORD =0x8
};

struct VertexData
{
    GLfloat* vertices;
    GLfloat* normals;
    GLfloat* colors;
    GLfloat* texcoords;
};

extern void drawSimpleMesh( int formats, unsigned int numVertices, VertexData data,
                            GLenum drawMode, GLuint start=0, GLuint count=0 );
extern void drawIndexedMesh( int formats, unsigned int numVertices, VertexData data,
                             GLenum drawMode, unsigned int numIndices, GLuint* indices );

struct TextureObject
{
    GLuint id;
    GLenum imageFormat;
    GLint internalFormat;
    unsigned int width;
    unsigned int height;
    unsigned char* bits;
};

extern TextureObject* createTexture( unsigned int w, unsigned int h, GLenum image_format=GL_RGB,
                                     GLint internal_format=GL_RGB, GLenum data_type=GL_UNSIGNED_BYTE );
extern void destroyTexture( TextureObject* tobj );

enum ProgramType
{
    NO_PROGRAM = 0,
    BLUR_PROGRAM
};

extern GLuint createProgram( ProgramType type );
extern void useProgram( GLuint id );

#endif
