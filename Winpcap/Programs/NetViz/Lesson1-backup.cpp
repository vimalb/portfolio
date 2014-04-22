/*
 *		This Code Was Created By Arturo "IRIX" Montieri 2000
 *		based on Jeff Molofee's OpenGL framework
 *		If You've Found This Code Useful, Please Let Me Know.
 *		Visit the great OpenGL site -> nehe.gamedev.net
 */
#include <stdio.h>			// Header File For Standard Input/Output
#include <stdarg.h>			// Header File For Variable Argument Routines
#include <math.h>			// Header File For Math Stuff
#include <windows.h>		// Header File For Windows
#include <gl\glut.h>		// Header File For The GLUT Library
#include <gl\glaux.h>		// Header File For The GLAUX Library


typedef struct
{
	float x,y,z;
	float ax,ay,az;
}camera_t,*camera_t_ptr;




#define bust			1 
#define pelvis			2
#define right_arm		3
#define left_arm		4
#define right_thigh		5
#define left_thigh		6
#define right_forearm	7
#define left_forearm	8
#define right_leg		9
#define left_leg		10
#define head			11


//material definitions



GLfloat mat_specularWHITE[] ={255.0,255.0,255.0,1.0};
GLfloat mat_ambientWHITE[] ={255.0,255.0,255.0,1.0};
GLfloat mat_diffuseWHITE[] ={255.0,255.0,255.0,1.0};
GLfloat mat_shininessWHITE[] ={128.0 * 0.4};

GLfloat mat_specularGRAY[] ={0.75,0.75,0.75,1.0};
GLfloat mat_ambientGRAY[] ={0.5,0.5,0.5,1.0};
GLfloat mat_diffuseGRAY[] ={0.50,0.50,0.50,1.0};
GLfloat mat_shininessGRAY[] ={128.0 * 0.6};

GLfloat mat_specularBLUE[] ={0.75,0.75,0.75,1.0};
GLfloat mat_ambientBLUE[] ={0,0,1,1.0};
GLfloat mat_diffuseBLUE[] ={0.50,0.50,0.50,1.0};
GLfloat mat_shininessBLUE[] ={128.0 };

GLfloat mat_specularGREEN[] ={0.633, 0.727811, 0.633,1.0};
GLfloat mat_ambientGREEN[] ={0.0215, 0.1745, 0.0215,1.0};
GLfloat mat_diffuseGREEN[] ={0.07568, 0.61424, 0.07568,1.0};
GLfloat mat_shininessGREEN[] ={128.0};

GLfloat mat_specularYELLOW[] ={0.75,0.75,0.75,1.0};
GLfloat mat_ambientYELLOW[] ={1,1,0,1.0};
GLfloat mat_diffuseYELLOW[] ={0.50,0.50,0.50,1.0};
GLfloat mat_shininessYELLOW[] ={128.0};

GLfloat mat_specularRED[] ={0.75,0.75,0.75,1.0};
GLfloat mat_ambientRED[] ={1.0,0.0,0.0,1.0};
GLfloat mat_diffuseRED[] ={0.50,0.50,0.50,1.0};
GLfloat mat_shininessRED[] ={128.0};


/*----------------------------------GLOBALS------------------------------------*/





HDC			hDC=NULL;		// Private GDI Device Context
HGLRC		hRC=NULL;		// Permanent Rendering Context
HWND		hWnd=NULL;		// Holds Our Window Handle
HINSTANCE	hInstance;		// Holds The Instance Of The Application

bool	keys[256];			// Array Used For The Keyboard Routine
bool	active=TRUE;		// Window Active Flag Set To TRUE By Default
bool	fullscreen=TRUE;	// Fullscreen Flag Set To Fullscreen Mode By Default

float bust_angle_y=0.0f;
float bust_angle_x=0.0f;
float right_arm_angle=0.0f;
float left_arm_angle=0.0f;
float right_forearm_angle=0.0f;
float left_forearm_angle=0.0f;
float left_thigh_angle=0.0f;
float right_thigh_angle=0.0f;
float right_leg_angle=0.0f;
float left_leg_angle=0.0f;

float lightturn,lightturn1;
float heading;

camera_t camera;

const float piover180 = 0.0174532925f;




