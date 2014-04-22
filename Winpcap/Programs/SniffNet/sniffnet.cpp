#include <afxwin.h>


#include <fstream.h>
#include <string.h>
#include <stdlib.h>
#include <iomanip.h>
#include <time.h>

#include "..\..\Include\packet32.h"
#include "..\..\Include\ntddndis.h"

#define IDlog 3001
#define IDselectadapter 3002
#define IDstartnetmonitor 3011
#define IDstopnetmonitor 3012
#define IDnetmonitor 3013
#define IDnettimer 3015
#define IDkillme 3016

#define IDintegrate 4001

int killvalue;
int monitoring;



void showme(char *value)
{
	MessageBox(NULL, value, "String", MB_OK);
}

void showme(int value)
{
	char buf[20];
	_gcvt(value, 15, buf);
	MessageBox(NULL, buf, "Number", MB_OK);
}





char qchar[30];
char *intdec(double value)
{
	_gcvt(value, 8, qchar);
	char *ret;
	ret = strstr(qchar, ".");
	if(ret != NULL)
	{
		*ret = '\0';
	}
	ret = qchar;
	return ret;
}
char *inthex(double value)
{
	int x;
	int y;
	x = (int)value;
	x = x%256;
	x = x/16;
	y = (int)value;
	y = y%16;	
	char tbuf[5];
	if(x>10)
	{
		qchar[0] = 55+x;
	}
	else
	{
		_gcvt(x, 3, tbuf);
		qchar[0] = tbuf[0];
	}
	if(y>10)
	{
		qchar[1] = 55+y;
	}
	else
	{
		_gcvt(y, 3, tbuf);
		qchar[1] = tbuf[0];
	}
	qchar[2] = '\0';
	char *ret;
	ret = qchar;
	return ret;
}


struct dispfilter
{
	dispfilter *upper;
	char match[4000]; // hex string to match
	int location; // -1 for anywhere, else 0 based index
	int color; // RGB color display value, -1 if hidden
	dispfilter *lower;
};

dispfilter *basefilter;
dispfilter *capfilter;





int print; // 0 = don't print, 1 = print
char printfilename[100]; // if print, append to this file

int datamode; // 0 = hex, 1 = ASCII

int adapter; // select adapter number on computer
char fontname[100]; 
int poscounter;

//HMODULE packetlib;
LPADAPTER sadapter;
LPPACKET spacket;
char *sbuffer[512000];


unsigned int zeros[65536];

// For pretty colors
unsigned int integrationtime; // Time at which rxbytes and txbytes are zeroed = 1 period
unsigned int rxbytes[65536]; // bytes received by host
unsigned int txbytes[65536]; // bytes sent by host
unsigned char mac[65536][6]; // mac addresses
unsigned int incomingbytes;
unsigned int outgoingbytes;
unsigned int withinbytes;
bool existing[65536];


unsigned int ipclass; // subnet ip 
unsigned int mask; // subnet mask

#define hsize 65 // Change this and DIE, BITCH!
unsigned int txhistory[hsize][65536]; // buffer of 64 history values
unsigned int rxhistory[hsize][65536]; // buffer of 64 history values
unsigned int incominghistory[hsize]; // buffer of 64 history values
unsigned int outgoinghistory[hsize]; // buffer of 64 history values
unsigned int withinhistory[hsize]; // buffer of 64 history values


int hiscount;
int resolution; // how many periods per history cycle
int rescount;

int curhost;
bool dirty;
int dispoff; // display offset





char *toip(int myhost)
{
	char x[10];
	char *cng;
	_gcvt(ipclass/0x01000000, 5, x);
	cng = strstr(x, ".");
	if(cng != NULL)
	{
		*cng = '\0';
	}
	strcpy(qchar, x);
	strcat(qchar, ".");
	_gcvt((ipclass & 0x00ffffff)/0x00010000, 5, x);
	cng = strstr(x, ".");
	if(cng != NULL)
	{
		*cng = '\0';
	}
	strcat(qchar, x);
	strcat(qchar, ".");
	_gcvt(myhost/256, 5, x);
	cng = strstr(x, ".");
	if(cng != NULL)
	{
		*cng = '\0';
	}
	strcat(qchar, x);
	strcat(qchar, ".");
	_gcvt(myhost%256, 5, x);
	cng = strstr(x, ".");
	if(cng != NULL)
	{
		*cng = '\0';
	}
	strcat(qchar, x);
	while(strlen(qchar) < 15)
	{
		strcat(qchar, " ");
	}
	return qchar;

}


int toprx[128];
int toptx[128];
int toptot[128];

unsigned int ftrxbytes[65536];
unsigned int fttxbytes[65536];

void caltop(unsigned int *rx, unsigned int *tx) // must be arrays of length 
{

	memcpy((void *)ftrxbytes, (void *)rx, 65536);
	memcpy((void *)fttxbytes, (void *)tx, 65536);
	rx=ftrxbytes;
	tx=fttxbytes;

	bool ckrx;
	bool cktx;
	bool cktot;

	for(int ctx=0; ctx<128; ctx++)
	{
		toprx[ctx] = 1;
		toptx[ctx] = 1;
		toptot[ctx] = 1;
	}
	for(int ctxa=1; ctxa<65536; ctxa++)
	{
		if(existing[ctxa])
		{
			ckrx=true;
			cktx = true;
			cktot = true;
			for(int lookat=0; lookat<128; lookat++)
			{
				// check for pos lookat rx
				if(ckrx && (rx[ctxa] > rx[toprx[lookat]]) )
				{
					for(int updater=127; updater>lookat; updater--)
					{
						toprx[updater] = toprx[updater-1];
					}
					toprx[lookat] = ctxa;
					ckrx = false;
				}

				if(cktx && (tx[ctxa] > tx[toptx[lookat]]) )
				{
					for(int updater=127; updater>lookat; updater--)
					{
						toptx[updater] = toptx[updater-1];
					}
					toptx[lookat] = ctxa;
					cktx = false;
				}

				if(cktot && (rx[ctxa]+tx[ctxa] > rx[toptot[lookat]]+tx[toptot[lookat]]) )
				{
					for(int updater=127; updater>lookat; updater--)
					{
						toptot[updater] = toptot[updater-1];
					}
					toptot[lookat] = ctxa;
					cktot = false;
				}

			}
		}
	}


}





