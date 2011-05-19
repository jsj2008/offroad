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

#include <FirstPersonCamera.h>

class load_exception : public std::runtime_error {
public:
  load_exception() : std::runtime_error("Unknown error!") {}
  load_exception(const std::string& message) : std::runtime_error(message) {}
  load_exception(const QString& message) : std::runtime_error(message.toUtf8().constData()) {}
};

template <class T>
T clamp(const T& value, const T& low, const T& high) {
  return value < low ? low : (value > high ? high : value);
}

vec3 btToQt(const btVector3& v) {
  return vec3(v.x(), v.y(), v.z());
}

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

mat4 toMat4(const btScalar* columnMajor) {
  qreal mat[16];
  for (int i = 0; i < 16; ++i) mat[i] = columnMajor[i];
  return mat4(mat).transposed();
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
    QString fullPath = QFileInfo(fileName).absoluteFilePath();
    if (loadedShaders.contains(fullPath))
      return loadedShaders[fullPath];

    QDomDocument doc;
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly)) {
      QString error = "Error opening file ";
      error += fileName;
      throw load_exception(error.toStdString());
    }

    QString msg;
    if (!doc.setContent(&file, false, &msg)) {
      file.close();
      QString error = "Error loading file ";
      error += fileName;
      error += " [";
      error += msg + "]";
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

    std::cout << "Loaded shader " << fullPath.toStdString() << std::endl;
    loadedShaders[fullPath] = shader;
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
    QString fullPath = QFileInfo(fileName).absoluteFilePath();
    if (loadedTextures.contains(fullPath))
      return loadedTextures[fullPath];

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
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    Texture* texture = new Texture;
    texture->id = id;
    texture->height = glImg.height();
    texture->width = glImg.width();
    textures << texture;

    loadedTextures[fullPath] = texture;
    std::cout << "Loaded texture " << fileName << std::endl;
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

  void setUniform1i(const char* name, int value) {
    glUniform1i(glGetUniformLocation(currentProgram, name), value);
  }

  void setUniform1d(const char* name, double value) {
    glUniform1f(glGetUniformLocation(currentProgram, name), value);
  }

  void setUniform1f(const char* name, float value) {
    glUniform1f(glGetUniformLocation(currentProgram, name), value);
  }

  void setUniform2f(const char* name, const vec2& value) {
    glUniform2f(glGetUniformLocation(currentProgram, name), value.x(), value.y());
  }

  void setUniform3f(const char* name, const vec3& value) {
    glUniform3f(glGetUniformLocation(currentProgram, name), value.x(), value.y(), value.z());
  }

  void setUniform4f(const char* name, const vec4& value) {
    glUniform4f(glGetUniformLocation(currentProgram, name), value.x(), value.y(), value.z(), value.w());
  }

  void setUniformMat4(const char* name, const mat4& value) {
    GLfloat mat[4*4]; // TODO: this assumes sizeof qreal == sizeof double
    const qreal* data = value.constData();
    for (int i = 0; i < 16; ++i)
      mat[i] = data[i];
    glUniformMatrix4fv(glGetUniformLocation(currentProgram, name), 1, GL_FALSE, mat);
  }

  void setUniform4fv(const char* name, const GLfloat* value) {
    glUniformMatrix4fv(glGetUniformLocation(currentProgram, name), 1, GL_FALSE, value);
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

    QString fullPath = QFileInfo(fileName).absoluteFilePath();
    if (loadedMeshes.contains(fullPath))
      return loadedMeshes[fullPath];

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

    if (version < 1)
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
    // TODO: implement VertexFormat structures
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexSize, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexSize, BUFFER_OFFSET(3*4));
    glEnableVertexAttribArray(1);
    if (vertexSize >= 32) {
      glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertexSize, BUFFER_OFFSET(6*4));
      glEnableVertexAttribArray(2);
    }
    if (vertexSize >= 40) {
      glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, vertexSize, BUFFER_OFFSET(8*4));
      glEnableVertexAttribArray(3);
    }
    if (vertexSize >= 48) {
      glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, vertexSize, BUFFER_OFFSET(10*4));
      glEnableVertexAttribArray(4);
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

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

    loadedMeshes[fullPath] = mesh;
    std::cout << "Loaded mesh " << fileName <<  std::endl;
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
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
      char* log = new char[1000];
      glGetShaderInfoLog(shader, 1000, NULL, log);
      QString info(log);
      std::cout << info.trimmed().toUtf8().constData() << std::endl;
      delete [] log;
    }

    return compiled;
  }

  QList<Shader*> shaders;
  QList<Texture*> textures;
  QList<IndexBuffer*> indexBuffers;
  QList<VertexBuffer*> vertexBuffers;
  QList<Mesh*> meshes;

  QHash<QString, Shader*> loadedShaders;
  QHash<QString, Texture*> loadedTextures;
  QHash<QString, Mesh*> loadedMeshes;

  GLuint currentProgram;
};

