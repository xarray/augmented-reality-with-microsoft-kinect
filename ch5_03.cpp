#include <GL/freeglut.h>
#include <MSHTML.h>
#include <NuiApi.h>
#include <iostream>
#include <sstream>
#include <vector>

INuiSensor* context = NULL;
HANDLE colorStreamHandle = NULL;
HANDLE depthStreamHandle = NULL;

NUI_TRANSFORM_SMOOTH_PARAMETERS smoothParams;
GLfloat cursors[6];
GLfloat lastCursors[6];
unsigned int holdGestureCount[2] = {0};
unsigned int swipeGestureCount[2] = {0};

bool isMouseDown = false;

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

void guessGesture( unsigned int index, bool inRange )
{
    float distance = sqrt(powf(cursors[index*3]-lastCursors[index*3], 2.0f)
                        + powf(cursors[1+index*3]-lastCursors[1+index*3], 2.0f));
    if ( distance<0.02 )
    {
        holdGestureCount[index]++;
        swipeGestureCount[index] = 0;
    }
    else if ( distance>0.05 )
    {
        holdGestureCount[index] = 0;
        swipeGestureCount[index]++;
    }
    else
    {
        holdGestureCount[index] = 0;
        swipeGestureCount[index] = 0;
    }
}

void updateSkeletonData( NUI_SKELETON_DATA& data )
{
    POINT coordInDepth;
    USHORT depth = 0;
    GLfloat yMin = 0.0f, zMax = 0.0f;
    for ( int i=0; i<6; ++i ) lastCursors[i] = cursors[i];
    
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
    
    guessGesture( 0, (yMin<cursors[1] && cursors[2]<zMax) );
    guessGesture( 1, (yMin<cursors[4] && cursors[5]<zMax) );
    
    // Send left hand cursor as Windows event
    if ( cursors[2]<zMax )
    {
        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dx = (LONG)(65535 * cursors[0]);
        input.mi.dy = (LONG)(65535 * (1.0 - cursors[1]));
        input.mi.dwExtraInfo = GetMessageExtraInfo();
        
        if ( holdGestureCount[0]>30 )
        {
            if ( !isMouseDown )
                input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN;
            else
                input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP;
            isMouseDown = !isMouseDown;
            holdGestureCount[0] = 0;
        }
        else if ( swipeGestureCount[0]>1 )
        {
            input.mi.dwFlags = MOUSEEVENTF_WHEEL;
            input.mi.mouseData = WHEEL_DELTA;
        }
        else
            input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
        SendInput( 1, &input, sizeof(INPUT) );
    }
}

void update()
{
    NUI_SKELETON_FRAME skeletonFrame = {0};
    HRESULT hr = context->NuiSkeletonGetNextFrame( 0, &skeletonFrame );
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
    glutCreateWindow( "ch5_03_Emulating_Mouse" );
    //glutFullScreen();
    
    glutIdleFunc( update );
    glutDisplayFunc( render );
    glutReshapeFunc( reshape );
    glutKeyboardFunc( keyEvents );
    
    if ( !initializeKinect() ) return 1;
    
    smoothParams.fCorrection = 0.5f;
    smoothParams.fJitterRadius = 1.0f;
    smoothParams.fMaxDeviationRadius = 0.5f;
    smoothParams.fPrediction = 0.4f;
    smoothParams.fSmoothing = 0.2f;
    
    glutMainLoop();
    
    destroyKinect();
    return 0;
}
