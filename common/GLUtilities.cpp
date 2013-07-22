#include "GLUtilities.h"

#ifndef GL_ARB_vertex_shader
#define GL_VERTEX_SHADER_ARB 0x8B31
#endif

#ifndef GL_ARB_fragment_shader
#define GL_FRAGMENT_SHADER_ARB 0x8B30
#endif

#ifndef GL_VERSION_2_0
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#endif

typedef GLuint (APIENTRY *CreateProgramProc)();
typedef void (APIENTRY *LinkProgramProc)( GLuint program );
typedef void (APIENTRY *UseProgramProc)( GLuint program );
typedef void (APIENTRY *GetProgramivProc)( GLuint program, GLenum pname, GLint* params );
typedef GLuint (APIENTRY *CreateShaderProc)( GLenum type );
typedef void (APIENTRY *ShaderSourceProc)( GLuint shader, GLsizei count, const char** string, const GLint* length );
typedef void (APIENTRY *CompileShaderProc)( GLuint shader );
typedef void (APIENTRY *GetShaderivProc)( GLuint shader, GLenum pname, GLint* params );
typedef GLint (APIENTRY *GetUniformLocationProc)( GLuint program, const char* name );
typedef void (APIENTRY *Uniform1iProc)( GLint location, GLint v0 );
typedef void (APIENTRY *AttachShaderProc)( GLuint program, GLuint shader );

CreateProgramProc      glCreateProgram;
LinkProgramProc        glLinkProgram;
UseProgramProc         glUseProgram;
GetProgramivProc       glGetProgram;
CreateShaderProc       glCreateShader;
ShaderSourceProc       glShaderSource;
CompileShaderProc      glCompileShader;
GetShaderivProc        glGetShader;
GetUniformLocationProc glGetUniformLocation;
Uniform1iProc          glUniform1i;
AttachShaderProc       glAttachObject;

bool g_programInitialized = false;
static void initializeProgram()
{
    glCreateProgram = (CreateProgramProc)wglGetProcAddress("glCreateProgramObjectARB");
    glLinkProgram = (LinkProgramProc)wglGetProcAddress("glLinkProgramARB");
    glUseProgram = (UseProgramProc)wglGetProcAddress("glUseProgramObjectARB");
    glGetProgram = (GetProgramivProc)wglGetProcAddress("glGetProgramiv");
    glCreateShader = (CreateShaderProc)wglGetProcAddress("glCreateShaderObjectARB");
    glShaderSource = (ShaderSourceProc)wglGetProcAddress("glShaderSourceARB");
    glCompileShader = (CompileShaderProc)wglGetProcAddress("glCompileShaderARB");
    glGetShader = (GetShaderivProc)wglGetProcAddress("glGetShaderiv");
    glGetUniformLocation = (GetUniformLocationProc)wglGetProcAddress("glGetUniformLocationARB");
    glUniform1i = (Uniform1iProc)wglGetProcAddress("glUniform1iARB");
    glAttachObject = (AttachShaderProc)wglGetProcAddress("glAttachObjectARB");
    g_programInitialized = true;
}

void drawSimpleMesh( int formats, unsigned int numVertices, VertexData data,
                     GLenum drawMode, GLuint start, GLuint count )
{
    if ( formats&WITH_NORMAL )
    {
        glEnableClientState( GL_NORMAL_ARRAY );
        glNormalPointer( GL_FLOAT, 0, data.normals );
    }
    if ( formats&WITH_COLOR )
    {
        glEnableClientState( GL_COLOR_ARRAY );
        glColorPointer( 4, GL_FLOAT, 0, data.colors );
    }
    if ( formats&WITH_TEXCOORD )
    {
        glEnableClientState( GL_TEXTURE_COORD_ARRAY );
        glTexCoordPointer( 2, GL_FLOAT, 0, data.texcoords );
    }
    if ( formats&WITH_POSITION )
    {
        glEnableClientState( GL_VERTEX_ARRAY );
        glVertexPointer( 3, GL_FLOAT, 0, data.vertices );
    }
    
    glDrawArrays( drawMode, start, count>0 ? count : numVertices );
    
    if ( formats&WITH_NORMAL ) glDisableClientState( GL_NORMAL_ARRAY );
    if ( formats&WITH_COLOR ) glDisableClientState( GL_COLOR_ARRAY );
    if ( formats&WITH_TEXCOORD ) glDisableClientState( GL_TEXTURE_COORD_ARRAY );
    if ( formats&WITH_POSITION ) glDisableClientState( GL_VERTEX_ARRAY );
}

void drawIndexedMesh( int formats, unsigned int numVertices, VertexData data,
                      GLenum drawMode, unsigned int numIndices, GLuint* indices )
{
    if ( formats&WITH_POSITION )
    {
        glEnableClientState( GL_VERTEX_ARRAY );
        glVertexPointer( 3, GL_FLOAT, 0, data.vertices );
    }
    if ( formats&WITH_NORMAL )
    {
        glEnableClientState( GL_NORMAL_ARRAY );
        glVertexPointer( 3, GL_FLOAT, 0, data.normals );
    }
    if ( formats&WITH_COLOR )
    {
        glEnableClientState( GL_COLOR_ARRAY );
        glVertexPointer( 4, GL_FLOAT, 0, data.colors );
    }
    if ( formats&WITH_TEXCOORD )
    {
        glEnableClientState( GL_TEXTURE_COORD_ARRAY );
        glTexCoordPointer( 2, GL_FLOAT, 0, data.texcoords );
    }
    
    glDrawElements( drawMode, numIndices, GL_UNSIGNED_INT, indices );
    
    if ( formats&WITH_POSITION ) glDisableClientState( GL_VERTEX_ARRAY );
    if ( formats&WITH_NORMAL ) glDisableClientState( GL_NORMAL_ARRAY );
    if ( formats&WITH_COLOR ) glDisableClientState( GL_COLOR_ARRAY );
    if ( formats&WITH_TEXCOORD ) glDisableClientState( GL_TEXTURE_COORD_ARRAY );
}