// most recent frames (bytes)
unsigned int trxbytes[65536];
unsigned int ttxbytes[65536];
int ttotbytes;
int tobytes;
int tibytes;
int twbytes;



// most recent bandwidths (kbps)
char obwidth[50];
char wbwidth[50];
char ibwidth[50]; 












class introscreen : public CFrameWnd
{
public:
	introscreen();



	void cleanup()
	{

		
		delete log;
		delete startnetmonitor;
		delete stopnetmonitor;
		delete netmonitor;

		delete selectadapter;

		dispfilter *eraser;
		eraser = basefilter;
		while(eraser != NULL)
		{
			basefilter = eraser;
			eraser = eraser->upper;
			delete basefilter;
		}



//		if(packetlib != NULL)
//		{
//			FreeLibrary(packetlib);
//		}

	}
	afx_msg void OnClose()
	{
		cleanup();
		DestroyWindow();
	}

	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

	afx_msg void MHstartnetmonitor();
	afx_msg void MHstopnetmonitor();
	afx_msg void MHselectadapter();

	afx_msg void OnTimer(UINT nIDEvent);

	void printpacket(unsigned char *data, int datalength);
	void integrate();
	void badintegrate();

	void drawhost(CDC *drawme);
	void drawhistory(CDC *drawme);


	CButton *startnetmonitor;
	CButton *stopnetmonitor;
	CEdit *netmonitor;
	CListBox *log;

	CListBox *selectadapter;



private:

	DECLARE_MESSAGE_MAP()
};


BEGIN_MESSAGE_MAP(introscreen, CFrameWnd)
	ON_WM_CLOSE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_COMMAND(IDstartnetmonitor, MHstartnetmonitor)
	ON_COMMAND(IDstopnetmonitor, MHstopnetmonitor)
	ON_LBN_DBLCLK(IDselectadapter, MHselectadapter)
	ON_WM_TIMER()
END_MESSAGE_MAP()


CWnd *workit;
CDC *drawme;
CDC *drawback;



introscreen::introscreen()
{
	Create(NULL, "NTnetop 2.0", WS_VISIBLE|WS_OVERLAPPEDWINDOW, CRect(0,0,1024,740), NULL, NULL);


	

	monitoring = 0;
	dispoff = 0;



//	packetlib = LoadLibrary("packet.dll");
	
	log = new CListBox;
	log->Create(WS_CHILD|WS_VSCROLL|WS_BORDER|LBS_DISABLENOSCROLL|LBS_NOINTEGRALHEIGHT|LBS_NOTIFY, CRect(30,300,250,380), this, IDlog);

	selectadapter = new CListBox;
	selectadapter->Create(WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_BORDER|LBS_DISABLENOSCROLL|LBS_NOINTEGRALHEIGHT|LBS_NOTIFY, CRect(30,30,430,130), this, IDselectadapter);

	netmonitor = new CEdit;
	netmonitor->Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_CENTER|ES_READONLY, CRect(460,30,600,60), this, IDnetmonitor);
	netmonitor->SetWindowText("Select Adapter");

	startnetmonitor = new CButton;
	startnetmonitor->Create("Start", BS_PUSHBUTTON|WS_TABSTOP|WS_CHILD|WS_BORDER|SS_CENTER, CRect(280,440,468,465), this, IDstartnetmonitor);	

	stopnetmonitor = new CButton;															   
	stopnetmonitor->Create("Stop", BS_PUSHBUTTON|WS_TABSTOP|WS_CHILD|WS_BORDER|SS_CENTER, CRect(280,470,468,495), this, IDstopnetmonitor);	



	for(int ctbx=0; ctbx<65536; ctbx++)
	{
		zeros[ctbx] = 0;
		existing[ctbx] = false;
		rxbytes[ctbx] = 0;
		txbytes[ctbx] = 0;
		for(int ctxc=0; ctxc<hsize; ctxc++)
		{
			rxhistory[ctxc][ctbx] = 0;
			txhistory[ctxc][ctbx] = 0;
		}
		for(ctxc=0; ctxc<8; ctxc++)
		{
			mac[ctbx][ctxc] = 0;
		}
	}
	

			char AdapterList[10][1024];

//			ULONG (*PacketGetAdapterNames) (PTSTR, PULONG);
//			PacketGetAdapterNames = (ULONG (*) (PTSTR, PULONG)) GetProcAddress(packetlib, "PacketGetAdapterNames"); 
			
//			LPADAPTER (*PacketOpenAdapter) (LPTSTR);
//			PacketOpenAdapter = (LPADAPTER (*) (LPTSTR)) GetProcAddress(packetlib, "PacketOpenAdapter"); 

			char adnamemod[1024];

//  The following block of code has been shamelessly lifted from Testapp.c

