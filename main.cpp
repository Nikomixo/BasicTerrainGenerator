#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <ctype.h>
#include <ctime>

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef WIN32
#include <windows.h>
#pragma warning(disable:4996)
#endif

#include "glew.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include "glut.h"


// title of these windows:

const char *WINDOWTITLE = "Basic Terrain Generator";
const char *GLUITITLE   = "User Interface Window";

// what the glui package defines as true and false:

const int GLUITRUE  = true;
const int GLUIFALSE = false;

// the escape key:

const int ESCAPE = 0x1b;

// initial window size:

const int INIT_WINDOW_SIZE = 900;

// multiplication factors for input interaction:
//  (these are known from previous experience)

const float ANGFACT = 1.f;
const float SCLFACT = 0.005f;

// minimum allowable scale factor:

const float MINSCALE = 0.05f;

// scroll wheel button values:

const int SCROLL_WHEEL_UP   = 3;
const int SCROLL_WHEEL_DOWN = 4;

// equivalent mouse movement when we click the scroll wheel:

const float SCROLL_WHEEL_CLICK_FACTOR = 5.f;

// active mouse buttons (or them together):

const int LEFT   = 4;
const int MIDDLE = 2;
const int RIGHT  = 1;

// which projection:

enum Projections
{
	ORTHO,
	PERSP
};

// which button:

enum ButtonVals
{
	GENERATE,
	RESET,
	QUIT
};

// window background color (rgba):

const GLfloat BACKCOLOR[ ] = { 0., 0., 0., 1. };

// line width for the axes:

const GLfloat AXES_WIDTH   = 3.;

// the color numbers:
// this order must match the radio button order, which must match the order of the color names,
// 	which must match the order of the color RGB values

// fog parameters:

const GLfloat FOGCOLOR[4] = { .0f, .0f, .0f, 1.f };
const GLenum  FOGMODE     = GL_LINEAR;
const GLfloat FOGDENSITY  = 0.30f;
const GLfloat FOGSTART    = 1.5f;
const GLfloat FOGEND      = 4.f;


// non-constant global variables:

int		ActiveButton;			// current button that is down
GLuint	AxesList;				// list to hold the axes
int		AxesOn;					// != 0 means to draw the axes
int		DebugOn;				// != 0 means to print debugging info
int		MainWindow;				// window id for main graphics window
float	Scale;					// scaling factor
int		ShadowsOn;				// != 0 means to turn shadows on
int		WhichProjection;		// ORTHO or PERSP
int		Xmouse, Ymouse;			// mouse values
float	Xrot, Yrot;				// rotation angles in degrees

// can (will) be used as a Vec2 if z is ignored
struct Point {
	float x;
	float y;
	float z = 0;
};

struct Tree {
	Point lightCoord;
	float r;
	float g;
	float b;
};

Tree trees[7];

// constants



// original permutation set Ken Perlin used from https://cs.nyu.edu/~perlin/noise/, but repeated twice to avoid overflow
float permutation[256 * 2];
const float		perlinZ = 0.8;
const float		perlinFactor = 16;

// big tree heights
const float		treeHeight = 2.5;
float			canopySize = 0.5;

// w/h of perlin noise array
const int	worldSize  = 256;
const float squareSize = 0.15;
const float maxHeight = 5.;

Point	worldMap[worldSize][worldSize];

GLuint	mapListLines;
GLuint	mapListGround;
GLuint	treeList;

int		groundOn;				// != 0 means turn ground on
int		linesOn;				// != 0 means turn lines on
int		treesOn;				// != 0 means turn trees on
// function prototypes:

void	Animate( );
void	Display( );
void	DoAxesMenu( int );
void	DoDebugMenu( int );
void	DoMainMenu( int );
void	DoProjectMenu( int );
void	DoRasterString( float, float, float, char * );
void	DoStrokeString( float, float, float, float, char * );
void	InitGraphics( );
void	InitLists( );
void	InitMenus( );
void	Keyboard( unsigned char, int, int );
void	MouseButton( int, int, int, int );
void	MouseMotion( int, int );
void	Reset( );
void	Resize( int, int );
void	Visibility( int );

void			Axes( float );

unsigned char *	BmpToTexture( char *, int *, int * );
int				ReadInt( FILE * );
short			ReadShort( FILE * );

void			HsvRgb( float[3], float [3] );

void			Cross(float[3], float[3], float[3]);
float			Dot(float [3], float [3]);
float			Unit(float [3], float [3]);

void			initMap();

float			fade(double x);
float			interpolate(float a, float b, float x);
float			gradient(int hash, float x, float y, float z);
float			exaggerate(float y);

float	randFloat(float min, float max);
int		randInt(int min, int max);

void	SetPointLight(int ilight, float x, float y, float z, float r, float g, float b);
void	SetSpotLight(int ilight, float x, float y, float z, float xdir, float ydir, float zdir, float r, float g, float b);
float* Array3(float a, float b, float c);
// main program:

