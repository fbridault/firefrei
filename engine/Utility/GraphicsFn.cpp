#include "GraphicsFn.hpp"

#include <iostream>
#include "../Maths/CPoint.hpp"

using namespace std;

static GLUquadricObj *quadObj;

#define QUAD_OBJ_INIT() { if(!quadObj) initQuadObj(); }

static void initQuadObj (void)
{
	quadObj = gluNewQuadric ();
	if (!quadObj)
		cerr << ("out of memory.");
}

void UGraphicsFn::SolidSphere (GLfloat radius, GLint slices, GLint stacks)
{
	QUAD_OBJ_INIT ();
	gluQuadricDrawStyle (quadObj, GLU_FILL);
	gluQuadricNormals (quadObj, GLU_SMOOTH);
	/* If we ever changed/used the texture or orientation state
	   of quadObj, we'd need to change it to the defaults here
	   with gluQuadricTexture and/or gluQuadricOrientation. */
	gluSphere (quadObj, radius, slices, stacks);
}

void UGraphicsFn::SolidCone (	GLfloat base, GLfloat height, GLint slices, GLint stacks)
{
	QUAD_OBJ_INIT ();
	gluQuadricDrawStyle (quadObj, GLU_FILL);
	gluQuadricNormals (quadObj, GLU_SMOOTH);
	/* If we ever changed/used the texture or orientation state
	   of quadObj, we'd need to change it to the defaults here
	   with gluQuadricTexture and/or gluQuadricOrientation. */
	gluCylinder (quadObj, base, 0.0, height, slices, stacks);
}

void UGraphicsFn::SolidCylinder (	GLfloat base, GLfloat height, GLint slices, GLint stacks)
{
	QUAD_OBJ_INIT ();
	gluQuadricDrawStyle (quadObj, GLU_FILL);
	gluQuadricNormals (quadObj, GLU_SMOOTH);
	/* If we ever changed/used the texture or orientation state
	   of quadObj, we'd need to change it to the defaults here
	   with gluQuadricTexture and/or gluQuadricOrientation. */
	gluCylinder (quadObj, base, base, height, slices, stacks);
}

void UGraphicsFn::SolidDisk (GLfloat rayon, GLint slices, GLint loops)
{
	QUAD_OBJ_INIT ();
	gluQuadricDrawStyle (quadObj, GLU_FILL);
	gluQuadricNormals (quadObj, GLU_SMOOTH);
	/* If we ever changed/used the texture or orientation state
	   of quadObj, we'd need to change it to the defaults here
	   with gluQuadricTexture and/or gluQuadricOrientation. */
	gluDisk (quadObj, 0, rayon, slices, loops);
}

void UGraphicsFn::SolidBox (const CPoint& ptMin, const CPoint& ptMax)
{
	glColor4f(1.0,1.0,0.0,0.0);
	glBegin(GL_QUADS);
	glVertex3f(ptMin.x,ptMin.y,ptMin.z);
	glVertex3f(ptMin.x,ptMax.y,ptMin.z);
	glVertex3f(ptMax.x,ptMax.y,ptMin.z);
	glVertex3f(ptMax.x,ptMin.y,ptMin.z);

	glVertex3f(ptMin.x,ptMin.y,ptMax.z);
	glVertex3f(ptMax.x,ptMin.y,ptMax.z);
	glVertex3f(ptMax.x,ptMax.y,ptMax.z);
	glVertex3f(ptMin.x,ptMax.y,ptMax.z);

	glVertex3f(ptMin.x,ptMin.y,ptMin.z);
	glVertex3f(ptMin.x,ptMin.y,ptMax.z);
	glVertex3f(ptMin.x,ptMax.y,ptMax.z);
	glVertex3f(ptMin.x,ptMax.y,ptMin.z);

	glVertex3f(ptMax.x,ptMin.y,ptMin.z);
	glVertex3f(ptMax.x,ptMax.y,ptMin.z);
	glVertex3f(ptMax.x,ptMax.y,ptMax.z);
	glVertex3f(ptMax.x,ptMin.y,ptMax.z);

	glVertex3f(ptMin.x,ptMin.y,ptMin.z);
	glVertex3f(ptMax.x,ptMin.y,ptMin.z);
	glVertex3f(ptMax.x,ptMin.y,ptMax.z);
	glVertex3f(ptMin.x,ptMin.y,ptMax.z);

	glVertex3f(ptMin.x,ptMax.y,ptMin.z);
	glVertex3f(ptMin.x,ptMax.y,ptMax.z);
	glVertex3f(ptMax.x,ptMax.y,ptMax.z);
	glVertex3f(ptMax.x,ptMax.y,ptMin.z);
	glEnd();
}

//void UGraphicsFn::renderBitmapString(float x, float y, float height, void *font, const char *string)
//{
//  const char *c;
//  glRasterPos2f(x, height-y);
//  for (c=string; *c != '\0'; c++)
//    glutBitmapCharacter(font, *c);
//}
