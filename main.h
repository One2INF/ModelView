#pragma once

#include "wx/defs.h"
#include "wx/app.h"
#include "wx/menu.h"
#include "wx/dcclient.h"
#include "wx/wfstream.h"
#include "wx/glcanvas.h"
#include "dxfrenderer.h"
#include "wx/wx.h"
#include "ModelView.h"


class MyApp : public wxApp
{
public:
  bool OnInit() override;
};

wxIMPLEMENT_APP(MyApp);

class MyFrame : public wxFrame
{
public:
  MyFrame();

private:
  ModelView *modelview;

  void OnHello(wxCommandEvent& event);
  void OnExit(wxCommandEvent& event);
  void OnAbout(wxCommandEvent& event);
  void OnOpen(wxCommandEvent& event);
};

enum
{
  ID_Hello = 1
};