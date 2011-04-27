#include <App.h>

void App::initializeGL() {
}

void App::paintGL() {
  glClear(GL_COLOR_BUFFER_BIT);
}

void App::resizeGL() {
}

void App::keyPressEvent(QKeyEvent* event) {
}

int main(int argc, char** args) {
  QApplication qapp(argc, args);
  App app;
  app.show();
  return qapp.exec();
}
