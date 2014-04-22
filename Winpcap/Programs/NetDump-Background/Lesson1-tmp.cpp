/*
 *		This Code Was Created By Arturo "IRIX" Montieri 2000
 *		based on Jeff Molofee's OpenGL framework
 *		If You've Found This Code Useful, Please Let Me Know.
 *		Visit the great OpenGL site -> nehe.gamedev.net
 */

/*
 * Copyright (c) 1999 - 2002
 *	Politecnico di Torino.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the Politecnico
 * di Torino, and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */


/*
 * A lot of this was modified by Vimal. You can do whatever you want to with it,
 * subject to the above two copyright notices. Frankly, I don't care.
 */







#include "..\..\Include\packet32.h"
#include "..\..\Include\ntddndis.h"

#include <stdio.h>			// Header File For Standard Input/Output
#include <stdlib.h>
#include <fstream.h>
#include <conio.h>
#include <stdarg.h>			// Header File For Variable Argument Routines
#include <math.h>			// Header File For Math Stuff
#include <windows.h>		// Header File For Windows
#include <gl\glut.h>		// Header File For The GLUT Library
#include <gl\glaux.h>		// Header File For The GLAUX Library





// winpcap stuffs
#define Max_Num_Adapter 10

void printpacket(unsigned char *data, int datalength);
char *toip(int myhost);
char *tobwidth(double bwidth);
char *tomac(unsigned char *mac);

unsigned int localnet; // ipclass
unsigned int mask; // subnet mask

	
	//define a pointer to an ADAPTER structure

	LPADAPTER  lpAdapter = 0;

	//define a pointer to a PACKET structure

	LPPACKET   lpPacket;

	DWORD      dwErrorCode;

	DWORD dwVersion;
	DWORD dwWindowsMajorVersion;

	//unicode strings (winnt)
	WCHAR		AdapterName[8192]; // string that contains a list of the network adapters
	WCHAR		*temp,*temp1;

	//ascii strings (win95)
	char		AdapterNamea[8192]; // string that contains a list of the network adapters
	char		*tempa,*temp1a;


	int			AdapterNum=0,Open;
	ULONG		AdapterLength;
	
	char buffer[256000];  // buffer to hold the data coming from the driver

	struct bpf_stat stat;

	char        AdapterList[Max_Num_Adapter][1024];



// network data stuffs




// data parameters

#define packettimer 1 // check for and process packets
#define sampletimer 2 // update data structures
#define viztimera 3 // update viz structure
#define viztimerb 4 // move camera
#define halfsecondtimer 5 // one second incremental counter

int halfsecondval;
bool halfvals[20]; //  1 for random timer, 

void checkpackets();
void updatehosts();
void updateviz();
void movecamera();

// overall data

int sampleperiod;
#define defaultperiod 1000

int ageridgrhost;
#define defaultridgrhost 0

int ageridgrdsthost;
#define defaultridgrdsthost 1


// grob data stuffs

#define networklist 31
#define pi 3.14159

struct grdsthost
{
	grdsthost *upper;
	unsigned int ipaddr; // IP Address of destination host
	int databytes[10]; // how many bytes transferred to/from this host
	double bwidth; // bandwidth in Kbps
	double color[3]; // last know rgb values
	int age; // number of consecutive empty periods
	grdsthost *lower;

};	

struct grhost
{
	grhost *upper;
	unsigned int ipaddr;

	int tobytes[10]; // bytes to host
	int frombytes[10]; // bytes from host
	
	double tobwidth; // bandwidth to host in Kbps
	double frombwidth; // bandwidth from host in Kbps

	unsigned char mac[6]; // mac address

	int numconnected;

	double color[3]; // last know rgb values
	int age; // number of consecutive empty periods
	grdsthost *cap;
	grdsthost *base;
	grhost *lower;
};

grhost *basegrhost;
grhost *capgrhost;

grhost *allhosts[65536];

grhost *newgrhost()
{
	grhost *newbie;
	newbie = new grhost;
	newbie->base = new grdsthost;
	newbie->cap = new grdsthost;
	newbie->base->lower = NULL;
	newbie->base->upper = newbie->cap;
	newbie->cap->lower = newbie->base;
	newbie->cap->upper = NULL;

	newbie->color[0] = 0;
	newbie->color[1] = 0;
	newbie->color[2] = 0;
	
	newbie->age = 0;	
	
	newbie->lower = capgrhost->lower;
	capgrhost->lower->upper = newbie;
	newbie->upper = capgrhost;
	capgrhost->lower = newbie;

	return newbie;
}

void delgrhost(grhost *delme)
{

	allhosts[delme->ipaddr & 0x0000ffff] = NULL;

	delme->upper->lower = delme->lower;
	delme->lower->upper = delme->upper;

	grdsthost *delmeb;
	delmeb = delme->base;
	while(delmeb != NULL)
	{
		delme->base = delmeb;
		delmeb = delmeb->upper;
		delete delme->base;
	}

	delete delme;

}

grdsthost *newgrdsthost(grhost *addto)
{
	grdsthost *newbie;
	newbie = new grdsthost;

	newbie->color[0] = 0;
	newbie->color[1] = 0;
	newbie->color[2] = 0;
		
	newbie->age = 0;
	
	newbie->lower = addto->cap->lower;
	addto->cap->lower->upper = newbie;
	newbie->upper = addto->cap;
	addto->cap->lower = newbie;

	return newbie;
}

void delgrdsthost(grdsthost *delme)
{
	delme->upper->lower = delme->lower;
	delme->lower->upper = delme->upper;
	delete delme;
}


void initializedata()
{
	basegrhost = new grhost;
	capgrhost = new grhost;
	basegrhost->lower = NULL;
	basegrhost->upper = capgrhost;
	capgrhost->lower = basegrhost;
	capgrhost->upper = NULL;

	for(int ctx=0; ctx<65536; ctx++)
	{
		allhosts[ctx] = NULL;
	}


}



// stats
char topupload[5][30];
char topdownload[5][30];
char toptotal[5][30];
char topconnect[5][30];


char netsource[30];
char netsink[30];
char netwithin[30];
char nettotal[30];

int tonet[10];
int fromnet[10];
int withinnet[10];

double nettobwidth;
double netfrombwidth;
double netwithinbwidth;


unsigned int curfocus; // which host are we looking at right now?
int hostmode; // host modes: 0 = none, 1 = upload follower, 2 = download follower, 3 = total follower, 4 = connect follower, 5 = random
int vizmode; // viz modes: 0 = auto zoom, 1 = auto spin, 2 = manual 

