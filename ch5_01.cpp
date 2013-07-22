#include <GL/freeglut.h>
#include <MSHTML.h>
#include <NuiApi.h>
#include <iostream>
#include <sstream>
#include <vector>
#include "common/TextureManager.h"
#include "common/GLUtilities.h"

INuiSensor* context = NULL;
HANDLE colorStreamHandle = NULL;
HANDLE depthStreamHandle = NULL;
TextureObject* playerDepthTexture = NULL;
const unsigned int backgroundTexID = 1;

NUI_TRANSFORM_SMOOTH_PARAMETERS smoothParams;
GLfloat cursors[6];
GLfloat cursorColors[8];

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

void updateImageFrame( NUI_IMAGE_FRAME& imageFrame )
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
                unsigned char* ptr = playerDepthTexture->bits + 2 * (i * 640 + j);
                if ( NuiDepthPixelToPlayerIndex(bufferWord[j])>0 )
                {
                    *(ptr + 0) = 200;
                    *(ptr + 1) = 40;
                }
                else
                {
                    *(ptr + 0) = 0;
                    *(ptr + 1) = 0;
                }
            }
        }
        
        TextureObject* tobj = playerDepthTexture;
        glBindTexture( GL_TEXTURE_2D, tobj->id );
        glTexImage2D( GL_TEXTURE_2D, 0, tobj->internalFormat, tobj->width, tobj->height,
                      0, tobj->imageFormat, GL_UNSIGNED_BYTE, tobj->bits );
    }
    nuiTexture->UnlockRect( 0 );
}

void updateSkeletonData( NUI_SKELETON_DATA& data )
{
    POINT coordInDepth;
    USHORT depth = 0;
    GLfloat yMin = 0.0f, zMax = 0.0f;
    for ( int i=0; i<NUI_SKELETON_POSITION_COUNT; ++i )
    {
        NuiTransformSkeletonToDepthImage(
            data.SkeletonPositions[i], &coordInDepth.x, &coordInDepth.y,
            &depth, NUI_IMAGE_RESOLUTION_640x480 );
        
        if ( i==NUI_SKELETON_POSITION_SPINE )
        {
            yMin = 1.0f - (GLfloat)coordInDepth.y / 480.0f;
            zMax = (GLfloat)NuiDepthPixelToDepth(depth) * 0.00025f;
        }
        else if ( i==NUI_SKELETON_POSITION_HAND_LEFT )
        {
            cursors[0] = (GLfloat)coordInDepth.x / 640.0f;
            cursors[1] = 1.0f - (GLfloat)coordInDepth.y / 480.0f;
            cursors[2] = (GLfloat)NuiDepthPixelToDepth(depth) * 0.00025f;
        }
        else if ( i==NUI_SKELETON_POSITION_HAND_RIGHT )
        {
            cursors[3] = (GLfloat)coordInDepth.x / 640.0f;
            cursors[4] = 1.0f - (GLfloat)coordInDepth.y / 480.0f;
            cursors[5] = (GLfloat)NuiDepthPixelToDepth(depth) * 0.00025f;
        }
    }
    
    if ( cursors[1]<yMin || zMax<cursors[2] ) cursorColors[3] = 0.2f;
    else cursorColors[3] = 1.0f;
    if ( cursors[4]<yMin || zMax<cursors[5] ) cursorColors[7] = 0.2f;
    else cursorColors[7] = 1.0f;
}

void update()
{
    NUI_IMAGE_FRAME depthFrame;
    HRESULT hr = context->NuiImageStreamGetNextFrame( depthStreamHandle, 0, &depthFrame );
    if ( SUCCEEDED(hr) )
    {
        updateImageFrame( depthFrame );
        context->NuiImageStreamReleaseFrame( depthStreamHandle, &depthFrame );
    }
    
    NUI_SKELETON_FRAME skeletonFrame = {0};
    hr = context->NuiSkeletonGetNextFrame( 0, &skeletonFrame );
    if ( SUCCEEDED(hr) )
    {
        context->NuiTransformSmooth( &skeletonFrame, &smoothParams );
        for ( int n=0; n<NUI_SKELETON_COUNT; ++n )
        {
            NUI_SKELETON_DATA& data = skeletonFrame.SkeletonData[n];
            if ( data.eTrackingState==NUI_SKELETON_TRACKED )
            {
                updateSkeletonData( data );
                break;
            }
        }
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
    
    // Blend with depth quad
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glTranslatef( 0.0f, 0.0f, 0.1f );
    
    glBindTexture( GL_TEXTURE_2D, playerDepthTexture->id );
    drawSimpleMesh( WITH_POSITION|WITH_TEXCOORD, 4, meshData, GL_QUADS );
    
    // Draw cursors
    glDisable( GL_TEXTURE_2D );
    glPointSize( 50.0f );
    
    VertexData cursorData = { &(cursors[0]), NULL, &(cursorColors[0]), NULL };
    drawSimpleMesh( WITH_POSITION|WITH_COLOR, 2, cursorData, GL_POINTS );
    
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
    glutCreateWindow( "ch5_01_Hand_Cursors" );
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
    playerDepthTexture = createTexture(640, 480, GL_LUMINANCE_ALPHA, 2);
    
    smoothParams.fCorrection = 0.5f;
    smoothParams.fJitterRadius = 1.0f;
    smoothParams.fMaxDeviationRadius = 0.5f;
    smoothParams.fPrediction = 0.4f;
    smoothParams.fSmoothing = 0.2f;
    for ( int i=0; i<8; ++i ) cursorColors[i] = 1.0f;
    
    glutMainLoop();
    
    destroyTexture( playerDepthTexture );
    destroyKinect();
    return 0;
}