void DrawRobot(void);
void init_structures(void);
void struct_bust(void);
void struct_pelvis(void);
void struct_right_thigh(void);
void struct_left_thigh(void);
void struct_right_arm(void);
void struct_left_arm(void);
void struct_right_forearm(void);
void struct_left_forearm(void);
void struct_right_leg(void);
void struct_left_leg(void);
void struct_head(void);
void lights(void);
void SetMaterial(GLfloat spec[], GLfloat amb[], GLfloat diff[], GLfloat shin[]);

LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// Declaration For WndProc




GLvoid ReSizeGLScene(GLsizei width, GLsizei height)		// Resize And Initialize The GL Window
{
	if (height==0)										// Prevent A Divide By Zero By
	{
		height=1;										// Making Height Equal One
	}

	glViewport(0,0,width,height);						// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.1f,50.0f);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();
	glTranslatef(0,1,-10);
}



/*-----------------------------INIT DISPLAY LISTS------------------------*/
void init_structures(void)
{
	struct_head();
	struct_bust();
	struct_pelvis();
	struct_right_thigh();
	struct_left_thigh();
	struct_right_arm();
	struct_left_arm();
	struct_right_forearm();
	struct_left_forearm();
	struct_right_leg();
	struct_left_leg();
}




/*-----------------------------INIT GL-----------------------------------*/

int InitGL(GLvoid)										// All Setup For OpenGL Goes Here
{
	
	glShadeModel(GL_SMOOTH);							// Enable Smooth Shading

	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations

	
	glFrontFace(GL_CCW);
	glPolygonMode(GL_FRONT,GL_FILL);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	
	
	init_structures();
	
	return TRUE;										// Initialization Went OK
}


/*-------------------------------LIGHTS STUFF-----------------------------------*/
void lights(void)
{

  GLfloat position[] =  {0.0, 0.0, 2.0, 1.0};


  glRotatef((GLfloat) lightturn1, 1.0, 0.0, 0.0);
  glRotatef((GLfloat) lightturn, 0.0, 1.0, 0.0);
  //glRotatef(0.0, 1.0, 0.0, 0.0);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_NORMALIZE);
  glDepthFunc(GL_LESS);
  //glPolygonMode(GL_FRONT, GL_FILL);

  glLightfv(GL_LIGHT0, GL_POSITION, position);
  glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 80.0);

  glTranslatef(0.0, 0.0, 2.0);
  glDisable(GL_LIGHTING);
  glutWireCube(0.1);
  glEnable(GL_LIGHTING);
}
/*--------------------------------------------------------------------------------*/


void SetMaterial(GLfloat spec[], GLfloat amb[], GLfloat diff[], GLfloat shin[])
{

  glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
  glMaterialfv(GL_FRONT, GL_SHININESS, shin);
  glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, diff);
}



void struct_bust(void)
{
	
	glNewList(bust,GL_COMPILE);
	//SetMaterial(mat_specularWHITE, mat_ambientWHITE, mat_diffuseWHITE, mat_shininessWHITE);
	SetMaterial(mat_specularGRAY, mat_ambientGRAY, mat_diffuseGRAY, mat_shininessGRAY);
	glColor3f(1.0,1.0,1.0);
	glScalef(1.8,1.0,1.0);
	glutSolidSphere(0.6,10,5);
	glTranslatef(0.0,-0.75,0.0);
	glScalef(1.0,1.0,1.0);
	SetMaterial(mat_specularYELLOW, mat_ambientYELLOW, mat_diffuseYELLOW, mat_shininessYELLOW);
	glColor3f(1.0,1.0,0.0);
	glutSolidCube(0.5);
	//glDisable(GL_LIGHT0);
	
	glTranslatef(0,0.3,0);
	SetMaterial(mat_specularGRAY, mat_ambientGRAY, mat_diffuseGRAY, mat_shininessGRAY);
	glColor3ub(128,128,128);
	glutSolidSphere(0.5,5,5);
	
	/*SetMaterial(mat_specularBLUE, mat_ambientBLUE, mat_diffuseBLUE, mat_shininessBLUE);
	glColor3ub(0,0,255);
	glScalef(1.0,0.2,1.0);
	glutSolidCube(1.0);*/
	glScalef(1.0,0.2,1.0);
	glEndList();
}