App::App(const QGLFormat& format, ConfigurationWindow* configWin) : QGLWidget(format, 0), maxSteering(0.5) {
  this->configWin = configWin;
  renderer = NULL;
  cam = NULL;
  timer = NULL;
  drawDebugInfo = false;
  accelerating = breaking = steeringLeft = steeringRight = false;
  vehicleCam = false;
  engineForce = breakingForce = vehicleSteering = 0;
  mouseFree = true;
  stipple = true;
  blurEnabled = false;
  drawAabb = false;
  shadows = true;
  camDir = btVector3(0,1,0);
  framesDrawn = 0;
  fps = "";
}

App::~App() {
  this->cleanUpPhysics();

  glDeleteFramebuffersEXT(1, &fbo);
  glDeleteRenderbuffersEXT(1, &depthBuffer);

  glDeleteFramebuffersEXT(1, &shadowFbo);

  if (scene)
    delete scene;

  if (renderer)
    delete renderer;

  if (cam)
    delete cam;

  if (timer)
    delete timer;

  if (fpsTimer)
    delete fpsTimer;
}

void App::initializeGL() {
  this->setWindowTitle("Offroad 2");
  this->setMouseTracking(true);
  this->grabKeyboard();
  this->setCursor(Qt::BlankCursor);
  timer = new QElapsedTimer();
  timer->start();

  fpsTimer = new QTimer(this);
  connect(fpsTimer, SIGNAL(timeout()), this, SLOT(updateFps()));
  fpsTimer->start(1000);

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

  GLint maxbuffers;
  glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxbuffers);
  std::cout << "Max color attachments: " << maxbuffers << std::endl;

  glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxbuffers);
  std::cout << "Max draw buffers: " << maxbuffers << std::endl;

  std::cout << "sizeof(GLfloat) = " << sizeof(GLfloat) << std::endl;

  renderer = new Renderer();

  try {
    wheel = renderer->addMesh("content/wheel.mesh");
    truck = renderer->addMesh("content/truck.mesh");
    diffuse = renderer->addShader("content/diffuse.shader");
    diffuseTextured = renderer->addShader("content/diffuse-textured.shader");
    wheelTexture = renderer->addTexture("content/truck-tex.jpg");
    skyDome = renderer->addMesh("content/sky-dome.mesh");
    scattering = renderer->addShader("content/scattering.shader");
    blur = renderer->addShader("content/blur.shader");
    plain = renderer->addShader("content/plain.shader");

    this->setupPhysics();

    scene = new BlenderScene("content/level1/level1.scene", dynamicsWorld, renderer);

  } catch (load_exception& e) {
    std::cout << "Exception: " << e.what() << std::endl;
    this->parentWidget()->close();
    return;
  }

  cam = new FirstPersonCamera;
  checkErrors();

  //configWin->addPointerValue("Rotation", &rotation, 0, 360);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE);

  rttWidth = 1024;
  rttHeight = 1024;

  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  glGenRenderbuffers(1, &depthBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, rttWidth, rttHeight);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

  glGenTextures(1, &colorBuffer);
  glBindTexture(GL_TEXTURE_2D, colorBuffer);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rttWidth, rttHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  //glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);

  glGenTextures(1, &depthTexture);
  glBindTexture(GL_TEXTURE_2D, depthTexture);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  //glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, rttWidth, rttHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
  glBindTexture(GL_TEXTURE_2D, 0);

  // TODO: this is just wrong
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, depthTexture, 0);

  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

  if (GL_FRAMEBUFFER_COMPLETE != status) {
    std::cout << "There was an error completing the FBO setup!" << std::endl;
    this->parentWidget()->close();
    return;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Build another FBO for shadows.
  shadowMapWidth = 1024;
  shadowMapHeight = 1024;
	
  glGenTextures(1, &shadowDepthTexture);
  glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapWidth, shadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
  glBindTexture(GL_TEXTURE_2D, 0);

  glGenFramebuffers(1, &shadowFbo);
  glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo);

  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
 
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowDepthTexture, 0);

  status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
  if (GL_FRAMEBUFFER_COMPLETE != status) {
    std::cout << "Error while creating shadow FBO!" << std::endl;
    this->parentWidget()->close();
    return;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  std::cout << "Init complete!" << std::endl;
}