void drawCanopy(float x, float y, float z) {
	glBegin(GL_QUADS);
	glVertex3f(x + canopySize, y - canopySize, z + canopySize);
	glVertex3f(x + canopySize, y - canopySize, z - canopySize);
	glVertex3f(x + canopySize, y + canopySize, z - canopySize);
	glVertex3f(x + canopySize, y + canopySize, z + canopySize);

	glVertex3f(x - canopySize, y - canopySize, z + canopySize);
	glVertex3f(x - canopySize, y + canopySize, z + canopySize);
	glVertex3f(x - canopySize, y + canopySize, z - canopySize);
	glVertex3f(x - canopySize, y - canopySize, z - canopySize);

	glVertex3f(x - canopySize, y + canopySize, z + canopySize);
	glVertex3f(x + canopySize, y + canopySize, z + canopySize);
	glVertex3f(x + canopySize, y + canopySize, z - canopySize);
	glVertex3f(x - canopySize, y + canopySize, z - canopySize);

	glVertex3f(x - canopySize, y - canopySize, z + canopySize);
	glVertex3f(x - canopySize, y - canopySize, z - canopySize);
	glVertex3f(x + canopySize, y - canopySize, z - canopySize);
	glVertex3f(x + canopySize, y - canopySize, z + canopySize);

	glVertex3f(x - canopySize, y - canopySize, z + canopySize);
	glVertex3f(x + canopySize, y - canopySize, z + canopySize);
	glVertex3f(x + canopySize, y + canopySize, z + canopySize);
	glVertex3f(x - canopySize, y + canopySize, z + canopySize);

	glVertex3f(x - canopySize, y - canopySize, z - canopySize);
	glVertex3f(x - canopySize, y + canopySize, z - canopySize);
	glVertex3f(x + canopySize, y + canopySize, z - canopySize);
	glVertex3f(x + canopySize, y - canopySize, z - canopySize);
	glEnd();
}


// this is implementing Ken Perlin's improved noise from here https://cs.nyu.edu/~perlin/noise/ 
float perlin(float x, float y) {
	// find unit square that contains point
	int ix = (int) floor(x) & 0xFF; //bind x and y to 0, 255 to avoid overflow
	int iy = (int) floor(y) & 0xFF; 
	int iz = (int) floor(perlinZ) & 0xFF;

	x -= floor(x); // relative x, y in square
	y -= floor(y);
	float z = perlinZ - floor(perlinZ);

	double u = fade(x);
	double v = fade(y);
	double w = fade(z);
	
	//hash coordinates of the cube corners
	int A = permutation[ix] + iy;
	int AA = permutation[A] + iz;
	int AB = permutation[A + 1] + iz;
	int B = permutation[ix + 1] + iy;
	int BA = permutation[B] + iz;
	int BB = permutation[B + 1] + iz;

	float x1, x2, y1, y2;
	x1 = interpolate(gradient(permutation[AA], x, y, z), gradient(permutation[BA], x - 1, y, z), u);
	x2 = interpolate(gradient(permutation[AB], x, y - 1, z), gradient(permutation[BB], x - 1, y - 1, z), u);
	y1 = interpolate(x1, x2, v);

	x1 = interpolate(gradient(permutation[AA + 1], x, y, z - 1), gradient(permutation[BA + 1], x - 1, y, z - 1), u);
	x2 = interpolate(gradient(permutation[AB + 1], x, y - 1, z - 1), gradient(permutation[BB + 1], x - 1, y - 1, z - 1), u);
	y2 = interpolate(x1, x2, v);

	float res = interpolate(y1, y2, w); //[-1, 1]
	return res * maxHeight;
}

//  ease curve function by perlin 
//	easier to read:
//  f(x) = 6x^5 - 15x^4 + 10x^3
float fade(double x) {
	return x * x * x * (x * (x * 6 - 15) + 10);
}

//basic linear interpolation
float interpolate(float a, float b, float x) {
	return a + x * (b - a);
}
 
// gradient function retrieved from http://riven8192.blogspot.com/2010/08/calculate-perlinnoise-twice-as-fast.html
float gradient(int hash, float x, float y, float z) {
	switch (hash & 0xF)
	{
		case 0x0: return  x + y;
		case 0x1: return -x + y;
		case 0x2: return  x - y;
		case 0x3: return -x - y;
		case 0x4: return  x + z;
		case 0x5: return -x + z;
		case 0x6: return  x - z;
		case 0x7: return -x - z;
		case 0x8: return  y + z;
		case 0x9: return -y + z;
		case 0xA: return  y - z;
		case 0xB: return -y - z;
		case 0xC: return  y + x;
		case 0xD: return -y + z;
		case 0xE: return  y - x;
		case 0xF: return -y - z;
		default: return 0; // never happens
	}
}

// initializes a random permutation array
void initPermutations() {
	std::srand(std::time(NULL));
	for (int i = 0; i < 256; i++) {
		permutation[i] = ((float)std::rand() / RAND_MAX) * 255;
		permutation[i + 256] = permutation[i]; //duplicating the array so we don't get overflow later
	}
}

// polynomial function to exaggerate the peaks and keep the valleys.
float exaggerate(float y) {
	return (pow((0.02 * y), 4) + pow((0.2 * y), 2)) * maxHeight;
}

void initMap() {
	initPermutations();
	for (int i = 0; i < worldSize ; i++) {
		for (int j = 0; j < worldSize; j++) {
			worldMap[i][j].x = ((i) * squareSize);
			worldMap[i][j].z = ((j) * squareSize);

			worldMap[i][j].y = exaggerate(perlin((float)i / ((float)worldSize / perlinFactor), (float)j / ((float)worldSize / perlinFactor)));
		}
	}
}

// random 
float randFloat(float min, float max) {
	std::srand(std::time(NULL));
	return (((float)std::rand() / RAND_MAX) * (max - min)) + min;
}

int randInt(int min, int max) {
	return (int)(std::rand() % (max - min + 1)) + min;
}

int
main( int argc, char *argv[ ] )
{
	// turn on the glut package:
	// (do this before checking argc and argv since it might
	// pull some command line arguments out)

	glutInit( &argc, argv );

	// setup all the graphics stuff:

	InitGraphics( );

	// create the display structures that will not change:

	InitLists( );

	// init all the global variables used by Display( ):
	// this will also post a redisplay

	Reset( );

	// setup all the user interface stuff:

	InitMenus( );

	// draw the scene once and wait for some interaction:
	// (this will never return)

	glutSetWindow( MainWindow );
	glutMainLoop( );

	// glutMainLoop( ) never actually returns
	// the following line is here to make the compiler happy:

	return 0;
}


// this is where one would put code that is to be called
// everytime the glut main loop has nothing to do
//
// this is typically where animation parameters are set
//
// do not call Display( ) from here -- let glutPostRedisplay( ) do it

