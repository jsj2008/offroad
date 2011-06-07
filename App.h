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

const QString HIGHSCORE_FILENAME = "highscore";
const int WIN_WIDTH = 800;
const int WIN_HEIGHT = 600;
const int RTT_WIDTH = 800;
const int RTT_HEIGHT = 600;
const int SHADOWMAP_WIDTH = 512;
const int SHADOWMAP_HEIGHT = 512;

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

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

// Frustum culling, heavily inspired by Humus (http://www.humus.name/).
class Frustum {
public:
  Frustum() {}
  Frustum(const mat4& mvp) {
    update(mvp);
  };
 
  void update(const mat4& mvp) {
    const qreal* data = mvp.constData();
    planes[FrustumLeft  ] = Plane(data[12] - data[0], data[13] - data[1], data[14] - data[2],  data[15] - data[3]);
    planes[FrustumRight ] = Plane(data[12] + data[0], data[13] + data[1], data[14] + data[2],  data[15] + data[3]);

    planes[FrustumTop   ] = Plane(data[12] - data[4], data[13] - data[5], data[14] - data[6],  data[15] - data[7]);
    planes[FrustumBottom] = Plane(data[12] + data[4], data[13] + data[5], data[14] + data[6],  data[15] + data[7]);

    planes[FrustumFar   ] = Plane(data[12] - data[8], data[13] - data[9], data[14] - data[10], data[15] - data[11]);
    planes[FrustumNear  ] = Plane(data[12] + data[8], data[13] + data[9], data[14] + data[10], data[15] + data[11]);
  }

  bool containsAabb(const btVector3& aabbMin, const btVector3& aabbMax) {
    float minX = aabbMin.x();
    float minY = aabbMin.y();
    float minZ = aabbMin.z();
    float maxX = aabbMax.x();
    float maxY = aabbMax.y();
    float maxZ = aabbMax.z();

    for (int i = 0; i < 6; ++i) {
      if (planes[i].dist(btVector3(minX, minY, minZ)) > 0) continue;
      if (planes[i].dist(btVector3(minX, minY, maxZ)) > 0) continue;
      if (planes[i].dist(btVector3(minX, maxY, minZ)) > 0) continue;
      if (planes[i].dist(btVector3(minX, maxY, maxZ)) > 0) continue;
      if (planes[i].dist(btVector3(maxX, minY, minZ)) > 0) continue;
      if (planes[i].dist(btVector3(maxX, minY, maxZ)) > 0) continue;
      if (planes[i].dist(btVector3(maxX, maxY, minZ)) > 0) continue;
      if (planes[i].dist(btVector3(maxX, maxY, maxZ)) > 0) continue;
        return false;
    }
    return true;
  }

private:
  enum FrustumPlane {
    FrustumLeft = 0,
    FrustumRight,
    FrustumTop,
    FrustumBottom,
    FrustumFar,
    FrustumNear
  };

  struct Plane {
    Plane() {}
    Plane(float x, float y, float z, float offset) {
      normal = btVector3(x, y, z);
      float invLen = 1.0f / normal.length();
      normal *= invLen;
      this->offset = offset * invLen;
    }

    float dist(const btVector3& pos) const {
      return normal.dot(pos) + offset;
    }

    btVector3 normal;
    float offset;
  };

  Plane planes[6];
};

struct RenderContext {
  vec3 sunDirection;
  mat4 sunModelView;
  mat4 sunProjection;
  mat4 modelView;
  mat4 projection;
  GLuint depthBuffer;
  Frustum viewFrustum;
  bool frustumCulling;
  int objectsDrawn;
};

class BlenderScene {
public:
  BlenderScene(const char* fileName, btDynamicsWorld* world, Renderer* renderer);
  ~BlenderScene();

  void draw(qint64 delta, RenderContext& ctx);

private:
  struct RenderableObject {
    RenderableObject() {
      mesh = NULL; // TODO: nulptr?
      texture0 = NULL;
      texture1 = NULL;
      texture2 = NULL;
      texture3 = NULL;
      texture4 = NULL;
      shader = NULL;
      body = NULL;
      ghost = false;
      transparent = false;
    }

    bool operator < (const BlenderScene::RenderableObject& other) const {
      return shader < other.shader; // Comparing addresses (really simple way to sort objects by shaders).
    }

    Mesh* mesh;
    Texture* texture0;
    Texture* texture1;
    Texture* texture2;
    Texture* texture3;
    Texture* texture4;
    Shader* shader;
    btRigidBody* body;
    QString name;
    bool ghost;
    bool transparent;
    btTransform transform;
  };

  btBulletWorldImporter* importer;
  btDynamicsWorld* world;
  Renderer* renderer;
  QList<RenderableObject> objects;
};

class App : public QGLWidget {
  Q_OBJECT

public:
  App(const QGLFormat& format, ConfigurationWindow* configWin);
  virtual ~App();

private slots:
  void updateFps();

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
  void drawVehicle(RenderContext& ctx, mat4& modelView, bool shadow=true);

  RenderContext ctx;
  int framesDrawn;
  QString fps;
  QTimer* fpsTimer;

  ConfigurationWindow* configWin;
  Renderer* renderer;

  Mesh* wheel;
  Mesh* truck;
  Mesh* chassisMesh;

  Texture* carTexture;
  Texture* wheelTexture;

  Shader* tyre;
  Shader* chassisShader;
  Shader* env;
  Texture* envCubemap;

  GLuint fbo;
  GLuint depthTexture;
  GLuint colorBuffer;

  Shader* blur;
  bool blurEnabled;
  mat4 previousModelView;

  GLuint shadowFbo;
  GLuint shadowDepthTexture;
  bool shadows;
  int shadowMapWidth;
  int shadowMapHeight;
  Shader* plain;

  BlenderScene* scene;
  FirstPersonCamera* cam;
  float fov;
  QElapsedTimer* timer;

  bool mouseFree;
  bool vehicleCam;
  btVector3 camDir;
  btVector3 chassisPos;

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
  bool drawAabb;

  bool accelerating, breaking, steeringLeft, steeringRight;
  float engineForce, breakingForce, vehicleSteering;
  const float maxSteering;

  QFont infoFont;
  QFont hudFont;
  QFont timeFont;
  QFont highscoreFont;
  bool infoShown;
  QElapsedTimer* trackTimer;
  int state;
  int numResets;

  enum State {
    Counting,
    Racing,
    Highscore
  };

  QList<QPair<QString, qint64> > highscore;

  Texture* speedometerBack;
  Texture* speedometerFront;
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
