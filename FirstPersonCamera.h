#ifndef FIRST_PERSON_CAMERA_H
#define FIRST_PERSON_CAMERA_H

class FirstPersonCamera {
public:
  FirstPersonCamera(const vec3& position = vec3(0,0,10)) {
    this->position = position;
    this->direction = position+vec3(0,0,1);
    movingForward = false;
    movingBackward = false;
    movingLeft = false;
    movingRight = false;
    yaw = pitch = 0.;
  }

  void processKeyPress(int key) {
    switch (key) {
      case Qt::Key_W:
        movingForward = true; break;
      case Qt::Key_A:
        movingLeft = true; break;
      case Qt::Key_S:
        movingBackward = true; break;
      case Qt::Key_D:
        movingRight = true; break;
    }
  }

  void processKeyRelease(int key) {
    switch (key) {
      case Qt::Key_W:
        movingForward = false; break;
      case Qt::Key_A:
        movingLeft = false; break;
      case Qt::Key_S:
        movingBackward = false; break;
      case Qt::Key_D:
        movingRight = false; break;
    }
  }

  void processMouseMotion(int dx, int dy) {
    yaw -= dx/100.0;
    pitch -= dy/100.0;
    if (pitch < -M_PI/2)
      pitch = -M_PI/2;
    if (pitch > M_PI/2)
      pitch = M_PI/2;
    direction = vec3(cos(yaw), sin(pitch), sin(yaw));
    direction.normalize();
  }

  void setTransform(float factor, mat4& modelView) {
    if (movingForward)
      position += direction * factor;
    if (movingBackward)
      position -= direction * factor;
    if (movingLeft)
      position += QVector3D::crossProduct(direction, vec3(0,1,0)).normalized() * factor;
    if (movingRight)
      position -= QVector3D::crossProduct(direction, vec3(0,1,0)).normalized() * factor;

    vec3 pos = vec3(position.x(), position.z(), position.y());
    vec3 dir = vec3(direction.x(), direction.z(), direction.y());
    modelView.lookAt(pos, pos+dir, vec3(0,0,1));
  }

  vec3 getPosition() {
    return position;
  }

  vec3 getDirection() {
    return direction;
  }

private:
  vec3 position, direction;
  float yaw, pitch;
  bool movingForward, movingBackward, movingLeft, movingRight;

  // TODO
  float speed;
  float mouseSensitivity;
};

#endif