void App::setupPhysics() {
  float connectionHeight = -0.1;
  int rightIndex = 0;
  int upIndex = 2;
  int forwardIndex = 1;
  float rollInfluence = 1.1f;
  float	wheelRadius = 0.5;
  float	wheelWidth = 0.5f;
  float	wheelFriction = 100;
  float	suspensionStiffness = 40.f;
  float	suspensionDamping = 3.3f;
  float	suspensionCompression = 4.4f;
  btVector3 wheelDirection(0,0,-1);
  btVector3 wheelAxle(1,0,0);
  btScalar suspensionRestLength(0.8);
  float vehicleMass = 1200;

  collisionConfiguration = new btDefaultCollisionConfiguration();
  dispatcher = new btCollisionDispatcher(collisionConfiguration);
  btVector3 worldMin(-1000,-1000,-1000);
  btVector3 worldMax(1000,1000,1000);
  overlappingPairCache = new btAxisSweep3(worldMin, worldMax);
  constraintSolver = new btSequentialImpulseConstraintSolver();
  dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, constraintSolver, collisionConfiguration);
  dynamicsWorld->setGravity(btVector3(0,0,-10));

  //btCollisionShape *chassisShape = new btBoxShape(btVector3(0.8, 1.8, 0.7));
  btCollisionShape* chassisShape = new btCapsuleShape(0.7, 2);
  btCompoundShape* compound = new btCompoundShape();
  btTransform localTrans;
  localTrans.setIdentity();

  compound->addChildShape(localTrans, chassisShape);

  wheelShape = new btCylinderShapeX(btVector3(wheelWidth / 2, 0.4, 0.4));

  btVector3 cp1( 1.0,  1.4, connectionHeight);
  btVector3 cp2(-1.0,  1.4, connectionHeight);
  btVector3 cp3( 1.0, -1.2, connectionHeight);
  btVector3 cp4(-1.0, -1.2, connectionHeight);

  btVector3 off(0,0,-0.4);

  btTransform tr1;
  tr1.setIdentity();
  tr1.setOrigin(cp1 + off);
  compound->addChildShape(tr1, wheelShape);

  btTransform tr2;
  tr2.setIdentity();
  tr2.setOrigin(cp2 + off);
  compound->addChildShape(tr2, wheelShape);

  btTransform tr3;
  tr3.setIdentity();
  tr3.setOrigin(cp3 + off);
  compound->addChildShape(tr3, wheelShape);

  btTransform tr4;
  tr4.setIdentity();
  tr4.setOrigin(cp4 + off);
  compound->addChildShape(tr4, wheelShape);

  btVector3 localInertia(0,0,0);
  btTransform startTransform;
  startTransform.setIdentity();
  startTransform.setOrigin(btVector3(5, 5, 5));
  startTransform.setRotation(btQuaternion(btVector3(0,0,1), M_PI/2));
  chassisShape->calculateLocalInertia(vehicleMass, localInertia);
  btDefaultMotionState* chassisState = new btDefaultMotionState(startTransform);
  btRigidBody::btRigidBodyConstructionInfo chassisCI(vehicleMass, chassisState, compound, localInertia);
  chassis = new btRigidBody(chassisCI);
  chassis->setActivationState(DISABLE_DEACTIVATION);
  dynamicsWorld->addRigidBody(chassis);

  vehicleRaycaster = new btDefaultVehicleRaycaster(dynamicsWorld);
  vehicle = new btRaycastVehicle(tuning, chassis, vehicleRaycaster);
  dynamicsWorld->addVehicle(vehicle);

  vehicle->setCoordinateSystem(rightIndex,upIndex,forwardIndex);

  vehicle->addWheel(cp1, wheelDirection, wheelAxle, suspensionRestLength, wheelRadius, tuning, true);
  vehicle->addWheel(cp2, wheelDirection, wheelAxle, suspensionRestLength, wheelRadius, tuning, true);
  vehicle->addWheel(cp3, wheelDirection, wheelAxle, suspensionRestLength, wheelRadius, tuning, false);
  vehicle->addWheel(cp4, wheelDirection, wheelAxle, suspensionRestLength, wheelRadius, tuning, false);

  for (int i = 0; i < vehicle->getNumWheels(); ++i) {
    btWheelInfo& wheel = vehicle->getWheelInfo(i);
    wheel.m_suspensionStiffness = suspensionStiffness;
    wheel.m_wheelsDampingRelaxation = suspensionDamping;
    wheel.m_wheelsDampingCompression = suspensionCompression;
    wheel.m_frictionSlip = wheelFriction;
    wheel.m_rollInfluence = rollInfluence;
  }

  debugDrawer = new DebugDrawer();
  dynamicsWorld->setDebugDrawer(debugDrawer);
}

