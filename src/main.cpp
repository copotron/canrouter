// Copyright (c) 2018 Maruf Maniruzzaman
// Based on wxwidgets.org library using minimal sample app
// You will need zeromq and wxwidgets library to compile and run this app.

#include "wx/wxprec.h"
#include <wx/listctrl.h>
#include <wx/tokenzr.h>

#define ZMQ_STATIC 1
#include <zmq.h>

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

// ----------------------------------------------------------------------------
// resources
// ----------------------------------------------------------------------------

// the application icon (under Windows it is in resources and even
// though we could still include the XPM here it would be unused)
#ifndef wxHAS_IMAGES_IN_RESOURCES
    #include "../sample.xpm"
#endif

// this can be in a header file, so that other classes can use the event type
wxDECLARE_EVENT(wxEVT_COMMAND_MYTHREAD_UPDATE, wxThreadEvent);

// this must be in a .CPP file so that it only exists once in the project
wxDEFINE_EVENT(wxEVT_COMMAND_MYTHREAD_UPDATE, wxThreadEvent);

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// IDs for the controls and the menu commands
enum
{
	// menu items
	Minimal_Quit = wxID_EXIT,

	// it is important for the id corresponding to the "About" command to have
	// this standard value as otherwise it won't be handled properly under Mac
	// (where it is special and put into the "Apple" menu)
	Minimal_About = wxID_ABOUT
};



class MyFrame;

class MyThread : public wxThread
{
public:
	MyThread(wxEvtHandler *handler)
		: wxThread(wxTHREAD_DETACHED)
	{
		m_pHandler = handler;
	}
	~MyThread();
protected:
	virtual ExitCode Entry();
	wxEvtHandler *m_pHandler;
};

wxThread::ExitCode MyThread::Entry()
{
	void *context = zmq_ctx_new();
	void *responder = zmq_socket(context, ZMQ_ROUTER);
	int rc = zmq_bind(responder, "tcp://*:5555");
	assert(rc == 0);

	while (!TestDestroy())
	{
		char buffer[100];
		buffer[99] = 0;
		int rcv_len = zmq_recv(responder, buffer, 100, 0); // id

		rcv_len = zmq_recv(responder, buffer, 100, 0); // data
		
		printf("Received %s\n", buffer);

		auto evt = new wxThreadEvent(wxEVT_COMMAND_MYTHREAD_UPDATE);
		evt->SetString(std::string(buffer, rcv_len));
		// ... do a bit of work...
		wxQueueEvent(m_pHandler, evt);
	}
	// signal the event handler that this thread is going to be destroyed
	// NOTE: here we assume that using the m_pHandler pointer is safe,
	//       (in this case this is assured by the MyFrame destructor)
	//wxQueueEvent(m_pHandler, new wxThreadEvent(wxEVT_COMMAND_MYTHREAD_UPDATE));

	return (wxThread::ExitCode)0;     // success
}
MyThread::~MyThread()
{
	//wxCriticalSectionLocker enter(m_pHandler->m_pThreadCS);
	// the thread is being destroyed; make sure not to leave dangling pointers around
	//m_pHandler->m_pThread = NULL;
}

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// Define a new application type, each program should derive a class from wxApp
class MyApp : public wxApp
{
	MyThread * m_pThread;
public:
    // override base class virtuals
    // ----------------------------

    // this one is called on application startup and is a good place for the app
    // initialization (doing it here and not in the ctor allows to have an error
    // return: if OnInit() returns false, the application terminates)
    virtual bool OnInit() wxOVERRIDE;
};

// Define a new frame type: this is going to be our main frame
class MyFrame : public wxFrame
{
	wxListCtrl* m_item_list;
	bool GetItemByData(long data, wxListItem& info);

public:
    // ctor(s)
    MyFrame(const wxString& title);

	void OnThreadUpdate(wxCommandEvent& evt);	

    // event handlers (these functions should _not_ be virtual)
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);

private:	
    // any class wishing to process wxWidgets events must use this macro
    wxDECLARE_EVENT_TABLE();
};


// ----------------------------------------------------------------------------
// event tables and other macros for wxWidgets
// ----------------------------------------------------------------------------

// the event tables connect the wxWidgets events with the functions (event
// handlers) which process them. It can be also done at run-time, but for the
// simple menu events like this the static method is much simpler.
wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(Minimal_Quit,  MyFrame::OnQuit)
    EVT_MENU(Minimal_About, MyFrame::OnAbout)
	EVT_COMMAND(wxID_ANY, wxEVT_COMMAND_MYTHREAD_UPDATE, MyFrame::OnThreadUpdate)
wxEND_EVENT_TABLE()

// Create a new application object: this macro will allow wxWidgets to create
// the application object during program execution (it's better than using a
// static object for many reasons) and also implements the accessor function
// wxGetApp() which will return the reference of the right type (i.e. MyApp and
// not wxApp)
wxIMPLEMENT_APP(MyApp);

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// the application class
// ----------------------------------------------------------------------------

