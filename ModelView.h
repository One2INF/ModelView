#pragma once

#include "wx/glcanvas.h"
#include "wx/defs.h"
#include "wx/wx.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


struct GLData
{
  float beginx, beginy;  // position of mouse
  float quat[4];         // orientation of object
  float zoom;            // field of view in degrees
};


class ModelView : public wxGLCanvas
{
public:
  ModelView(wxWindow* parent, wxWindowID id = wxID_ANY,
    const wxPoint& pos = wxDefaultPosition,
    const wxSize& size = wxDefaultSize, long style = 0,
    const wxString& name = "ModelView");

  virtual ~ModelView();

  void InitGl(void);
  int LoadModel(const wxString& file);

  void OnPaint(wxPaintEvent& event);
  void OnMouse(wxMouseEvent& event);
  void OnSize(wxSizeEvent& WXUNUSED(event));

  void ResetProjectionMode(void);

private:
  GLuint scene_list = 0;
  C_STRUCT aiVector3D scene_min, scene_max, scene_center;
  const aiScene* scene = NULL;
  wxGLContext* glRC;
  GLData gldata;

  void recursive_render(const C_STRUCT aiScene* sc, const C_STRUCT aiNode* nd);
  void get_bounding_box_for_node(const C_STRUCT aiNode* nd,
                                 C_STRUCT aiVector3D* min,
                                 C_STRUCT aiVector3D* max,
                                 C_STRUCT aiMatrix4x4* trafo);
  void get_bounding_box(C_STRUCT aiVector3D* min, C_STRUCT aiVector3D* max);

};