BlenderScene::BlenderScene(const char* fileName, btDynamicsWorld* world, Renderer* renderer) {
  if (!QFile::exists(fileName))
    throw load_exception(QString("Scene file ") + fileName + " does not exist!");

  this->world = world;
  this->renderer = renderer;
  importer = NULL;

  QFileInfo info(fileName);
  QString path = info.absolutePath() + QDir::separator();
  QString bulletFile = path + info.baseName() + ".bullet";

  bool physicsDataPresent = false;

  if (QFile::exists(bulletFile)) {
    importer = new btBulletWorldImporter(world);

    if (!importer->loadFile(bulletFile.toStdString().c_str())) {
      std::cout << "Could not load physics data from " << bulletFile.toStdString() << "!" << std::endl;
    }
    else {
      physicsDataPresent = true;
      std::cout << "Num collision shapes: " << importer->getNumCollisionShapes() << std::endl
        << "Num rigid bodies: " << importer->getNumRigidBodies() << std::endl
        << "Num constraints: " << importer->getNumConstraints() << std::endl
        << "Num bvhs: " << importer->getNumBvhs() << std::endl
        << "Num triangle info maps: " << importer->getNumTriangleInfoMaps() << std::endl;
    }
  }
  else
    std::cout << "Could not load physics data from " << bulletFile.toStdString() << "!" << std::endl;

  QDomDocument doc;
  QFile file(fileName);

  if (!file.open(QIODevice::ReadOnly)) {
    QString error = "Error opening file ";
    error += fileName;
    throw load_exception(error.toStdString());
  }

  QString msg;
  if (!doc.setContent(&file, false, &msg)) {
    file.close();
    QString error = "Error loading file ";
    error += fileName;
    error += " [";
    error += msg + "]";
    throw load_exception(error.toStdString());
  }

  file.close();

  QDomNodeList nodes = doc.elementsByTagName("object");

  for (unsigned int i = 0; i < nodes.length(); ++i) {
    QDomElement e = nodes.at(i).toElement();
    RenderableObject object;
    if (e.hasAttribute("name"))
      object.name = e.attribute("name");

    if (e.hasAttribute("texture0"))
      object.texture0 = renderer->addTexture((path+e.attribute("texture0")).toStdString().c_str());
    if (e.hasAttribute("texture1"))
      object.texture1 = renderer->addTexture((path+e.attribute("texture1")).toStdString().c_str());
    if (e.hasAttribute("texture2"))
      object.texture2 = renderer->addTexture((path+e.attribute("texture2")).toStdString().c_str());
    if (e.hasAttribute("texture3"))
      object.texture3 = renderer->addTexture((path+e.attribute("texture3")).toStdString().c_str());

    if (e.hasAttribute("mesh"))
      object.mesh = renderer->addMesh((path+e.attribute("mesh")).toStdString().c_str());

    if (e.hasAttribute("shader"))
      object.shader = renderer->addShader((path+e.attribute("shader")).toStdString().c_str());

    if (physicsDataPresent) {
      object.body = importer->getRigidBodyByName(object.name.toStdString().c_str());
      if (object.body != NULL && object.body->getMotionState() == NULL) {
        std::cout << "Adding " << object.name.toStdString() << "..." << std::endl;
        btVector3 translation = object.body->getCenterOfMassPosition();
        btQuaternion orientation = object.body->getOrientation();
        btTransform transform(orientation, translation);
        btDefaultMotionState* state = new btDefaultMotionState(transform);
        object.body->setMotionState(state);
      }
    }

    if (object.body == NULL) {
      std::cout << "Adding a ghost called " << object.name.toStdString() << "..." << std::endl;
      object.ghost = true;
      QDomElement position = nodes.at(i).firstChildElement("position");
      QDomElement rotation = nodes.at(i).firstChildElement("rotation");
      if (position.isNull() || rotation.isNull() ||
        !position.hasAttribute("x") || !position.hasAttribute("y") || !position.hasAttribute("z") ||
        !rotation.hasAttribute("x") || !rotation.hasAttribute("y") || !rotation.hasAttribute("z") || !rotation.hasAttribute("w")) {
        std::cout << "Incorrectly formatted .scene file (missing attributes/nodes)!" << std::endl;
        continue;
      }

      btVector3 pos(position.attribute("x").toFloat(), position.attribute("y").toFloat(), position.attribute("z").toFloat());
      btQuaternion rot(rotation.attribute("x").toFloat(),
          rotation.attribute("y").toFloat(),
          rotation.attribute("z").toFloat(),
          rotation.attribute("w").toFloat());
      object.transform.setIdentity();
      object.transform.setOrigin(pos);
      object.transform.setRotation(rot);
    }

    objects << object;
  }

  std::cout << "Added " << objects.size() << " objects to the world." << std::endl;
}

BlenderScene::~BlenderScene() {
  delete importer;
}