void struct_pelvis(void)
{
	glNewList(pelvis,GL_COMPILE);	
	SetMaterial(mat_specularYELLOW, mat_ambientYELLOW, mat_diffuseYELLOW, mat_shininessYELLOW);
	glColor3f(1.0,1.0,0.0);
	glTranslatef(0.0,-0.75,0.0);
	glScalef(0.5,2.0,1.0);
	glutSolidCube(0.5);
	glEndList();
}




void struct_right_arm(void)
{
	glNewList(right_arm,GL_COMPILE);
	SetMaterial(mat_specularGRAY, mat_ambientGRAY, mat_diffuseGRAY, mat_shininessGRAY);
	glColor3f(1.0,1.0,0.0);
	glTranslatef(-1.1,0.25,0.0);
	glScalef(0.5,0.5,0.5);
	glColor3ub(128,128,128);
	glutSolidSphere(0.5,20,20);

	glTranslatef(0.0,-1.10,0.0);
	glScalef(0.5,1.5,0.5);
	SetMaterial(mat_specularWHITE, mat_ambientWHITE, mat_diffuseWHITE, mat_shininessWHITE);
	glColor3ub(255,255,255);
	glutSolidCube(1.0);


	glEndList();

}

void struct_right_forearm(void)
{
	glNewList(right_forearm,GL_COMPILE);
	

	//glTranslatef(0.0,-0.7,0.0);
	glTranslatef(-1.1,-0.7,0.0);
	glScalef(0.5,0.5,0.5);
	SetMaterial(mat_specularGRAY, mat_ambientGRAY, mat_diffuseGRAY, mat_shininessGRAY);
	glColor3ub(128,128,128);
	glutSolidSphere(0.4,20,20);
	
	glTranslatef(0.0,-1.3,0.0);
	glScalef(0.8,2.0,1.0);
	SetMaterial(mat_specularRED, mat_ambientRED, mat_diffuseRED, mat_shininessRED);
	glColor3ub(255,0,0);
	glutSolidCube(1.0);
	
	//palmo
	glScalef(.5,1.0,1.0);
	glTranslatef(0.0,-0.4,0.0);
	glutSolidCube(.5);
	
	//pollice
	glTranslatef(0.0,-0.25,0.0);
	glutSolidCube(.3);
	glEndList();

}


void struct_left_arm(void)
{
	glNewList(left_arm,GL_COMPILE);
	
	glTranslatef(1.1,0.25,0.0);
	glScalef(0.5,0.5,0.5);
	SetMaterial(mat_specularGRAY, mat_ambientGRAY, mat_diffuseGRAY, mat_shininessGRAY);
	glColor3ub(128,128,128);
	glutSolidSphere(0.5,20,20);

	glTranslatef(0.0,-1.10,0.0);
	glScalef(0.5,1.5,0.5);
	SetMaterial(mat_specularWHITE, mat_ambientWHITE, mat_diffuseWHITE, mat_shininessWHITE);
	glColor3ub(255,255,255);
	glutSolidCube(1.0);
	glEndList();

}


void struct_left_forearm(void)
{
	glNewList(left_forearm,GL_COMPILE);
	
	glTranslatef(1.1,-0.7,0.0);
	glScalef(0.5,0.5,0.5);
	SetMaterial(mat_specularGRAY, mat_ambientGRAY, mat_diffuseGRAY, mat_shininessGRAY);
	glColor3ub(128,128,128);
	glutSolidSphere(0.4,20,20);
	
	glTranslatef(0.0,-1.3,0.0);
	glScalef(0.8,2.0,1.0);
	SetMaterial(mat_specularRED, mat_ambientRED, mat_diffuseRED, mat_shininessRED);
	glColor3ub(255,0,0);
	glutSolidCube(1.0);
	
	//palmo
	glScalef(.5,1.0,1.0);
	glTranslatef(0.0,-0.4,0.0);
	glutSolidCube(.5);
	
	//pollice
	glTranslatef(0.0,-0.25,0.0);
	glutSolidCube(.3);
	glEndList();

}

