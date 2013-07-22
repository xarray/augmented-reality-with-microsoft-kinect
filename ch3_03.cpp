#include <GL/freeglut.h>
#include <MSHTML.h>
#include <NuiApi.h>
#include <iostream>
#include <sstream>
#include "common/TextureManager.h"
#include "common/GLUtilities.h"

INuiSensor* context = NULL;
HANDLE colorStreamHandle = NULL;
HANDLE depthStreamHandle = NULL;
TextureObject* colorTexture = NULL;
TextureObject* playerColorTexture = NULL;

const unsigned int backgroundTexID = 1;

bool initializeKinect()
{
    int numKinects = 0;
    HRESULT hr = NuiGetSensorCount( &numKinects );
    if ( FAILED(hr) || numKinects<=0 )
    {
        std::cout << "No Kinect device found." << std::endl;
        return false;
    }
    
    hr = NuiCreateSensorByIndex( 0, &context );
    if ( FAILED(hr) )
    {
        std::cout << "Failed to connect to Kinect device." << std::endl;
        return false;
    }
    
    DWORD nuiFlags = NUI_INITIALIZE_FLAG_USES_SKELETON | NUI_INITIALIZE_FLAG_USES_COLOR |
                     NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX;
    hr = context->NuiInitialize( nuiFlags );
    if ( FAILED(hr) )
    {
        std::cout << "Failed to intialize Kinect: " << std::hex << (long)hr << std::dec << std::endl;
        return false;
    }
    
    hr = context->NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, 0, 2, NULL, &colorStreamHandle );
    if ( FAILED(hr) )
    {
        std::cout << "Unable to create color stream: " << std::hex << (long)hr << std::dec << std::endl;
        return false;
    }
    
    hr = context->NuiImageStreamOpen( NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, NUI_IMAGE_RESOLUTION_640x480,
                                      0, 2, NULL, &depthStreamHandle );
    if ( FAILED(hr) )
    {
        std::cout << "Unable to create depth stream: " << std::hex << (long)hr << std::dec << std::endl;
        return false;
    }
    
    hr = context->NuiSkeletonTrackingEnable( NULL, 0 );
    if ( FAILED(hr) )
    {
        std::cout << "Unable to start tracking skeleton." << std::endl;
        return false;
    }
    return true;
}

bool destroyKinect()
{
    if ( context )
    {
        context->NuiShutdown();
        return true;
    }
    return false;
}

bool setPlayerColorPixel( const USHORT depthValue, int x, int y, unsigned char alpha )
{
    unsigned char* ptr = playerColorTexture->bits + 4 * (y * 640 + x);
    if ( NuiDepthPixelToPlayerIndex(depthValue)>0 )
    {
        LONG colorX = 0, colorY = 0;
        context->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(
            NUI_IMAGE_RESOLUTION_640x480, NUI_IMAGE_RESOLUTION_640x480, NULL,
            x, y, depthValue, &colorX, &colorY );
        if ( colorX>=640 || colorY>=480 ) return false;
        
        unsigned char* colorPtr = colorTexture->bits + 3 * (colorY * 640 + colorX);
        *(ptr + 0) = *(colorPtr + 0);
        *(ptr + 1) = *(colorPtr + 1);
        *(ptr + 2) = *(colorPtr + 2);
        *(ptr + 3) = alpha;
    }
    else
    {
        *(ptr + 0) = 0;
        *(ptr + 1) = 0;
        *(ptr + 2) = 0;
        *(ptr + 3) = 0;
    }
    return true;
}