// 'Main program' equivalent: the program execution "starts" here
bool MyApp::OnInit()
{
    // call the base class initialization method, currently it only parses a
    // few common command-line options but it could be do more in the future
    if ( !wxApp::OnInit() )
        return false;

    // create the main application window
    MyFrame *frame = new MyFrame("CAN Message Viewer");

    // and show it (the frames, unlike simple controls, are not shown when
    // created initially)
	wxSize size(800, 500);
	frame->SetSize(size);
    frame->Show(true);

	m_pThread = new MyThread(frame);
	if (m_pThread->Run() != wxTHREAD_NO_ERROR)
	{
		wxLogError("Can't create the thread!");
		delete m_pThread;
		m_pThread = NULL;
	}

    // success: wxApp::OnRun() will be called which will enter the main message
    // loop and the application will run. If we returned false here, the
    // application would exit immediately.
    return true;
}


// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

// frame constructor
MyFrame::MyFrame(const wxString& title)
       : wxFrame(NULL, wxID_ANY, title)
{
    // set the frame icon
    SetIcon(wxICON(sample));

#if wxUSE_MENUS
    // create a menu bar
    wxMenu *fileMenu = new wxMenu;

    // the "About" item should be in the help menu
    wxMenu *helpMenu = new wxMenu;
    helpMenu->Append(Minimal_About, "&About\tF1", "Show about dialog");

    fileMenu->Append(Minimal_Quit, "E&xit\tAlt-X", "Quit this program");

    // now append the freshly created menu to the menu bar...
    wxMenuBar *menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, "&File");
    menuBar->Append(helpMenu, "&Help");

    // ... and attach this menu bar to the frame
    SetMenuBar(menuBar);
#else // !wxUSE_MENUS
    // If menus are not available add a button to access the about box
    wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* aboutBtn = new wxButton(this, wxID_ANY, "About...");
    aboutBtn->Bind(wxEVT_BUTTON, &MyFrame::OnAbout, this);
    sizer->Add(aboutBtn, wxSizerFlags().Center());
#endif // wxUSE_MENUS/!wxUSE_MENUS

#if wxUSE_STATUSBAR
    // create a status bar just for fun (by default with 1 pane only)
    CreateStatusBar(2);
    SetStatusText("Ready");
#endif // wxUSE_STATUSBAR

	wxPanel* mainPane = new wxPanel(this);
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

	m_item_list = new wxListCtrl(mainPane, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT);
	
	wxListItem col0;
	col0.SetId(0);
	col0.SetText(_("ID"));
	col0.SetWidth(50);
	m_item_list->InsertColumn(0, col0);

	wxListItem col1;
	col1.SetId(1);
	col1.SetText(_("DATA1"));
	m_item_list->InsertColumn(1, col1);

	wxListItem col2;
	col2.SetId(2);
	col2.SetText(_("DATA2"));
	m_item_list->InsertColumn(2, col2);

	wxListItem col3;
	col3.SetId(3);
	col3.SetText(_("DATA3"));
	m_item_list->InsertColumn(3, col3);

	wxListItem col4;
	col4.SetId(4);
	col4.SetText(_("DATA4"));
	m_item_list->InsertColumn(4, col4);

	wxListItem col5;
	col5.SetId(5);
	col5.SetText(_("DATA5"));
	m_item_list->InsertColumn(5, col5);

	wxListItem col6;
	col6.SetId(6);
	col6.SetText(_("DATA6"));
	m_item_list->InsertColumn(6, col6);


	wxListItem col7;
	col7.SetId(7);
	col7.SetText(_("DATA7"));
	m_item_list->InsertColumn(7, col7);
	
	sizer->Add(m_item_list, 1, wxEXPAND | wxALL, 10);
	mainPane->SetSizer(sizer);
}

bool MyFrame::GetItemByData(long data, wxListItem& info) {
	for (long id = 0; id < m_item_list->GetItemCount(); id++) {
		auto itemData = m_item_list->GetItemData(id);
		if (data == itemData) {
			info.m_itemId = id;

			m_item_list->GetItem(info);
			return true;
		}
	}

	return false;
}

void MyFrame::OnThreadUpdate(wxCommandEvent& evt) {
	auto data = evt.GetString();

	if (data.length() == 0) {
		return;
	}
	//OutputDebugStringW(data);
	wxStringTokenizer tokenizer(data, " ");
	wxListItem item;	
		
	int col = 0;

	while (tokenizer.HasMoreTokens())
	{
		wxString token = tokenizer.GetNextToken();
		// process token here		
		if (col == 0) {	
			long d = 0;
			if (token.ToLong(&d)) {
				if (GetItemByData(d, item))
				{
					item.GetId();
				}
				else {
					long id = m_item_list->GetItemCount();
					item.SetText(data);
					item.SetId(id);
					item.SetData(d);
					m_item_list->InsertItem(item);					
				}
			}
			else {
				return;
			}
		}
		
		m_item_list->SetItem(item.m_itemId, col, token);
		
		//OutputDebugStringW(token);
		//OutputDebugStringW(L"\r\n");
		col++;

		if (col >= m_item_list->GetColumnCount()) {
			break;
		}
	}
	

}

// event handlers

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    // true is to force the frame to close
    Close(true);
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox(wxString::Format
                 (
                    "Welcome to %s!\n"
                    "\n"
                    "This is the simple CAN message viewer\n"
                    "running under %s.",
                    wxVERSION_STRING,
                    wxGetOsDescription()
                 ),
                 "About CAN Message Viewer",
                 wxOK | wxICON_INFORMATION,
                 this);
}
