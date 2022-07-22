#include "glview.h"

#include <QKeyEvent>
#include <QOpenGLExtraFunctions>
#include <QPainter>
#include <QTime>

#include <QDebug>

GLView::GLView() :
	QOpenGLWidget(), m_lastTime(QTime::currentTime().msec()), m_nbFrames(0)
{
	//initialize label to display fps
	m_fpsLabel = new QLabel("000 fps", this);
	m_fpsLabel->setFont(QFont("Liberation mono", 10));
	m_fpsLabel->setAlignment(Qt::AlignLeft);
	m_fpsLabel->setStyleSheet("QLabel { color : white; margin-left: " + QString::number(m_fpsLabel->width() / 5) +  "; }");

	//connect the timer to the update slot
	connect(&m_timer, SIGNAL(timeout()), this, SLOT(update()));
	m_timer.start(0);
}

GLView::~GLView()
{
	makeCurrent();
	delete m_program;
	delete m_vbo;
}

static const char *vertexShaderSource = R"(
    layout(location = 0) in vec4 vertex;
    layout(location = 1) in vec3 normal;
    out vec3 vert;
    out vec3 vertNormal;
    uniform mat4 projMatrix;
    uniform mat4 camMatrix;
    uniform mat4 worldMatrix;
    uniform mat3 normalWorldMatrix;
    void main() {
       vert = vec3(worldMatrix * vertex);
       vertNormal = normalWorldMatrix * normal;
       gl_Position = projMatrix * camMatrix * worldMatrix * vertex;
    })";

static const char *fragmentShaderSource = R"(
    in highp vec3 vert;
    in highp vec3 vertNormal;
    out highp vec4 fragColor;
    uniform highp vec3 lightPos;
    uniform highp vec3 cameraPos;
    void main() {
       highp vec3 L = normalize(lightPos - vert);
       highp float NL = max(dot(normalize(vertNormal), L), 0.0);
       highp float specular = 0.0;
       if(NL > 0.0) {
           highp vec3 R = reflect(-L, vertNormal);
           highp vec3 V = normalize(cameraPos - vert);
           highp float RV = max(dot(R,V), 0.0);
           specular = pow(RV, 50.0);
       }
       highp vec3 specularColor = specular * vec3(1.0,1.0,1.0);
       highp vec3 color = vec3(0.4, 0.4, 1.0);
       highp vec3 ambientColor = 0.2 * color;
       highp vec3 diffuseColor = 0.8 * NL * color;
       fragColor = vec4(clamp(specularColor+ambientColor+diffuseColor, 0.0, 1.0), 1.0);
    })";

QByteArray versionedShaderCode(const char *src)
{
	//to get the right version depending on the platform used
	QByteArray versionedSrc;

	if (QOpenGLContext::currentContext()->isOpenGLES())
		versionedSrc.append(QByteArrayLiteral("#version 300 es\n"));
	else
		versionedSrc.append(QByteArrayLiteral("#version 330\n"));

	versionedSrc.append(src);
	return versionedSrc;
}

