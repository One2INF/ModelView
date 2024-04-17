#include "ModelView.h"
#include <GL/glu.h>
extern "C"
{
#include "trackball.h"
}

#define aisgl_min(x,y) (x<y?x:y)
#define aisgl_max(x,y) (y>x?y:x)


ModelView::ModelView(wxWindow* parent, wxWindowID id,
                     const wxPoint& pos,
                     const wxSize& size, long style,
                     const wxString& name)
         : wxGLCanvas(parent, id, NULL, pos, size, style | wxFULL_REPAINT_ON_RESIZE, name)
{
  glRC = new wxGLContext(this);
  InitGl();

  gldata.beginx = 0.0f;
  gldata.beginy = 0.0f;
  gldata.zoom = 45.0f;
  trackball(gldata.quat, 0.0f, 0.0f, 0.0f, 0.0f);

  Bind(wxEVT_PAINT, &ModelView::OnPaint, this);
  Bind(wxEVT_MOTION, &ModelView::OnMouse, this);
  Bind(wxEVT_SIZE, &ModelView::OnSize, this);
}

ModelView::~ModelView()
{
  if(scene)
    aiReleaseImport(scene);
}

void ModelView::get_bounding_box_for_node(const C_STRUCT aiNode* nd,
  C_STRUCT aiVector3D* min,
  C_STRUCT aiVector3D* max,
  C_STRUCT aiMatrix4x4* trafo)
{
  C_STRUCT aiMatrix4x4 prev;
  unsigned int n = 0, t;

  prev = *trafo;
  aiMultiplyMatrix4(trafo, &nd->mTransformation);

  for (; n < nd->mNumMeshes; ++n) {
    const C_STRUCT aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];
    for (t = 0; t < mesh->mNumVertices; ++t) {

      C_STRUCT aiVector3D tmp = mesh->mVertices[t];
      aiTransformVecByMatrix4(&tmp, trafo);

      min->x = aisgl_min(min->x, tmp.x);
      min->y = aisgl_min(min->y, tmp.y);
      min->z = aisgl_min(min->z, tmp.z);

      max->x = aisgl_max(max->x, tmp.x);
      max->y = aisgl_max(max->y, tmp.y);
      max->z = aisgl_max(max->z, tmp.z);
    }
  }

  for (n = 0; n < nd->mNumChildren; ++n) {
    get_bounding_box_for_node(nd->mChildren[n], min, max, trafo);
  }
  *trafo = prev;
}

void ModelView::get_bounding_box(C_STRUCT aiVector3D* min, C_STRUCT aiVector3D* max)
{
  C_STRUCT aiMatrix4x4 trafo;
  aiIdentityMatrix4(&trafo);

  min->x = min->y = min->z = 1e10f;
  max->x = max->y = max->z = -1e10f;
  get_bounding_box_for_node(scene->mRootNode, min, max, &trafo);
}

int ModelView::LoadModel(const wxString& file)
{
  if(scene)
  {
    aiReleaseImport(scene);
    scene = NULL;
  }

  scene = aiImportFile(file.c_str(), aiProcessPreset_TargetRealtime_MaxQuality);
  if(scene)
  {
    std::cout<< "open " << file << " succeed\n";

    get_bounding_box(&scene_min, &scene_max);
    scene_center.x = (scene_min.x + scene_max.x) / 2.0f;
    scene_center.y = (scene_min.y + scene_max.y) / 2.0f;
    scene_center.z = (scene_min.z + scene_max.z) / 2.0f;
    printf("min: %f %f\r\n", scene_min.x, scene_min.y);
    printf("max: %f %f\r\n", scene_max.x, scene_max.y);

    if(scene_list)
    {
      glDeleteLists(scene_list, 1);
      scene_list = 0;
    }

    if(scene_list == 0)
    {
      scene_list = glGenLists(1);
      glNewList(scene_list, GL_COMPILE);
      recursive_render(scene, scene->mRootNode);
      glEndList();
    }

    ResetProjectionMode();

    PostSizeEventToParent();
    Refresh();
    return 1;
  }

  std::cout << "open " << file << " failed\n";
  return 0;
}

