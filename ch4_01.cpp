#include <GL/freeglut.h>
#include <MSHTML.h>
#include <NuiApi.h>
#include <iostream>
#include <sstream>
#include "common/GLUtilities.h"

INuiSensor* context = NULL;
HANDLE colorStreamHandle = NULL;
HANDLE depthStreamHandle = NULL;
TextureObject* colorTexture = NULL;

GLfloat skeletonVertices[NUI_SKELETON_POSITION_COUNT][3];
GLuint skeletonIndices[38] = {
    NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_SPINE,
    NUI_SKELETON_POSITION_SPINE, NUI_SKELETON_POSITION_SHOULDER_CENTER,
    NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_HEAD,
    // Left arm
    NUI_SKELETON_POSITION_SHOULDER_LEFT, NUI_SKELETON_POSITION_ELBOW_LEFT,
    NUI_SKELETON_POSITION_ELBOW_LEFT, NUI_SKELETON_POSITION_WRIST_LEFT,
    NUI_SKELETON_POSITION_WRIST_LEFT, NUI_SKELETON_POSITION_HAND_LEFT,
    // Right arm
    NUI_SKELETON_POSITION_SHOULDER_RIGHT, NUI_SKELETON_POSITION_ELBOW_RIGHT,
    NUI_SKELETON_POSITION_ELBOW_RIGHT, NUI_SKELETON_POSITION_WRIST_RIGHT,
    NUI_SKELETON_POSITION_WRIST_RIGHT, NUI_SKELETON_POSITION_HAND_RIGHT,
    // Left leg
    NUI_SKELETON_POSITION_HIP_LEFT, NUI_SKELETON_POSITION_KNEE_LEFT,
    NUI_SKELETON_POSITION_KNEE_LEFT, NUI_SKELETON_POSITION_ANKLE_LEFT,
    NUI_SKELETON_POSITION_ANKLE_LEFT, NUI_SKELETON_POSITION_FOOT_LEFT,
    // Right leg
    NUI_SKELETON_POSITION_HIP_RIGHT, NUI_SKELETON_POSITION_KNEE_RIGHT,
    NUI_SKELETON_POSITION_KNEE_RIGHT, NUI_SKELETON_POSITION_ANKLE_RIGHT,
    NUI_SKELETON_POSITION_ANKLE_RIGHT, NUI_SKELETON_POSITION_FOOT_RIGHT,
    // Others
    NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_LEFT,
    NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_RIGHT,
    NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_LEFT,
    NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_RIGHT
};

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
                unsigned char* ptr = colorTexture->bits + 3 * (i * 640 + j);
                *(ptr + 0) = line[4 * j + 2];
                *(ptr + 1) = line[4 * j + 1];
                *(ptr + 2) = line[4 * j + 0];
            }
        }
        
        TextureObject* tobj = colorTexture;
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
    for ( int i=0; i<NUI_SKELETON_POSITION_COUNT; ++i )
    {
        NuiTransformSkeletonToDepthImage(
            data.SkeletonPositions[i], &coordInDepth.x, &coordInDepth.y,
            &depth, NUI_IMAGE_RESOLUTION_640x480 );
        skeletonVertices[i][0] = (GLfloat)coordInDepth.x / 640.0f;           // 0.0 - 1.0
        skeletonVertices[i][1] = 1.0f - (GLfloat)coordInDepth.y / 480.0f;    // 1.0 - 0.0
        
        // Transform depth to [0, 1], assuming original depth from 0 to 4000 in millmeters
        skeletonVertices[i][2] = (GLfloat)NuiDepthPixelToDepth(depth) * 0.00025f;
    }
}

void update()
{
    NUI_IMAGE_FRAME colorFrame;
    HRESULT hr = context->NuiImageStreamGetNextFrame( colorStreamHandle, 0, &colorFrame );
    if ( SUCCEEDED(hr) )
    {
        updateImageFrame( colorFrame );
        context->NuiImageStreamReleaseFrame( colorStreamHandle, &colorFrame );
    }
    
    NUI_SKELETON_FRAME skeletonFrame = {0};
    hr = context->NuiSkeletonGetNextFrame( 0, &skeletonFrame );
    if ( SUCCEEDED(hr) )
    {
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
    
    // Draw color quad
    GLfloat vertices[][3] = {
        { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f },
        { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }
    };
    GLfloat texcoords[][2] = {
        {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}
    };
    VertexData meshData = { &(vertices[0][0]), NULL, NULL, &(texcoords[0][0]) };
    
    glBindTexture( GL_TEXTURE_2D, colorTexture->id );
    drawSimpleMesh( WITH_POSITION|WITH_TEXCOORD, 4, meshData, GL_QUADS );
    
    // Draw skeleton
    glDisable( GL_TEXTURE_2D );
    glLineWidth( 5.0f );
    
    VertexData skeletonData = { &(skeletonVertices[0][0]), NULL, NULL, NULL };
    drawIndexedMesh( WITH_POSITION, NUI_SKELETON_POSITION_COUNT, skeletonData,
                    GL_LINES, 38, skeletonIndices );
    
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
    glutCreateWindow( "ch4_01_Skeleton_Drawer" );
    glutFullScreen();
    
    glutIdleFunc( update );
    glutDisplayFunc( render );
    glutReshapeFunc( reshape );
    glutKeyboardFunc( keyEvents );
    
    if ( !initializeKinect() ) return 1;
    colorTexture = createTexture(640, 480, GL_RGB, 3);
    
    glutMainLoop();
    
    destroyTexture( colorTexture );
    destroyKinect();
    return 0;
}
