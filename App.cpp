#include <iostream>
#include <GL/glew.h>
#include <stdexcept>
#include <string>
#include <fstream>

#include <App.h>
#include <QtXml>
#include <QDomDocument>
#include <QDomElement>
#include <QList>

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

typedef QVector2D vec2;
typedef QVector3D vec3;
typedef QVector4D vec4;
typedef QMatrix3x3 mat3;
typedef QMatrix4x4 mat4;

#include <FirstPersonCamera.h>

class load_exception : public std::runtime_error {
public:
  load_exception() : std::runtime_error("Unknown error!") {}
  load_exception(const std::string& message) : std::runtime_error(message) {}
  load_exception(const QString& message) : std::runtime_error(message.toUtf8().constData()) {}
};

void checkErrors() {
  using namespace std;
  GLenum error = glGetError();
  switch (error) {
    case GL_NO_ERROR:          cout << "OpenGL error: none" << endl; break;
    case GL_INVALID_ENUM:      cout << "OpenGL error: invalid enum" << endl; break;
    case GL_INVALID_VALUE:     cout << "OpenGL error: invalid value" << endl; break;
    case GL_INVALID_OPERATION: cout << "OpenGL error: invalid operation" << endl; break;
    case GL_STACK_OVERFLOW:    cout << "OpenGL error: stack overflow" << endl; break;
    case GL_OUT_OF_MEMORY:     cout << "OpenGL error: out of memory" << endl; break;
    case GL_TABLE_TOO_LARGE:   cout << "OpenGL error: table too large" << endl; break;
    default:                   cout << "OpenGL error: unknown" << endl; break;
  }
}

class VertexBuffer {
private:
  friend class Renderer;
  GLuint id;
  unsigned int size;
};

class IndexBuffer {
private:
  friend class Renderer;
  GLuint id;
  unsigned int size;
};

class Texture {
private:
  friend class Renderer;
  GLuint id;
  unsigned int width, height;
};

class Shader {
private:
  friend class Renderer;
  GLenum vertexShader, pixelShader, program;
};

class Mesh {
public:
  ~Mesh() {
    // TODO: delete vao?
  }

private:
  friend class Renderer;
  IndexBuffer* indexBuffer;
  VertexBuffer* vertexBuffer;
  GLuint vao;
  unsigned int nVertices, vertexSize, nIndices, indexSize;
  unsigned int version;
};

class Renderer {
public:
  Renderer() {
    currentProgram = 0;
  }

  ~Renderer() {
    Shader* shader;
    foreach (shader, shaders) {
      glDeleteShader(shader->vertexShader);
      glDeleteShader(shader->pixelShader);
      glDeleteProgram(shader->program);
      delete shader;
    }

    Texture* texture;
    foreach (texture, textures) {
      glDeleteTextures(1, &(texture->id));
      delete texture;
    }

    IndexBuffer* indexBuffer;
    foreach (indexBuffer, indexBuffers) {
      glDeleteBuffers(1, &(indexBuffer->id));
      delete indexBuffer;
    }

    VertexBuffer* vertexBuffer;
    foreach (vertexBuffer, vertexBuffers) {
      glDeleteBuffers(1, &(vertexBuffer->id));
      delete vertexBuffer;
    }

    Mesh* mesh;
    foreach (mesh, meshes) {
      delete mesh;
    }
  }

