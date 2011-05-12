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

#include <debug-drawer/GLDebugDrawer.h> // TODO: perfomance!

class FirstPersonCamera;
class Renderer;
class Mesh;
class VertexBuffer;
class IndexBuffer;
class Shader;
class Texture;

class ConfigurationWindow;

class BlenderScene {
public:
  BlenderScene(const char* fileName, btDynamicsWorld* world, Renderer* renderer);
  ~BlenderScene();

  void draw(qint64 delta);

private:
  btBulletWorldImporter* importer;
  btDynamicsWorld* world;
  Renderer* renderer;

  struct RenderableObject {
    RenderableObject() {
      mesh = NULL; // TODO: nulptr?
      color = NULL;
      normal = NULL;
      shader = NULL;
      body = NULL;
    }

    Mesh* mesh;
    Texture* color;
    Texture* normal;
    Shader* shader;
    btRigidBody* body;
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
  void cleanUpPhysics();

  ConfigurationWindow* configWin;
  Renderer* renderer;
  Mesh* wheel;
  Mesh* truck;
  Mesh* skyDome;
  Texture* wheelTexture;
  Shader* diffuse;
  Shader* wheelShader;
  Shader* scattering;
  Shader* terrainShader;
  VertexBuffer* terrainVB;
  IndexBuffer* terrainIB;
  Texture* grass;
  GLuint heightfieldId;
  float vehicleCamRotation;

  BlenderScene* scene;

  FirstPersonCamera* cam;
  QElapsedTimer* timer;

  bool mouseFree;
  bool vehicleCam;

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
  float* heightfield;
  int heightfieldWidth;
  int heightfieldHeight;
  float yTrans;

  GLDebugDrawer* debugDrawer;
  bool drawDebugInfo;

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