//	Testapp.c is part of the winpcap developer pack examples.
/*
 * Copyright (c) 1999, 2000
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
			
			
			int        i;

			DWORD dwVersion;
			DWORD dwWindowsMajorVersion;

			//unicode strings (winnt)
			WCHAR		AdapterName[8192]; // string that contains a list of the network adapters
			WCHAR		*temp,*temp1;



			int			AdapterNum=0;
			ULONG		AdapterLength;
			
			
			// obtain the name of the adapters installed on this machine
			AdapterLength=512;
			
			i=0;	


			
	// the data returned by PacketGetAdapterNames is different in Win95 and in WinNT.
	// We have to check the os on which we are running
	dwVersion=GetVersion();
	dwWindowsMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
	if (!(dwVersion >= 0x80000000 && dwWindowsMajorVersion >= 4))
	{  // Windows NT
		AdapterLength = sizeof(AdapterName);

		ofstream outer;
		outer.open("debug.txt", ios::out);

		if(PacketGetAdapterNames((char *)AdapterName,&AdapterLength)==FALSE){
//			fprintf(debugout, "Unable to retrieve the list of the adapters!\n");
//			return -1;
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
				outer << i << "\n";

			}
	
		temp++;
		}
	  
		AdapterNum=i;
		for (i=0;i<AdapterNum;i++)
		{
			swprintf((unsigned short *)adnamemod, L"\n%d- %s\n",i+1,AdapterList[i]);
			selectadapter->AddString(adnamemod);
		}

		outer.close();
	}



//// end shamelessly lifted code

	sadapter = NULL;
	spacket = NULL;

	killvalue = 0;

	basefilter = new dispfilter;
	capfilter = new dispfilter;
	basefilter->lower = NULL;
	basefilter->upper = capfilter;
	capfilter->lower = basefilter;
	capfilter->upper = NULL;



	int noip[4];
	noip[0] = 0;
	noip[1] = 0;
	noip[2] = 0;
	noip[3] = 0;










	print = 0;
	integrationtime = 500;
	resolution = 120;
	rescount = 0;
	hiscount = 0;
	ipclass = 0x12ee0000;
	mask = 0xffff0000;
	curhost = 0;
	dirty = true;
//	logthreshold = 3000;


	ifstream readpref;
	readpref.open("ntnetop.dat", ios::in);
	char linebuf[4000];
	readpref.get(linebuf, 3999);
	if(strstr(linebuf, "Valid!") == linebuf)
	{
		readpref.ignore(200, '\n');
		readpref.get(linebuf, 3999);
		while(strcmp(linebuf, "") == 0)
		{
			readpref.ignore(200, '\n');
			readpref.get(linebuf, 3999);
		}

		while(strstr(linebuf, "filters") != linebuf)
		{
			if(strstr(linebuf, "print") == linebuf)
			{
				readpref.ignore(200, '\n');
				readpref.get(linebuf, 3999);
				print = atoi(linebuf);
				readpref.ignore(200, '\n');
				readpref.get(printfilename, 99);
				ofstream outer;
				outer.open(printfilename, ios::out);
				outer << "\n";
				outer.close();
			}

			if(strstr(linebuf, "integrate") == linebuf)
			{
				readpref.ignore(200, '\n');
				readpref.get(linebuf, 3999);
				integrationtime = atoi(linebuf);
			}

			if(strstr(linebuf, "resolution") == linebuf)
			{
				readpref.ignore(200, '\n');
				readpref.get(linebuf, 3999);
				resolution = atoi(linebuf);
			}
			
			if(strstr(linebuf, "ipclass") == linebuf)
			{
				readpref.ignore(200, '\n');
				readpref.get(linebuf, 3999);

				char *ipa;
				char *ipb;
				char *ipc;
				char *ipd;
				ipa = linebuf;
				ipb = strstr(ipa, ".");
				if(ipb != NULL)
				{
					ipb++;
					ipc = strstr(ipb, ".");
					if(ipc != NULL)
					{
						ipc++;
						ipd = strstr(ipc, ".");
					}
					if(ipd != NULL)
					{
						ipd++;
					}
					unsigned int locip;
					int tempx;
					tempx = atoi(ipa);
					locip = tempx * 0x1000000;
					tempx = atoi(ipb);
					locip = locip + (tempx * 0x10000);
					tempx = atoi(ipc);
					locip = locip + (tempx * 0x100);
					tempx = atoi(ipd);
					locip = locip + (tempx * 0x1);
					ipclass = locip;
				}



				readpref.ignore(200, '\n');
				readpref.get(linebuf, 3999);

				ipa = linebuf;
				ipb = strstr(ipa, ".");
				if(ipb != NULL)
				{
					ipb++;
					ipc = strstr(ipb, ".");
					if(ipc != NULL)
					{
						ipc++;
						ipd = strstr(ipc, ".");
					}
					if(ipd != NULL)
					{
						ipd++;
					}
					unsigned int locip;
					int tempx;
					tempx = atoi(ipa);
					locip = tempx * 0x1000000;
					tempx = atoi(ipb);
					locip = locip + (tempx * 0x10000);
					tempx = atoi(ipc);
					locip = locip + (tempx * 0x100);
					tempx = atoi(ipd);
					locip = locip + (tempx * 0x1);
					mask = locip;
				}
			}

			readpref.ignore(200, '\n');
			readpref.get(linebuf, 3999);
			while(strcmp(linebuf, "") == 0)
			{
				readpref.ignore(200, '\n');
				readpref.get(linebuf, 3999);
			}
		}

		readpref.ignore(200, '\n');
		readpref.get(linebuf, 3999);
		while(strcmp(linebuf, "") == 0)
		{
			readpref.ignore(200, '\n');
			readpref.get(linebuf, 3999);
		}


		while(strstr(linebuf, "endfilters") != linebuf)
		{
			dispfilter *newbie;
			newbie = new dispfilter;
			capfilter->lower->upper = newbie;
			newbie->lower = capfilter->lower;
			newbie->upper = capfilter;
			capfilter->lower = newbie;
			strcpy(newbie->match, linebuf);
			readpref.ignore(200, '\n');
			readpref.get(linebuf, 3999);
			newbie->location = atoi(linebuf);
			readpref.ignore(200, '\n');
			readpref.get(linebuf, 3999);
			newbie->color =	atoi(linebuf);


			readpref.ignore(200, '\n');
			readpref.get(linebuf, 3999);
			while(strcmp(linebuf, "") == 0)
			{
				readpref.ignore(200, '\n');
				readpref.get(linebuf, 3999);
			}
		}

	}

	readpref.close();
	
	ipclass = ipclass & mask;



//	if(packetlib == NULL)
//	{
//		SetTimer(IDkillme, 100, NULL);
//	}


}


afx_msg void introscreen::MHselectadapter()
{
	adapter = selectadapter->GetCurSel() + 1;	

	selectadapter->ModifyStyle(WS_VISIBLE, 0, 0);
//	log->ModifyStyle(0, WS_VISIBLE, SWP_SHOWWINDOW);
//	log->InvalidateRect(NULL);
	netmonitor->SetWindowText("Not monitoring net");

	netmonitor->MoveWindow(CRect(280,405,468,435));

	startnetmonitor->ModifyStyle(0, WS_VISIBLE, SWP_SHOWWINDOW);
	startnetmonitor->InvalidateRect(NULL);
	stopnetmonitor->ModifyStyle(0, WS_VISIBLE, SWP_SHOWWINDOW);
	stopnetmonitor->InvalidateRect(NULL);



	InvalidateRect(NULL);
}



char AdapterList[10][1024];


afx_msg void introscreen::MHstartnetmonitor()
{
	if(monitoring == 0)
	{
		if(sadapter == NULL)
		{

			char AdapterList[10][1024];

//			ULONG (*PacketGetAdapterNames) (PTSTR, PULONG);
//			PacketGetAdapterNames = (ULONG (*) (PTSTR, PULONG)) GetProcAddress(packetlib, "PacketGetAdapterNames"); 
			
//			LPADAPTER (*PacketOpenAdapter) (LPTSTR);
//			PacketOpenAdapter = (LPADAPTER (*) (LPTSTR)) GetProcAddress(packetlib, "PacketOpenAdapter"); 

//  The following block of code has been shamelessly lifted from Testapp.c

//	Testapp.c is part of the winpcap developer pack examples.
/*
 * Copyright (c) 1999, 2000
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
			
			
			
			int        i;

			DWORD dwVersion;
			DWORD dwWindowsMajorVersion;

			//unicode strings (winnt)
			WCHAR		AdapterName[8192]; // string that contains a list of the network adapters
			WCHAR		*temp,*temp1;



			int			AdapterNum=0,Open;
			ULONG		AdapterLength;
			
			
			// obtain the name of the adapters installed on this machine
			AdapterLength=512;
			
			i=0;	

	// the data returned by PacketGetAdapterNames is different in Win95 and in WinNT.
	// We have to check the os on which we are running
	dwVersion=GetVersion();
	dwWindowsMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
	if (!(dwVersion >= 0x80000000 && dwWindowsMajorVersion >= 4))
	{  // Windows NT
		AdapterLength = sizeof(AdapterName);

		if(PacketGetAdapterNames((char *)AdapterName,&AdapterLength)==FALSE){
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
	  
		
	}


			

///// This is the end of the blatently lifted code.

			Open = adapter;

			sadapter = PacketOpenAdapter(AdapterList[Open-1]);

			if((sadapter != NULL) && (sadapter->hFile != INVALID_HANDLE_VALUE))
			{
				
//				BOOLEAN (*PacketSetHwFilter) (LPADAPTER,ULONG);
//				PacketSetHwFilter = (BOOLEAN (*) (LPADAPTER,ULONG)) GetProcAddress(packetlib, "PacketSetHwFilter"); 
				PacketSetHwFilter(sadapter, NDIS_PACKET_TYPE_PROMISCUOUS);

//				BOOLEAN (*PacketSetBuff) (LPADAPTER,int);
//				PacketSetBuff = (BOOLEAN (*) (LPADAPTER,int)) GetProcAddress(packetlib, "PacketSetBuff"); 
				PacketSetBuff(sadapter, 512000);

//				LPPACKET (*PacketAllocatePacket) ();
//				PacketAllocatePacket = (LPPACKET (*) ()) GetProcAddress(packetlib, "PacketAllocatePacket"); 
				spacket =  PacketAllocatePacket();

//				VOID (*PacketInitPacket) (LPPACKET, PVOID, UINT);
//				PacketInitPacket = (VOID (*) (LPPACKET, PVOID, UINT)) GetProcAddress(packetlib, "PacketInitPacket"); 
				PacketInitPacket(spacket, (char *)sbuffer, 512000);

				poscounter = 0;

				SetTimer(IDnettimer, 5, NULL);
				SetTimer(IDintegrate, integrationtime, NULL);
			}
			else
			{
				MessageBox("Could not open adapter.", "Error");
				sadapter = NULL;
			}
		}

		drawback = NULL;
		monitoring = 1;
		netmonitor->SetWindowText("Monitoring net");
	}
}

int gst;

afx_msg void introscreen::MHstopnetmonitor()
{
	if(monitoring == 1)
	{
		KillTimer(IDnettimer);
		KillTimer(IDintegrate);
		if(drawback != NULL)
		{
			drawback->DeleteDC();
			delete drawback;
		}
		if(sadapter != NULL)
		{
//			VOID (*PacketFreePacket) (LPPACKET);
//			PacketFreePacket = (VOID (*) (LPPACKET)) GetProcAddress(packetlib, "PacketFreePacket"); 
			PacketFreePacket(spacket);

//			VOID (*PacketCloseAdapter) (LPADAPTER);
//			PacketCloseAdapter = (VOID (*) (LPADAPTER)) GetProcAddress(packetlib, "PacketCloseAdapter"); 
			PacketCloseAdapter(sadapter);
			sadapter = NULL;
		}
		monitoring = 0;
		netmonitor->SetWindowText("Not monitoring net");
  	 }
}







void introscreen::printpacket(unsigned char *data, int datalength)
{
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
			
			if( (psrc & mask) == ipclass)
			{				
				offset = psrc & 0x0000ffff;
				txbytes[offset] += datalength;
				txhistory[hiscount][offset] += datalength;
				addme++;
				existing[offset] = true;
				memcpy((void *)mac[offset], (void *)(readme+6), 6);
				//sourced it
			}
			if( (pdest & mask) == ipclass)
			{
				offset = pdest & 0x0000ffff;
				rxbytes[offset] += datalength;
				rxhistory[hiscount][offset] += datalength;
				addme = addme+2;
				//sinked it
			}
			if(addme == 1)
			{
				outgoingbytes += datalength;
				outgoinghistory[hiscount] += datalength;
			}
			else if(addme == 2)
			{
				incomingbytes += datalength;
				incominghistory[hiscount] += datalength;
			}
			else if(addme == 3)
			{
				withinbytes += datalength;
				withinhistory[hiscount] += datalength;
			}


		}
	}

}



afx_msg void introscreen::OnTimer(UINT nIDEvent)
{

	
	
	if(nIDEvent == IDnettimer)
	{
		if(sadapter != NULL)
		{

//			BOOLEAN (*PacketReceivePacket) (LPADAPTER,LPPACKET,BOOLEAN);
//			PacketReceivePacket = (BOOLEAN (*) (LPADAPTER,LPPACKET,BOOLEAN)) GetProcAddress(packetlib, "PacketReceivePacket"); 
			PacketReceivePacket(sadapter, spacket, TRUE);



			char *data;
			data = (char *)spacket->Buffer;
			int tbytes;
			tbytes = spacket->ulBytesReceived;
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
	}
	if(nIDEvent == IDkillme)
	{
		killvalue++;
		if(killvalue > 3)
		{
			KillTimer(IDkillme);
			MessageBox("Could not find packet.dll", "Error");
			SendMessage(WM_CLOSE);
		}
	}
	if(nIDEvent == IDintegrate)
	{
		integrate();		
	}


}




#define roff 30
#define toff 33
#define bsize 15
#define lwidth 1

void introscreen::integrate()
{
	ttotbytes = incomingbytes+outgoingbytes+withinbytes;
	tobytes = outgoingbytes;
	tibytes = incomingbytes;
	twbytes = withinbytes;
	
	incomingbytes = 0;
	outgoingbytes = 0;
	withinbytes = 0;
	
	memcpy((void *)trxbytes, (void *)rxbytes, 262144);
	memcpy((void *)ttxbytes, (void *)txbytes, 262144);
	memcpy((void *)rxbytes, (void *)zeros, 262144);
	memcpy((void *)txbytes, (void *)zeros, 262144);

	CDC *drawme;
	drawme = this->GetDC();

	drawhost(drawme);

	rescount++;
	rescount = rescount%resolution;
	if(rescount == 0)
	{
		hiscount++;
		hiscount = hiscount%hsize;
		memcpy((void *)txhistory[hiscount], (void *)zeros, 262144);
		memcpy((void *)rxhistory[hiscount], (void *)zeros, 262144);
		outgoinghistory[hiscount] = 0;
		incominghistory[hiscount] = 0;
		withinhistory[hiscount] = 0;

		
		dirty = true;
	} 

	if(dirty)
	{
		drawhistory(drawme);
		dirty = false;
	}


	int index;


	CBrush brusha(RGB(0,0,0));
	CBrush brushaa(RGB(0,0,127));
	CBrush brushb(RGB(31,255,0));
	CBrush brushc(RGB(63,255,0));
	CBrush brushd(RGB(95,255,0));
	CBrush brushe(RGB(127,255,0));
	CBrush brushf(RGB(159,255,0));
	CBrush brushg(RGB(191,255,0));
	CBrush brushh(RGB(223,255,0));
	CBrush brushi(RGB(255,255,0));
	CBrush brushj(RGB(255,223,0));
	CBrush brushk(RGB(255,191,0));
	CBrush brushl(RGB(255,159,0));
	CBrush brushm(RGB(255,127,0));
	CBrush brushn(RGB(255,95,0));
	CBrush brusho(RGB(255,63,0));
	CBrush brushp(RGB(255,31,0));
	CBrush brushq(RGB(255,0,0));


	for(int row=0; row<24; row++)
	{
		for(int col=0; col<64; col++)
		{
			index = 256*dispoff+(row*64)+col;
			double kilobps;
			kilobps = trxbytes[index] + ttxbytes[index];
			kilobps = kilobps*8000/(integrationtime*1024);
			if(kilobps == 0)
			{
				if(existing[index])
				{
					drawme->FillRect(CRect(roff+bsize*col+lwidth,toff+bsize*row+lwidth,roff+bsize*col+bsize,toff+bsize*row+bsize), &brushaa);

				}
				else
				{
					drawme->FillRect(CRect(roff+bsize*col+lwidth,toff+bsize*row+lwidth,roff+bsize*col+bsize,toff+bsize*row+bsize), &brusha);

				}
			}
			else if(kilobps < 50)
			{
				drawme->FillRect(CRect(roff+bsize*col+lwidth,toff+bsize*row+lwidth,roff+bsize*col+bsize,toff+bsize*row+bsize), &brushb);
			}
			else if(kilobps < 100)
			{
				drawme->FillRect(CRect(roff+bsize*col+lwidth,toff+bsize*row+lwidth,roff+bsize*col+bsize,toff+bsize*row+bsize), &brushc);
			}
			else if(kilobps < 200)
			{
				drawme->FillRect(CRect(roff+bsize*col+lwidth,toff+bsize*row+lwidth,roff+bsize*col+bsize,toff+bsize*row+bsize), &brushd);
			}
			else if(kilobps < 300)
			{
				drawme->FillRect(CRect(roff+bsize*col+lwidth,toff+bsize*row+lwidth,roff+bsize*col+bsize,toff+bsize*row+bsize), &brushe);
			}
			else if(kilobps < 400)
			{
				drawme->FillRect(CRect(roff+bsize*col+lwidth,toff+bsize*row+lwidth,roff+bsize*col+bsize,toff+bsize*row+bsize), &brushf);
			}
			else if(kilobps < 500)
			{
				drawme->FillRect(CRect(roff+bsize*col+lwidth,toff+bsize*row+lwidth,roff+bsize*col+bsize,toff+bsize*row+bsize), &brushg);
			}
			else if(kilobps < 650)
			{
				drawme->FillRect(CRect(roff+bsize*col+lwidth,toff+bsize*row+lwidth,roff+bsize*col+bsize,toff+bsize*row+bsize), &brushh);
			}
			else if(kilobps < 800)
			{
				drawme->FillRect(CRect(roff+bsize*col+lwidth,toff+bsize*row+lwidth,roff+bsize*col+bsize,toff+bsize*row+bsize), &brushi);
			}
			else if(kilobps < 950)
			{
				drawme->FillRect(CRect(roff+bsize*col+lwidth,toff+bsize*row+lwidth,roff+bsize*col+bsize,toff+bsize*row+bsize), &brushj);
			}
			else if(kilobps < 1100)
			{
				drawme->FillRect(CRect(roff+bsize*col+lwidth,toff+bsize*row+lwidth,roff+bsize*col+bsize,toff+bsize*row+bsize), &brushk);
			}
			else if(kilobps < 1250)
			{
				drawme->FillRect(CRect(roff+bsize*col+lwidth,toff+bsize*row+lwidth,roff+bsize*col+bsize,toff+bsize*row+bsize), &brushl);
			}
			else if(kilobps < 1400)
			{
				drawme->FillRect(CRect(roff+bsize*col+lwidth,toff+bsize*row+lwidth,roff+bsize*col+bsize,toff+bsize*row+bsize), &brushm);
			}
			else if(kilobps < 1600)
			{
				drawme->FillRect(CRect(roff+bsize*col+lwidth,toff+bsize*row+lwidth,roff+bsize*col+bsize,toff+bsize*row+bsize), &brushn);
			}
			else if(kilobps < 1800)
			{
				drawme->FillRect(CRect(roff+bsize*col+lwidth,toff+bsize*row+lwidth,roff+bsize*col+bsize,toff+bsize*row+bsize), &brusho);
			}
			else if(kilobps < 2000)
			{
				drawme->FillRect(CRect(roff+bsize*col+lwidth,toff+bsize*row+lwidth,roff+bsize*col+bsize,toff+bsize*row+bsize), &brushp);
			}
			else 
			{
				drawme->FillRect(CRect(roff+bsize*col+lwidth,toff+bsize*row+lwidth,roff+bsize*col+bsize,toff+bsize*row+bsize), &brushq);
			}




		}


	}

	






	char message[100];
	char bwidth[50];
	double tkbps = ttotbytes;
	tkbps = tkbps*8000/(integrationtime*1024);
	_gcvt(tkbps, 6, bwidth);
	strcpy(message, bwidth);
	strcat(message, " Kbps");

	

//	netmonitor->SetWindowText("hi");
//	caltop(rxbytes, txbytes);
	netmonitor->SetWindowText(message);


	drawme->DeleteDC();


}



#define hhisx 19 
#define hhisy 690
#define hhisw 7
#define hhish 182

#define thisx 490 
#define thisy 690
#define thisw 8
#define thish 260

#define upx 997
#define upy 33
#define downx 997
#define downy 391
#define dirw 15


void introscreen::drawhost(CDC *drawme)
{
	int myhost;
	myhost = curhost;

	char ip[30];
	char x[10];
	strcpy(ip, toip(myhost));
	
	
	char rbwidth[50];
	char sbwidth[50];


	double tkbps = trxbytes[myhost];
	tkbps = tkbps*8000/(integrationtime*1024);
	_gcvt(tkbps, 6, rbwidth);
	while(strlen(rbwidth) < 7)
	{
		strcat(rbwidth, " ");
	}

	tkbps = ttxbytes[myhost];
	tkbps = tkbps*8000/(integrationtime*1024);
	_gcvt(tkbps, 6, sbwidth);
	while(strlen(sbwidth) < 7)
	{
		strcat(sbwidth, " ");
	}

	tkbps = tobytes;
	tkbps = tkbps*8000/(integrationtime*1024);
	_gcvt(tkbps, 6, obwidth);
	while(strlen(obwidth) < 7)
	{
		strcat(obwidth, " ");
	}


	tkbps = tibytes;
	tkbps = tkbps*8000/(integrationtime*1024);
	_gcvt(tkbps, 6, ibwidth);
	while(strlen(ibwidth) < 7)
	{
		strcat(ibwidth, " ");
	}

	tkbps = twbytes;
	tkbps = tkbps*8000/(integrationtime*1024);
	_gcvt(tkbps, 6, wbwidth);
	while(strlen(wbwidth) < 7)
	{
		strcat(wbwidth, " ");
	}


	
	char message[1000];
	strcpy(message,   "IP Address: ");
	strcat(message, ip);
	strcat(message, "\nTo host:    ");
	strcat(message, rbwidth);
	strcat(message, " Kbps");
	strcat(message, "\nFrom host:  ");
	strcat(message, sbwidth);
	strcat(message, " Kbps");

	strcat(message, "\n\nTo subnet:     ");
	strcat(message, ibwidth);
	strcat(message, " Kbps");
	strcat(message, "\nFrom subnet:   ");
	strcat(message, obwidth);
	strcat(message, " Kbps");
	strcat(message, "\nWithin subnet: ");
	strcat(message, wbwidth);
	strcat(message, " Kbps");








	drawme->SetTextColor(RGB(0,0,0));

	CFont packetfont;
	packetfont.CreateFont(12,0,0,0,FW_NORMAL,0,0,0,0,0,0,0,0,"Courier");

	CBrush backbrush;
	backbrush.CreateSolidBrush(RGB(255,255,255));




	CPen backpen;
	backpen.CreatePen(PS_SOLID, 14, RGB(255,255,255));
	drawme->SelectObject(&backpen);
	drawme->SelectObject(&backbrush);
	drawme->SelectObject(&packetfont);

			
	DrawTextEx(*drawme, message, strlen(message), CRect(30,405,250,570), DT_NOCLIP, NULL);	


	strcpy(message, "");
	double logmin;
	logmin = integrationtime*resolution;
	logmin = logmin*4/60000;
	if(logmin > 1)
	{
		_gcvt(logmin, 3, x);
		strcat(message, "Log parameters: ");
		strcat(message, x);
		strcat(message, " Min, ");
	}
	else
	{
		_gcvt(logmin*60, 3, x);
		strcat(message, "Log parameters: ");
		strcat(message, x);
		strcat(message, " Sec, ");
	}
//	strcat(message, "\nTraffic interval: ");
	strcat(message, "1000");
	strcat(message, " Kbps");

	DrawTextEx(*drawme, message, strlen(message), CRect(620,405,830,430), DT_NOCLIP, NULL);	

	strcpy(message, "Host Log");
	DrawTextEx(*drawme, message, strlen(message), CRect(210,697,500,730), DT_NOCLIP, NULL);	

	strcpy(message, "Subnet Log");
	DrawTextEx(*drawme, message, strlen(message), CRect(707,697,800,730), DT_NOCLIP, NULL);	


	CPen gridpen;
	gridpen.CreatePen(PS_SOLID, 1, RGB(64,64,64));
	drawme->SelectObject(&gridpen);
	for(int ctx=0; ctx<65; ctx++)
	{
		drawme->MoveTo(roff+(ctx*bsize),toff);
		drawme->LineTo(roff+(ctx*bsize),toff+bsize*24);
	}
	for(ctx=0; ctx<25; ctx++)
	{
		drawme->MoveTo(roff,toff+(ctx*bsize));
		drawme->LineTo(roff+64*bsize+1,toff+(ctx*bsize));
	}

	CPen markpen;
	markpen.CreatePen(PS_SOLID, 1, RGB(255,0,0));
	drawme->SelectObject(&markpen);
	for(ctx=0; ctx<65; ctx=ctx+4)
	{
		drawme->MoveTo(roff+(ctx*bsize),toff-11);
		drawme->LineTo(roff+(ctx*bsize),toff);
		_gcvt(ctx, 5, message);
		char *changer;
		changer = strstr(message, ".");
		if(changer != NULL)
		{
			*changer = '\0';
		}
		if(ctx>10)
		{
			DrawTextEx(*drawme, message, strlen(message), CRect(roff+(ctx*bsize)-8,toff-26,roff+(ctx*bsize)+3,toff-14), DT_NOCLIP, NULL);	
		}
		else
		{
			DrawTextEx(*drawme, message, strlen(message), CRect(roff+(ctx*bsize)-4,toff-26,roff+(ctx*bsize)+3,toff-14), DT_NOCLIP, NULL);	
		}

	}

	
	drawme->FillRect(CRect(roff-25,toff-10,roff,toff+25*bsize+6), &backbrush);
	for(ctx=0; ctx<25; ctx=ctx+4)
	{
		_gcvt(dispoff+ctx/4, 5, message);
		char *changer;
		changer = strstr(message, ".");
		if(changer != NULL)
		{
			*changer = '\0';
		}
		if(strlen(message) == 1)
		{
			drawme->MoveTo(roff-11,toff+(ctx*bsize));
			drawme->LineTo(roff,toff+(ctx*bsize));
			DrawTextEx(*drawme, message, strlen(message), CRect(roff-22,toff+(ctx*bsize)-6,roff-12,toff+(ctx*bsize)+6), DT_NOCLIP, NULL);	
		}
		else if(strlen(message) == 2)
		{
			drawme->MoveTo(roff-9,toff+(ctx*bsize));
			drawme->LineTo(roff,toff+(ctx*bsize));
			DrawTextEx(*drawme, message, strlen(message), CRect(roff-25,toff+(ctx*bsize)-6,roff-12,toff+(ctx*bsize)+6), DT_NOCLIP, NULL);	
		}
		else
		{
			drawme->MoveTo(roff-7,toff+(ctx*bsize));
			drawme->LineTo(roff,toff+(ctx*bsize));
			DrawTextEx(*drawme, message, strlen(message), CRect(roff-27,toff+(ctx*bsize)-6,roff-12,toff+(ctx*bsize)+6), DT_NOCLIP, NULL);	
		}

	}

}





void introscreen::drawhistory(CDC *drawme)
{

	drawme->SetTextColor(RGB(0,0,0));

	CFont packetfont;
	packetfont.CreateFont(12,0,0,0,FW_NORMAL,0,0,0,0,0,0,0,0,"Courier");

	CBrush arrowfillbrush;
	arrowfillbrush.CreateSolidBrush(RGB(128,128,128));

	CBrush backbrush;
	backbrush.CreateSolidBrush(RGB(255,255,255));
	CBrush tfillbrush;
	tfillbrush.CreateSolidBrush(RGB(255,0,0));
	CBrush rfillbrush;
	rfillbrush.CreateSolidBrush(RGB(0,255,0));

	CBrush ofillbrush;
	ofillbrush.CreateSolidBrush(RGB(255,0,0));
	CBrush wfillbrush;
	wfillbrush.CreateSolidBrush(RGB(255,255,0));
	CBrush ifillbrush;
	ifillbrush.CreateSolidBrush(RGB(0,255,0));

	
	CPen backpen;
	backpen.CreatePen(PS_SOLID, 1, RGB(0,0,0));
	CPen redpen;
	redpen.CreatePen(PS_SOLID, 2, RGB(255,0,0));
	CPen greenpen;
	greenpen.CreatePen(PS_SOLID, 2, RGB(0,255,0));
	
	drawme->SelectObject(&redpen);
	drawme->SelectObject(&backbrush);
	drawme->SelectObject(&packetfont);

			
//	DrawTextEx(*drawme, message, strlen(message), CRect(30,380,250,490), DT_NOCLIP, NULL);	

//	drawme->FillRect(CRect(upx, upy,upx+dirw,upy+dirw), &arrowfillbrush);
//	drawme->FillRect(CRect(downx, downy,downx+dirw,downy-dirw), &arrowfillbrush);
	char uplet[3];
	char downlet[3];
	strcpy(downlet, ">>");
	strcpy(uplet, "<<");
	DrawTextEx(*drawme, uplet, strlen(uplet), CRect(upx,upy,upx+5,upy+5), DT_NOCLIP, NULL);	
	DrawTextEx(*drawme, downlet, strlen(downlet), CRect(downx,downy-dirw+2,downx+5,downy-dirw+7), DT_NOCLIP, NULL);	

	int offset;
	offset = hiscount+1;
	int myhost;
	myhost = curhost;
	double theight;
	double rheight;

	drawme->FillRect(CRect(hhisx-1,hhisy-hhish-1,hhisx+64*hhisw+1,hhisy+1), &backbrush);


	for(int ctx=0; ctx<64; ctx++)
	{
		int idx = (offset+ctx)%65;
		theight = txhistory[idx][myhost];
		rheight = rxhistory[idx][myhost];
	
		theight = (theight*8000)/(1024*integrationtime*resolution);
		rheight = (rheight*8000)/(1024*integrationtime*resolution);

		if(theight > 7000)
		{
			theight = 7000;
			rheight = 0;
		}
		else if((theight + rheight) > 7000)
		{
			rheight = 7000-theight;

		}
		theight =  theight*hhish/7000;
		rheight =  rheight*hhish/7000;

		drawme->FillRect(CRect(hhisx+(ctx*hhisw),hhisy-(int)theight,hhisx+(ctx*hhisw)+hhisw,hhisy), &tfillbrush);		
		drawme->FillRect(CRect(hhisx+(ctx*hhisw),hhisy-(int)(theight+rheight),hhisx+(ctx*hhisw)+hhisw,hhisy-int(theight)), &rfillbrush);		

	
	}


	
	drawme->SelectObject(&backpen);
	for(ctx=0; ctx<17; ctx++)
	{
		drawme->MoveTo(hhisx+(ctx*4*hhisw),hhisy);
		drawme->LineTo(hhisx+(ctx*4*hhisw),hhisy-hhish-1);
	}
	for(ctx=0; ctx<8; ctx++)
	{
		drawme->MoveTo(hhisx,hhisy-hhish+(ctx*hhish/7));
		drawme->LineTo(hhisx+64*hhisw,hhisy-hhish+(ctx*hhish/7));
	}


/////////////
	drawme->FillRect(CRect(thisx-1,thisy-thish-1,thisx+64*thisw+1,thisy+1), &backbrush);

	double oheight;
	double iheight;
	double wheight;

	for(ctx=0; ctx<64; ctx++)
	{
		int idx = (offset+ctx)%65;
		oheight = outgoinghistory[idx];
		iheight = incominghistory[idx];
		wheight = withinhistory[idx];
	
		oheight = (oheight*8000)/(1024*integrationtime*resolution);
		iheight = (iheight*8000)/(1024*integrationtime*resolution);
		wheight = (wheight*8000)/(1024*integrationtime*resolution);

		if(oheight > 10000)
		{
			oheight = 10000;
			iheight = 0;
			wheight = 0;
		}
		else if((oheight + wheight) > 10000)
		{
			wheight = 10000-oheight;
			iheight = 0;
		}
		else if((oheight + wheight + iheight) > 10000)
		{
			iheight = 10000-oheight-wheight;
		}

		oheight =  oheight*thish/10000;
		wheight =  wheight*thish/10000;
		iheight =  iheight*thish/10000;

		drawme->FillRect(CRect(thisx+(ctx*thisw),thisy-(int)oheight,thisx+(ctx*thisw)+thisw,thisy), &ofillbrush);		
		drawme->FillRect(CRect(thisx+(ctx*thisw),thisy-(int)(oheight+wheight),thisx+(ctx*thisw)+thisw,thisy-int(oheight)), &wfillbrush);
		drawme->FillRect(CRect(thisx+(ctx*thisw),thisy-(int)(oheight+wheight+iheight),thisx+(ctx*thisw)+thisw,thisy-int(oheight+wheight)), &ifillbrush);
		

	
	}


	
	drawme->SelectObject(&backpen);
	for(ctx=0; ctx<17; ctx++)
	{
		drawme->MoveTo(thisx+(ctx*4*thisw),thisy);
		drawme->LineTo(thisx+(ctx*4*thisw),thisy-thish-1);
	}
	for(ctx=0; ctx<11; ctx++)
	{
		drawme->MoveTo(thisx,thisy-thish+(ctx*thish/10));
		drawme->LineTo(thisx+64*thisw,thisy-thish+(ctx*thish/10));
	}

















}




afx_msg void introscreen::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	int ax;
	int ay;
	ax = point.x-roff;
	ay = point.y-toff;
	ax = ax/bsize;
	ay = ay/bsize;
	if( (ax >= 0) && (ax < 64) && (ay >=0) && (ay < 24)) 
	{
		int idx;
		idx = 64*ay+dispoff*256+ax;
		curhost = idx;
	}
	else
	{
		if((point.x >= upx) && (point.x <= upx+dirw) && (point.y >= upy) && (point.y <= upy+dirw))
		{
			dispoff = dispoff-5;
			if(dispoff < 0)
			{
				dispoff = 0;
			}
		}
		if((point.x >= downx) && (point.x <= downx+dirw) && (point.y >= downy-dirw) && (point.y <= downy))
		{
			dispoff = dispoff+5;
			if(dispoff > 249)
			{
				dispoff = 249;
			}
		}
	}
	dirty = true;
}

afx_msg void introscreen::OnLButtonDown(UINT nFlags, CPoint point)
{
	int ax;
	int ay;

	ax = point.x-roff;
	ay = point.y-toff;
	ax = ax/bsize;
	ay = ay/bsize;

	if( (ax >= 0) && (ax < 64) && (ay >=0) && (ay < 24)) 
	{
		int idx;
		idx = 64*ay+dispoff*256+ax;
		curhost = idx;
	}
	else
	{
		if((point.x >= upx) && (point.x <= upx+dirw) && (point.y >= upy) && (point.y <= upy+dirw))
		{
			dispoff = dispoff-1;
			if(dispoff < 0)
			{
				dispoff = 0;
			}
		}
		if((point.x >= downx) && (point.x <= downx+dirw) && (point.y >= downy-dirw) && (point.y <= downy))
		{
			dispoff = dispoff+1;
			if(dispoff > 249)
			{
				dispoff = 249;
			}
		}
		dirty = true;

	}
	dirty = true;
}





class myapplication : public CWinApp
{
public:						 
	virtual BOOL InitInstance()
	{
		m_pMainWnd = new introscreen;
		return TRUE;
	}
} runit;