  Shader* addShader(const char* fileName) {
    QDomDocument doc;
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly)) {
      QString error = "Error opening file ";
      error += fileName;
      throw load_exception(error.toStdString());
    }

    if (!doc.setContent(&file)) {
      file.close();
      QString error = "Error while loading file ";
      error += fileName;
      throw load_exception(error.toStdString());
    }

    file.close();

    QString vertexShaderSource;
    QString pixelShaderSource;
    QDomNodeList shaders = doc.elementsByTagName("shader");

    for (unsigned int i = 0; i < shaders.length(); ++i) {
      QDomElement e = shaders.at(i).toElement();
      if (e.attribute("type") == "vertex")
        vertexShaderSource = e.text();
      else if (e.attribute("type") == "pixel" || e.attribute("type") == "fragment")
        pixelShaderSource = e.text();
    }



    GLuint program = glCreateProgram();
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint pixelShader = glCreateShader(GL_FRAGMENT_SHADER);

    // TODO: yuck, ugly
    QStringList splitted = vertexShaderSource.split('\n');
    char** lines = new char*[splitted.size()];
    for (int i = 0; i < splitted.size(); ++i) {
      std::string line = splitted[i].toStdString();
      lines[i] = new char[line.size()+1];
      strcpy(lines[i], line.c_str());
    }
    glShaderSource(vertexShader, splitted.size(), (const GLchar**)(lines), NULL);
    for (int i = 0; i < splitted.size(); ++i)
      delete [] lines[i];
    delete [] lines;

    splitted = pixelShaderSource.split('\n');
    lines = new char*[splitted.size()];
    for (int i = 0; i < splitted.size(); ++i) {
      std::string line = splitted[i].toStdString();
      lines[i] = new char[line.size()+1];
      strcpy(lines[i], line.c_str());
    }
    glShaderSource(pixelShader, splitted.size(), (const GLchar**)(lines), NULL);
    for (int i = 0; i < splitted.size(); ++i)
      delete [] lines[i];
    delete [] lines;

    glCompileShader(vertexShader);
    if (!checkSuccess(vertexShader)) {
      QString error = "Failed to compile vertex shader ";
      error += fileName;
      error += "!";
      throw load_exception(error.toStdString());
    }

    glCompileShader(pixelShader);
    if (!checkSuccess(pixelShader)) {
      QString error = "Failed to compile pixel shader ";
      error += fileName;
      error += "!";
      throw load_exception(error.toStdString());
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, pixelShader);

    QDomNodeList attrNodes = doc.elementsByTagName("attribute");
    for (uint i = 0; i < attrNodes.length(); ++i) {
      QDomElement e = attrNodes.at(i).toElement();
      if (!e.hasAttribute("unit") || !e.hasAttribute("name")) {
        std::cout << "Shader " << fileName << " attribute incorrectly specified!" << std::endl; // TODO: make sure toInt didn't fail!
        continue;
      }
      glBindAttribLocation(program, e.attribute("unit").toInt(), e.attribute("name").toStdString().c_str());
    }

    glLinkProgram(program);

    Shader* shader = new Shader;
    shader->program = program;
    shader->vertexShader = vertexShader;
    shader->pixelShader = pixelShader;
    this->shaders << shader;
    return shader;
  }

  VertexBuffer* addVertexBuffer(const char* data, unsigned int sizeInBytes, GLenum hint) {
    GLuint id;
    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, sizeInBytes, data, hint);

    VertexBuffer* buffer = new VertexBuffer;
    buffer->id = id;
    buffer->size = sizeInBytes;
    vertexBuffers << buffer;
    return buffer;
  }

  Texture* addTexture(const char* fileName) {
    GLuint id;
    QImage img;
    QImage glImg;

    if (!img.load(fileName)) {
      QString error = "Error opening ";
      error += fileName;
      throw load_exception(error);
    }

    glImg = QGLWidget::convertToGLFormat(img);
    if(glImg.isNull()) {
      QString error = "Error converting ";
      error += fileName;
      error += " to a format OpenGL prefers!";
      throw load_exception(error);
    }

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
            glImg.width(), glImg.height(),
            0, GL_RGBA, GL_UNSIGNED_BYTE, glImg.bits());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    Texture* texture = new Texture;
    texture->id = id;
    texture->height = glImg.height();
    texture->width = glImg.width();
    textures << texture;
    return texture;
  }

  IndexBuffer* addIndexBuffer(const char* data, unsigned int sizeInBytes, GLenum hint) {
    GLuint id;
    glGenBuffers(1, &id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeInBytes, data, hint);

    IndexBuffer* buffer = new IndexBuffer;
    buffer->id = id;
    buffer->size = sizeInBytes;
    indexBuffers << buffer;
    return buffer;
  }

  void setTexture(const char* samplerName, Texture* texture, unsigned int unit) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture->id);
    if (currentProgram != 0)
      glUniform1i(glGetUniformLocation(currentProgram, samplerName), unit);
  }

  void setShader(Shader* shader) {
    glUseProgram(shader->program);
    currentProgram = shader->program;
  }

  void disableShaders() {
    glUseProgram(0);
    currentProgram = 0;
  }

  void setUniform(const char* name, const vec2& value) {
    glUniform2f(glGetUniformLocation(currentProgram, name), value.x(), value.y());
  }

  void setUniform(const char* name, const vec3& value) {
    glUniform3f(glGetUniformLocation(currentProgram, name), value.x(), value.y(), value.z());
  }

  void setUniform(const char* name, const vec4& value) {
    glUniform4f(glGetUniformLocation(currentProgram, name), value.x(), value.y(), value.z(), value.w());
  }

  void drawGrid(uint numOfLines, float halfSize) {
    glBegin(GL_LINES);
    glColor3f(1,1,1);
    float spacing = halfSize*2./numOfLines;
    for (uint i = 0; i < numOfLines+1; i++) {
      glVertex3f(-halfSize + i*spacing, 0., -halfSize);
      glVertex3f(-halfSize + i*spacing, 0., halfSize);
      glVertex3f(-halfSize, 0, -halfSize + i*spacing);
      glVertex3f(halfSize, 0, -halfSize + i*spacing);
    }
    glEnd();
  }

  Mesh* addMesh(const char* fileName) {
    // Format:
    // format version     |
    // nVertices          |
    // nIndices           |  4 bytes each
    // vertexSize         |
    // indexSize          |
    // vertexBuffer (nVertices * vertexSize)
    // indexBuffer (nIndices * indexSize)
    std::ifstream file;
    file.open(fileName, std::ios::binary);

    if (!file.is_open()) {
      QString error = "Error opening file ";
      error += fileName;
      error += "!";
      throw load_exception(error);
    }

    unsigned int version, nVertices, nIndices, vertexSize, indexSize;
    file.read(reinterpret_cast<char*>(&version), 4); // TODO: endianness?
    file.read(reinterpret_cast<char*>(&nVertices), 4);
    file.read(reinterpret_cast<char*>(&nIndices), 4);
    file.read(reinterpret_cast<char*>(&vertexSize), 4);
    file.read(reinterpret_cast<char*>(&indexSize), 4);

    if (version != 1)
      throw load_exception(QString("Unknown mesh version!"));

    std::vector<char> data;
    data.resize(nVertices * vertexSize);
    file.read(reinterpret_cast<char*>(&data[0]), nVertices * vertexSize);
    if (file.eof()) {
      QString error = "End of file reached while reading ";
      error += fileName;
      error += "!";
      throw load_exception(error);
    }
    VertexBuffer* vertexBuffer = this->addVertexBuffer(reinterpret_cast<char*>(&data[0]), nVertices * vertexSize, GL_STATIC_DRAW);

    data.resize(nIndices * indexSize);
    file.read(reinterpret_cast<char*>(&data[0]), nIndices * indexSize);
    if (file.eof()) {
      QString error = "End of file reached while reading ";
      error += fileName;
      error += "!";
      throw load_exception(error);
    }
    IndexBuffer* indexBuffer = this->addIndexBuffer(reinterpret_cast<char*>(&data[0]), nIndices * indexSize, GL_STATIC_DRAW);

    file.close();

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->id);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer->id);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexSize, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexSize, BUFFER_OFFSET(3*4));
    glEnableVertexAttribArray(1);
    if (vertexSize == 32) // includes uvs
      glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertexSize, BUFFER_OFFSET(6*4));

    if (vertexSize == 32)
      glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    Mesh* mesh = new Mesh;
    mesh->version = version;
    mesh->vertexBuffer = vertexBuffer;
    mesh->indexBuffer = indexBuffer;
    mesh->nVertices = nVertices;
    mesh->vertexSize = vertexSize;
    mesh->nIndices = nIndices;
    mesh->indexSize = indexSize;
    mesh->vao = vao;
    meshes << mesh;

    std::cout << "Mesh " << fileName << " loaded." << std::endl;
    return mesh;
  }

  void drawMesh(Mesh* mesh) {
    glBindVertexArray(mesh->vao);
    glDrawElements(GL_TRIANGLES, mesh->nIndices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
  }

  void setIndexBuffer(IndexBuffer* buffer) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->id);
  }

  void setVertexBuffer(VertexBuffer* buffer) {
    glBindBuffer(GL_ARRAY_BUFFER, buffer->id);
  }

