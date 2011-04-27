#include <QApplication>
#include <QKeyEvent>
#include <QWidget>
#include <QtOpenGL>

class App : public QGLWidget {
  Q_OBJECT

public:
  App(QWidget* parent = 0) {}
  ~App() {}

private:
  void initializeGL();
  void paintGL();
  void resizeGL();
  void keyPressEvent(QKeyEvent* event);
};