void
Animate( )
{
	// put animation stuff in here -- change some global variables
	// for Display( ) to find:

	// force a call to Display( ) next time it is convenient:

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// draw the complete scene:

void
Display( )
{
	// set which window we want to do the graphics into:

	glutSetWindow( MainWindow );


	// erase the background:

	glDrawBuffer( GL_BACK );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glEnable( GL_DEPTH_TEST );


	// specify shading to be flat:

	glShadeModel( GL_FLAT );


	// set the viewport to a square centered in the window:

	GLsizei vx = glutGet( GLUT_WINDOW_WIDTH );
	GLsizei vy = glutGet( GLUT_WINDOW_HEIGHT );
	GLsizei v = vx < vy ? vx : vy;			// minimum dimension
	GLint xl = ( vx - v ) / 2;
	GLint yb = ( vy - v ) / 2;
	glViewport( xl, yb,  v, v );


	// set the viewing volume:

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );
	if( WhichProjection == ORTHO )
		glOrtho( -2.f, 2.f,     -2.f, 2.f,     0.1f, 1000.f );
	else
		gluPerspective( 70.f, 1.f,	0.1f, 1000.f );


	// place the objects into the scene:

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );


	// set the eye position, look-at position, and up-vector:

	gluLookAt( 3.f, 3.f, 9.f,     0.f, 0.f, 0.f,     0.f, 1.f, 0.f );


	// rotate the scene:

	glRotatef( (GLfloat)Yrot, 0.f, 1.f, 0.f );
	glRotatef( (GLfloat)Xrot, 1.f, 0.f, 0.f );


	// uniformly scale the scene:

	if( Scale < MINSCALE )
		Scale = MINSCALE;
	glScalef( (GLfloat)Scale, (GLfloat)Scale, (GLfloat)Scale );

	glEnable(GL_LIGHTING);
	GLfloat ambienceParams[4] = { .3, .3, .3, 1.0 };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambienceParams);

	glShadeModel(GL_SMOOTH);
	glEnable(GL_NORMALIZE);

	// drawing the world
	if (groundOn) {
		glColor3f(1., 1., 1.);
		glCallList(mapListGround);
	}

	SetSpotLight(GL_LIGHT0, 0., 5000., 0., 0., 0., 0., 0.1, 0.1, 0.1);
	
	glPushMatrix();
		SetPointLight(GL_LIGHT1, trees[0].lightCoord.x, 10., trees[0].lightCoord.z, trees[0].r, trees[0].g, trees[0].b);
		SetPointLight(GL_LIGHT2, trees[1].lightCoord.x, 10., trees[1].lightCoord.z, trees[1].r, trees[1].g, trees[1].b);
		SetPointLight(GL_LIGHT3, trees[2].lightCoord.x, 10., trees[2].lightCoord.z, trees[2].r, trees[2].g, trees[2].b);
		SetPointLight(GL_LIGHT4, trees[3].lightCoord.x, 10., trees[3].lightCoord.z, trees[3].r, trees[3].g, trees[3].b);
		SetPointLight(GL_LIGHT5, trees[4].lightCoord.x, 10., trees[4].lightCoord.z, trees[4].r, trees[4].g, trees[4].b);
		SetPointLight(GL_LIGHT6, trees[5].lightCoord.x, 10., trees[5].lightCoord.z, trees[5].r, trees[5].g, trees[5].b);
		SetPointLight(GL_LIGHT7, trees[6].lightCoord.x, 10., trees[6].lightCoord.z, trees[6].r, trees[6].g, trees[6].b);
	glPopMatrix();


	glDisable(GL_LIGHTING);

	if (treesOn) {
		glCallList(treeList);
	}

	if (linesOn) {
		if (groundOn) {
			glColor3f(0., 0., 0.);
		}
		else {
			glColor3f(1., 1., 1.);
		}
		glCallList(mapListLines);
	}

	// possibly draw the axes:

	if( AxesOn != 0 )
	{
		glColor3f(1., 1., 1.);
		glCallList( AxesList );
	}


	// since we are using glScalef( ), be sure the normals get unitized:

	glDisable( GL_DEPTH_TEST );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );
	gluOrtho2D( 0.f, 100.f,     0.f, 100.f );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );

	// swap the double-buffered framebuffers:
	glutSwapBuffers( );

	// be sure the graphics buffer has been sent:
	glFlush( );
}