private:
  bool checkSuccess(GLenum shader) {
    char* log = new char[1000];
    glGetShaderInfoLog(shader, 1000, NULL, log);
    QString info(log);
    std::cout << info.trimmed().toUtf8().constData() << std::endl;
    delete [] log;

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    return compiled;
  }

  QList<Shader*> shaders;
  QList<Texture*> textures;
  QList<IndexBuffer*> indexBuffers;
  QList<VertexBuffer*> vertexBuffers;
  QList<Mesh*> meshes;

  GLuint currentProgram;
};


App::App(const QGLFormat& format, QWidget* parent = 0) : QGLWidget(format, parent) {
  renderer = NULL;
  cam = NULL;
  timer = NULL;
  drawDebugInfo = false;
}

App::~App() {
  this->cleanUpPhysics();

  if (renderer)
    delete renderer;

  if (cam)
    delete cam;

  if (timer)
    delete timer;
}

void App::initializeGL() {
  this->setWindowTitle("Offroad");
  this->setMouseTracking(true);
  this->grabKeyboard();
  this->setCursor(Qt::BlankCursor);
  timer = new QElapsedTimer();
  timer->start();
  state = true;

  GLenum err = glewInit();
  if (GLEW_OK != err) {
    std::cout << "Error initializing glew: " << glewGetErrorString(err) << std::endl;
    return;
  }

  GLint value;
  glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &value);
  std::cout << "Max texture units: " << value << std::endl;

  glGetIntegerv(GL_MAX_LIGHTS, &value);
  std::cout << "Max lights: " << value << std::endl;

  glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &value);
  std::cout << "Max vertex attribs: " << value << std::endl;

  glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &value);
  std::cout << "Max vertex texture units: " << value << std::endl;

  glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &value);
  std::cout << "Max vertex uniforms: " << value << std::endl;

  std::cout << "sizeof(GLfloat) = " << sizeof(GLfloat) << std::endl;

  renderer = new Renderer();

  try {
    wheel = renderer->addMesh("content/wheel.mesh");
    truck = renderer->addMesh("content/truck.mesh");
    diffuse = renderer->addShader("content/diffuse.shader");
    wheelShader = renderer->addShader("content/wheel.shader");
    wheelTexture = renderer->addTexture("content/truck-tex.jpg");
  } catch (load_exception& e) {
    std::cout << "Exception: " << e.what() << std::endl;
    this->parentWidget()->close();
    return;
  }

  cam = new FirstPersonCamera;
  checkErrors();

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE);

  this->setupPhysics();

  std::cout << "Init complete!" << std::endl;
}