void InitMaterials(void)
{
  static const GLfloat ambient[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
  static const GLfloat diffuse[4] = { 0.5f, 1.0f, 1.0f, 1.0f };
  static const GLfloat position0[4] = { 0.0f, 0.0f, 20.0f, 0.0f };
  static const GLfloat position1[4] = { 0.0f, 0.0f, -20.0f, 0.0f };
  static const GLfloat front_mat_shininess[1] = { 60.0f };
  static const GLfloat front_mat_specular[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
  static const GLfloat front_mat_diffuse[4] = { 0.5f, 0.28f, 0.38f, 1.0f };
  /*
  static const GLfloat back_mat_shininess[1] = {60.0f};
  static const GLfloat back_mat_specular[4] = {0.5f, 0.5f, 0.2f, 1.0f};
  static const GLfloat back_mat_diffuse[4] = {1.0f, 1.0f, 0.2f, 1.0f};
  */
  static const GLfloat lmodel_ambient[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
  static const GLfloat lmodel_twoside[1] = { GL_FALSE };

  glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, position0);
  glEnable(GL_LIGHT0);

  glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
  glLightfv(GL_LIGHT1, GL_POSITION, position1);
  glEnable(GL_LIGHT1);

  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
  glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);
  glEnable(GL_LIGHTING);

  glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, front_mat_shininess);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, front_mat_specular);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, front_mat_diffuse);
}

void ModelView::InitGl(void)
{
  SetCurrent(*glRC);

  static const GLfloat light0_pos[4] = { -50.0f, 50.0f, 0.0f, 0.0f };

  // white light
  static const GLfloat light0_color[4] = { 0.6f, 0.6f, 0.6f, 1.0f };
  static const GLfloat light1_pos[4] = { 50.0f, 50.0f, 0.0f, 0.0f };

  // cold blue light
  static const GLfloat light1_color[4] = { 0.4f, 0.4f, 1.0f, 1.0f };

  /* remove back faces */
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);

  /* speedups */
  glEnable(GL_DITHER);
  glShadeModel(GL_SMOOTH);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
  glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);

  /* light */
  glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_color);
  glLightfv(GL_LIGHT1, GL_POSITION, light1_pos);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_color);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);
  glEnable(GL_LIGHTING);

  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);
}

void color4_to_float4(const C_STRUCT aiColor4D* c, float f[4])
{
  f[0] = c->r;
  f[1] = c->g;
  f[2] = c->b;
  f[3] = c->a;
}

void set_float4(float f[4], float a, float b, float c, float d)
{
  f[0] = a;
  f[1] = b;
  f[2] = c;
  f[3] = d;
}

void apply_material(const C_STRUCT aiMaterial* mtl)
{
  float c[4];

  GLenum fill_mode;
  int ret1, ret2;
  C_STRUCT aiColor4D diffuse;
  C_STRUCT aiColor4D specular;
  C_STRUCT aiColor4D ambient;
  C_STRUCT aiColor4D emission;
  ai_real shininess, strength;
  int two_sided;
  int wireframe;
  unsigned int max;

  set_float4(c, 0.8f, 0.8f, 0.8f, 1.0f);
  if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
    color4_to_float4(&diffuse, c);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, c);

  set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
  if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &specular))
    color4_to_float4(&specular, c);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);

  set_float4(c, 0.2f, 0.2f, 0.2f, 1.0f);
  if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &ambient))
    color4_to_float4(&ambient, c);
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, c);

  set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
  if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &emission))
    color4_to_float4(&emission, c);
  glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, c);

  max = 1;
  ret1 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shininess, &max);
  if (ret1 == AI_SUCCESS) {
    max = 1;
    ret2 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS_STRENGTH, &strength, &max);
    if (ret2 == AI_SUCCESS)
      glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess * strength);
    else
      glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
  }
  else {
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
    set_float4(c, 0.0f, 0.0f, 0.0f, 0.0f);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);
  }

  max = 1;
  if (AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_ENABLE_WIREFRAME, &wireframe, &max))
    fill_mode = wireframe ? GL_LINE : GL_FILL;
  else
    fill_mode = GL_FILL;
  glPolygonMode(GL_FRONT_AND_BACK, fill_mode);

  max = 1;
  if ((AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_TWOSIDED, &two_sided, &max)) && two_sided)
    glDisable(GL_CULL_FACE);
  else
    glEnable(GL_CULL_FACE);
}