// process: on every packet -	look up grhost - if not available, create new
//								then look up grdsthost - if not available, create new
//								add bytes to slot 0 of both grhost and grdsthost



void processpacket(int bytes, int local, unsigned int foreign, bool sourced)
{
	grhost *worker;
	if(allhosts[local] == NULL)
	{
		worker = newgrhost();
		allhosts[local] = worker;
		worker->ipaddr = (unsigned int) (localnet + (unsigned int)local);
		for(int ctx=0; ctx<10; ctx++)
		{
			worker->tobytes[ctx] = 0;
			worker->frombytes[ctx] = 0;
		}
	}

	worker = allhosts[local];

	grdsthost *workerb;
	workerb = worker->base->upper;

	while((workerb != worker->cap) && (workerb->ipaddr != foreign))
	{
		workerb = workerb->upper;
	}

	
	if(workerb == worker->cap)
	{

		workerb = newgrdsthost(worker);
		workerb->ipaddr = foreign;
		for(int ctx=0; ctx<10; ctx++)
		{
			workerb->databytes[ctx] = 0;
		}
	}
	
	workerb->databytes[0] = workerb->databytes[0] + bytes;

	if(sourced)
	{
		worker->frombytes[0] = worker->frombytes[0] + bytes;
	}
	else
	{
		worker->tobytes[0] = worker->tobytes[0] + bytes;
	}

}


void printpacket(unsigned char *data, int datalength)
{

	int local;
	unsigned int foreign;
	int bytes;


	unsigned char *readme;
	readme = data;
	if(datalength >= 34)
	{
		if( (*(readme+12) == 8) && (*(readme+13) == 0) )
		{
			
			unsigned int psrc;
			unsigned int pdest;
			unsigned int offset;

			psrc = 0;
			pdest = 0;
			psrc = (*(readme+26)*0x01000000) + (*(readme+27)*0x00010000) + (*(readme+28)*0x00000100) + *(readme+29);
			pdest = (*(readme+30)*0x01000000) + (*(readme+31)*0x00010000) + (*(readme+32)*0x00000100) + *(readme+33);

			int addme = 0;
			
			if( (psrc & mask) == localnet)
			{				
				offset = psrc & 0x0000ffff;
				local = offset;
				foreign = pdest;
				bytes = datalength;

				if((local & (mask ^ 0xffffffff)) != (mask ^ 0xffffffff))
				{
					processpacket(bytes, local, foreign, true);
					memcpy((void *) (allhosts[local]->mac), (void *)(readme+6), 6);
				}
				addme++;
				//sourced it
			}
			if( (pdest & mask) == localnet)
			{
				offset = pdest & 0x0000ffff;
				local = offset;
				foreign = psrc;
				bytes = datalength;
				if((local & (mask ^ 0xffffffff)) != (mask ^ 0xffffffff))
				{
					processpacket(bytes, local, foreign, false);
				}
				addme = addme+2;
				//sinked it
			}
			if(addme == 1)
			{
				fromnet[0] = fromnet[0] + datalength;
			}
			else if(addme == 2)
			{
				tonet[0] = tonet[0] + datalength;
			}
			else if(addme == 3)
			{
				withinnet[0] = withinnet[0] + datalength;
			}


		}
	}
	
}


void checkpackets() // check for and process packets
{

	PacketReceivePacket(lpAdapter,lpPacket,TRUE);	


	char *data;
	data = (char *)lpPacket->Buffer;
	int tbytes;
	tbytes = lpPacket->ulBytesReceived;
	int reader = 0;
	while(reader < tbytes)
	{
		bpf_hdr *header;
		header=(struct bpf_hdr *)(data+reader);
		int datalength = header->bh_datalen;
		int totallength = header->bh_caplen;
		reader = reader + header->bh_hdrlen;
		char *readme;
		readme = data+reader;
		printpacket((unsigned char *)readme, datalength);
		reader = Packet_WORDALIGN(reader+datalength);
	}


//	MessageBox(NULL, "processing packets", "Hello", MB_OK);

}

grhost *fup[5];
grhost *fdown[5];
grhost *ftot[5];
grhost *fconn[5];


void checktophost(grhost *looker)
{
	
	int pos;
	grhost *wkup;
	grhost *wkdown;
	grhost *wktot;
	grhost *wkconn;
	grhost *tswap;

	wkup = looker;
	wkdown = looker;
	wktot = looker;
	wkconn = looker;


	for(pos=4; pos>=0; pos--)
	{
		if(fup[pos] != NULL)
		{
			if(wkup->frombwidth >= fup[pos]->frombwidth)
			{
				tswap = wkup;
				wkup = fup[pos];
				fup[pos] = tswap;
			}
		}
		else
		{
			if( (wkup != fup[0]) && (wkup != fup[1]) && (wkup != fup[2]) && (wkup != fup[3]) && (wkup != fup[4]) ) 
			{
				fup[pos] = wkup;
			}
		}

		if(fdown[pos] != NULL)
		{
			if(wkdown->tobwidth >= fdown[pos]->tobwidth)
			{
				tswap = wkdown;
				wkdown = fdown[pos];
				fdown[pos] = tswap;
			}
		}
		else
		{
			if( (wkdown != fdown[0]) && (wkdown != fdown[1]) && (wkdown != fdown[2]) && (wkdown != fdown[3]) && (wkdown != fdown[4]) ) 
			{
				fdown[pos] = wkdown;
			}
		}

		if(ftot[pos] != NULL)
		{
			if((wktot->frombwidth + wktot->tobwidth) >= (ftot[pos]->frombwidth + ftot[pos]->tobwidth))
			{
				tswap = wktot;
				wktot = ftot[pos];
				ftot[pos] = tswap;
			}
		}
		else
		{
			if( (wktot != ftot[0]) && (wktot != ftot[1]) && (wktot != ftot[2]) && (wktot != ftot[3]) && (wktot != ftot[4]) ) 
			{
				ftot[pos] = wktot;
			}
		}

		if(fconn[pos] != NULL)
		{
			if(wkconn->numconnected >= fconn[pos]->numconnected)
			{
				tswap = wkconn;
				wkconn = fconn[pos];
				fconn[pos] = tswap;
			}
		}
		else
		{
			if( (wkconn != fconn[0]) && (wkconn != fconn[1]) && (wkconn != fconn[2]) && (wkconn != fconn[3]) && (wkconn != fconn[4]) ) 
			{
				fconn[pos] = wkconn;
			}
		}
	}
}


