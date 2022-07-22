#ifndef GLVIEW_H
#define GLVIEW_H

#include "geometry.h"

#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLExtraFunctions>
#include <QTimer>
#include <QVector2D>
#include <QVector3D>

#include <QtWidgets/QLabel>

class GLView : public QOpenGLWidget, public QOpenGLExtraFunctions
{
		Q_OBJECT
	public:
		GLView();
		~GLView();

		//must override these 3 methods
		void initializeGL() override;
		void paintGL() override;
		void resizeGL(int w, int h) override;

	private: //opengl stuff
		bool m_uniformsDirty = true;

		int m_cameraPosLoc = 0;
		int m_camMatrixLoc = 0;
		int m_lightPosLoc = 0;
		int m_normalWorldMatrixLoc = 0;
		int m_projMatrixLoc = 0;
		int m_worldMatrixLoc = 0;

		QMatrix4x4 m_proj;
		QMatrix4x4 m_world;

		QOpenGLBuffer *m_vbo = nullptr;
		QOpenGLShaderProgram *m_program = nullptr;
		QOpenGLVertexArrayObject *m_vao = nullptr;

		QVector3D m_eye = {0.0, 0.0, 3.0};
		QVector3D m_lightPos = {0.0, 0.0, 100.0};
		QVector3D m_target = {0.0, 0.0, 0.0};

		Geometry m_geometry;

	private: //fps counter stuff
		void displayFPS();

		int m_lastTime;
		int m_nbFrames;
		QLabel *m_fpsLabel;
		QTimer m_timer;

	protected: //QWidget callbacks
		void keyPressEvent(QKeyEvent *event) override;
		void mouseMoveEvent(QMouseEvent *event) override;
		void mousePressEvent(QMouseEvent *event) override;
};

#endif // GLVIEW_H
