#include <QApplication>
#include <QKeyEvent>
#include <QWidget>
#include <QtOpenGL>
#include <QElapsedTimer>

class btVehicleTuning;
struct btVehicleRaycaster;
class btCollisionShape;

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <BulletCollision/CollisionShapes/btConcaveShape.h>
#include <BulletDynamics/Vehicle/btRaycastVehicle.h>
#include <debug-drawer/GLDebugDrawer.h>

class FirstPersonCamera;
class Renderer;
class Mesh;
class VertexBuffer;
class IndexBuffer;
class Shader;
class Texture;

class App : public QGLWidget {
  Q_OBJECT

public:
  App(const QGLFormat& format, QWidget* parent);
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

  Renderer* renderer;
  Mesh* wheel;
  Mesh* truck;
  Texture* wheelTexture;
  Shader* diffuse;
  Shader* wheelShader;
  FirstPersonCamera* cam;
  QElapsedTimer* timer;

  bool state;

  btDynamicsWorld* dynamicsWorld;
  btRigidBody* chassis;
  btBroadphaseInterface* overlappingPairCache;
  btCollisionDispatcher* dispatcher;
  btConstraintSolver* constraintSolver;
  btDefaultCollisionConfiguration* collisionConfiguration;
  //btTriangleIndexVertexArray* indexVertexArrays;

  btRaycastVehicle::btVehicleTuning tuning;
  btVehicleRaycaster* vehicleRaycaster;
  btRaycastVehicle* vehicle;
  btCollisionShape* wheelShape;

  btRigidBody* groundBody;
  btCollisionShape* groundShape;
  float* heightfield;

  GLDebugDrawer* debugDrawer;
  bool drawDebugInfo;

  bool accelerating, breaking, steeringLeft, steeringRight;
  float engineForce, breakingForce, vehicleSteering;
};

class LocationSlider : public QSlider {
  Q_OBJECT
};

class ConfigurationWindow : public QWidget {
  Q_OBJECT

public:
  ConfigurationWindow();

  void addSliderValue(const char* name, float* var);

private:
  //LocationSlider* xSlider;
  //LocationSlider* ySlider;
};