void struct_left_thigh(void)
{
	glNewList(left_thigh,GL_COMPILE);
	
	glTranslatef(0.3,-1.2,0.0);
	glScalef(2.0,2.0,2.0);
	SetMaterial(mat_specularGRAY, mat_ambientGRAY, mat_diffuseGRAY, mat_shininessGRAY);
	glColor3ub(128,128,128);
	glutSolidSphere(0.1,20,20);

	glTranslatef(0.0,-0.3,0.0);
	glScalef(0.5,1.5,0.5);
	SetMaterial(mat_specularWHITE, mat_ambientWHITE, mat_diffuseWHITE, mat_shininessWHITE);
	glColor3ub(255,255,255);
	glutSolidCube(0.3);
	glEndList();
}


void struct_left_leg(void)
{
	glNewList(left_leg,GL_COMPILE);
	
	//glTranslatef(0.0,-0.2,0.0);
	glTranslatef(0.3,-2.3,0.0);
	glScalef(2.0,1.3,2.0);
	SetMaterial(mat_specularGRAY, mat_ambientGRAY, mat_diffuseGRAY, mat_shininessGRAY);
	glColor3ub(128,128,128);
	glutSolidSphere(0.1,20,20);
	
	glTranslatef(0.0,-0.5,0.0);
	glScalef(0.5,2.0,0.5);
	SetMaterial(mat_specularBLUE, mat_ambientBLUE, mat_diffuseBLUE, mat_shininessBLUE);
	glColor3ub(0,0,255);
	glutSolidCube(0.5);
	
	//foot
	glColor3ub(255,0,0);
	glTranslatef(0.0,-0.25,0.1);
	glScalef(0.8,0.1,1.4);
	glutSolidCube(0.5);
	glEndList();
}

void struct_right_thigh(void)
{
	glNewList(right_thigh,GL_COMPILE);
	glTranslatef(-0.3,-1.2,0.0);
	glScalef(2.0,2.0,2.0);
	SetMaterial(mat_specularGRAY, mat_ambientGRAY, mat_diffuseGRAY, mat_shininessGRAY);
	glColor3ub(128,128,128);
	glutSolidSphere(0.1,20,20);

	glTranslatef(0.0,-0.3,0.0);
	glScalef(0.5,1.5,0.5);
	SetMaterial(mat_specularWHITE, mat_ambientWHITE, mat_diffuseWHITE, mat_shininessWHITE);
	glColor3ub(255,255,255);
	glutSolidCube(0.3);

	glEndList();
}


void struct_right_leg(void)
{
	glNewList(right_leg,GL_COMPILE);
		
	glTranslatef(-0.3,-2.3,0.0);
	glScalef(2.0,1.3,2.0);
	SetMaterial(mat_specularGRAY, mat_ambientGRAY, mat_diffuseGRAY, mat_shininessGRAY);
	glColor3ub(128,128,128);
	glutSolidSphere(0.1,20,20);
	
	glTranslatef(0.0,-0.5,0.0);
	glScalef(0.5,2.0,0.5);
	SetMaterial(mat_specularBLUE, mat_ambientBLUE, mat_diffuseBLUE, mat_shininessBLUE);
	glColor3ub(0,0,255);
	glutSolidCube(0.5);
	
	//foot
	glColor3ub(255,0,0);
	glTranslatef(0.0,-0.25,0.1);
	glScalef(0.8,0.1,1.4);
	glutSolidCube(0.5);


	glEndList();
}


void struct_head(void)
{
	glNewList(head,GL_COMPILE);
	//SetMaterial(mat_specularWHITE, mat_ambientWHITE, mat_diffuseWHITE, mat_shininessWHITE);
	SetMaterial(mat_specularYELLOW, mat_ambientYELLOW, mat_diffuseYELLOW, mat_shininessYELLOW);
	glColor3ub(255,255,255);
	glTranslatef(0,6,0);
	glScalef(0.3,2.0,0.5);
	glScalef(1.0,1.3,1.0);
	glutSolidSphere(0.5,10,10);
	
	SetMaterial(mat_specularBLUE, mat_ambientBLUE, mat_diffuseBLUE, mat_shininessBLUE);
	glColor3ub(0,0,255);
	glTranslatef(0.0,0.2,0.2);
	glScalef(0.8,0.4,0.5);
	glutSolidCube(1.0);

	glEndList();
}