void updatehosts()
{

	nettobwidth = 0;
	netfrombwidth = 0;
	netwithinbwidth = 0;

	for(int ctxc=0; ctxc<5; ctxc++)
	{
		fup[ctxc] = NULL;
		fdown[ctxc] = NULL;
		ftot[ctxc] = NULL;
		fconn[ctxc] = NULL;
	}

	for(int ctxb=9; ctxb>0; ctxb--)
	{
		tonet[ctxb] = tonet[ctxb-1];
		nettobwidth = nettobwidth + tonet[ctxb-1];		
		fromnet[ctxb] = fromnet[ctxb-1];
		netfrombwidth = netfrombwidth + fromnet[ctxb-1];				
		withinnet[ctxb] = withinnet[ctxb-1];
		netwithinbwidth = netwithinbwidth + withinnet[ctxb-1];		
		
	}
	tonet[0] = 0;
	fromnet[0] = 0;
	withinnet[0] = 0;

	nettobwidth = nettobwidth*8/(9*(double)sampleperiod);
	netfrombwidth = netfrombwidth*8/(9*(double)sampleperiod);
	netwithinbwidth = netwithinbwidth*8/(9*(double)sampleperiod);

	strcpy(netsource, tobwidth(netfrombwidth));
	strcpy(netsink, tobwidth(nettobwidth));	
	strcpy(netwithin, tobwidth(netwithinbwidth));
	strcpy(nettotal, tobwidth(netfrombwidth+nettobwidth+netwithinbwidth));

	grhost *lookat;
	lookat = basegrhost->upper;



	


	while(lookat != capgrhost)
	{
		lookat->tobwidth = 0;
		lookat->frombwidth = 0;
		for(int ctx=9; ctx>0; ctx--)
		{
			lookat->tobytes[ctx] = lookat->tobytes[ctx-1];
			lookat->tobwidth = lookat->tobwidth + lookat->tobytes[ctx-1];
			lookat->frombytes[ctx] = lookat->frombytes[ctx-1];
			lookat->frombwidth = lookat->frombwidth + lookat->frombytes[ctx-1];
		}
		lookat->tobytes[0] = 0;
		lookat->frombytes[0] = 0;

		lookat->tobwidth = lookat->tobwidth*8/(9*(double)sampleperiod);
		lookat->frombwidth = lookat->frombwidth*8/(9*(double)sampleperiod);


		if((lookat->tobwidth + lookat->frombwidth) == 0)
		{
			lookat->age = lookat->age + 1;
		}
		else
		{
			lookat->age = 0;
		}
		

		if((lookat->age > ageridgrhost) && (ageridgrhost != 0))
		{
			lookat = lookat->lower;
			delgrhost(lookat->upper);
		}
		else
		{
			grdsthost *lookatb;
			lookatb = lookat->base->upper;
			while(lookatb != lookat->cap)
			{

				lookatb->bwidth = 0;

				for(int ctxb=9; ctxb>0; ctxb--)
				{
					lookatb->databytes[ctxb] = lookatb->databytes[ctxb-1];
					lookatb->bwidth = lookatb->bwidth + lookatb->databytes[ctxb-1];
				}
				lookatb->databytes[0] = 0;

				lookatb->bwidth = lookatb->bwidth*8/(9*(double)sampleperiod);
		

				if(lookatb->bwidth == 0)
				{
					lookatb->age = lookatb->age + 1;
					if((lookatb->age > ageridgrdsthost) && (ageridgrdsthost != 0))
					{
						lookatb = lookatb->lower;
						delgrdsthost(lookatb->upper);
					}
				}
				else
				{
					lookatb->age = 0;
				}

				lookatb = lookatb->upper;
			}

			lookat->numconnected = 0;
			lookatb = lookat->base->upper;
			while(lookatb != lookat->cap)
			{
				lookat->numconnected++;
				lookatb = lookatb->upper;
			}

			// do top check here
			if((lookat->ipaddr & (mask ^ 0xffffffff)) != (mask ^ 0xffffffff))
			{
				checktophost(lookat);
			}

		}
		lookat = lookat->upper;
	}

// update top 5 char lists
	for(int ctxd=0; ctxd<5; ctxd++)
	{
		char rank[4];
		strcpy(rank, "1. ");
		rank[0] = 53-ctxd;

		strcpy(topupload[ctxd], rank);
		if(fup[ctxd] != NULL)
		{
			strcat(topupload[ctxd], toip(fup[ctxd]->ipaddr));
		}

		strcpy(topdownload[ctxd], rank);
		if(fdown[ctxd] != NULL)
		{
			strcat(topdownload[ctxd], toip(fdown[ctxd]->ipaddr));
		}

		strcpy(toptotal[ctxd], rank);
		if(ftot[ctxd] != NULL)
		{
			strcat(toptotal[ctxd], toip(ftot[ctxd]->ipaddr));
		}

		strcpy(topconnect[ctxd], rank);
		if(fconn[ctxd] != NULL)
		{
			strcat(topconnect[ctxd], toip(fconn[ctxd]->ipaddr));
		}

	}


	if(hostmode == 1)
	{
		if(fup[4] != NULL)
		{
			curfocus = fup[4]->ipaddr;
		}
	}
	else if(hostmode == 2)
	{
		if(fdown[4] != NULL)
		{
			curfocus = fdown[4]->ipaddr;
		}
	}
	else if(hostmode == 3)
	{
		if(ftot[4] != NULL)
		{
			curfocus = ftot[4]->ipaddr;
		}
	}
	else if(hostmode == 4)
	{
		if(fconn[4] != NULL)
		{
			curfocus = fconn[4]->ipaddr;
		}
	}





}





typedef struct
{
	float x,y,z;
	float ax,ay,az;
}camera_t,*camera_t_ptr;








/*----------------------------------GLOBALS------------------------------------*/





HDC			hDC=NULL;		// Private GDI Device Context
HGLRC		hRC=NULL;		// Permanent Rendering Context
HWND		hWnd=NULL;		// Holds Our Window Handle
HINSTANCE	hInstance;		// Holds The Instance Of The Application

bool	keys[256];			// Array Used For The Keyboard Routine
bool	active=TRUE;		// Window Active Flag Set To TRUE By Default
bool	fullscreen=TRUE;	// Fullscreen Flag Set To Fullscreen Mode By Default


