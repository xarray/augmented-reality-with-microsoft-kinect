#include <GL/freeglut.h>
#include <iostream>

const unsigned int backgroundTexID = 1;

void update()
{
    glutPostRedisplay();
}

void render()
{
    glClear( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT );
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
    glutCreateWindow( "ch2_01_OpenGL_Env" );
    glutFullScreen();
    
    glutIdleFunc( update );
    glutDisplayFunc( render );
    glutReshapeFunc( reshape );
    glutKeyboardFunc( keyEvents );
    
    glutMainLoop();
    return 0;
}
