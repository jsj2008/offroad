#include <QApplication>
#include <QKeyEvent>
#include <QWidget>
#include <QtOpenGL>
#include <QElapsedTimer>

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <BulletCollision/CollisionShapes/btConcaveShape.h>
#include <btBulletWorldImporter.h>

#include <vehicle/btRaycastVehicle.h>

class FirstPersonCamera;
class Renderer;
class Mesh;
class VertexBuffer;
class IndexBuffer;
class Shader;
class Texture;
class ConfigurationWindow;

typedef QVector2D vec2;
typedef QVector3D vec3;
typedef QVector4D vec4;
typedef QMatrix4x4 mat4;

class DebugDrawer : public btIDebugDraw {
private:
  int debugMode;

public:
  DebugDrawer();
  virtual ~DebugDrawer();

  void setModelViewProj(mat4& modelView, mat4& proj);

  virtual void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color);
  virtual void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &fromColor, const btVector3 &toColor);
  virtual void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color);
  virtual void reportErrorWarning(const char* warningString);
  virtual void draw3dText(const btVector3 &location, const char *textString);
  virtual void setDebugMode(int debugMode);
  virtual int getDebugMode() const;
};

class BlenderScene {
public:
  BlenderScene(const char* fileName, btDynamicsWorld* world, Renderer* renderer);
  ~BlenderScene();

  void draw(qint64 delta, const mat4& proj, const mat4& modelView);

private:
  btBulletWorldImporter* importer;
  btDynamicsWorld* world;
  Renderer* renderer;

  struct RenderableObject {
    RenderableObject() {
      mesh = NULL; // TODO: nulptr?
      texture0 = NULL;
      texture1 = NULL;
      texture2 = NULL;
      texture3 = NULL;
      shader = NULL;
      body = NULL;
      ghost = false;
    }

    Mesh* mesh;
    Texture* texture0;
    Texture* texture1;
    Texture* texture2;
    Texture* texture3;
    Shader* shader;
    btRigidBody* body;
    QString name;
    bool ghost;
    btTransform transform;
  };

  QList<RenderableObject> objects;
};

class App : public QGLWidget {
  Q_OBJECT

public:
  App(const QGLFormat& format, ConfigurationWindow* configWin);
  ~App();

private:
  void initializeGL();
  void paintGL();
  void resizeGL(int width, int height);
  void keyPressEvent(QKeyEvent* event);
  void keyReleaseEvent(QKeyEvent* event);
  void mouseMoveEvent(QMouseEvent* event);
  void mousePressEvent(QMouseEvent* event);

  void setupPhysics();
  void updatePhysics(qint64 delta);
  void cleanUpPhysics();

  ConfigurationWindow* configWin;
  Renderer* renderer;
  Mesh* wheel;
  Mesh* truck;
  Mesh* skyDome;
  Texture* wheelTexture;
  Shader* diffuse;
  Shader* diffuseTextured;
  Shader* scattering;

  GLuint fbo;
  GLuint depthBuffer;
  GLuint depthTexture;
  GLuint colorBuffer;

  int rttWidth, rttHeight;
  Shader* blur;
  mat4 previousModelView;

  BlenderScene* scene;
  FirstPersonCamera* cam;
  QElapsedTimer* timer;

  bool mouseFree;
  bool vehicleCam;
  btVector3 camDir;

  btDynamicsWorld* dynamicsWorld;
  btRigidBody* chassis;
  btBroadphaseInterface* overlappingPairCache;
  btCollisionDispatcher* dispatcher;
  btConstraintSolver* constraintSolver;
  btDefaultCollisionConfiguration* collisionConfiguration;

  btRaycastVehicle::btVehicleTuning tuning;
  btVehicleRaycaster* vehicleRaycaster;
  btRaycastVehicle* vehicle;
  btCollisionShape* wheelShape;

  btRigidBody* groundBody;
  btCollisionShape* groundShape;

  DebugDrawer* debugDrawer;
  bool drawDebugInfo;
  bool stipple;

  bool accelerating, breaking, steeringLeft, steeringRight;
  float engineForce, breakingForce, vehicleSteering;
  const float maxSteering;
};

class PointerSlider : public QSlider {
  Q_OBJECT

public:
  PointerSlider(float* var, float min, float max);

private slots:
  void updateVar(int value);

private:
  float* var;
  float min, max;
};

class ConfigurationWindow : public QWidget {
  Q_OBJECT

public:
  ConfigurationWindow();
  ~ConfigurationWindow();

  void addPointerValue(const char* name, float* var, float min, float max);

private:
  QGridLayout* mainLayout;
};
