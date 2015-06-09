#ifndef SPHERE_BALL_H
#define SPHERE_BALL_H

#include <osg/Geode>
#include <osg/Shape>
#include <osg/Geometry>
#include <osg/ShapeDrawable>
#include <osgManipulator/TrackballDragger>
#include <osgManipulator/TranslateAxisDragger>

#include "types.h"
#include "renderable.h"

namespace osgManipulator
{
	class TranslateAxisDragger;
	class TrackballDragger;
}

class SphereBall: public Renderable
{
public:
	SphereBall(void);
	virtual ~SphereBall(void);

	virtual const char* className() const {return "SphereBall";}

	void init(void);

	osg::Vec3 getCenter() const;
	double getRadius() const;

protected:
	virtual void updateImpl();
	void visualizeSphere(void);

private:

	osg::ref_ptr<osg::MatrixTransform>                  center_;
	double												radius_;
	osg::ref_ptr<osg::MatrixTransform>					visualization_;
	osg::ref_ptr<osgManipulator::TranslateAxisDragger>  axis_dragger_;
	osg::ref_ptr<osgManipulator::TrackballDragger>      trackball_dragger_;

	bool              initilized_;
};


#endif