void BlenderScene::draw(qint64 delta,
    const mat4& proj,
    const mat4& modelView,
    GLuint depthTexture, // TODO: refactor this ASAP
    btRaycastVehicle* vehicle) {

  mat4 lightModelView;
  mat4 lightProj;
  lightProj.ortho(-3, 3, -3, 3, -10, 100);

  btVector3 center = vehicle->getChassisWorldTransform().getOrigin();
  vec3 cen = vec3(center.x(), center.y(), center.z());
  lightModelView.lookAt(
    cen + vec3(10, 10, 10),
    cen,
    vec3(0,1,0));

  const GLfloat bias[16] = {
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 0.5, 0.0,
    0.5, 0.5, 0.5, 1.0};

  for (int i = 0; i < objects.size(); ++i) {
    RenderableObject object = objects.at(i);

    // TODO: sort objects by shader
    // TODO: put objects into some kind of space tree

    if (object.shader == NULL || object.mesh == NULL)
      continue;

    renderer->setShader(object.shader);

    if (object.texture0 != NULL)
      renderer->setTexture("texture0", object.texture0, 0);
    if (object.texture1 != NULL)
      renderer->setTexture("texture1", object.texture1, 1);
    if (object.texture2 != NULL)
      renderer->setTexture("texture2", object.texture2, 2);
    if (object.texture3 != NULL)
      renderer->setTexture("texture3", object.texture3, 3);

    mat4 modelViewTop = modelView;
    mat4 model;

    if (object.body != NULL) {
      btMotionState* state = object.body->getMotionState();
      btTransform transform;
      btScalar matrix[16];
      state->getWorldTransform(transform);
      transform.getOpenGLMatrix(matrix);
      modelViewTop *= toMat4(matrix);
      model *= toMat4(matrix);
    }
    else {
      btScalar matrix[16];
      object.transform.getOpenGLMatrix(matrix);
      modelViewTop *= toMat4(matrix);
      model *= toMat4(matrix);
    }

    // TODO: this overrides second unit, make this more general
    glActiveTexture(GL_TEXTURE0 + 5);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    renderer->setUniform1i("depth", 5);
    renderer->setUniform4fv("bias", bias);
    renderer->setUniformMat4("shadowProj", lightProj);
    renderer->setUniformMat4("shadowModelView", lightModelView);
    renderer->setUniformMat4("model", model);

    vec3 light_dir = vec3(0,1,3).normalized();
    renderer->setUniform3f("light_dir", light_dir);
    renderer->setUniformMat4("proj", proj);
    renderer->setUniformMat4("modelView", modelViewTop);
    renderer->drawMesh(object.mesh);
  }
}

void App::cleanUpPhysics() {
  delete dynamicsWorld;
  // major TODO !!
}

void App::updatePhysics(qint64 delta) {
  if (accelerating)
    engineForce = 2000;
  else
    engineForce = 0;

  float speed = vehicle->getCurrentSpeedKmHour();

  if (breaking) {
    if (speed < 0.1) {
      engineForce = -500;
      breakingForce = 0;
    }
    else
      breakingForce = 300;
  }
  else
    breakingForce = 0;

  vehicle->applyEngineForce(engineForce, 2);
  vehicle->applyEngineForce(engineForce, 3);
  vehicle->setBrake(breakingForce, 2);
  vehicle->setBrake(breakingForce, 3);

  if (steeringLeft)
    vehicleSteering += 0.004 * delta / (speed*0.05+1);

  if (steeringRight)
    vehicleSteering -= 0.004 * delta / (speed*0.05+1);

  // Gravitate steering to 0 based on the distance traveled.
  float distanceTraveled = speed * delta * 0.00001;
  vehicleSteering += (vehicleSteering > 0 ? -1:1) * distanceTraveled;
  vehicleSteering = clamp(vehicleSteering, -maxSteering, maxSteering);

  vehicle->setSteeringValue(vehicleSteering, 0);
  vehicle->setSteeringValue(vehicleSteering, 1);

  dynamicsWorld->stepSimulation(delta/700.f, 30);
}

void App::updateFps() {
  fps = QString("%1").arg(framesDrawn / 1.);
  framesDrawn = 0;
}

