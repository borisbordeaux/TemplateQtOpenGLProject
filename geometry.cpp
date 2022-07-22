#include "geometry.h"

#include <QDebug>

Geometry::Geometry():
	m_count(0)
{
	//resize the data
	m_data.resize(6 * 6); //3 add * 6 float per add

	//define each point coordinates
	const GLfloat x1 = -1.0f;
	const GLfloat y1 =  0.0f;
	const GLfloat x2 =  1.0f;
	const GLfloat y2 =  0.0f;
	const GLfloat x3 =  0.0f;
	const GLfloat y3 =  1.0f;
	const GLfloat x4 =  0.0f;
	const GLfloat y4 = -1.0f;

	//add 2 triangles
	triangle(x1, y1, x2, y2, x3, y3);
	triangle(x1, y1, x4, y4, x2, y2);
}

void Geometry::add(const QVector3D &v, const QVector3D &n)
{
	//append the given coordinate and normal to the data
	GLfloat *p = m_data.data() + m_count;
    //write coordinates first, then normal
	*p++ = v.x();
	*p++ = v.y();
	*p++ = v.z();
	*p++ = n.x();
	*p++ = n.y();
	*p++ = n.z();

	//update the count value
	m_count += 6;
}

void Geometry::triangle(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat x3, GLfloat y3)
{
    //compute the normal of the triangle
	QVector3D n = QVector3D::normal(QVector3D(x2 - x1, y2 - y1, 0.0f), QVector3D(x3 - x1, y3 - y1, 0.0f));
    //add each point with their normal
	add(QVector3D(x1, y1, 0.0f), n);
	add(QVector3D(x2, y2, 0.0f), n);
	add(QVector3D(x3, y3, 0.0f), n);
}
