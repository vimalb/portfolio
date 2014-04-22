#include <afxwin.h>
#include <afxsock.h>
#include "vimemailserver.h"

#include <fstream.h>
#include <string.h>
#include <stdlib.h>
#include <iomanip.h>

#define sockport 25
#define myhost1 "mobilion"
#define myhost2 "MOBILION"
#define myhost3 "mit.edu"
#define myhost4 "MIT.EDU"

CListBox *log;

class chatsocket;

class introscreen;
introscreen *nomain;

struct textblock
{
	textblock *upper;
	char *textbuf;
	int buflen;
	textblock *lower;
};


class chatsocket : public CSocket
{
public:
	int status;
	
	int datamode;

	textblock *basereceiver;
	textblock *capreceiver;

	textblock *basemessage;
	textblock *capmessage;

	char sender[501];

	char reader[4097];
	chatsocket()
	{
		status = 1;
		basereceiver = new textblock;
		capreceiver = new textblock;
		basereceiver->lower = NULL;
		basereceiver->upper = capreceiver;
		capreceiver->lower = basereceiver;
		capreceiver->upper = NULL;
		basemessage = new textblock;
		capmessage = new textblock;
		basemessage->upper = capmessage;
		capmessage->lower = basemessage;
		capmessage->upper = NULL;

		

		datamode = 0;

	}