void App::paintGL() {
  qint64 delta = timer->restart();
  updatePhysics(delta);

  if (shadows) {
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo);
    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0,0, shadowMapWidth, shadowMapHeight);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    mat4 modelView;
    mat4 proj;
    vec3 center = btToQt(vehicle->getChassisWorldTransform().getOrigin());
    modelView.lookAt(
      center + vec3(10, 10, 10),
      center,
      vec3(0,1,0));
    proj.ortho(-3, 3, -3, 3, -10, 100);

    mat4 modelViewTop = modelView;

    renderer->setShader(plain);

    btScalar matrix[16];
    vehicle->getChassisWorldTransform().getOpenGLMatrix(matrix);
    modelViewTop *= toMat4(matrix);
    modelViewTop.rotate(90, vec3(1,0,0));
    modelViewTop.rotate(180, vec3(0,1,0));
    modelViewTop.scale(0.35, 0.35, 0.35);
    renderer->setUniformMat4("proj", proj);
    renderer->setUniformMat4("modelView", modelViewTop);
    renderer->drawMesh(truck);

    for (int i = 0; i < vehicle->getNumWheels(); ++i) {
      vehicle->updateWheelTransform(i, true);
      vehicle->getWheelInfo(i).m_worldTransform.getOpenGLMatrix(matrix);
      modelViewTop = modelView;
      modelViewTop *= toMat4(matrix);
      modelViewTop.scale(0.4, 0.3, 0.3);
      renderer->setUniformMat4("modelView", modelViewTop);
      renderer->drawMesh(wheel);
    }

    glPopAttrib();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  }

  if (blurEnabled) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0,0, rttWidth, rttHeight);
  }

  glEnable(GL_DEPTH_TEST);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  mat4 modelView;
  mat4 proj;
  proj.perspective(75., float(width()) / height(), 0.5, 700.);

  if (vehicleCam) {
    btVector3 forward = vehicle->getForwardVector();
    camDir = camDir.lerp(forward, delta*0.001);
    btVector3 origin = vehicle->getChassisWorldTransform().getOrigin();
    btVector3 pos = origin - 4*camDir + btVector3(0,0,2.5);
    btVector3 aboveVehicle = origin + btVector3(0,0,0.5);
    modelView.lookAt(
      vec3(pos.x(), pos.y(), pos.z()),
      vec3(aboveVehicle.x(), aboveVehicle.y(), aboveVehicle.z()),
      vec3(0,0,1));
  }
  else
    cam->setTransform(delta / 100., modelView);

  mat4 modelViewTop = modelView;

  renderer->setShader(diffuse);
  btScalar matrix[16];
  vehicle->getChassisWorldTransform().getOpenGLMatrix(matrix);
  modelViewTop *= toMat4(matrix);
  modelViewTop.rotate(90, vec3(1,0,0));
  modelViewTop.rotate(180, vec3(0,1,0));
  modelViewTop.scale(0.35, 0.35, 0.35);
  renderer->setUniformMat4("proj", proj);
  renderer->setUniformMat4("modelView", modelViewTop);
  renderer->drawMesh(truck);

  renderer->setShader(diffuseTextured);
  renderer->setTexture("texture", wheelTexture, 0);

  for (int i = 0; i < vehicle->getNumWheels(); ++i) {
    if (!shadows) {
      vehicle->updateWheelTransform(i, true);
    }
    vehicle->getWheelInfo(i).m_worldTransform.getOpenGLMatrix(matrix);
    modelViewTop = modelView;
    modelViewTop *= toMat4(matrix);
    modelViewTop.scale(0.4, 0.3, 0.3);
    renderer->setUniformMat4("proj", proj);
    renderer->setUniformMat4("modelView", modelViewTop);
    renderer->drawMesh(wheel);
  }

  scene->draw(delta, proj, modelView, shadowDepthTexture, vehicle);