float lightturn,lightturn1;
float heading;

camera_t camera;

const float piover180 = 0.0174532925f;




void DrawNetwork(void);
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
	
	
	return TRUE;										// Initialization Went OK
}


/*-------------------------------LIGHTS STUFF-----------------------------------*/
void lights(void)
{

  glDisable(GL_LIGHTING);
}
/*--------------------------------------------------------------------------------*/


double rotor;
double irad;
bool iradcross;

double flop;

void movecamera() // move camera
{

	camera.x = irad*cos(rotor*pi/180);
	camera.z = irad*sin(rotor*pi/180);

	if(vizmode == 0)
	{
		if(flop == 0)
		{
			camera.ay = rotor+90 - (30-irad)/30*180*atan(1/(irad))/pi;
		}
		else
		{
			camera.ay = rotor+90 + (30-irad)/30*180*atan(1/(irad))/pi;
		}
	}
	else
	{
		camera.ay = rotor+90;
	}

	if(irad > 29.8)
	{
		if(iradcross == false)
		{
			flop--;
			if(flop < 0)
			{
				flop = 1;
			}
		}
		iradcross = true;
	}
	else
	{
		iradcross = false;
	}


	if(vizmode == 0)
	{
		camera.ax = 2*rotor;
		irad = 10+20*cos(rotor*pi/180);
		rotor = (int)(rotor + 1)%360;
	}
	else if(vizmode == 1)
	{
		camera.ax = 2*rotor;
		rotor = (int)(rotor + 1)%360;

	}

//	MessageBox(NULL, "moving camera", "Hello", MB_OK);
}


#define pixsize 0.02

	



void drawworda(char *drawme)
{
	
		for(unsigned int cntrw=0; cntrw<strlen(drawme); cntrw++)
		{
			
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, drawme[cntrw]);
//			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, drawme[cntrw]);
//			glutBitmapCharacter(GLUT_BITMAP_9_BY_15, drawme[cntrw]);
		}
}

void drawwordb(char *drawme)
{
	
		for(unsigned int cntrw=0; cntrw<strlen(drawme); cntrw++)
		{
			
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, drawme[cntrw]);
//			glutBitmapCharacter(GLUT_BITMAP_9_BY_15, drawme[cntrw]);
		}
}


#define curves 10

void closedcylinder(float radius, float height, float r, float g, float b)
{
	GLUquadricObj *temp;

	temp = gluNewQuadric(); 

	glColor3f(r,g,b);
	gluCylinder(temp, radius, radius, height, curves, 1);

	glColor3f(0.35,0.35,0.35);
	glTranslatef(0,0,0);
	glRotatef(180,0,1,0);
	gluDisk(temp, 0, radius, curves, 1);
	glRotatef(-180,0,1,0);	

	glColor3f(0.35,0.35,0.35);
	glTranslatef(0,0,height);	
	gluDisk(temp, 0, radius, curves, 1);
	glTranslatef(0,0,-1*height);

	


	gluDeleteQuadric(temp);

}


double targetcolor[3];


void settargetcolor(double bandwidth)
{

	targetcolor[0] = 0;
	targetcolor[1] = 0;
	targetcolor[2] = 0.25;
		
	if(bandwidth == 0)
	{
		targetcolor[0] = 0;
		targetcolor[1] = 0;
		targetcolor[2] = 0.5;
	}
	else if (bandwidth < 10)
	{
		targetcolor[0] = 0;
		targetcolor[1] = 1;
		targetcolor[2] = 0;
	}
	else if (bandwidth < 20)
	{
		targetcolor[0] = 0.1;
		targetcolor[1] = 1;
		targetcolor[2] = 0;
	}
	else if (bandwidth < 40)
	{
		targetcolor[0] = 0.2;
		targetcolor[1] = 1;
		targetcolor[2] = 0;
	}
	else if (bandwidth < 60)
	{
		targetcolor[0] = 0.3;
		targetcolor[1] = 1;
		targetcolor[2] = 0;
	}
	else if (bandwidth < 80)
	{
		targetcolor[0] = 0.4;
		targetcolor[1] = 1;
		targetcolor[2] = 0;
	}
	else if (bandwidth < 100)
	{
		targetcolor[0] = 0.5;
		targetcolor[1] = 1;
		targetcolor[2] = 0;
	}
	else if (bandwidth < 150)
	{
		targetcolor[0] = 0.6;
		targetcolor[1] = 1;
		targetcolor[2] = 0;
	}
	else if (bandwidth < 200)
	{
		targetcolor[0] = 0.7;
		targetcolor[1] = 1;
		targetcolor[2] = 0;
	}
	else if (bandwidth < 250)
	{
		targetcolor[0] = 0.8;
		targetcolor[1] = 1;
		targetcolor[2] = 0;
	}
	else if (bandwidth < 300)
	{
		targetcolor[0] = 0.9;
		targetcolor[1] = 1;
		targetcolor[2] = 0;
	}
	else if (bandwidth < 400)
	{
		targetcolor[0] = 1;
		targetcolor[1] = 1;
		targetcolor[2] = 0;
	}
	else if (bandwidth < 500)
	{
		targetcolor[0] = 1;
		targetcolor[1] = 0.9;
		targetcolor[2] = 0;
	}
	else if (bandwidth < 600)
	{
		targetcolor[0] = 1;
		targetcolor[1] = 0.8;
		targetcolor[2] = 0;
	}
	else if (bandwidth < 700)
	{
		targetcolor[0] = 1;
		targetcolor[1] = 0.7;
		targetcolor[2] = 0;
	}
	else if (bandwidth < 800)
	{
		targetcolor[0] = 1;
		targetcolor[1] = 0.6;
		targetcolor[2] = 0;
	}
	else if (bandwidth < 900)
	{
		targetcolor[0] = 1;
		targetcolor[1] = 0.5;
		targetcolor[2] = 0;
	}
	else if (bandwidth < 1000)
	{
		targetcolor[0] = 1;
		targetcolor[1] = 0.4;
		targetcolor[2] = 0;
	}
	else if (bandwidth < 1100)
	{
		targetcolor[0] = 1;
		targetcolor[1] = 0.3;
		targetcolor[2] = 0;
	}
	else if (bandwidth < 1200)
	{
		targetcolor[0] = 1;
		targetcolor[1] = 0.2;
		targetcolor[2] = 0;
	}
	else if (bandwidth < 1300)
	{
		targetcolor[0] = 1;
		targetcolor[1] = 0.1;
		targetcolor[2] = 0;
	}
	else 
	{
		targetcolor[0] = 1;
		targetcolor[1] = 0;
		targetcolor[2] = 0;
	}
}


