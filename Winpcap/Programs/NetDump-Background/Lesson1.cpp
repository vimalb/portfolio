//
// GLSAMPLE.CPP
//  by Blaine Hodge
//

// Includes

#include "..\..\Include\packet32.h"
#include "..\..\Include\ntddndis.h"

#include <stdio.h>			// Header File For Standard Input/Output
#include <stdlib.h>
#include <fstream.h>
#include <conio.h>
#include <stdarg.h>			// Header File For Variable Argument Routines
#include <math.h>

#include <windows.h>
#include <gl/gl.h>
#include <gl\glut.h>		// Header File For The GLUT Library
#include <gl\glaux.h>		// Header File For The GLAUX Library





// winpcap stuffs
#define Max_Num_Adapter 10

void printpacket(unsigned char *data, int datalength);

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


void drawpacketbuffer();


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

#define packettimer 1 // check for and process packets


//////////// Packet functions

void printpacket(unsigned char *data, int datalength);

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



}














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







// Function Declarations

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void EnableOpenGL(HWND hWnd, HDC * hDC, HGLRC * hRC);
void DisableOpenGL(HWND hWnd, HDC hDC, HGLRC hRC);

// WinMain
HDC hDC;
HWND hWnd;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
				   LPSTR lpCmdLine, int iCmdShow)
{
	WNDCLASS wc;
	HGLRC hRC;
	MSG msg;
	BOOL quit = FALSE;
	

	
	
	
		
	
	// register window class
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon( NULL, IDI_APPLICATION );
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "GLSample";
	RegisterClass( &wc );
	
	// create main window

/*
	hWnd = CreateWindow( 
		"GLSample", "OpenGL Sample", 
		WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE,
		0, 0, 256, 256,
		NULL, NULL, hInstance, NULL );
*/


	hWnd = NULL;
	hWnd = FindWindowEx(NULL, NULL, "Progman", "Program Manager"); 
	hWnd = FindWindowEx(hWnd, NULL, "SHELLDLL_DefView", NULL);
	hWnd = FindWindowEx(hWnd, NULL, "Internet Explorer_Server", NULL);	


	if(hWnd == NULL)
	{
		hWnd = CreateWindow("GLSample", "OpenGL Sample", WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE, 0, 0, 256, 256, NULL, NULL, hInstance, NULL );
	}
  

	// enable OpenGL for the window
	EnableOpenGL( hWnd, &hDC, &hRC );

	


////////////// network stuffs


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


	SetTimer(hWnd, packettimer, 200, NULL); // how often to check for packets



///////////////end net stuffs




	// program main loop
	while ( !quit )
	{
		
		// check for messages
		if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE )  )
		{
			
			// handle or dispatch messages
			if ( msg.message == WM_QUIT ) 
			{
				quit = TRUE;
			} 
			else 
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
			
		} 
		else 
		{
			Sleep(30);

			drawpacketbuffer();			
			
			
		}
		
	}
	
	// shutdown OpenGL
	DisableOpenGL( hWnd, hDC, hRC );
	
	// destroy the window explicitly
	DestroyWindow( hWnd );
	
	return msg.wParam;
	
}


int bufptr;
unsigned char drawpacketbuf[100][2000];


void drawpacketbuffer()
{
	RECT wndsize;
	GetWindowRect(hWnd, &wndsize);

	double wndwidth = wndsize.right-wndsize.left - 8;
	double wndheight = wndsize.bottom-wndsize.top-27;


	double rwndpix = 2/wndheight;


		for(float cnty=0; cnty<56; cnty=cnty+1)
		{
			glRasterPos2f(0,-28*12*rwndpix+(12*cnty*rwndpix));
			drawworda("Network Statistics - ");
			
		}
	SwapBuffers( hDC );

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
					// sourced packet
//					processpacket(bytes, local, foreign, true);
//					memcpy((void *) (allhosts[local]->mac), (void *)(readme+6), 6);
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
					// sunk packet
//					processpacket(bytes, local, foreign, false);
				}
				addme = addme+2;
				//sinked it
			}



			if(addme == 1)
			{
			}
			else if(addme == 2)
			{
			}
			else if(addme == 3)
			{
			}


		}
	}
	
}


























// Window Procedure

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	
	switch (message)
	{
		
	case WM_CREATE:
		return 0;
		
	case WM_CLOSE:
		PostQuitMessage( 0 );
		return 0;

	case WM_TIMER:
		if(wParam == packettimer)
		{
			checkpackets();
		}
		return 0;
		

	case WM_DESTROY:
		return 0;
		
	case WM_KEYDOWN:
		switch ( wParam )
		{
			
		case VK_ESCAPE:
			PostQuitMessage(0);
			return 0;
			
		}
		return 0;
	
	default:
		return DefWindowProc( hWnd, message, wParam, lParam );
			
	}
	
}

// Enable OpenGL

void EnableOpenGL(HWND hWnd, HDC * hDC, HGLRC * hRC)
{
	PIXELFORMATDESCRIPTOR pfd;
	int format;
	
	// get the device context (DC)
	*hDC = GetDC( hWnd );
	
	// set the pixel format for the DC
	ZeroMemory( &pfd, sizeof( pfd ) );
	pfd.nSize = sizeof( pfd );
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 16;
	pfd.iLayerType = PFD_MAIN_PLANE;
	format = ChoosePixelFormat( *hDC, &pfd );
	SetPixelFormat( *hDC, format, &pfd );
	
	// create and enable the render context (RC)
	*hRC = wglCreateContext( *hDC );
	wglMakeCurrent( *hDC, *hRC );
	
}

// Disable OpenGL

void DisableOpenGL(HWND hWnd, HDC hDC, HGLRC hRC)
{
	wglMakeCurrent( NULL, NULL );
	wglDeleteContext( hRC );
	ReleaseDC( hWnd, hDC );
}
