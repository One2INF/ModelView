#include "main.h"
#include "wx/wxprec.h"
#include "wx/wx.h"


#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif
#include <GL/glu.h>


bool MyApp::OnInit()
{
  MyFrame* frame = new MyFrame();
  frame->Show(true);
  return true;
}

MyFrame::MyFrame()
  : wxFrame(nullptr, wxID_ANY, "Hello World")
{
  FILE* fp;
  AllocConsole();
  freopen_s(&fp, "CONIN$", "r", stdin);
  freopen_s(&fp, "CONOUT$", "w", stdout);
  freopen_s(&fp, "CONOUT$", "w", stderr);

  wxMenu* menuFile = new wxMenu;
  menuFile->Append(wxID_OPEN);
  menuFile->Append(ID_Hello, "&Hello...\tCtrl-H",
    "Help string shown in status bar for this menu item");
  menuFile->AppendSeparator();
  menuFile->Append(wxID_EXIT);

  wxMenu* menuHelp = new wxMenu;
  menuHelp->Append(wxID_ABOUT);

  wxMenuBar* menuBar = new wxMenuBar;
  menuBar->Append(menuFile, "&File");
  menuBar->Append(menuHelp, "&Help");

  SetMenuBar(menuBar);

  CreateStatusBar();
  SetStatusText("Welcome to wxWidgets!");

  Bind(wxEVT_MENU, &MyFrame::OnHello, this, ID_Hello);
  Bind(wxEVT_MENU, &MyFrame::OnAbout, this, wxID_ABOUT);
  Bind(wxEVT_MENU, &MyFrame::OnExit, this, wxID_EXIT);
  Bind(wxEVT_MENU, &MyFrame::OnOpen, this, wxID_OPEN);

  Show(true);
  Raise();

  modelview = new ModelView(this);
  modelview->InitGl();
  modelview->LoadModel("CM1.stl");
}

void MyFrame::OnExit(wxCommandEvent& event)
{
  Close(true);
}

void MyFrame::OnAbout(wxCommandEvent& event)
{
  wxMessageBox("This is a wxWidgets Hello World example",
    "About Hello World", wxOK | wxICON_INFORMATION);
}

void MyFrame::OnHello(wxCommandEvent& event)
{
  wxLogMessage("Hello world from wxWidgets!");
}

void MyFrame::OnOpen(wxCommandEvent& event)
{
printf("open\r\n");
  wxString name;
  wxFileDialog openFileDialog(this, _("Open model"), wxGetCwd(), "",
      "model (*.stl)|*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

  if (openFileDialog.ShowModal() == wxID_CANCEL)
    return;     // the user changed idea...

  // proceed loading the file chosen by the user;
  // this can be done with e.g. wxWidgets input streams:
  wxFileInputStream input_stream(openFileDialog.GetPath());
  if (!input_stream.IsOk())
  {
    wxLogError("Cannot open file '%s'.", openFileDialog.GetPath());
    return;
  }

  modelview->LoadModel(openFileDialog.GetPath());
}

