#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <qopengl.h>
#include <QVector3D>
#include <QList>

//batch of triangles that have to be drawn
//geometry is defined in the constructor

class Geometry
{
	public:
		Geometry();
		const GLfloat *constData() const { return m_data.constData(); }
		int count() const { return m_count; }
		int vertexCount() const { return m_count / 6; }

	private:
		void add(const QVector3D &v, const QVector3D &n);
		void triangle(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat x3, GLfloat y3);
		int m_count;

		QList<GLfloat> m_data;
};

#endif // GEOMETRY_H