	virtual void OnReceive(int nErrorCode)
	{
		if(nErrorCode == 0)
		{
			int numx = Receive(reader, 4096, 0);
			reader[numx] = '\0';
//			MessageBox(NULL, reader, "hi from MITmail", MB_OK);
			char reply[4097];

			if((strstr(reader, "EHLO") == &reader[0]) && (datamode == 0))
			{
				strcpy(reply, "554 vimal.mit.edu Says I don't do that shit.");
				strcat(reply, "\r\n");
//				MessageBox(NULL, reply, "hi", MB_OK);
				Send(reply, strlen(reply));
			}
			if((strstr(reader, "HELO") == &reader[0]) && (datamode == 0))
			{
				strcpy(reply, "250 vimal.mit.edu Says Wasssssup!!!");
				strcat(reply, "\r\n");
//				MessageBox(NULL, reply, "hi", MB_OK);
				Send(reply, strlen(reply));
			}
			if(strstr(reader, "MAIL") == &reader[0])
			{
				textblock *deleter;
				while(basereceiver->upper != capreceiver)
				{
					deleter = basereceiver->upper;
					deleter->upper->lower = deleter->lower;
					deleter->lower->upper = deleter->upper;
					delete deleter;
				}
				while(basemessage->upper != capmessage)
				{
					deleter = basemessage->upper;
					deleter->upper->lower = deleter->lower;
					deleter->lower->upper = deleter->upper;
					delete deleter;
				}
				datamode = 0;
				strcpy(sender, &reader[5]);
				strcpy(reply, "250 OK Bring it on!");
				strcat(reply, "\r\n");
//				MessageBox(NULL, reply, "hi", MB_OK);
				Send(reply, strlen(reply));
			}
			if((strstr(reader, "RCPT") == &reader[0]) && (datamode == 0))
			{
				
//				if((strstr(reader, myhost1) != NULL) || (strstr(reader, myhost2) != NULL) || (strstr(reader, myhost3) != NULL) || (strstr(reader, myhost4) != NULL))
				if(1)
				{
					textblock *adder;
					adder = new textblock;
					adder->textbuf = new char[strlen(&reader[5])];
					strcpy(adder->textbuf, &reader[5]);
					capreceiver->lower->upper = adder;
					adder->lower = capreceiver->lower;
					adder->upper = capreceiver;
					capreceiver->lower = adder;
					strcpy(reply, "250 OK Thank you sir, may I have another.");
					strcat(reply, "\r\n");
//					MessageBox(NULL, reply, "hi", MB_OK);
					Send(reply, strlen(reply));
				}
				else
				{
					strcpy(reply, "550 Failure Who the hell is that?");
					strcat(reply, "\r\n");
//					MessageBox(NULL, reply, "hi", MB_OK);
					Send(reply, strlen(reply));
				}

			}
			if((strstr(reader, "DATA") == &reader[0]) && (datamode == 0))
			{
				datamode = 1;
				strcpy(reply, "354 Well spit it out already \r\n.\r\n");
				strcat(reply, "\r\n");
//				MessageBox(NULL, reply, "hi", MB_OK);
				Send(reply, strlen(reply));
			}
			if(((strstr(reader, "\r\n.\r\n") == &reader[0]) || (strstr(reader, "QUIT") == &reader[0])) && (datamode == 1))
			{
				ofstream writer;
				writer.open("mail.txt", ios::app);
				writer << "\n\nNew mail:\n";
				writer << sender << "\n";
				textblock *read;
				read = basemessage->upper;
				while(read != capmessage)
				{
					writer << read->textbuf;
					read = read->upper;
				}
				writer << reader;
				datamode = 0;
				strcpy(reply, "250 OK Okey doke, will deliver.");
				strcat(reply, "\r\n");
//				MessageBox(NULL, reply, "hi", MB_OK);
				Send(reply, strlen(reply));
			}

			if((strstr(reader, "QUIT") == &reader[0]) && (datamode == 0))
			{
				datamode = 0;
				strcpy(reply, "221 vimal.mit.edu Well fine then, go away.");
				strcat(reply, "\r\n");
//				MessageBox(NULL, reply, "hi", MB_OK);
				Send(reply, strlen(reply));
			}

			if(datamode == 1)
			{
				textblock *adder;
				adder = new textblock;
				adder->textbuf = new char[numx+1];
				strcpy(adder->textbuf, reader);
				capmessage->lower->upper = adder;
				adder->lower = capmessage->lower;
				adder->upper = capmessage;
				capmessage->lower = adder;
				strcpy(reply, "250 More data please.");
				strcat(reply, "\r\n");
//				MessageBox(NULL, reply, "hi", MB_OK);
				Send(reply, strlen(reply));
			}



			if(strstr(reader, "RSET") == &reader[0])
			{
				textblock *deleter;
				while(basereceiver->upper != capreceiver)
				{
					deleter = basereceiver->upper;
					deleter->upper->lower = deleter->lower;
					deleter->lower->upper = deleter->upper;
					delete deleter;
				}
				while(basemessage->upper != capmessage)
				{
					deleter = basemessage->upper;
					deleter->upper->lower = deleter->lower;
					deleter->lower->upper = deleter->upper;
					delete deleter;
				}
				datamode = 0;
				strcpy(reply, "250 OK Quit jerking around!");
				strcat(reply, "\r\n");
//				MessageBox(NULL, reply, "hi", MB_OK);
				Send(reply, strlen(reply));
			}
			if((strstr(reader, "NOOP") == &reader[0]) && (datamode == 0))
			{
				strcpy(reply, "250 OK What the hell should I do?");
				strcat(reply, "\r\n");
//				MessageBox(NULL, reply, "hi", MB_OK);
				Send(reply, strlen(reply));
			}
		}
	}
	virtual void OnClose(int nErrorCode)
	{
		Close();
		textblock *deleter;
		deleter = basereceiver;
		while(deleter != NULL)
		{
			basereceiver = deleter;
			deleter = deleter->upper;
			delete basereceiver;
		}
		deleter = basemessage;
		while(deleter != NULL)
		{
			basemessage = deleter;
			deleter = deleter->upper;
			delete basemessage;
		}
		status = 0;
	}


private:
};


struct chatblock
{
	chatblock *upper;
	chatsocket receiver;
	chatblock *lower;
};