void ModelView::recursive_render(const C_STRUCT aiScene* sc, const C_STRUCT aiNode* nd)
{
  unsigned int i;
  unsigned int n = 0, t;
  C_STRUCT aiMatrix4x4 m = nd->mTransformation;

  /* update transform */
  aiTransposeMatrix4(&m);
  glPushMatrix();
  glMultMatrixf((float*)&m);

  /* draw all meshes assigned to this node */
  for (; n < nd->mNumMeshes; ++n) {
    const C_STRUCT aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];

    apply_material(sc->mMaterials[mesh->mMaterialIndex]);

    if (mesh->mNormals == NULL) {
      glDisable(GL_LIGHTING);
    }
    else {
      glEnable(GL_LIGHTING);
    }

    for (t = 0; t < mesh->mNumFaces; ++t) {
      const C_STRUCT aiFace* face = &mesh->mFaces[t];
      GLenum face_mode;

      switch (face->mNumIndices) {
      case 1: face_mode = GL_POINTS; break;
      case 2: face_mode = GL_LINES; break;
      case 3: face_mode = GL_TRIANGLES; break;
      default: face_mode = GL_POLYGON; break;
      }

      glBegin(face_mode);

      for (i = 0; i < face->mNumIndices; i++) {
        int index = face->mIndices[i];
        if (mesh->mColors[0] != NULL)
          glColor4fv((GLfloat*)&mesh->mColors[0][index]);
        if (mesh->mNormals != NULL)
          glNormal3fv(&mesh->mNormals[index].x);
        glVertex3fv(&mesh->mVertices[index].x);
      }

      glEnd();
    }

  }

  for (n = 0; n < nd->mNumChildren; ++n) {
    recursive_render(sc, nd->mChildren[n]);
  }

  glPopMatrix();
}

void ModelView::OnPaint(wxPaintEvent& event)
{
  if(!scene)
    return;

  wxPaintDC dc(this);

  SetCurrent(*glRC);

  glClearColor(0.3f, 0.4f, 0.6f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0f, 0.0f, -20.0f);
  GLfloat m[4][4];
  build_rotmatrix(m, gldata.quat);
  glMultMatrixf(&m[0][0]);

  const wxSize ClientSize = GetClientSize() * GetContentScaleFactor();
  glViewport(0, 0, ClientSize.x, ClientSize.y);

  float tmp = scene_max.x - scene_min.x;
  tmp = aisgl_max(scene_max.y - scene_min.y, tmp);
  tmp = aisgl_max(scene_max.z - scene_min.z, tmp);
  tmp = 10.f / tmp;
  glScalef(tmp, tmp, tmp);
  glTranslatef(-scene_center.x, -scene_center.y, -scene_center.z);

  glCallList(scene_list);

  glFlush();
  SwapBuffers();
}

void ModelView::OnMouse(wxMouseEvent& event)
{
  if(event.Dragging())
  {
    wxSize sz(GetClientSize());
    float spin_quat[4];
    trackball(spin_quat, 
      (2*gldata.beginx - sz.x) / sz.x,
      (sz.y - 2*gldata.beginy) / sz.y,
      float(2*event.GetX() - sz.x) / sz.x,
      float(sz.y - 2*event.GetY()) / sz.y);

    add_quats(spin_quat, gldata.quat, gldata.quat);

    /* orientation has changed, redraw mesh */
    Refresh(false);
  }

  gldata.beginx = event.GetX();
  gldata.beginy = event.GetY();
}

void ModelView::ResetProjectionMode(void)
{
  if (!IsShownOnScreen())
    return;

  // This is normally only necessary if there is more than one wxGLCanvas
  // or more than one wxGLContext in the application.
  SetCurrent(*glRC);

  const wxSize ClientSize = GetClientSize() * GetContentScaleFactor();

  // It's up to the application code to update the OpenGL viewport settings.
  // In order to avoid extensive context switching, consider doing this in
  // OnPaint() rather than here, though.
  glViewport(0, 0, ClientSize.x, ClientSize.y);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45, double(ClientSize.x) / ClientSize.y, 1, 100);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void ModelView::OnSize(wxSizeEvent& WXUNUSED(event))
{
  // Reset the OpenGL view aspect.
  // This is OK only because there is only one canvas that uses the context.
  // See the cube sample for that case that multiple canvases are made current with one context.
  ResetProjectionMode();
}