void DrawRobot(void)
{
	/*----------------BUST & HEAD-------------*/
	glPushMatrix();	
		//glTranslatef(0,0,-6);
		glRotatef(bust_angle_y,0,1,0);
		glRotatef(bust_angle_x,1,0,0);
		glPushMatrix();
			glCallList(bust);
			glCallList(head);
		glPopMatrix();
		
		
		/*-------------------RIGHT ARM & FOREARM----------*/
		glPushMatrix();
			glTranslatef(0,0.25,0);
			glRotatef(right_arm_angle,1,0,0);
			glTranslatef(0,-0.25,0);
			glPushMatrix();
				glCallList(right_arm);
			glPopMatrix();
			
			glPushMatrix();
				glTranslatef(1.25,-0.7,0);
				glRotatef(right_forearm_angle,1,0,0);	
				glTranslatef(-1.25,0.7,0);
				glCallList(right_forearm);
			glPopMatrix();
		glPopMatrix();
		

		/*------------------LEFT ARM & FOREARM---------*/
		glPushMatrix();
			glTranslatef(0,0.25,0);
			glRotatef(left_arm_angle,1,0,0);
			glTranslatef(0,-0.25,0);
			glPushMatrix();
				glCallList(left_arm);
			glPopMatrix();

			glPushMatrix();
				glTranslatef(1.25,-0.7,0);
				glRotatef(left_forearm_angle,1,0,0);	
				glTranslatef(-1.25,0.7,0);
				glCallList(left_forearm);
			glPopMatrix();

		glPopMatrix();
		
		

		/*---------------------------PELVIS-------------------------*/
		glPushMatrix();
			glPushMatrix();
				glCallList(pelvis);
			glPopMatrix();
		
			/*--------------------RIGHT THIGH & LEG----------*/
			glPushMatrix();
			glTranslatef(-0.3,-1.2,0.0);
			glRotatef(right_thigh_angle,1.0,0.0,0.0);
			glTranslatef(0.3,1.2,0.0);	
				glPushMatrix();
					glCallList(right_thigh);
				glPopMatrix();

				glPushMatrix();
					glTranslatef(-0.3,-2.3,0.0);
					glRotatef(right_leg_angle,1.0,0.0,0.0);
					glTranslatef(0.3,2.3,0.0);
					glCallList(right_leg);
				glPopMatrix();
			glPopMatrix();

			/*--------------------LEFT THIGH & LEG--------*/
			glPushMatrix();
				glTranslatef(0.3,-1.2,0.0);
				glRotatef(left_thigh_angle,1.0,0.0,0.0);
				glTranslatef(-0.3,1.2,0.0);
				glPushMatrix();
					glCallList(left_thigh);
				glPopMatrix();

				glPushMatrix();
					glTranslatef(0.3,-2.3,0.0);
					glRotatef(left_leg_angle,1.0,0.0,0.0);
					glTranslatef(-0.3,2.3,0.0);
					glCallList(left_leg);
				glPopMatrix();
			glPopMatrix();
		glPopMatrix();
	glPopMatrix();
	
	
}

int DrawGLScene(GLvoid)									// Here's Where We Do All The Drawing
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer
	

	/*------------CAMERA STUFF-----------*/
	glPushMatrix();
	
	glRotatef(360-camera.ay,0,1,0);
	glTranslatef(-camera.x,0,-camera.z);



	
	/*------------LIGHTS-----------------*/
	glPushMatrix();
		lights();
	glPopMatrix();
	
	DrawRobot();
	glPopMatrix();
	

	return TRUE;										// Everything Went OK
}