TextureObject* createTexture( unsigned int w, unsigned int h, GLenum image_format,
                              GLint internal_format, GLenum data_type )
{
    TextureObject* tobj = new TextureObject;
    tobj->width = w;
    tobj->height = h;
    tobj->imageFormat = image_format;
    tobj->internalFormat = internal_format;
    
    unsigned int numComponents = 1;
    switch ( image_format )
    {
    case GL_LUMINANCE_ALPHA: numComponents = 2; break;
    case GL_RGB: case GL_BGR_EXT: numComponents = 3; break;
    case GL_RGBA: case GL_BGRA_EXT: numComponents = 4; break;
    default: break;
    }
    tobj->bits = new unsigned char[w * h * numComponents];
    
    glGenTextures( 1, &(tobj->id) );
    glBindTexture( GL_TEXTURE_2D, tobj->id );
    glTexImage2D( GL_TEXTURE_2D, 0, internal_format, w, h, 0, image_format, data_type, tobj->bits );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    return tobj;
}

void destroyTexture( TextureObject* tobj )
{
    if ( tobj )
    {
        glDeleteTextures( 1, &(tobj->id) );
        delete tobj->bits;
        delete tobj;
    }
}

GLuint createProgram( ProgramType type )
{
    if ( !g_programInitialized ) initializeProgram();
    GLuint programID = glCreateProgram();
    GLuint vertexShaderID = glCreateShader( GL_VERTEX_SHADER_ARB );
    GLuint fragmentShaderID = glCreateShader( GL_FRAGMENT_SHADER_ARB );
    
    GLint canCompile = 0;
    switch ( type )
    {
    case BLUR_PROGRAM:
        {
            static const char* vertexCode = {
                "void main() {\n"
                "    gl_Position = ftransform();\n"
                "    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
                "}\n"
            };
            glShaderSource( vertexShaderID, 1, &vertexCode, NULL );
            glCompileShader( vertexShaderID );
            glGetShader( vertexShaderID, GL_COMPILE_STATUS, &canCompile );
            if ( canCompile==GL_FALSE ) return 0;
            
            static const char* fragmentCode = {
                "uniform sampler2D defaultTex;\n"
                "void main() {\n"
                "    vec2 uv = gl_TexCoord[0].st;\n"
                "    vec2 off0 = vec2(1.0/gl_FragCoord.x, 0.0);\n"
                "    vec2 off1 = vec2(0.0, 1.0/gl_FragCoord.y);\n"
                "    vec2 off2 = vec2(1.0/gl_FragCoord.x, 1.0/gl_FragCoord.y);\n"
                "    vec2 off3 = vec2(1.0/gl_FragCoord.x,-1.0/gl_FragCoord.y);\n"
                
                "    vec4 c11 = texture2D(defaultTex, uv);\n"
                "    vec4 c01 = texture2D(defaultTex, uv - off0);\n"
                "    vec4 c21 = texture2D(defaultTex, uv + off0);\n"
                "    vec4 c10 = texture2D(defaultTex, uv - off1);\n"
                "    vec4 c12 = texture2D(defaultTex, uv + off1);\n"
                "    vec4 c00 = texture2D(defaultTex, uv - off2);\n"
                "    vec4 c22 = texture2D(defaultTex, uv + off2);\n"
                "    vec4 c20 = texture2D(defaultTex, uv - off3);\n"
                "    vec4 c02 = texture2D(defaultTex, uv + off3);\n"
                
                "    vec4 target = 4.0 * c11 + 2.0 * (c01 + c21 + c10 + c12)\n"
                "                + (c00 + c02 + c20 + c22);\n"
                "    gl_FragColor = target / 16.0;\n"
                "}\n"
            };
            glShaderSource( fragmentShaderID, 1, &fragmentCode, NULL );
            glCompileShader( fragmentShaderID );
            glGetShader( fragmentShaderID, GL_COMPILE_STATUS, &canCompile );
            if ( canCompile==GL_FALSE ) return 0;
        }
        break;
    default: break;
    }
    
    glAttachObject( programID, vertexShaderID );
    glAttachObject( programID, fragmentShaderID );
    glLinkProgram( programID );
    
    glGetProgram( programID, GL_LINK_STATUS, &canCompile );
    if ( canCompile==GL_FALSE ) return 0;
    return programID;
}

void useProgram( GLuint id )
{
    if ( !g_programInitialized ) initializeProgram();
    glUseProgram( id );
    
    if ( id!=0 )
    {
        GLint loc = glGetUniformLocation( id, "defaultTex" );
        glUniform1i( loc, 0 );
    }
}