void
DoLinesMenu(int id)
{
	linesOn = id;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

void
DoGroundMenu(int id)
{
	groundOn = id;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}


void
DoTreesMenu(int id)
{
	treesOn = id;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

void
DoAxesMenu( int id )
{
	AxesOn = id;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


void
DoDebugMenu( int id )
{
	DebugOn = id;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}

// main menu callback:

void
DoMainMenu( int id )
{
	switch( id )
	{
		case GENERATE:
			glDeleteLists(treeList, 1);
			glDeleteLists(mapListLines, 1);
			glDeleteLists(mapListGround, 1);
			glDeleteLists(AxesList, 1);
			InitLists();
			break;
		case RESET:
			Reset( );
			break;

		case QUIT:
			// gracefully close out the graphics:
			// gracefully close the graphics window:
			// gracefully exit the program:
			glutSetWindow( MainWindow );
			glFinish( );
			glutDestroyWindow( MainWindow );
			exit( 0 );
			break;

		default:
			fprintf( stderr, "Don't know what to do with Main Menu ID %d\n", id );
	}

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


void
DoProjectMenu( int id )
{
	WhichProjection = id;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// use glut to display a string of characters using a raster font:

void
DoRasterString( float x, float y, float z, char *s )
{
	glRasterPos3f( (GLfloat)x, (GLfloat)y, (GLfloat)z );

	char c;			// one character to print
	for( ; ( c = *s ) != '\0'; s++ )
	{
		glutBitmapCharacter( GLUT_BITMAP_TIMES_ROMAN_24, c );
	}
}


// use glut to display a string of characters using a stroke font:

void
DoStrokeString( float x, float y, float z, float ht, char *s )
{
	glPushMatrix( );
		glTranslatef( (GLfloat)x, (GLfloat)y, (GLfloat)z );
		float sf = ht / ( 119.05f + 33.33f );
		glScalef( (GLfloat)sf, (GLfloat)sf, (GLfloat)sf );
		char c;			// one character to print
		for( ; ( c = *s ) != '\0'; s++ )
		{
			glutStrokeCharacter( GLUT_STROKE_ROMAN, c );
		}
	glPopMatrix( );
}

// initialize the glui window:

void
InitMenus( )
{
	glutSetWindow( MainWindow );

	int axesmenu = glutCreateMenu( DoAxesMenu );
	glutAddMenuEntry( "Off",  0 );
	glutAddMenuEntry( "On",   1 );

	int linesmenu = glutCreateMenu(DoLinesMenu);
	glutAddMenuEntry("Off", 0);
	glutAddMenuEntry("On", 1);

	int groundmenu = glutCreateMenu(DoGroundMenu);
	glutAddMenuEntry("Off", 0);
	glutAddMenuEntry("On", 1);

	int treesmenu = glutCreateMenu(DoTreesMenu);
	glutAddMenuEntry("Off", 0);
	glutAddMenuEntry("On", 1);

	int debugmenu = glutCreateMenu( DoDebugMenu );
	glutAddMenuEntry( "Off",  0 );
	glutAddMenuEntry( "On",   1 );

	int projmenu = glutCreateMenu( DoProjectMenu );
	glutAddMenuEntry( "Orthographic",  ORTHO );
	glutAddMenuEntry( "Perspective",   PERSP );

	int mainmenu = glutCreateMenu( DoMainMenu );
	glutAddMenuEntry("Generate", GENERATE);
	glutAddSubMenu("Lines", linesmenu);
	glutAddSubMenu("Ground", groundmenu);
	glutAddSubMenu("Trees", treesmenu);

	glutAddSubMenu(   "Axes",          axesmenu);

	glutAddSubMenu(   "Projection",    projmenu );
	glutAddMenuEntry( "Reset",         RESET );
	glutAddSubMenu(   "Debug",         debugmenu);
	glutAddMenuEntry( "Quit",          QUIT );

// attach the pop-up menu to the right mouse button:

	glutAttachMenu( GLUT_RIGHT_BUTTON );
}



// initialize the glut and OpenGL libraries:
// also setup callback functions

void
InitGraphics( )
{	
	// request the display modes:
	// ask for red-green-blue-alpha color, double-buffering, and z-buffering:

	glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );

	// set the initial window configuration:

	glutInitWindowPosition( 0, 0 );
	glutInitWindowSize( INIT_WINDOW_SIZE, INIT_WINDOW_SIZE );

	// open the window and set its title:

	MainWindow = glutCreateWindow( WINDOWTITLE );
	glutSetWindowTitle( WINDOWTITLE );

	// set the framebuffer clear values:

	glClearColor( BACKCOLOR[0], BACKCOLOR[1], BACKCOLOR[2], BACKCOLOR[3] );

	// setup the callback functions:
	// DisplayFunc -- redraw the window
	// ReshapeFunc -- handle the user resizing the window
	// KeyboardFunc -- handle a keyboard input
	// MouseFunc -- handle the mouse button going down or up
	// MotionFunc -- handle the mouse moving with a button down
	// PassiveMotionFunc -- handle the mouse moving with a button up
	// VisibilityFunc -- handle a change in window visibility
	// EntryFunc	-- handle the cursor entering or leaving the window
	// SpecialFunc -- handle special keys on the keyboard
	// SpaceballMotionFunc -- handle spaceball translation
	// SpaceballRotateFunc -- handle spaceball rotation
	// SpaceballButtonFunc -- handle spaceball button hits
	// ButtonBoxFunc -- handle button box hits
	// DialsFunc -- handle dial rotations
	// TabletMotionFunc -- handle digitizing tablet motion
	// TabletButtonFunc -- handle digitizing tablet button hits
	// MenuStateFunc -- declare when a pop-up menu is in use
	// TimerFunc -- trigger something to happen a certain time from now
	// IdleFunc -- what to do when nothing else is going on

	glutSetWindow( MainWindow );
	glutDisplayFunc( Display );
	glutReshapeFunc( Resize );
	glutKeyboardFunc( Keyboard );
	glutMouseFunc( MouseButton );
	glutMotionFunc( MouseMotion );
	glutPassiveMotionFunc(MouseMotion);
	//glutPassiveMotionFunc( NULL );
	glutVisibilityFunc( Visibility );
	glutEntryFunc( NULL );
	glutSpecialFunc( NULL );
	glutSpaceballMotionFunc( NULL );
	glutSpaceballRotateFunc( NULL );
	glutSpaceballButtonFunc( NULL );
	glutButtonBoxFunc( NULL );
	glutDialsFunc( NULL );
	glutTabletMotionFunc( NULL );
	glutTabletButtonFunc( NULL );
	glutMenuStateFunc( NULL );
	glutTimerFunc( -1, NULL, 0 );

	// setup glut to call Animate( ) every time it has
	// 	nothing it needs to respond to (which is most of the time)
	// we don't need to do this for this program, and really should set the argument to NULL
	// but, this sets us up nicely for doing animation

	glutIdleFunc( Animate );

	// init the glew package (a window must be open to do this):

#ifdef WIN32
	GLenum err = glewInit( );
	if( err != GLEW_OK )
	{
		fprintf( stderr, "glewInit Error\n" );
	}
	else
		fprintf( stderr, "GLEW initialized OK\n" );
	fprintf( stderr, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
#endif

}


// initialize the display lists that will not change:
// (a display list is a way to store opengl commands in
//  memory so that they can be played back efficiently at a later time
//  with a call to glCallList( )


void
InitLists( )
{
	initMap();

	glutSetWindow( MainWindow );
	float rgb[3];
	float hsv[3] = { 360., 1.0, 1.0 };

	//offset makes 0,0 the center of the world
	float offset = - ((worldSize / 2. - 1.) * squareSize) - (squareSize / 2);

	mapListLines = glGenLists( 1 );
	glNewList(mapListLines, GL_COMPILE);
	for (int i = 0; i < worldSize - 1; i++) {
		glBegin(GL_LINE_STRIP);
		for (int j = 0; j < worldSize; j++) {
			glVertex3f(worldMap[i][j].x + offset, worldMap[i][j].y + 0.01, worldMap[i][j].z + offset);
			glVertex3f(worldMap[i + 1][j].x + offset, worldMap[i + 1][j].y + 0.01, worldMap[i + 1][j].z + offset);
		}
		glEnd();
	}
	for (int i = 0; i < worldSize; i++) {
		glBegin(GL_LINE_STRIP);
		for (int j = 0; j < worldSize; j++) {
			glVertex3f(worldMap[i][j].x + offset, worldMap[i][j].y + 0.01, worldMap[i][j].z + offset);
		}
		glEnd();
	}
	glEndList();


	mapListGround = glGenLists(1);
	glNewList(mapListGround, GL_COMPILE);
	for (int i = 0; i < worldSize - 1; i++) {
		glBegin(GL_TRIANGLE_STRIP);
		for (int j = 0; j < worldSize; j++) {

			glNormal3f(worldMap[i][j].x + offset, worldMap[i][j].y + 2., worldMap[i][j].z + offset);
			glVertex3f(worldMap[i][j].x + offset, worldMap[i][j].y, worldMap[i][j].z + offset);
			glNormal3f(worldMap[i + 1][j].x + offset, worldMap[i + 1][j].y + 2., worldMap[i + 1][j].z + offset);
			glVertex3f(worldMap[i + 1][j].x + offset, worldMap[i + 1][j].y, worldMap[i + 1][j].z + offset);

		}
		glEnd();
	}
	glEndList();

	
	// generate trees
	treeList = glGenLists(1);
	glNewList(treeList, GL_COMPILE);
	glLineWidth(5.);
	hsv[0] = randInt(0, 360);
	for (int i = 0; i < 7; i++) {
		hsv[0] = hsv[0] + 20 % 360;
		HsvRgb(hsv, rgb);

		trees[i].r = rgb[0];
		trees[i].g = rgb[1];
		trees[i].b = rgb[2];

		glColor3fv(rgb);

		int x = (i % 2 == 0) ? randInt(0, worldSize / 3) : randInt(worldSize / 3 * 2, worldSize);
		int z = (i > 4) ? randInt(0, worldSize / 3) : randInt(worldSize / 3 * 2, worldSize);
		
		glBegin(GL_LINE_STRIP);
			glVertex3f(worldMap[x][z].x + offset, worldMap[x][z].y, worldMap[x][z].z + offset);
			glVertex3f(worldMap[x][z].x + offset, worldMap[x][z].y + treeHeight, worldMap[x][z].z + offset);
		glEnd();

		drawCanopy(worldMap[x][z].x + offset, worldMap[x][z].y + treeHeight * 0.9, worldMap[x][z].z + offset);

		trees[i].lightCoord.x = worldMap[x][z].x + offset;
		trees[i].lightCoord.y = worldMap[x][z].y + treeHeight;
		trees[i].lightCoord.z = worldMap[x][z].z + offset;
		
	}
	glLineWidth(1.);
	glEndList();

	// create the axes:

	AxesList = glGenLists( 1 );
	glNewList( AxesList, GL_COMPILE );
		glLineWidth( AXES_WIDTH );
			Axes( 1.5 );
		glLineWidth( 1. );
	glEndList( );
}


// the keyboard callback:

void
Keyboard( unsigned char c, int x, int y )
{
	if( DebugOn != 0 )
		fprintf( stderr, "Keyboard: '%c' (0x%0x)\n", c, c );

	switch( c )
	{
		case 'r':
		case 'R':
			glDeleteLists(treeList, 1);
			glDeleteLists(mapListLines, 1);
			glDeleteLists(mapListGround, 1);
			glDeleteLists(AxesList, 1);
			InitLists();
			break;
		case 'o':
		case 'O':
			WhichProjection = ORTHO;
			break;

		case 'p':
		case 'P':
			WhichProjection = PERSP;
			break;

		case 'q':
		case 'Q':
		case ESCAPE:
			DoMainMenu( QUIT );	// will not return here
			break;				// happy compiler

		default:
			fprintf( stderr, "Don't know what to do with keyboard hit: '%c' (0x%0x)\n", c, c );
	}

	// force a call to Display( ):

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// called when the mouse button transitions down or up:

void
MouseButton( int button, int state, int x, int y )
{
	int b = 0;			// LEFT, MIDDLE, or RIGHT

	if( DebugOn != 0 )
		fprintf( stderr, "MouseButton: %d, %d, %d, %d\n", button, state, x, y );

	
	// get the proper button bit mask:

	switch( button )
	{
		case GLUT_LEFT_BUTTON:
			b = LEFT;		break;

		case GLUT_MIDDLE_BUTTON:
			b = MIDDLE;		break;

		case GLUT_RIGHT_BUTTON:
			b = RIGHT;		break;

		case SCROLL_WHEEL_UP:
			Scale += SCLFACT * SCROLL_WHEEL_CLICK_FACTOR;
			// keep object from turning inside-out or disappearing:
			if (Scale < MINSCALE)
				Scale = MINSCALE;
			break;

		case SCROLL_WHEEL_DOWN:
			Scale -= SCLFACT * SCROLL_WHEEL_CLICK_FACTOR;
			// keep object from turning inside-out or disappearing:
			if (Scale < MINSCALE)
				Scale = MINSCALE;
			break;

		default:
			b = 0;
			fprintf( stderr, "Unknown mouse button: %d\n", button );
	}

	// button down sets the bit, up clears the bit:

	if( state == GLUT_DOWN )
	{
		Xmouse = x;
		Ymouse = y;
		ActiveButton |= b;		// set the proper bit
	}
	else
	{
		ActiveButton &= ~b;		// clear the proper bit
	}

	glutSetWindow(MainWindow);
	glutPostRedisplay();

}


// called when the mouse moves while a button is down:

void
MouseMotion( int x, int y )
{
	int dx = x - Xmouse;		// change in mouse coords
	int dy = y - Ymouse;

	if( ( ActiveButton & LEFT ) != 0 )
	{
		Xrot += ( ANGFACT*dy );
		Yrot += ( ANGFACT*dx );
	}

	if( ( ActiveButton & MIDDLE ) != 0 )
	{
		Scale += SCLFACT * (float) ( dx - dy );

		// keep object from turning inside-out or disappearing:

		if( Scale < MINSCALE )
			Scale = MINSCALE;
	}

	Xmouse = x;			// new current position
	Ymouse = y;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// reset the transformations and the colors:
// this only sets the global variables --
// the glut main loop is responsible for redrawing the scene

void
Reset( )
{
	ActiveButton = 0;
	AxesOn = 0;
	DebugOn = 0;
	Scale  = 1.0;
	ShadowsOn = 0;
	WhichProjection = PERSP;
	Xrot = Yrot = 0.;
	groundOn = 1;
	linesOn = 0;
	treesOn = 0;
}


// called when user resizes the window:

void
Resize( int width, int height )
{
	// don't really need to do anything since window size is
	// checked each time in Display( ):

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// handle a change to the window's visibility:

void
Visibility ( int state )
{
	if( DebugOn != 0 )
		fprintf( stderr, "Visibility: %d\n", state );

	if( state == GLUT_VISIBLE )
	{
		glutSetWindow( MainWindow );
		glutPostRedisplay( );
	}
	else
	{
		// could optimize by keeping track of the fact
		// that the window is not visible and avoid
		// animating or redrawing it ...
	}
}



///////////////////////////////////////   HANDY UTILITIES:  //////////////////////////


// the stroke characters 'X' 'Y' 'Z' :

static float xx[ ] = { 0.f, 1.f, 0.f, 1.f };

static float xy[ ] = { -.5f, .5f, .5f, -.5f };

static int xorder[ ] = { 1, 2, -3, 4 };

static float yx[ ] = { 0.f, 0.f, -.5f, .5f };

static float yy[ ] = { 0.f, .6f, 1.f, 1.f };

static int yorder[ ] = { 1, 2, 3, -2, 4 };

static float zx[ ] = { 1.f, 0.f, 1.f, 0.f, .25f, .75f };

static float zy[ ] = { .5f, .5f, -.5f, -.5f, 0.f, 0.f };

static int zorder[ ] = { 1, 2, 3, 4, -5, 6 };

// fraction of the length to use as height of the characters:
const float LENFRAC = 0.10f;

// fraction of length to use as start location of the characters:
const float BASEFRAC = 1.10f;

//	Draw a set of 3D axes:
//	(length is the axis length in world coordinates)

void
Axes( float length )
{
	glBegin( GL_LINE_STRIP );
		glVertex3f( length, 0., 0. );
		glVertex3f( 0., 0., 0. );
		glVertex3f( 0., length, 0. );
	glEnd( );
	glBegin( GL_LINE_STRIP );
		glVertex3f( 0., 0., 0. );
		glVertex3f( 0., 0., length );
	glEnd( );

	float fact = LENFRAC * length;
	float base = BASEFRAC * length;

	glBegin( GL_LINE_STRIP );
		for( int i = 0; i < 4; i++ )
		{
			int j = xorder[i];
			if( j < 0 )
			{
				
				glEnd( );
				glBegin( GL_LINE_STRIP );
				j = -j;
			}
			j--;
			glVertex3f( base + fact*xx[j], fact*xy[j], 0.0 );
		}
	glEnd( );

	glBegin( GL_LINE_STRIP );
		for( int i = 0; i < 5; i++ )
		{
			int j = yorder[i];
			if( j < 0 )
			{
				
				glEnd( );
				glBegin( GL_LINE_STRIP );
				j = -j;
			}
			j--;
			glVertex3f( fact*yx[j], base + fact*yy[j], 0.0 );
		}
	glEnd( );

	glBegin( GL_LINE_STRIP );
		for( int i = 0; i < 6; i++ )
		{
			int j = zorder[i];
			if( j < 0 )
			{
				
				glEnd( );
				glBegin( GL_LINE_STRIP );
				j = -j;
			}
			j--;
			glVertex3f( 0.0, fact*zy[j], base + fact*zx[j] );
		}
	glEnd( );

}

// read a BMP file into a Texture:

#define VERBOSE				false
#define BMP_MAGIC_NUMBER	0x4d42
#ifndef BI_RGB
#define BI_RGB				0
#define BI_RLE8				1
#define BI_RLE4				2
#endif


// bmp file header:
struct bmfh
{
	short bfType;		// BMP_MAGIC_NUMBER = "BM"
	int bfSize;		// size of this file in bytes
	short bfReserved1;
	short bfReserved2;
	int bfOffBytes;		// # bytes to get to the start of the per-pixel data
} FileHeader;

// bmp info header:
struct bmih
{
	int biSize;		// info header size, should be 40
	int biWidth;		// image width
	int biHeight;		// image height
	short biPlanes;		// #color planes, should be 1
	short biBitCount;	// #bits/pixel, should be 1, 4, 8, 16, 24, 32
	int biCompression;	// BI_RGB, BI_RLE4, BI_RLE8
	int biSizeImage;
	int biXPixelsPerMeter;
	int biYPixelsPerMeter;
	int biClrUsed;		// # colors in the palette
	int biClrImportant;
} InfoHeader;



// read a BMP file into a Texture:

unsigned char *
BmpToTexture( char *filename, int *width, int *height )
{
	FILE *fp;
#ifdef _WIN32
        errno_t err = fopen_s( &fp, filename, "rb" );
        if( err != 0 )
        {
		fprintf( stderr, "Cannot open Bmp file '%s'\n", filename );
		return NULL;
        }
#else
		fp = fopen( filename, "rb" );
		if( fp == NULL )
		{
			fprintf( stderr, "Cannot open Bmp file '%s'\n", filename );
			return NULL;
		}
#endif

	FileHeader.bfType = ReadShort( fp );


	// if bfType is not BMP_MAGIC_NUMBER, the file is not a bmp:

	if( VERBOSE ) fprintf( stderr, "FileHeader.bfType = 0x%0x = \"%c%c\"\n",
			FileHeader.bfType, FileHeader.bfType&0xff, (FileHeader.bfType>>8)&0xff );
	if( FileHeader.bfType != BMP_MAGIC_NUMBER )
	{
		fprintf( stderr, "Wrong type of file: 0x%0x\n", FileHeader.bfType );
		fclose( fp );
		return NULL;
	}


	FileHeader.bfSize = ReadInt( fp );
	if( VERBOSE )	fprintf( stderr, "FileHeader.bfSize = %d\n", FileHeader.bfSize );

	FileHeader.bfReserved1 = ReadShort( fp );
	FileHeader.bfReserved2 = ReadShort( fp );

	FileHeader.bfOffBytes = ReadInt( fp );


	InfoHeader.biSize = ReadInt( fp );
	InfoHeader.biWidth = ReadInt( fp );
	InfoHeader.biHeight = ReadInt( fp );

	const int nums = InfoHeader.biWidth;
	const int numt = InfoHeader.biHeight;

	InfoHeader.biPlanes = ReadShort( fp );

	InfoHeader.biBitCount = ReadShort( fp );
	if( VERBOSE )	fprintf( stderr, "InfoHeader.biBitCount = %d\n", InfoHeader.biBitCount );

	InfoHeader.biCompression = ReadInt( fp );
	if( VERBOSE )	fprintf( stderr, "InfoHeader.biCompression = %d\n", InfoHeader.biCompression );

	InfoHeader.biSizeImage = ReadInt( fp );
	if( VERBOSE )	fprintf( stderr, "InfoHeader.biSizeImage = %d\n", InfoHeader.biSizeImage );

	InfoHeader.biXPixelsPerMeter = ReadInt( fp );
	InfoHeader.biYPixelsPerMeter = ReadInt( fp );

	InfoHeader.biClrUsed = ReadInt( fp );
	if( VERBOSE )	fprintf( stderr, "InfoHeader.biClrUsed = %d\n", InfoHeader.biClrUsed );

	InfoHeader.biClrImportant = ReadInt( fp );

	// fprintf( stderr, "Image size found: %d x %d\n", ImageWidth, ImageHeight );

	// pixels will be stored bottom-to-top, left-to-right:
	unsigned char *texture = new unsigned char[ 3 * nums * numt ];
	if( texture == NULL )
	{
		fprintf( stderr, "Cannot allocate the texture array!\n" );
		return NULL;
	}

	// extra padding bytes:

	int requiredRowSizeInBytes = 4 * ( ( InfoHeader.biBitCount*InfoHeader.biWidth + 31 ) / 32 );
	if( VERBOSE )	fprintf( stderr, "requiredRowSizeInBytes = %d\n", requiredRowSizeInBytes );

	int myRowSizeInBytes = ( InfoHeader.biBitCount*InfoHeader.biWidth + 7 ) / 8;
	if( VERBOSE )	fprintf( stderr, "myRowSizeInBytes = %d\n", myRowSizeInBytes );

	int numExtra = requiredRowSizeInBytes - myRowSizeInBytes;
	if( VERBOSE )	fprintf( stderr, "New NumExtra padding = %d\n", numExtra );


	// this function does not support compression:

	if( InfoHeader.biCompression != 0 )
	{
		fprintf( stderr, "Wrong type of image compression: %d\n", InfoHeader.biCompression );
		fclose( fp );
		return NULL;
	}
	
	// we can handle 24 bits of direct color:
	if( InfoHeader.biBitCount == 24 )
	{
		rewind( fp );
		fseek( fp, FileHeader.bfOffBytes, SEEK_SET );
		int t;
		unsigned char *tp;
		for( t = 0, tp = texture; t < numt; t++ )
		{
			for( int s = 0; s < nums; s++, tp += 3 )
			{
				*(tp+2) = fgetc( fp );		// b
				*(tp+1) = fgetc( fp );		// g
				*(tp+0) = fgetc( fp );		// r
			}

			for( int e = 0; e < numExtra; e++ )
			{
				fgetc( fp );
			}
		}
	}

	// we can also handle 8 bits of indirect color:
	if( InfoHeader.biBitCount == 8 && InfoHeader.biClrUsed == 256 )
	{
		struct rgba32
		{
			unsigned char r, g, b, a;
		};
		struct rgba32 *colorTable = new struct rgba32[ InfoHeader.biClrUsed ];

		rewind( fp );
		fseek( fp, sizeof(struct bmfh) + InfoHeader.biSize - 2, SEEK_SET );
		for( int c = 0; c < InfoHeader.biClrUsed; c++ )
		{
			colorTable[c].r = fgetc( fp );
			colorTable[c].g = fgetc( fp );
			colorTable[c].b = fgetc( fp );
			colorTable[c].a = fgetc( fp );
			if( VERBOSE )	fprintf( stderr, "%4d:\t0x%02x\t0x%02x\t0x%02x\t0x%02x\n",
				c, colorTable[c].r, colorTable[c].g, colorTable[c].b, colorTable[c].a );
		}

		rewind( fp );
		fseek( fp, FileHeader.bfOffBytes, SEEK_SET );
		int t;
		unsigned char *tp;
		for( t = 0, tp = texture; t < numt; t++ )
		{
			for( int s = 0; s < nums; s++, tp += 3 )
			{
				int index = fgetc( fp );
				*(tp+0) = colorTable[index].r;	// r
				*(tp+1) = colorTable[index].g;	// g
				*(tp+2) = colorTable[index].b;	// b
			}

			for( int e = 0; e < numExtra; e++ )
			{
				fgetc( fp );
			}
		}

		delete[ ] colorTable;
	}

	fclose( fp );

	*width = nums;
	*height = numt;
	return texture;
}

int
ReadInt( FILE *fp )
{
	const unsigned char b0 = fgetc( fp );
	const unsigned char b1 = fgetc( fp );
	const unsigned char b2 = fgetc( fp );
	const unsigned char b3 = fgetc( fp );
	return ( b3 << 24 )  |  ( b2 << 16 )  |  ( b1 << 8 )  |  b0;
}

short
ReadShort( FILE *fp )
{
	const unsigned char b0 = fgetc( fp );
	const unsigned char b1 = fgetc( fp );
	return ( b1 << 8 )  |  b0;
}


// function to convert HSV to RGB
// 0.  <=  s, v, r, g, b  <=  1.
// 0.  <= h  <=  360.
// when this returns, call:
//		glColor3fv( rgb );

void
HsvRgb( float hsv[3], float rgb[3] )
{
	// guarantee valid input:

	float h = hsv[0] / 60.f;
	while( h >= 6. )	h -= 6.;
	while( h <  0. ) 	h += 6.;

	float s = hsv[1];
	if( s < 0. )
		s = 0.;
	if( s > 1. )
		s = 1.;

	float v = hsv[2];
	if( v < 0. )
		v = 0.;
	if( v > 1. )
		v = 1.;

	// if sat==0, then is a gray:

	if( s == 0.0 )
	{
		rgb[0] = rgb[1] = rgb[2] = v;
		return;
	}

	// get an rgb from the hue itself:
	
	float i = (float)floor( h );
	float f = h - i;
	float p = v * ( 1.f - s );
	float q = v * ( 1.f - s*f );
	float t = v * ( 1.f - ( s * (1.f-f) ) );

	float r=0., g=0., b=0.;			// red, green, blue
	switch( (int) i )
	{
		case 0:
			r = v;	g = t;	b = p;
			break;
	
		case 1:
			r = q;	g = v;	b = p;
			break;
	
		case 2:
			r = p;	g = v;	b = t;
			break;
	
		case 3:
			r = p;	g = q;	b = v;
			break;
	
		case 4:
			r = t;	g = p;	b = v;
			break;
	
		case 5:
			r = v;	g = p;	b = q;
			break;
	}


	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;
}

void
Cross(float v1[3], float v2[3], float vout[3])
{
	float tmp[3];
	tmp[0] = v1[1] * v2[2] - v2[1] * v1[2];
	tmp[1] = v2[0] * v1[2] - v1[0] * v2[2];
	tmp[2] = v1[0] * v2[1] - v2[0] * v1[1];
	vout[0] = tmp[0];
	vout[1] = tmp[1];
	vout[2] = tmp[2];
}

float
Dot(float v1[3], float v2[3])
{
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

float
Unit(float vin[3], float vout[3])
{
	float dist = vin[0] * vin[0] + vin[1] * vin[1] + vin[2] * vin[2];
	if (dist > 0.0)
	{
		dist = sqrtf(dist);
		vout[0] = vin[0] / dist;
		vout[1] = vin[1] / dist;
		vout[2] = vin[2] / dist;
	}
	else
	{
		vout[0] = vin[0];
		vout[1] = vin[1];
		vout[2] = vin[2];
	}
	return dist;
}

void
SetPointLight(int ilight, float x, float y, float z, float r, float g, float b)
{
	glLightfv(ilight, GL_POSITION, Array3(x, y, z));
	glLightfv(ilight, GL_AMBIENT, Array3(0., 0., 0.));
	glLightfv(ilight, GL_DIFFUSE, Array3(r, g, b));
	glLightfv(ilight, GL_SPECULAR, Array3(r, g, b));
	glLightf(ilight, GL_CONSTANT_ATTENUATION, 1.);
	glLightf(ilight, GL_LINEAR_ATTENUATION, 0.);
	glLightf(ilight, GL_QUADRATIC_ATTENUATION, 0.);
	glEnable(ilight);
}

void
SetSpotLight(int ilight, float x, float y, float z, float xdir, float ydir, float zdir, float r, float g, float b)
{
	glLightfv(ilight, GL_POSITION, Array3(x, y, z));
	glLightfv(ilight, GL_SPOT_DIRECTION, Array3(xdir, ydir, zdir));
	glLightf(ilight, GL_SPOT_EXPONENT, 1.);
	//glLightf(ilight, GL_SPOT_CUTOFF, 45.);
	glLightfv(ilight, GL_AMBIENT, Array3(0., 0., 0.));
	glLightfv(ilight, GL_DIFFUSE, Array3(r, g, b));
	glLightfv(ilight, GL_SPECULAR, Array3(r, g, b));
	//glLightf(ilight, GL_CONSTANT_ATTENUATION, 1.);
	//glLightf(ilight, GL_LINEAR_ATTENUATION, 0.);
	//glLightf(ilight, GL_QUADRATIC_ATTENUATION, 0.);
	glEnable(ilight);
}

float*
Array3(float a, float b, float c)
{
	static float array[4];

	array[0] = a;
	array[1] = b;
	array[2] = c;
	array[3] = 1.;
	return array;
}