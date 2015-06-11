
#include "main_window.h"
#include "osg_viewer_widget.h"
#include "sphere_ball.h"

SphereBall::SphereBall(void)
	:center_(new osg::MatrixTransform),
	radius_(20),
	initilized_(false),
	visualization_(new osg::MatrixTransform),
	axis_dragger_(new osgManipulator::TranslateAxisDragger),
	trackball_dragger_(new osgManipulator::TrackballDragger)
{
	hidden_ = true;

	double point_radius = 2;
	osg::ref_ptr<osg::Sphere> sphere(new osg::Sphere(osg::Vec3(0, 0, 0), point_radius));

	osg::ref_ptr<osg::ShapeDrawable> center_drawable(new osg::ShapeDrawable(sphere.get()));
	center_drawable->setColor(osg::Vec4(0.0, 0.0, 0.5, 1.0));
	osg::ref_ptr<osg::Geode> center_geode = new osg::Geode;
	center_geode->addDrawable(center_drawable);
	center_->addChild(center_geode);

	axis_dragger_->setupDefaultGeometry();
	axis_dragger_->setHandleEvents(true);
	axis_dragger_->setActivationModKeyMask(osgGA::GUIEventAdapter::MODKEY_CTRL);
	axis_dragger_->setActivationKeyEvent('d');

	trackball_dragger_->setupDefaultGeometry();
	trackball_dragger_->setHandleEvents(true);
	trackball_dragger_->setActivationModKeyMask(osgGA::GUIEventAdapter::MODKEY_CTRL);
	trackball_dragger_->setActivationKeyEvent('d');

	axis_dragger_->addTransformUpdating(center_);
	axis_dragger_->addTransformUpdating(visualization_);
	trackball_dragger_->addTransformUpdating(center_);
	trackball_dragger_->addTransformUpdating(visualization_);

	axis_dragger_->addTransformUpdating(trackball_dragger_);
	trackball_dragger_->addTransformUpdating(axis_dragger_);

	center_->setMatrix(osg::Matrix::translate(osg::Vec3(0, 0, 0)));
}

SphereBall::~SphereBall(void)
{
}

osg::Vec3 SphereBall::getCenter() const
{
	return center_->getMatrix().getTrans();
}

double SphereBall::getRadius() const
{
	return radius_;
}

void SphereBall::init(void)
{
	if (initilized_)
		return;

	osg::BoundingSphere boundingSphere = MainWindow::getInstance()->getOSGViewerWidget()->getBound();
	double radius = boundingSphere.radius();


	center_->setMatrix(osg::Matrix::translate(boundingSphere.center()));

	initilized_ = true;

	return;
}

void SphereBall::visualizeSphere()
{
	osg::BoundingSphere boundingSphere = MainWindow::getInstance()->getOSGViewerWidget()->getBound();
	double radius = boundingSphere.radius();

	osg::ref_ptr<osg::Sphere> sphere(new osg::Sphere(osg::Vec3(0, 0, 0), radius_));

	osg::ref_ptr<osg::ShapeDrawable> center_drawable(new osg::ShapeDrawable(sphere.get()));
	center_drawable->setColor(osg::Vec4(0.0, 0.0, 0.8, 1.0));
	osg::ref_ptr<osg::Geode> center_geode = new osg::Geode;
	center_geode->addDrawable(center_drawable);
	center_->addChild(center_geode);

	osg::Vec3 center = center_->getMatrix().getTrans();
	float t_scale = radius/4;
	float r_scale = radius/6;
	osg::Matrix flip(osg::Matrix::rotate(osg::Vec3(0, 1, 0), osg::Vec3(0, -1, 0)));        
	axis_dragger_->setMatrix(flip*osg::Matrix::scale(t_scale, t_scale, t_scale)*osg::Matrix::translate(center));
	trackball_dragger_->setMatrix(osg::Matrix::scale(r_scale, r_scale, r_scale)*osg::Matrix::translate(center));

	addChild(center_);
	addChild(visualization_);
	addChild(trackball_dragger_);
	addChild(axis_dragger_);

	return;
}

void SphereBall::updateImpl()
{
	visualizeSphere();
	
	return;
}