/*
  renderer->setShader(scattering);

  const float PI = M_PI;
	const float kr = 0.0025f;		// Rayleigh scattering constant
	const float kr_4pi = kr*4.0f*PI;
	const float km = 0.0010f;		// Mie scattering constant
	const float km_4pi = km*4.0f*PI;
	const float e_sun = 20.0f;		// Sun brightness constant
	const float g = -0.990f;		// The Mie phase asymmetry factor
	//const float exposure = 2.0f;

	const float inner_radius = 200;
	const float outer_radius = inner_radius*1.025;

  vec3 wave_length = vec3(0.650, 0.570, 0.475);
  wave_length *= wave_length * wave_length * wave_length;
  vec3 inv_wave_length = vec3(1 / wave_length.x(), 1 / wave_length.y(), 1 / wave_length.z());

	const float rayleigh_scale_depth = 0.25f;
	//const float mie_scale_depth = 0.1f;

  vec3 lightDir = vec3(0, 1000, 0).normalized();
  vec3 cam_pos = cam->getPosition();
  //std::cout << cam_pos.x() << " " << cam_pos.y() << " " << cam_pos.z() << std::endl;

  renderer->setUniform("cam_pos", cam_pos);
  renderer->setUniform("light_dir", lightDir);
	renderer->setUniform("inv_wave_length", inv_wave_length);
	renderer->setUniform("cam_height", (cam_pos + vec3(0,inner_radius,0)).length());
	renderer->setUniform("inner_radius", inner_radius);
	renderer->setUniform("inner_radius_2", inner_radius*inner_radius);
	renderer->setUniform("outer_radius", outer_radius);
	renderer->setUniform("outer_radius_2", outer_radius*outer_radius);
	renderer->setUniform("kr_e_sun", kr*e_sun);
	renderer->setUniform("km_e_sun", km*e_sun);
	renderer->setUniform("kr_4pi", kr_4pi);
	renderer->setUniform("km_4pi", km_4pi);
	renderer->setUniform("scale", 1.0f / (outer_radius - inner_radius));
	renderer->setUniform("scale_depth", rayleigh_scale_depth);
	renderer->setUniform("scale_over_scale_depth", (1.0f / (outer_radius - inner_radius)) / rayleigh_scale_depth);
	renderer->setUniform("g", g);
	renderer->setUniform("g_2", g*g);
*/
  //glPushMatrix();

  //glTranslatef(0,-inner_radius, 0);

  /*GLUquadric* pSphere = gluNewQuadric();
	gluSphere(pSphere, outer_radius, 100, 50);
	gluDeleteQuadric(pSphere);*/
  /*glScalef(1,1,1);
  renderer->drawMesh(skyDome);*/
  //glPopMatrix();

  if (blurEnabled) {
    glPopAttrib();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0,0, this->width(), this->height());
    glMatrixMode(GL_PROJECTION); // TODO: this must also go
    glLoadIdentity();
    glOrtho(-1,1,-1,1,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    renderer->setShader(blur);
    renderer->setUniformMat4("previousModelView", previousModelView);
    renderer->setUniformMat4("proj", proj);
    renderer->setUniformMat4("modelViewInverse", modelView.inverted());
    renderer->setUniformMat4("projInverse", proj.inverted());
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);
    renderer->setUniform1i("color", 0);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    renderer->setUniform1i("depth", 1);

    glBegin(GL_QUADS);
    glVertex2f(-1,1);
    glVertex2f(1,1);
    glVertex2f(1,-1);
    glVertex2f(-1,-1);
    glEnd();
  }

  renderer->disableShaders();
  glEnable(GL_DEPTH_TEST);
  //glDepthMask(GL_FALSE);

  if (drawDebugInfo) {
    // TODO: convert this crap to shader based.
    if (stipple) {
      glDisable(GL_DEPTH_TEST);
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(1, 0x00FF);
    }

    DebugDrawer* dd = static_cast<DebugDrawer*>(dynamicsWorld->getDebugDrawer());
    dd->setModelViewProj(modelView, proj);
    if (drawAabb)
      dd->setDebugMode(btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawAabb);
    else
      dd->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
    dynamicsWorld->debugDrawWorld();

    if (stipple) {
      // Second pass for visible edges.
      glEnable(GL_DEPTH_TEST);
      glDisable(GL_LINE_STIPPLE);
      dynamicsWorld->debugDrawWorld();
    }
  }

  previousModelView = modelView;
  
  glActiveTexture(GL_TEXTURE0);

  QFont font("Chicken Butt");
  font.setPointSize(42);
  glColor3f(0,0,0);
  renderText(50,50, "The Fps: " + fps, font);

  framesDrawn++;

  update();
}

void App::resizeGL(int width, int height) {
  /*glViewport(0, 0, width, height); // TODO: replace
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45., float(width)/height, 0.1, 1000.);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();*/
}

void App::keyPressEvent(QKeyEvent* event) {
  switch (event->key()) {
    case Qt::Key_Escape:
      this->parentWidget()->close();
      break;
    case Qt::Key_Space:
      this->releaseKeyboard();
      this->setCursor(Qt::ArrowCursor);
      mouseFree = false;
      break;
    case Qt::Key_G:
      drawDebugInfo = !drawDebugInfo;
      break;
    case Qt::Key_Up:
      accelerating = true;
      break;
    case Qt::Key_Down:
      breaking = true;
      break;
    case Qt::Key_Left:
      steeringLeft = true;
      break;
    case Qt::Key_Right:
      steeringRight = true;
      break;
    case Qt::Key_V:
      vehicleCam = !vehicleCam;
      break;
    case Qt::Key_H:
      stipple = !stipple;
      break;
    case Qt::Key_B:
      blurEnabled = !blurEnabled;
      break;
    case Qt::Key_J:
      drawAabb = !drawAabb;
      break;
    case Qt::Key_L:
      shadows = !shadows;
      break;

    case Qt::Key_Backspace: { // HAHA: jump case label error is just weird even for C++
      btTransform transform;
      transform.setIdentity();
      transform.setOrigin(btVector3(5, 5, 5));
      transform.setRotation(btQuaternion(btVector3(0,0,1), M_PI/2));
      chassis->setCenterOfMassTransform(transform);
      chassis->setLinearVelocity(btVector3(0,0,0));
      chassis->setAngularVelocity(btVector3(0,0,0));
      vehicleSteering = 0;
      break;
      }

    case Qt::Key_R: {
      btTransform transform;
      transform.setIdentity();
      btVector3 origin = vehicle->getChassisWorldTransform().getOrigin();
      btVector3 forward = vehicle->getForwardVector();
      forward.setZ(0);
      forward.normalize();
      float angle = forward.angle(btVector3(0,1,0));
      if (forward.x() > 0)
        angle = -angle;
      origin.setZ(5);
      transform.setOrigin(origin);
      transform.setRotation(btQuaternion(btVector3(0,0,1), angle));
      chassis->setCenterOfMassTransform(transform);
      chassis->setLinearVelocity(btVector3(0,0,0));
      chassis->setAngularVelocity(btVector3(0,0,0));
      vehicleSteering = 0;
      break;
      }
  }

  cam->processKeyPress(event->key());
}