GLvoid KillGLWindow(GLvoid)								// Properly Kill The Window
{
	if (fullscreen)										// Are We In Fullscreen Mode?
	{
		ChangeDisplaySettings(NULL,0);					// If So Switch Back To The Desktop
		ShowCursor(TRUE);								// Show Mouse Pointer
	}

	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL,NULL))					// Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))						// Are We Able To Delete The RC?
		{
			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;										// Set RC To NULL
	}

	if (hDC && !ReleaseDC(hWnd,hDC))					// Are We Able To Release The DC
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;										// Set DC To NULL
	}

	if (hWnd && !DestroyWindow(hWnd))					// Are We Able To Destroy The Window?
	{
		MessageBox(NULL,"Could Not Release hWnd.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hWnd=NULL;										// Set hWnd To NULL
	}

	if (!UnregisterClass("OpenGL",hInstance))			// Are We Able To Unregister Class
	{
		MessageBox(NULL,"Could Not Unregister Class.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hInstance=NULL;									// Set hInstance To NULL
	}
}

/*	This Code Creates Our OpenGL Window.  Parameters Are:					*
 *	title			- Title To Appear At The Top Of The Window				*
 *	width			- Width Of The GL Window Or Fullscreen Mode				*
 *	height			- Height Of The GL Window Or Fullscreen Mode			*
 *	bits			- Number Of Bits To Use For Color (8/16/24/32)			*
 *	fullscreenflag	- Use Fullscreen Mode (TRUE) Or Windowed Mode (FALSE)	*/
 
BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag)
{
	GLuint		PixelFormat;			// Holds The Results After Searching For A Match
	WNDCLASS	wc;						// Windows Class Structure
	DWORD		dwExStyle;				// Window Extended Style
	DWORD		dwStyle;				// Window Style
	RECT		WindowRect;				// Grabs Rectangle Upper Left / Lower Right Values
	WindowRect.left=(long)0;			// Set Left Value To 0
	WindowRect.right=(long)width;		// Set Right Value To Requested Width
	WindowRect.top=(long)0;				// Set Top Value To 0
	WindowRect.bottom=(long)height;		// Set Bottom Value To Requested Height

	fullscreen=fullscreenflag;			// Set The Global Fullscreen Flag

	hInstance			= GetModuleHandle(NULL);				// Grab An Instance For Our Window
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc		= (WNDPROC) WndProc;					// WndProc Handles Messages
	wc.cbClsExtra		= 0;									// No Extra Window Data
	wc.cbWndExtra		= 0;									// No Extra Window Data
	wc.hInstance		= hInstance;							// Set The Instance
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground	= NULL;									// No Background Required For GL
	wc.lpszMenuName		= NULL;									// We Don't Want A Menu
	wc.lpszClassName	= "OpenGL";								// Set The Class Name

	if (!RegisterClass(&wc))									// Attempt To Register The Window Class
	{
		MessageBox(NULL,"Failed To Register The Window Class.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;											// Return FALSE
	}
	
	if (fullscreen)												// Attempt Fullscreen Mode?
	{
		DEVMODE dmScreenSettings;								// Device Mode
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));	// Makes Sure Memory's Cleared
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Size Of The Devmode Structure
		dmScreenSettings.dmPelsWidth	= width;				// Selected Screen Width
		dmScreenSettings.dmPelsHeight	= height;				// Selected Screen Height
		dmScreenSettings.dmBitsPerPel	= bits;					// Selected Bits Per Pixel
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{
			// If The Mode Fails, Offer Two Options.  Quit Or Use Windowed Mode.
			if (MessageBox(NULL,"The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?","NeHe GL",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
			{
				fullscreen=FALSE;		// Windowed Mode Selected.  Fullscreen = FALSE
			}
			else
			{
				// Pop Up A Message Box Letting User Know The Program Is Closing.
				MessageBox(NULL,"Program Will Now Close.","ERROR",MB_OK|MB_ICONSTOP);
				return FALSE;									// Return FALSE
			}
		}
	}

	if (fullscreen)												// Are We Still In Fullscreen Mode?
	{
		dwExStyle=WS_EX_APPWINDOW;								// Window Extended Style
		dwStyle=WS_POPUP;										// Windows Style
		ShowCursor(FALSE);										// Hide Mouse Pointer
	}
	else
	{
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			// Window Extended Style
		dwStyle=WS_OVERLAPPEDWINDOW;							// Windows Style
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// Adjust Window To True Requested Size

	// Create The Window
	if (!(hWnd=CreateWindowEx(	dwExStyle,							// Extended Style For The Window
								"OpenGL",							// Class Name
								title,								// Window Title
								dwStyle |							// Defined Window Style
								WS_CLIPSIBLINGS |					// Required Window Style
								WS_CLIPCHILDREN,					// Required Window Style
								0, 0,								// Window Position
								WindowRect.right-WindowRect.left,	// Calculate Window Width
								WindowRect.bottom-WindowRect.top,	// Calculate Window Height
								NULL,								// No Parent Window
								NULL,								// No Menu
								hInstance,							// Instance
								NULL)))								// Dont Pass Anything To WM_CREATE
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Window Creation Error.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	static	PIXELFORMATDESCRIPTOR pfd=				// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,							// Must Support Double Buffering
		PFD_TYPE_RGBA,								// Request An RGBA Format
		bits,										// Select Our Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		0,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		16,											// 16Bit Z-Buffer (Depth Buffer)  
		0,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};
	
	if (!(hDC=GetDC(hWnd)))							// Did We Get A Device Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Create A GL Device Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))	// Did Windows Find A Matching Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd))		// Are We Able To Set The Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(hRC=wglCreateContext(hDC)))				// Are We Able To Get A Rendering Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!wglMakeCurrent(hDC,hRC))					// Try To Activate The Rendering Context
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	ShowWindow(hWnd,SW_SHOW);						// Show The Window
	SetForegroundWindow(hWnd);						// Slightly Higher Priority
	SetFocus(hWnd);									// Sets Keyboard Focus To The Window
	ReSizeGLScene(width, height);					// Set Up Our Perspective GL Screen

	if (!InitGL())									// Initialize Our Newly Created GL Window
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Initialization Failed.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	return TRUE;									// Success
}

LRESULT CALLBACK WndProc(	HWND	hWnd,			// Handle For This Window
							UINT	uMsg,			// Message For This Window
							WPARAM	wParam,			// Additional Message Information
							LPARAM	lParam)			// Additional Message Information
{
	switch (uMsg)									// Check For Windows Messages
	{
		case WM_ACTIVATE:							// Watch For Window Activate Message
		{
			if (!HIWORD(wParam))					// Check Minimization State
			{
				active=TRUE;						// Program Is Active
			}
			else
			{
				active=FALSE;						// Program Is No Longer Active
			}

			return 0;								// Return To The Message Loop
		}

		case WM_SYSCOMMAND:							// Intercept System Commands
		{
			switch (wParam)							// Check System Calls
			{
				case SC_SCREENSAVE:					// Screensaver Trying To Start?
				case SC_MONITORPOWER:				// Monitor Trying To Enter Powersave?
				return 0;							// Prevent From Happening
			}
			break;									// Exit
		}

		case WM_CLOSE:								// Did We Receive A Close Message?
		{
			PostQuitMessage(0);						// Send A Quit Message
			return 0;								// Jump Back
		}

		case WM_KEYDOWN:							// Is A Key Being Held Down?
		{
			keys[wParam] = TRUE;					// If So, Mark It As TRUE
			return 0;								// Jump Back
		}

		case WM_KEYUP:								// Has A Key Been Released?
		{
			keys[wParam] = FALSE;					// If So, Mark It As FALSE
			return 0;								// Jump Back
		}

		case WM_SIZE:								// Resize The OpenGL Window
		{
			ReSizeGLScene(LOWORD(lParam),HIWORD(lParam));  // LoWord=Width, HiWord=Height
			return 0;								// Jump Back
		}
	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

int WINAPI WinMain(	HINSTANCE	hInstance,			// Instance
					HINSTANCE	hPrevInstance,		// Previous Instance
					LPSTR		lpCmdLine,			// Command Line Parameters
					int			nCmdShow)			// Window Show State
{
	MSG		msg;									// Windows Message Structure
	BOOL	done=FALSE;								// Bool Variable To Exit Loop

	// Ask The User Which Screen Mode They Prefer
	if (MessageBox(NULL,"Would You Like To Run In Fullscreen Mode?", "Start FullScreen?",MB_YESNO|MB_ICONQUESTION)==IDNO)
	{
		fullscreen=FALSE;							// Windowed Mode
	}

	// Create Our OpenGL Window
	if (!CreateGLWindow("NeHe's OpenGL Framework",640,480,16,fullscreen))
	{
		return 0;									// Quit If Window Was Not Created
	}

	while(!done)									// Loop That Runs While done=FALSE
	{
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))	// Is There A Message Waiting?
		{
			if (msg.message==WM_QUIT)				// Have We Received A Quit Message?
			{
				done=TRUE;							// If So done=TRUE
			}
			else									// If Not, Deal With Window Messages
			{
				TranslateMessage(&msg);				// Translate The Message
				DispatchMessage(&msg);				// Dispatch The Message
			}
		}
		else										// If There Are No Messages
		{
			// Draw The Scene.  Watch For ESC Key And Quit Messages From DrawGLScene()
			if ((active && !DrawGLScene()) || keys[VK_ESCAPE])							// Program Active?
				{
					done=TRUE;						// ESC Signalled A Quit
				}
			else								// Not Time To Quit, Update Screen
				{
					//DrawGLScene();					// Draw The Scene
					SwapBuffers(hDC);				// Swap Buffers (Double Buffering)
				}
		
			if (keys[VK_F1])						// Is F1 Being Pressed?
			{
				
				keys[VK_F1]=FALSE;					// If So Make Key FALSE
				KillGLWindow();						// Kill Our Current Window
				fullscreen=!fullscreen;				// Toggle Fullscreen / Windowed Mode
				// Recreate Our OpenGL Window
				if (!CreateGLWindow("NeHe's OpenGL Framework",640,480,16,fullscreen))
				{
					return 0;						// Quit If Window Was Not Created
				}
			}
			if (keys[VK_LEFT])
			
			{
				bust_angle_y-=5.2f;	
				//lightturn-=5 % 360;
			}
			
			if (keys[VK_RIGHT])
			
			{
				bust_angle_y+=5.2f;	
				//lightturn+=5 % 360;
			}

			if (keys[VK_UP])
			
				bust_angle_x+=5.2f;

			if (keys[VK_DOWN])
			
				bust_angle_x-=5.2f;

			if (keys['1'])
				right_arm_angle-=5.2f;
			if (keys['Q'])
				right_arm_angle+=5.2f;

			if (keys['2'])
			{
				right_forearm_angle-=5.2f;
				if (right_forearm_angle<-100.0f) right_forearm_angle=-100.0f;
			}
			if (keys['W'])
			{
				right_forearm_angle+=5.2f;
				if (right_forearm_angle>100.0f) right_forearm_angle=100.0f;
			}

			if (keys['3'])
				left_arm_angle-=5.2f;
			if (keys['E'])
				left_arm_angle+=5.2f;

			if (keys['4'])
			{
				left_forearm_angle-=5.2f;
				if (left_forearm_angle<-100.0f) left_forearm_angle=-100.0f;
			}
			if (keys['R'])
			{
				left_forearm_angle+=5.2f;
				if (left_forearm_angle>100.0f) left_forearm_angle=100.0f;
			}
			if (keys['5'])
			{
				left_thigh_angle-=5.2f;
				if (left_thigh_angle<-100.0f) left_thigh_angle=-100.0f;

			}
			if (keys['T'])
			{
				left_thigh_angle+=5.2f;
				if (left_thigh_angle>100.0f) left_thigh_angle=100.0f;

			}
			if (keys['6'])
			{
				left_leg_angle+=5.2f;
				if (left_leg_angle>100.0f) left_leg_angle=100.0f;
			}
			if (keys['Y'])
			{
				left_leg_angle-=5.2f;
				if (left_leg_angle<-100.0f) left_leg_angle=-100.0f;
			}
			if (keys['7'])
			{
				right_thigh_angle-=5.2f;
				if (right_thigh_angle<-100.0f) right_thigh_angle=-100.0f;
			}
			if (keys['U'])
			{
				right_thigh_angle+=5.2f;
				if (right_thigh_angle>100.0f) right_thigh_angle=100.0f;
			}
			if (keys['8'])
			{
				right_leg_angle+=5.2f;
				if (right_leg_angle>100.0f) right_leg_angle=100.0f;

			}
			if (keys['I'])
			{
				right_leg_angle-=5.2f;
				if (right_leg_angle<-100.0f) right_leg_angle=-100.0f;
			}

			if (keys[VK_PRIOR])
				lightturn+=5 % 360;
			if (keys[VK_NEXT])
				lightturn1+=5 % 360;

			
			if (keys['N'])
			{
				
				camera.x-=(float)sin(heading*piover180) * 0.15f;
				camera.z-=(float)cos(heading*piover180) * 0.15f;
			}
			if (keys['M'])
			{
				camera.x+=(float)sin(heading*piover180) * 0.15f;
				camera.z+=(float)cos(heading*piover180) * 0.15f;
				
			}
			if (keys['V'])
			{
				heading -= 1.0f;
				camera.ay=heading;
			}
			if (keys['B'])
			{
				heading+=1.0f;
				camera.ay=heading;
			}

		}
	}

	// Shutdown
	KillGLWindow();									// Kill The Window
	return (msg.wParam);							// Exit The Program
}
