#include <afxwin.h>
#include <afxsock.h>
#include "vimhttpserver.h"

#include <fstream.h>
#include <string.h>
#include <stdlib.h>
#include <iomanip.h>


CListBox *log;

class chatsocket;

class chatsocket : public CSocket
{
public:
	int status;
	char reader[1012];
	chatsocket()
	{
		status = 1;
	}
	virtual void OnReceive(int nErrorCode)
	{
		if(nErrorCode == 0)
		{
			int numx = Receive(reader, 1011, 0);
			reader[numx] = '\0';
			CString connector;
			char hostinfo[301];

			CTime entry = CTime::GetCurrentTime();
			connector = entry.Format("%B %d, %Y : %H:%M %S - " );
			strcpy(hostinfo, connector);

			unsigned int port;
			GetPeerName(connector, port);
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
			plog << reader << "\n";
			plog.close();
			
			
			if(strstr(reader, "GET") != NULL)
			{
				char filename[1005];
				int cread = 5;
				int cwrite = 0;
				while((reader[cread] != ' ') && (cread < 1000))
				{
					filename[cwrite] = reader[cread];
					cwrite++;
					cread++;
				}
				filename[cwrite] = '\0';
				if((strcmp(filename, "") == 0) || (strstr(filename, "..") != NULL))
				{
					strcpy(filename, "index.html");
				}
				char realfile[1095];
				strcpy(realfile, "serverpages\\");
				strcat(realfile, filename);
				

				int donesend;
				donesend = 0;
				CFile sendme;
				if(sendme.Open(realfile, CFile::modeRead) == 0)
				{
					strcpy(realfile, "serverpages\\index.html");
					if(sendme.Open(realfile, CFile::modeRead) == 0)
					{
						donesend = 1;
					}
				}
				if(donesend == 0)
				{
					char *sender;
					sender = new char[sendme.GetLength()];
					int cpx;
					cpx = sendme.Read(sender, sendme.GetLength());
					
					char buf[1000];
					for(int ctg = 0; ctg<1000; ctg++)
					{
						buf[ctg] = ' ';
					}
					if(strstr(realfile, "serverpages\\index.html") != NULL)
					{
						
						Send(buf, 1000);
					}
					Send(sender, cpx);
					delete sender;
				}
			}
			status = 0;
			Close();
		}
	}
	virtual void OnClose(int nErrorCode)
	{
		status = 0;
		Close();
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


		}
	}
private:
};

class nowindow : public CWnd
{
public:
	mysocket *server;
	void cleanup()
	{
		server->Close();
		delete server;
	}
	afx_msg void OnClose();


	nowindow()
	{
		server = new mysocket;
		server->Create(80, SOCK_STREAM, NULL);
		server->Listen(5);
	}
private:
	DECLARE_MESSAGE_MAP()
};


BEGIN_MESSAGE_MAP(nowindow, CWnd)
	ON_WM_CLOSE()
END_MESSAGE_MAP()



nowindow *nomain;

class introscreen : public CFrameWnd
{
public:
	introscreen();


	int hits;

	void cleanup()
	{
		delete log;
		nomain->OnClose();
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
	Create(NULL, "Vimnet HTTP Server", WS_VISIBLE|WS_OVERLAPPEDWINDOW, CRect(0,0,550,350), NULL, NULL);
	
	log = new CListBox;
	log->Create(WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_BORDER|LBS_DISABLENOSCROLL|LBS_NOINTEGRALHEIGHT, CRect(20,100,530,300), this, IDlog);

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
		nomain = new nowindow;
		m_pMainWnd = nomain;
		worker = new introscreen;
		return TRUE;
	}
} runit;

afx_msg void nowindow::OnClose()
{
	cleanup();
	DestroyWindow();
	PostQuitMessage(0);
}