void updateImageFrame( NUI_IMAGE_FRAME& imageFrame, bool isDepthFrame )
{
    INuiFrameTexture* nuiTexture = imageFrame.pFrameTexture;
    NUI_LOCKED_RECT lockedRect;
    nuiTexture->LockRect( 0, &lockedRect, NULL, 0 );
    if ( lockedRect.Pitch!=NULL )
    {
        const BYTE* buffer = (const BYTE*)lockedRect.pBits;
        for ( int i=0; i<480; ++i )
        {
            const BYTE* line = buffer + i * lockedRect.Pitch;
            const USHORT* bufferWord = (const USHORT*)line;
            for ( int j=0; j<640; ++j )
            {
                if ( !isDepthFrame )
                {
                    unsigned char* ptr = colorTexture->bits + 3 * (i * 640 + j);
                    *(ptr + 0) = line[4 * j + 2];
                    *(ptr + 1) = line[4 * j + 1];
                    *(ptr + 2) = line[4 * j + 0];
                }
                else
                    setPlayerColorPixel( bufferWord[j], j, i, 255 );
            }
        }
        
        TextureObject* tobj = (isDepthFrame ? playerColorTexture : colorTexture);
        glBindTexture( GL_TEXTURE_2D, tobj->id );
        glTexImage2D( GL_TEXTURE_2D, 0, tobj->internalFormat, tobj->width, tobj->height,
                      0, tobj->imageFormat, GL_UNSIGNED_BYTE, tobj->bits );
    }
    nuiTexture->UnlockRect( 0 );
}

void update()
{
    NUI_IMAGE_FRAME colorFrame;
    HRESULT hr = context->NuiImageStreamGetNextFrame( colorStreamHandle, 0, &colorFrame );
    if ( SUCCEEDED(hr) )
    {
        updateImageFrame( colorFrame, false );
        context->NuiImageStreamReleaseFrame( colorStreamHandle, &colorFrame );
    }
    
    NUI_IMAGE_FRAME depthFrame;
    hr = context->NuiImageStreamGetNextFrame( depthStreamHandle, 0, &depthFrame );
    if ( SUCCEEDED(hr) )
    {
        updateImageFrame( depthFrame, true );
        context->NuiImageStreamReleaseFrame( depthStreamHandle, &depthFrame );
    }
    glutPostRedisplay();
}

void render()
{
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClear( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT );
    glEnable( GL_TEXTURE_2D );
    
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho( 0.0, 1.0, 0.0, 1.0, -1.0, 1.0 );
    
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    
    // Draw background quad
    GLfloat vertices[][3] = {
        { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f },
        { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }
    };
    GLfloat texcoords[][2] = {
        {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}
    };
    VertexData meshData = { &(vertices[0][0]), NULL, NULL, &(texcoords[0][0]) };
    
    TextureManager::Inst()->BindTexture( backgroundTexID );
    drawSimpleMesh( WITH_POSITION|WITH_TEXCOORD, 4, meshData, GL_QUADS );
    
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glTranslatef( 0.0f, 0.0f, 0.1f );
    
    glBindTexture( GL_TEXTURE_2D, playerColorTexture->id );
    drawSimpleMesh( WITH_POSITION|WITH_TEXCOORD, 4, meshData, GL_QUADS );
    
    glDisable( GL_BLEND );
    glutSwapBuffers();
}

void reshape( int w, int h )
{
    glViewport( 0, 0, w, h );
}

void keyEvents( unsigned char key, int x, int y )
{
    switch ( key )
    {
    case 27: case 'Q': case 'q':
        glutLeaveMainLoop();
        return;
    }
    glutPostRedisplay();
}

int main( int argc, char** argv )
{
    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_RGB|GLUT_DOUBLE|GLUT_DEPTH|GLUT_MULTISAMPLE );
    glutCreateWindow( "ch3_03_Magic_Photographer" );
    glutFullScreen();
    
    glutIdleFunc( update );
    glutDisplayFunc( render );
    glutReshapeFunc( reshape );
    glutKeyboardFunc( keyEvents );
    
    if ( TextureManager::Inst()->LoadTexture("background.bmp", backgroundTexID, GL_BGR_EXT) )
    {
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    }
    
    if ( !initializeKinect() ) return 1;
    colorTexture = createTexture(640, 480, GL_RGB, 3);
    playerColorTexture = createTexture(640, 480, GL_RGBA, 4);
    
    glutMainLoop();
    
    destroyTexture( colorTexture );
    destroyTexture( playerColorTexture );
    destroyKinect();
    return 0;
}