class mysocket : public CSocket
{
public:
	chatblock *base;
	chatblock *cap;
	mysocket()
	{
		CSocket::CSocket();
		base = new chatblock;
		cap = new chatblock;
		base->lower = NULL;
		base->upper = cap;
		cap->lower = base;
		cap->upper = NULL;
		base->receiver.OnClose(0);
		cap->receiver.OnClose(0);
	}
	~mysocket()
	{
		cap = base;
		while(cap != NULL)
		{
			base = cap;
			cap = cap->upper;
			delete base;
		}
		CSocket::~CSocket();
	}
	virtual void OnAccept(int nErrorCode)
	{
		if(nErrorCode == 0)
		{
			chatblock *newbie;
			newbie = new chatblock;
			newbie->lower = cap->lower;
			cap->lower->upper = newbie;
			newbie->upper = cap;
			cap->lower = newbie;
			SOCKADDR source;
			int tpx = sizeof(source);
			Accept(newbie->receiver, &source, &tpx );

			CString connector;
			char hostinfo[301];
			
			CTime entry = CTime::GetCurrentTime();
			connector = entry.Format("%B %d, %Y : %H:%M %S - " );
			strcpy(hostinfo, connector);

			unsigned int port;
			newbie->receiver.GetPeerName(connector, port);
			char cprt[10];
			_gcvt(port, 8, cprt);
			strcat(hostinfo, connector);
			strcat(hostinfo, " : ");
			strcat(hostinfo, cprt);
			

			while(log->GetCount() > 999)
			{
				log->DeleteString(0);
			}
			log->AddString(hostinfo);
		
			ofstream plog;
			plog.open("log.txt", ios::app);
			plog << hostinfo << "\n";
			plog.close();
		
			char sval[101];
			strcpy(sval, "220 vimal.mit.edu Wasssssssup!!!");
			newbie->receiver.Send(sval, strlen(sval));
			strcpy(sval, "\r\n");
			newbie->receiver.Send(sval, strlen(sval));
		
		}
	}
private:
};




class introscreen : public CFrameWnd
{
public:
	introscreen();
	mysocket *server;


	int hits;

	void cleanup()
	{
		server->Close();
		delete server;
		delete log;
	}
	afx_msg void OnClose()
	{
		cleanup();
		DestroyWindow();
	}

	afx_msg void OnTimer(UINT nIDTimer)
	{
		chatblock *lookat;
		lookat = nomain->server->base->upper;
		while(lookat != nomain->server->cap)
		{
			if(lookat->receiver.status == 0)
			{
				lookat = lookat->lower;
				chatblock *deleter;
				deleter = lookat->upper;
				deleter->upper->lower = deleter->lower;
				deleter->lower->upper = deleter->upper;
				delete deleter;
				hits++;
				InvalidateRect(NULL);
			}
			lookat = lookat->upper;
		}
	}


	afx_msg void OnPaint()
	{
		CPaintDC DWname(this);

		CFont netfont;
		netfont.CreateFont(21,0,0,0,FW_BOLD,0,0,0,0,0,0,0,0,"Arial");
		DWname.SelectObject(&netfont);

		char chits[10];
		_gcvt(hits, 8, chits);

		DrawTextEx(DWname, chits, strlen(chits), CRect(40, 40, 100, 60), DT_SINGLELINE|DT_NOCLIP, NULL);	
	}


	
private:

	DECLARE_MESSAGE_MAP()
};


BEGIN_MESSAGE_MAP(introscreen, CFrameWnd)
	ON_WM_CLOSE()
	ON_WM_PAINT()
	ON_WM_TIMER()
END_MESSAGE_MAP()


introscreen::introscreen()
{
	Create(NULL, "Vimnet Email Server", WS_VISIBLE|WS_OVERLAPPEDWINDOW, CRect(0,0,550,350), NULL, NULL);

	nomain = this;
	
	log = new CListBox;
	log->Create(WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_BORDER|LBS_DISABLENOSCROLL|LBS_NOINTEGRALHEIGHT, CRect(20,100,530,300), this, IDlog);


	server = new mysocket;
	server->Create(sockport, SOCK_STREAM, NULL);
	server->Listen(5);

	hits = 0;
	SetTimer(IDcleanuptimer, 50, NULL);


}

introscreen *worker;

class myapplication : public CWinApp
{
public:						 
	virtual BOOL InitInstance()
	{
		AfxSocketInit();
		m_bAutoDelete = TRUE;
		m_pMainWnd = new introscreen;
		return TRUE;
	}
} runit;