char qchar[30];
char *toip(int myhost)
{
	char x[10];
	char *cng;
	_gcvt((myhost & 0xff000000)/0x01000000, 5, x);
	cng = strstr(x, ".");
	if(cng != NULL)
	{
		*cng = '\0';
	}
	strcpy(qchar, x);
	strcat(qchar, ".");
	_gcvt((myhost & 0x00ff0000)/0x00010000, 5, x);
	cng = strstr(x, ".");
	if(cng != NULL)
	{
		*cng = '\0';
	}
	strcat(qchar, x);
	strcat(qchar, ".");
	_gcvt((myhost & 0x0000ff00)/256, 5, x);
	cng = strstr(x, ".");
	if(cng != NULL)
	{
		*cng = '\0';
	}
	strcat(qchar, x);
	strcat(qchar, ".");
	_gcvt((myhost & 0x000000ff), 5, x);
	cng = strstr(x, ".");
	if(cng != NULL)
	{
		*cng = '\0';
	}
	strcat(qchar, x);
	return qchar;

}

char *tobwidth(double bwidth)
{
	if(bwidth < 1000)
	{
		sprintf(qchar, "%.2f Kbps", bwidth);
	}
	else
	{
		sprintf(qchar, "%.0f Kbps", bwidth);
	}

	return qchar;
}

