#include <GL/freeglut.h>
#include <MSHTML.h>
#include <NuiApi.h>
#include <iostream>
#include <sstream>

INuiSensor* context = NULL;
HANDLE colorStreamHandle = NULL;
HANDLE depthStreamHandle = NULL;
std::string hudText;

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
    
    DWORD nuiFlags = NUI_INITIALIZE_FLAG_USES_SKELETON | NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH;
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
    
    hr = context->NuiImageStreamOpen( NUI_IMAGE_TYPE_DEPTH, NUI_IMAGE_RESOLUTION_640x480, 0, 2, NULL, &depthStreamHandle );
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

void update()
{
    NUI_IMAGE_FRAME colorFrame;
    HRESULT hr = context->NuiImageStreamGetNextFrame( colorStreamHandle, 0, &colorFrame );
    if ( SUCCEEDED(hr) )
    {
        std::stringstream ss;
        ss << "Frame: " << colorFrame.dwFrameNumber << "  "
           << "Time stamp: " << (double)colorFrame.liTimeStamp.QuadPart * 0.001;
        hudText = ss.str();
        context->NuiImageStreamReleaseFrame( colorStreamHandle, &colorFrame );
    }
    glutPostRedisplay();
}

void render()
{
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClear( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT );
    
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho( 0.0, 1.0, 0.0, 1.0, -1.0, 1.0 );
    
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    
    glRasterPos2f( 0.01f, 0.01f );
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    glutBitmapString( GLUT_BITMAP_TIMES_ROMAN_24, (const unsigned char*)hudText.c_str() );
    
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
    glutCreateWindow( "ch2_02_Init_Kinect" );
    glutFullScreen();
    
    glutIdleFunc( update );
    glutDisplayFunc( render );
    glutReshapeFunc( reshape );
    glutKeyboardFunc( keyEvents );
    
    if ( !initializeKinect() ) return 1;
    glutMainLoop();
    destroyKinect();
    return 0;
}