void App::setupPhysics() {
  float connectionHeight = 1.2f;
  float cubeHalfExtents = 1;
  int rightIndex = 0;
  int upIndex = 1;
  int forwardIndex = 2;
  btVector3 wheelDirection(0,-1,0);
  btVector3 wheelAxle(-1,0,0);
  btScalar suspensionRestLength(0.6);
  float wheelRadius = 0.5f;
  float wheelWidth = 0.4f;
  float wheelFriction = 1000;
  float suspensionStiffness = 20.f;
  float suspensionDamping = 2.3f;
  float suspensionCompression = 4.4f;
  float rollInfluence = 0.1f;

  //groundShape = new btBoxShape(btVector3(50,3,50));
  //m_collisionShapes.push_back(groundShape);
  collisionConfiguration = new btDefaultCollisionConfiguration();
  dispatcher = new btCollisionDispatcher(collisionConfiguration);
  btVector3 worldMin(-1000,-1000,-1000);
  btVector3 worldMax(1000,1000,1000);
  overlappingPairCache = new btAxisSweep3(worldMin, worldMax);
  constraintSolver = new btSequentialImpulseConstraintSolver();
  dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, constraintSolver, collisionConfiguration);

  int heightfieldWidth = 100;
  int heightfieldHeight = 100;
  heightfield = new float[heightfieldWidth * heightfieldHeight];
  memset(heightfield, 0, sizeof(float)*heightfieldWidth*heightfieldHeight);

  groundShape = new btHeightfieldTerrainShape(heightfieldWidth, heightfieldHeight,
      heightfield,
      btScalar(1),
      btScalar(0),
      btScalar(0),
      1,
      PHY_FLOAT,
      false);
  btDefaultMotionState* terrainState = new btDefaultMotionState(btTransform(btQuaternion(btVector3(0,1,0), M_PI*0.5), btVector3(0,0,0)));
  btRigidBody::btRigidBodyConstructionInfo terrainCI(0, terrainState, groundShape, btVector3(0,0,0));
  groundBody = new btRigidBody(terrainCI);
  dynamicsWorld->addRigidBody(groundBody);

  btCollisionShape *chassisShape = new btBoxShape(btVector3(1.f,2.f, 0.5f));
  btCompoundShape *compound = new btCompoundShape();
  btTransform localTrans;
  localTrans.setIdentity();
  localTrans.setOrigin(btVector3(0,0,1));

  compound->addChildShape(localTrans, chassisShape);

  btDefaultMotionState* chassisState = new btDefaultMotionState();
  btRigidBody::btRigidBodyConstructionInfo chassisCI(0, chassisState, compound, btVector3(0,0,0));
  chassis = new btRigidBody(chassisCI);
  chassis->setActivationState(DISABLE_DEACTIVATION);

  wheelShape = new btCylinderShapeX(btVector3(wheelWidth, wheelRadius, wheelRadius));

  vehicleRaycaster = new btDefaultVehicleRaycaster(dynamicsWorld);
  vehicle = new btRaycastVehicle(tuning, chassis, vehicleRaycaster);

  dynamicsWorld->addVehicle(vehicle);

  btVector3 connectionPoint(cubeHalfExtents-(0.3*wheelWidth),connectionHeight,2*cubeHalfExtents-wheelRadius);
  vehicle->addWheel(connectionPoint, wheelDirection, wheelAxle, suspensionRestLength, wheelRadius, tuning, true);

  connectionPoint = btVector3(-cubeHalfExtents+(0.3*wheelWidth),connectionHeight,2*cubeHalfExtents-wheelRadius);
  vehicle->addWheel(connectionPoint, wheelDirection, wheelAxle, suspensionRestLength, wheelRadius, tuning, true);

  connectionPoint = btVector3(-cubeHalfExtents+(0.3*wheelWidth),connectionHeight,-2*cubeHalfExtents+wheelRadius);
  vehicle->addWheel(connectionPoint, wheelDirection, wheelAxle, suspensionRestLength, wheelRadius, tuning, false);

  connectionPoint = btVector3(cubeHalfExtents-(0.3*wheelWidth),connectionHeight,-2*cubeHalfExtents+wheelRadius);
  vehicle->addWheel(connectionPoint, wheelDirection, wheelAxle, suspensionRestLength, wheelRadius, tuning, false);

  for (int i = 0; i < vehicle->getNumWheels(); ++i) {
    btWheelInfo& wheel = vehicle->getWheelInfo(i);
    wheel.m_suspensionStiffness = suspensionStiffness;
    wheel.m_wheelsDampingRelaxation = suspensionDamping;
    wheel.m_wheelsDampingCompression = suspensionCompression;
    wheel.m_frictionSlip = wheelFriction;
    wheel.m_rollInfluence = rollInfluence;
  }

  engineForce = breakingForce = vehicleSteering = 0;

  debugDrawer = new GLDebugDrawer();
  dynamicsWorld->setDebugDrawer(debugDrawer);
}