void GLView::initializeGL()
{
	initializeOpenGLFunctions();

	// print information
	QString val = QString::fromLatin1((char *)glGetString(GL_VERSION));
	qDebug() << "OpenGL version : " << val;
	val = QString::fromLatin1((char *)glGetString(GL_SHADING_LANGUAGE_VERSION));
	qDebug() << "GLSL version : " << val;

	m_program = new QOpenGLShaderProgram();

	// Prepend the correct version directive to the sources. The rest is the
	// same, thanks to the common GLSL syntax.
	m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
	                                   versionedShaderCode(vertexShaderSource));
	m_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
	                                   versionedShaderCode(fragmentShaderSource));
	m_program->link();

	//find uniforms
	m_projMatrixLoc = m_program->uniformLocation("projMatrix");
	m_camMatrixLoc = m_program->uniformLocation("camMatrix");
	m_worldMatrixLoc = m_program->uniformLocation("worldMatrix");
	m_normalWorldMatrixLoc = m_program->uniformLocation("normalWorldMatrix");
	m_lightPosLoc = m_program->uniformLocation("lightPos");
	m_cameraPosLoc = m_program->uniformLocation("cameraPos");
    
    //bind the shader
    m_program->bind();
    
	// Create a VAO
    m_vao = new QOpenGLVertexArrayObject(this);
    m_vao->create();
    
    //bind the vao
	m_vao->bind();
    
    //create the vertex buffer
	m_vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
	m_vbo->create();
	m_vbo->bind();
    
    //allocate the memory and write the data
	m_vbo->allocate(m_geometry.constData(), m_geometry.count() * sizeof(GLfloat));
    
    //define how the data are organized in the buffer
    //we have 2 variables in the shader
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
    //the first is 3 floats for the coordinates
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat),
	                      nullptr);
    //the second is 3 floats for the normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat),
	                      reinterpret_cast<void *>(3 * sizeof(GLfloat)));
    //unbind everything
	m_vbo->release();
	m_program->release();
	m_vao->release();
    
    //setup clear color
	glClearColor(0.1, 0.1, 0.1, 1.0);
    
    //enable depth test and cull face
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	qDebug() << "OpenGL initialized";
}

void GLView::resizeGL(int w, int h)
{
    //when the window is resized, reset the projection matrix
	m_proj.setToIdentity();
	m_proj.perspective(45.0f, GLfloat(w) / h, 0.01f, 100.0f);
    //indicate that uniforms have to be updated
	m_uniformsDirty = true;
}

void GLView::paintGL()
{
    //each frame, compute and display fps
	displayFPS();
    
    //clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    //bind the shader
	m_program->bind();
    
    //if uniforms have to be updated
	if (m_uniformsDirty)
	{
		m_uniformsDirty = false;
        
        //create the camera matrix
		QMatrix4x4 camera;
		camera.lookAt(m_eye, m_target, QVector3D(0, 1, 0));
        
        //reset the world matrix
		m_world.setToIdentity();
        
        //send data to GPU
		m_program->setUniformValue(m_projMatrixLoc, m_proj);
		m_program->setUniformValue(m_camMatrixLoc, camera);
		m_program->setUniformValue(m_worldMatrixLoc, m_world);
		m_program->setUniformValue(m_normalWorldMatrixLoc, m_world.normalMatrix());
		m_program->setUniformValue(m_lightPosLoc, m_lightPos);
		m_program->setUniformValue(m_cameraPosLoc, m_eye);
	}
    
    //bind the vao
	m_vao->bind();
    
    //do the draw call
	glDrawArrays(GL_TRIANGLES, 0, m_geometry.vertexCount());
}

void GLView::displayFPS()
{
	// get the number of msec of the current time
	int currentTime = QTime::currentTime().msec();

	// increase the number of frame each frame
	m_nbFrames++;

	// when the last msec is greater than the current msec
	if (m_lastTime > currentTime)
	{
		// it means we changed the second
		m_fpsLabel->setText(QString::number(m_nbFrames) + " fps");

		//reset the number of frames
		m_nbFrames = 0;
	}

	// update the last time
	m_lastTime = currentTime;
}

void GLView::keyPressEvent(QKeyEvent *event)
{
    //close window on escape key pressed
	switch (event->key())
	{
		case Qt::Key_Escape:
			close();
			break;

		default:
			QOpenGLWidget::keyPressEvent(event);
	}
}

void GLView::mouseMoveEvent(QMouseEvent *event)
{
    //sample to debug things
	qDebug() << "moved at : (" << event->position().x() << ", " << event->position().y() << ")";
}

void GLView::mousePressEvent(QMouseEvent *event)
{
    //sample to debug things
	qDebug() << "touched at : (" << event->position().x() << ", " << event->position().y() << ")";
}