char *tomac(unsigned char *mac)
{
	sprintf(qchar, "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return qchar;
}




void renderhost(grhost *lookat)
{

	double radius;
	double height;

	radius = 0.25;
	height = 1;
	
	
	glTranslatef(0,0,10);

	if(lookat == NULL)
	{
		closedcylinder(radius, height, 0, 0, 0.25);
	}
	else
	{

		// bandwidth now in kbps


		settargetcolor(lookat->tobwidth + lookat->frombwidth);


		lookat->color[0] = lookat->color[0] + (targetcolor[0] - lookat->color[0])/5;
		lookat->color[1] = lookat->color[1] + (targetcolor[1] - lookat->color[1])/5;
		lookat->color[2] = lookat->color[2] + (targetcolor[2] - lookat->color[2])/5;

		if(allhosts[curfocus & 0x0000ffff] == lookat)
		{
			glTranslatef(0,0,-3*radius);	
			glColor3f(1,0,1);
			glutSolidSphere(radius, 10, 10);
			glTranslatef(0,0,3*radius);				
		}
		closedcylinder(radius, height, lookat->color[0], lookat->color[1], lookat->color[2]);

// draw connected hosts


		int numconnected;
		numconnected = 0;
		grdsthost *lookatb;
		lookatb = lookat->base->upper;
		while(lookatb != lookat->cap)
		{
			numconnected++;
			lookatb = lookatb->upper;
		}
		lookatb = lookat->base;

		double spacebetween;
		spacebetween = 0.5;

		// figure out fit with spacing = 0.75*spacebetween
		// if fit > left, arrange evenly
		// else insert at spacebetween

		double hdist;
		hdist = 1;

		double curoff;
		curoff = 0;
		
		double circum;
		double fit;
		int put;
		
		

		glTranslatef(0,0,height);	

		while(numconnected > 0)
		{

			circum = sin(curoff*pi/180);
			circum = circum * hdist;
			circum = circum * 2 * pi;

			fit = circum / (0.75*spacebetween);
			fit = ceil(fit+0.0000001);


			if(fit > numconnected)
			{
				put = numconnected;
			}
			else
			{
				put = circum / (spacebetween);
				put = ceil(put+0.0000001);
			}


			// render put worth of hosts
	
			glPushMatrix();
			for(double zrot = 0; zrot < 360; zrot = zrot + (360/put) + 1)
			{

				// who's your daddy?
				numconnected = numconnected - 1;
				if(lookatb->upper != NULL)
				{
					lookatb = lookatb->upper;
					// bandwidth now in kbps
					settargetcolor(lookatb->bwidth);


					lookatb->color[0] = lookatb->color[0] + (targetcolor[0] - lookatb->color[0])/5;
					lookatb->color[1] = lookatb->color[1] + (targetcolor[1] - lookatb->color[1])/5;
					lookatb->color[2] = lookatb->color[2] + (targetcolor[2] - lookatb->color[2])/5;


					// rotate about z axis for (360/put)+1
					glRotatef((360/put)+1, 0,0,1);

					// rotate about x for curoff
					glRotatef(curoff, 1,0,0);

					
					closedcylinder(radius/10, hdist, lookatb->color[0], lookatb->color[1], lookatb->color[2]);
					
					// translate along z for hdist
					glTranslatef(0,0,hdist);

					//draw top cylinder

					closedcylinder(radius/4, height/4, lookatb->color[0], lookatb->color[1], lookatb->color[2]);

					glTranslatef(0,0,-1*hdist);
					glRotatef(-1*curoff, 1,0,0);
				}
					


			}
			glPopMatrix();


			curoff = curoff + 10;
		}
		


		glTranslatef(0,0,-1*height);

		









	}

	glTranslatef(0,0,-10);
}


void DrawNetwork(void)
{
	glPushMatrix();	


	glCallList(networklist);


	glPopMatrix();
	
	
}




void updateviz() // update display structures
{
	int numhosts;

	numhosts = 0;
	grhost *lookat;

	lookat = basegrhost->upper;
	while(lookat != capgrhost)
	{
		numhosts++;
		lookat = lookat->upper;
	}


	// numhosts determines parameters
	double howmany;
	howmany = 4*pi/(numhosts+5);
	howmany = sqrt(howmany);


	char temp[40];
	_gcvt(howmany, 5, temp);
//	MessageBox(NULL, temp, "Number hosts", MB_OK);
	

	double xval;
	double yval;
	xval = 180/(pi/howmany);

	lookat = basegrhost->upper;


	glNewList(networklist,GL_COMPILE);
	for(float cntx=-90; cntx<90-xval/2; cntx = cntx+ xval)
	{
		yval = 360/(cos(pi*cntx/180)*2*pi/howmany);
		for(float cnty=0; cnty<360-yval/2; cnty = cnty + yval)
		{
			glRotatef(cnty,0,1,0);
			glRotatef(cntx,1,0,0);


			if(lookat == capgrhost)
			{
				// draw empty host
				renderhost(NULL);
			}
			else
			{
				renderhost(lookat);
				lookat = lookat->upper;
			}

			glRotatef(-cntx,1,0,0);
			glRotatef(-cnty,0,1,0);				

		}
	}
	glEndList();

	

}





double loff;
double roff;
double roffa;
double toffdownload;
double lineoff;
double toffnetwork;
double moff;
double toffmode;




int DrawGLScene(GLvoid)									// Here's Where We Do All The Drawing
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer
	

	/*------------CAMERA STUFF-----------*/
	glPushMatrix();

	glRotatef(camera.ay, 0,1,0);
	glTranslatef(camera.x,camera.y,camera.z);

	double rotax[2];
	if(camera.x == 0)
	{
		rotax[0] = camera.z;
		rotax[1] = 0;
	}
	else if(camera.z == 0)
	{
		rotax[0] = 0;
		rotax[1] = -camera.x;	
	}
	else
	{
		rotax[0] = camera.z;
		rotax[1] = -camera.x;
	}
	glRotatef(camera.ax,rotax[0],0,rotax[1]);



	RECT trect;
	GetWindowRect(hWnd, &trect);

	DrawNetwork();	
	glPopMatrix();

	glColor3b(127,127,127);

	RECT wndsize;
	GetWindowRect(hWnd, &wndsize);

	double wndwidth = wndsize.right-wndsize.left - 8;
	double wndheight = wndsize.bottom-wndsize.top-27;

	double rwndtop = 3.1;
	double rwndbottom = -5.1;
	double rwndleft = -(rwndtop-rwndbottom)*wndwidth/wndheight/2;
	double rwndright = (rwndtop-rwndbottom)*wndwidth/wndheight/2;

	double rwndpix = 8.2/wndheight;

	//-5.5 to 5.5 = 11 units to width = 640
	//-5.1 to 3.1 = 8.2 units to height = 480


	loff = rwndleft + 10*rwndpix;
	roff = rwndright - 88*rwndpix;
	roffa = rwndright - 185*rwndpix;
	toffdownload = rwndtop - 15*rwndpix;
	lineoff = 12*rwndpix;
	toffnetwork = rwndbottom + 4*lineoff + 10*rwndpix;
	moff = -65*rwndpix;
	toffmode = rwndbottom + 10*rwndpix;
	



// top 5 upload
	glColor3b(127,127,127);
	glRasterPos2f(loff,toffdownload);
	drawworda("Top 5 Upload");
	glColor3b(0,127,127);
	glRasterPos2f(loff,toffdownload-1*lineoff);
	drawworda(topupload[4]);
	glRasterPos2f(loff,toffdownload-2*lineoff);
	drawworda(topupload[3]);	
	glRasterPos2f(loff,toffdownload-3*lineoff);
	drawworda(topupload[2]);
	glRasterPos2f(loff,toffdownload-4*lineoff);
	drawworda(topupload[1]);	
	glRasterPos2f(loff,toffdownload-5*lineoff);
	drawworda(topupload[0]);

// top 5 download
	glColor3b(127,127,127);
	glRasterPos2f(loff,toffdownload-7*lineoff);
	drawworda("Top 5 Download");
	glColor3b(0,127,127);
	glRasterPos2f(loff,toffdownload-8*lineoff);
	drawworda(topdownload[4]);
	glRasterPos2f(loff,toffdownload-9*lineoff);
	drawworda(topdownload[3]);
	glRasterPos2f(loff,toffdownload-10*lineoff);
	drawworda(topdownload[2]);
	glRasterPos2f(loff,toffdownload-11*lineoff);
	drawworda(topdownload[1]);
	glRasterPos2f(loff,toffdownload-12*lineoff);
	drawworda(topdownload[0]);


// Top 5 Total
	glColor3b(127,127,127);
	glRasterPos2f(roff,toffdownload);
	drawworda("Top 5 Total");
	glColor3b(0,127,127);
	glRasterPos2f(roff,toffdownload-1*lineoff);
	drawworda(toptotal[4]);
	glRasterPos2f(roff,toffdownload-2*lineoff);
	drawworda(toptotal[3]);	
	glRasterPos2f(roff,toffdownload-3*lineoff);
	drawworda(toptotal[2]);
	glRasterPos2f(roff,toffdownload-4*lineoff);
	drawworda(toptotal[1]);	
	glRasterPos2f(roff,toffdownload-5*lineoff);
	drawworda(toptotal[0]);

// Top 5 Connected
	glColor3b(127,127,127);
	glRasterPos2f(roff,toffdownload-7*lineoff);
	drawworda("Top 5 Connected");
	glColor3b(0,127,127);
	glRasterPos2f(roff,toffdownload-8*lineoff);
	drawworda(topconnect[4]);
	glRasterPos2f(roff,toffdownload-9*lineoff);
	drawworda(topconnect[3]);
	glRasterPos2f(roff,toffdownload-10*lineoff);
	drawworda(topconnect[2]);
	glRasterPos2f(roff,toffdownload-11*lineoff);
	drawworda(topconnect[1]);
	glRasterPos2f(roff,toffdownload-12*lineoff);
	drawworda(topconnect[0]);

// Network Stats
	glColor3b(127,127,127);
	glRasterPos2f(loff,toffnetwork);
	drawworda("Network Statistics - ");
	drawworda(toip(localnet));
	glColor3b(0,127,127);
	glRasterPos2f(loff,toffnetwork-1*lineoff);
	drawworda("Total Bandwidth:");
	glRasterPos2f(loff+90*rwndpix,toffnetwork-1*lineoff);
	drawworda(nettotal);
	glRasterPos2f(loff,toffnetwork-2*lineoff);
	drawworda("To Subnet:");
	glRasterPos2f(loff+90*rwndpix,toffnetwork-2*lineoff);
	drawworda(netsink);
	glRasterPos2f(loff,toffnetwork-3*lineoff);
	drawworda("From Subnet:");
	glRasterPos2f(loff+90*rwndpix,toffnetwork-3*lineoff);
	drawworda(netsource);
	glRasterPos2f(loff,toffnetwork-4*lineoff);
	drawworda("Within Subnet:");
	glRasterPos2f(loff+90*rwndpix,toffnetwork-4*lineoff);
	drawworda(netwithin);


// Viz Mode
	glColor3b(127,127,127);
	glRasterPos2f(moff-10*rwndpix,toffdownload);
	drawworda("Visualization Mode: ");
	glColor3b(0,127,127);
	glRasterPos2f(moff+85*rwndpix,toffdownload);
	if(vizmode == 0)
	{
		drawworda("Automatic Zoom");
	}
	else if(vizmode == 1)
	{
		drawworda("Automatic Spin");
	}
	else if(vizmode == 2)
	{
		drawworda("Manual");
	}



// Trace Mode
	glColor3b(127,127,127);
	glRasterPos2f(moff,toffmode);
	drawworda("Trace Mode: ");
	glColor3b(0,127,127);
	glRasterPos2f(moff+60*rwndpix,toffmode);
	if(hostmode == 0)
	{
		drawworda("Selected Host");
	}
	else if(hostmode == 1)
	{
		drawworda("Top Upload");
	}
	else if(hostmode == 2)
	{
		drawworda("Top Download");
	}
	else if(hostmode == 3)
	{
		drawworda("Top Total");
	}
	else if(hostmode == 4)
	{
		drawworda("Top Connect");
	}
	else if(hostmode == 5)
	{
		drawworda("Random");
	}
	

	if(hostmode == 5)
	{
		if( ((halfsecondval%4) == 0) && (halfvals[0] == true) )
		{
			int numhosts;
			numhosts = 0;
			grhost *lkat;
			lkat = basegrhost->upper;
			while(lkat != capgrhost)
			{
				numhosts++;
				lkat = lkat->upper;
			}
			numhosts = rand()%numhosts;
			lkat = basegrhost->upper;
			for(int ctxv=0; ctxv<numhosts; ctxv++)
			{
				lkat = lkat->upper;
			}
			curfocus = lkat->ipaddr;
			halfvals[0] = false;
		}
	}


	if(allhosts[curfocus & 0x0000ffff] == NULL)
	{
		unsigned int nextx;
		nextx = ((curfocus & 0x0000ffff) + 1) % 65536;
		while((nextx != curfocus) && (allhosts[nextx] == NULL)) 
		{
			nextx = (nextx + 1) % 65536;		
		}
		curfocus = nextx;
	}

	if(allhosts[curfocus & 0x0000ffff] != NULL)
	{
// Host Stats
		grhost *lookme;
		lookme = allhosts[curfocus & 0x0000ffff];

		glColor3b(127,127,127);
		glRasterPos2f(roffa,toffnetwork);
		drawworda("Host Statistics - ");
		drawworda(toip(lookme->ipaddr));
		glColor3b(0,127,127);
		glRasterPos2f(roffa,toffnetwork-1*lineoff);
		drawworda("MAC Address:");
		glRasterPos2f(roffa+90*rwndpix,toffnetwork-1*lineoff);
		drawworda(tomac(lookme->mac));
		glRasterPos2f(roffa,toffnetwork-2*lineoff);
		drawworda("Total Bandwidth:");
		glRasterPos2f(roffa+90*rwndpix,toffnetwork-2*lineoff);
		drawworda(tobwidth(lookme->tobwidth + lookme->frombwidth));
		glRasterPos2f(roffa,toffnetwork-3*lineoff);
		drawworda("From Host:");
		glRasterPos2f(roffa+90*rwndpix,toffnetwork-3*lineoff);
		drawworda(tobwidth(lookme->frombwidth));
		glRasterPos2f(roffa,toffnetwork-4*lineoff);
		drawworda("To Host:");
		glRasterPos2f(roffa+90*rwndpix,toffnetwork-4*lineoff);
		drawworda(tobwidth(lookme->tobwidth));
	}


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

		case WM_TIMER:
		{
			if(wParam == packettimer)
			{
				checkpackets();
			}
			else if(wParam == sampletimer)
			{
				updatehosts();
			}
			else if(wParam == viztimera)
			{
				updateviz();
			}
			else if(wParam == viztimerb)
			{
				movecamera();
			}
			else if(wParam == halfsecondtimer)
			{
				halfsecondval++;
				for(int ctxg=0; ctxg<20; ctxg++)
				{
					halfvals[ctxg] = true;
				}
			}
			

			return 0;
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

	
//	if (MessageBox(NULL,"Would You Like To Run In Fullscreen Mode?", "Start FullScreen?",MB_YESNO|MB_ICONQUESTION)==IDNO)
//	{
		fullscreen=FALSE;							// Windowed Mode
//	}

	// Create Our OpenGL Window
	if (!CreateGLWindow("OpenGL Network Viz",640,480,16,fullscreen))
	{
		return 0;									// Quit If Window Was Not Created
	}

/// Initialize stuffs here

	initializedata();	
	sampleperiod = defaultperiod;
	localnet = 0x12EE0000;

	system("ipconfig | find \"IP Address\" > tempipconfig.txt");
	char ipline[1024];
	ifstream readconfig;
	readconfig.open("tempipconfig.txt", ios::in);
	readconfig.get(ipline, 600);
	readconfig.close();
	system("del tempipconfig.txt");


	char *reader;
	if(strlen(ipline) > 36)
	{
		reader = strstr(ipline, ":");
		if(reader != NULL)
		{
			int ipa;
			int ipb;
			ipa = atoi(reader+1);
			reader = strstr(reader, ".");
			ipb = atoi(reader+1);

			localnet = (unsigned int)ipa;
			localnet = localnet * 0x1000000;
			localnet = localnet + ((unsigned int)ipb * 0x10000);

		}
	}
	
	
	mask = 0xffff0000;

	ageridgrhost = defaultridgrhost;

	ageridgrdsthost = defaultridgrdsthost;


	int pickme; // which adapter to pick
	pickme = 1;

// initialize pcap here

	int        i;
	i=0;	

	FILE *debugout;
	debugout = fopen("config-output.txt", "a");

	// the data returned by PacketGetAdapterNames is different in Win95 and in WinNT.
	// We have to check the os on which we are running
	dwVersion=GetVersion();
	dwWindowsMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
	if (!(dwVersion >= 0x80000000 && dwWindowsMajorVersion >= 4))
	{  // Windows NT
		AdapterLength = sizeof(AdapterName);

		if(PacketGetAdapterNames((char *)AdapterName,&AdapterLength)==FALSE){
			fprintf(debugout, "Unable to retrieve the list of the adapters!\n");
			return -1;
		}
		temp=AdapterName;
		temp1=AdapterName;
		while ((*temp!='\0')||(*(temp-1)!='\0'))
		{
			if (*temp=='\0') 
			{
				memcpy(AdapterList[i],temp1,(temp-temp1)*2);
				temp1=temp+1;
				i++;
		}
	
		temp++;
		}
	  
		AdapterNum=i;
		for (i=0;i<AdapterNum;i++)
			fwprintf(debugout, L"\n%d- %s\n",i+1,AdapterList[i]);
		fprintf(debugout, "\n");
		
	}

	else	//windows 95
	{
		AdapterLength = sizeof(AdapterNamea);

		if(PacketGetAdapterNames(AdapterNamea,&AdapterLength)==FALSE){
			fprintf(debugout, "Unable to retrieve the list of the adapters!\n");
			return -1;
		}
		tempa=AdapterNamea;
		temp1a=AdapterNamea;

		while ((*tempa!='\0')||(*(tempa-1)!='\0'))
		{
			if (*tempa=='\0') 
			{
				memcpy(AdapterList[i],temp1a,tempa-temp1a);
				temp1a=tempa+1;
				i++;
			}
			tempa++;
		}
		  
		AdapterNum=i;
		for (i=0;i<AdapterNum;i++)
			fprintf(debugout, "\n%d- %s\n",i+1,AdapterList[i]);
		fprintf(debugout, "\n");

	}

	if(pickme > AdapterNum)
	{
		pickme = AdapterNum;
	}
	
	if(AdapterNum == 0)
	{
		return 0;
	}


	lpAdapter =   PacketOpenAdapter(AdapterList[pickme-1]);
	if (!lpAdapter || (lpAdapter->hFile == INVALID_HANDLE_VALUE))
	{
		dwErrorCode=GetLastError();
		fprintf(debugout, "Unable to open the adapter, Error Code : %lx\n",dwErrorCode); 

		return -1;
	}	

	if(PacketSetHwFilter(lpAdapter,NDIS_PACKET_TYPE_PROMISCUOUS)==FALSE){
			fprintf(debugout, "Warning: unable to set promiscuous mode!\n");
	}

	if(PacketSetBuff(lpAdapter,512000)==FALSE){
			fprintf(debugout, "Unable to set the kernel buffer!\n");
			return -1;
	}

	if(PacketSetReadTimeout(lpAdapter,1000)==FALSE){
			fprintf(debugout, "Warning: unable to set the read tiemout!\n");
	}

	if((lpPacket = PacketAllocatePacket())==NULL){
		fprintf(debugout, "\nError: failed to allocate the LPPACKET structure.");
		return (-1);
	}

	PacketInitPacket(lpPacket,(char*)buffer,256000);




	fclose(debugout);

/// done initializing, start the timers





	SetTimer(hWnd, packettimer, 200, NULL); // how often to check for packets
	SetTimer(hWnd, sampletimer, sampleperiod, NULL); // how often to update information
	SetTimer(hWnd, viztimera, 100, NULL); // viz structure animation rate (10 fps)
	SetTimer(hWnd, viztimerb, 40, NULL); // camera rate (25 fps)
	SetTimer(hWnd, halfsecondtimer, 500, NULL);

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
					SwapBuffers(hDC); // Swap Buffers (Double Buffering)

				}
		
			
			if (keys[VK_RIGHT])
			{
				hostmode = 0;
				curfocus = ((curfocus & 0x0000ffff) + 1)%65536; 
				if(allhosts[curfocus & 0x0000ffff] == NULL)
				{
					unsigned int nextx;
					nextx = ((curfocus & 0x0000ffff) + 1) % 65536;
					while((nextx != curfocus) && (allhosts[nextx] == NULL)) 
					{
						nextx = (nextx + 1) % 65536;		
					}
					curfocus = nextx;
				}
				keys[VK_RIGHT] = FALSE;
			}
			if (keys[VK_LEFT])
			{
				hostmode = 0;
				curfocus = ((curfocus & 0x0000ffff) - 1)%65536; 
				if(allhosts[curfocus & 0x0000ffff] == NULL)
				{
					unsigned int nextx;
					nextx = ((curfocus & 0x0000ffff) - 1) % 65536;
					while((nextx != curfocus) && (allhosts[nextx] == NULL)) 
					{
						nextx = (nextx - 1) % 65536;		
					}
					curfocus = nextx;
				}
				keys[VK_LEFT] = FALSE;
			}
			if (keys[VK_UP])
			{
				hostmode = 0;
				curfocus = ((curfocus & 0x0000ffff) + 256)%65536; 
				if(allhosts[curfocus & 0x0000ffff] == NULL)
				{
					unsigned int nextx;
					nextx = ((curfocus & 0x0000ffff) + 1) % 65536;
					while((nextx != curfocus) && (allhosts[nextx] == NULL)) 
					{
						nextx = (nextx + 1) % 65536;		
					}
					curfocus = nextx;
				}
				keys[VK_UP] = FALSE;
			}
			if (keys[VK_DOWN])
			{
				hostmode = 0;
				curfocus = ((curfocus & 0x0000ffff) - 256)%65536; 
				if(allhosts[curfocus & 0x0000ffff] == NULL)
				{
					unsigned int nextx;
					nextx = ((curfocus & 0x0000ffff) - 1) % 65536;
					while((nextx != curfocus) && (allhosts[nextx] == NULL)) 
					{
						nextx = (nextx - 1) % 65536;		
					}
					curfocus = nextx;
				}
				keys[VK_DOWN] = FALSE;
			}


			if (keys['T'])
			{
				hostmode = (hostmode + 1)%6;
				keys['T'] = FALSE;
			}

			if (keys['V'])
			{
				vizmode = (vizmode + 1)%3;
				keys['V'] = FALSE;
			}

			if(keys['R'])
			{
				camera.ax = 0;
				rotor = 0;
				irad = 30;
			}

			if((vizmode == 1) || (vizmode == 2))
			{
				if(keys['U'])
				{
					irad = max(1, irad - 0.4);
				}
				if(keys['O'])
				{
					irad = min(30, irad + 0.4);
				}
			}


			if(vizmode == 2)
			{
				if(keys['J'])
				{
					rotor = (int)(rotor-1)%360;
				}
				if(keys['L'])
				{
					rotor = (int)(rotor+1)%360;
				}
				if(keys['I'])
				{
					camera.ax = (int)(camera.ax+1)%360;
				}
				if(keys['K'])
				{
					camera.ax = (int)(camera.ax-1)%360;
				}


			}



		}
	}

	// Shutdown
	KillGLWindow();									// Kill The Window
	return (msg.wParam);							// Exit The Program
}