void App::cleanUpPhysics() {
  if (heightfield)
    delete [] heightfield;

  // major TODO !!
}

void App::paintGL() {
  qint64 delta = timer->restart();
  dynamicsWorld->stepSimulation(delta/3000.f, 10);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45., float(this->width()) / this->height(), 0.1, 1000.);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  cam->setTransform(delta / 100.);

  renderer->disableShaders();
  renderer->drawGrid(50, 10);

  renderer->setShader(diffuse);

  btScalar matrix[16];
  vehicle->getChassisWorldTransform().getOpenGLMatrix(matrix);
  glPushMatrix(); // TODO: modernize this!
  glMultMatrixf(matrix);
  renderer->drawMesh(truck);
  glPopMatrix();

  renderer->setShader(wheelShader);
  renderer->setTexture("wheel", wheelTexture, 0);

  if (accelerating) {
    engineForce = 1000;
    breakingForce = 0;
  }

  if (breaking) {
    breakingForce = 100;
    engineForce = 0;
  }

  vehicle->applyEngineForce(engineForce, 2);
  vehicle->applyEngineForce(engineForce, 3);
  vehicle->setBrake(breakingForce, 2);
  vehicle->setBrake(breakingForce, 3);

  if (steeringLeft)
    vehicleSteering += 0.04;

  if (steeringRight)
    vehicleSteering -= 0.04;

  // Front wheels
  vehicle->setSteeringValue(vehicleSteering, 0);
  vehicle->setSteeringValue(vehicleSteering, 1);

  for (int i = 0; i < vehicle->getNumWheels(); ++i) {
    vehicle->updateWheelTransform(i, true);
    vehicle->getWheelInfo(i).m_worldTransform.getOpenGLMatrix(matrix);
    glPushMatrix();
    glMultMatrixf(matrix);
    renderer->drawMesh(wheel);
    glPopMatrix();
  }

  renderer->disableShaders();

  if (drawDebugInfo) {
    dynamicsWorld->getDebugDrawer()->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
    dynamicsWorld->debugDrawWorld();
  }

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-.5, .5, .5, -.5, -1000, 1000);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  this->renderText(1,0,0, "FPS: 943"); // TODO: doesn't work anymore!

  this->update();
}