void App::keyReleaseEvent(QKeyEvent* event) {
  switch (event->key()) {
    case Qt::Key_Up:
      accelerating = false;
      break;
    case Qt::Key_Down:
      breaking = false;
      break;
    case Qt::Key_Left:
      steeringLeft = false;
      break;
    case Qt::Key_Right:
      steeringRight = false;
      break;
  }
  cam->processKeyRelease(event->key());
}

void App::mouseMoveEvent(QMouseEvent* event) {
  if (mouseFree) {
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
    mouseFree = true;
    this->grabKeyboard();
    this->setCursor(Qt::BlankCursor);
  }
}

DebugDrawer::DebugDrawer() {}
DebugDrawer::~DebugDrawer() {}

void DebugDrawer::setModelViewProj(mat4& modelView, mat4& proj) {
  glUseProgram(0);

  // TODO: hmmm...
  glMatrixMode(GL_PROJECTION);
  GLfloat mat[16];
  for (int i = 0; i < 16; ++i) mat[i] = proj.data()[i];
  glLoadMatrixf(mat);

  glMatrixMode(GL_MODELVIEW);
  for (int i = 0; i < 16; ++i) mat[i] = modelView.data()[i];
  glLoadMatrixf(mat);
}

void DebugDrawer::drawLine(const btVector3& from,const btVector3& to,const btVector3& fromColor, const btVector3& toColor) {
  // TODO: old crap old crap old crap
  glBegin(GL_LINES);
  glColor3f(fromColor.getX(), fromColor.getY(), fromColor.getZ());
  glVertex3d(from.getX(), from.getY(), from.getZ());
  glColor3f(toColor.getX(), toColor.getY(), toColor.getZ());
  glVertex3d(to.getX(), to.getY(), to.getZ());
  glEnd();
}

void DebugDrawer::drawLine(const btVector3& from,const btVector3& to,const btVector3& color) {
  this->drawLine(from, to, color, color);
}

void DebugDrawer::drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,btScalar distance,int lifeTime,const btVector3& color) {
}

void DebugDrawer::reportErrorWarning(const char* warningString) {
  std::cout << "Bullet warning: " << warningString << std::endl;
}

void DebugDrawer::draw3dText(const btVector3& location,const char* textString) {
}

void DebugDrawer::setDebugMode(int debugMode) {
  this->debugMode = debugMode;
}

int DebugDrawer::getDebugMode() const {
  return debugMode;
}

PointerSlider::PointerSlider(float* var, float min, float max) : QSlider(Qt::Horizontal) {
  this->var = var;
  this->min = min;
  this->max = max;
  this->setMinimum(0);
  this->setMaximum((int)((max-min)*100));

  connect(this, SIGNAL(valueChanged(int)), this, SLOT(updateVar(int)));
}

void PointerSlider::updateVar(int value) {
  *(this->var) = value / 100.f + min;
}

ConfigurationWindow::ConfigurationWindow() : QWidget(0, Qt::Window) {
  mainLayout = new QGridLayout;
  setLayout(mainLayout);
}

ConfigurationWindow::~ConfigurationWindow() {
  delete mainLayout;
}

void ConfigurationWindow::addPointerValue(const char* name, float* var, float min, float max) {
  PointerSlider* slider = new PointerSlider(var, min, max);
  QLabel* label = new QLabel(name);
  mainLayout->addWidget(label, 0,0, Qt::AlignTop|Qt::AlignLeft);
  mainLayout->addWidget(slider, 0,1);
}

int main(int argc, char** args) {
  QApplication qapp(argc, args);
  QGLFormat format;
  format.setSampleBuffers(true);
  ConfigurationWindow win;
  win.resize(600, 200);
  App app(format, &win);
  app.resize(800, 800);
  win.show();
  app.show();
  return qapp.exec();
}