void App::resizeGL(int width, int height) {
  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45., float(width)/height, 0.1, 1000.);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void App::keyPressEvent(QKeyEvent* event) {
  switch (event->key()) {
    case Qt::Key_Escape:
      this->parentWidget()->close();
      break;
    case Qt::Key_Space:
      this->releaseKeyboard();
      this->setCursor(Qt::ArrowCursor);
      state = false;
      break;
    case Qt::Key_B:
      drawDebugInfo = !drawDebugInfo;
      break;
    case Qt::Key_Up:
      accelerating = true;
      breaking = false;
      break;
    case Qt::Key_Down:
      accelerating = false;
      breaking = true;
    case Qt::Key_Left:
      steeringLeft = true;
      steeringRight = false;
      break;
    case Qt::Key_Right:
      steeringRight = true;
      steeringLeft = false;
      break;
  }

  cam->processKeyPress(event->key());
}

void App::keyReleaseEvent(QKeyEvent* event) {
  switch (event->key()) {
    case Qt::Key_Up:
      accelerating = false;
      breaking = true;
      break;
    case Qt::Key_Down:
      accelerating = true;
      breaking = false;
    case Qt::Key_Left:
      steeringLeft = false;
      steeringRight = true;
      break;
    case Qt::Key_Right:
      steeringRight = false;
      steeringLeft = true;
      break;
  }
  cam->processKeyRelease(event->key());
}

void App::mouseMoveEvent(QMouseEvent* event) {
  if (state) {
    int x = this->pos().x()+this->width()/2;
    int y = this->pos().y()+this->height()/2;

    int dx = event->globalX() - x;
    int dy = event->globalY() - y;
    if (dx != 0 || dy != 0) {
      cam->processMouseMotion(dx, dy);
      QCursor::setPos(x, y);
    }
  }
}

void App::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    state = true;
    this->grabKeyboard();
    this->setCursor(Qt::BlankCursor);
  }
}

ConfigurationWindow::ConfigurationWindow() : QWidget(0, Qt::Window) {
  /*QGridLayout* mainLayout = new QGridLayout;
  mainLayout->setSpacing(0);
  mainLayout->setContentsMargins(0,0,0,0);

  setLayout(mainLayout);*/
}

int main(int argc, char** args) {
  QApplication qapp(argc, args);
  QGLFormat format;
  format.setSampleBuffers(true);
  App app(format);
  app.resize(800, 800);
  ConfigurationWindow win;
  win.show();
  app.show();
  return qapp.exec();
}
