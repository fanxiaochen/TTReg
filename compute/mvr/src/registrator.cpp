#include <cstdio>

#include <QFileDialog>
#include <QMessageBox>
#include <QFutureWatcher>
#include <QtConcurrentRun>

#include <osg/Geode>
#include <osg/Shape>
#include <osg/Geometry>
#include <osg/LineSegment>
#include <osg/ShapeDrawable>
#include <osgManipulator/TrackballDragger>
#include <osgManipulator/TranslateAxisDragger>

#include <boost/shared_ptr.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/bind.hpp>

#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/registration/icp.h>
#include <pcl/registration/lum.h>
#include <pcl/registration/correspondence_estimation.h>


#include "main_window.h"

#include "point_cloud.h"
#include "math_solvers.h"
#include "parameter_manager.h"
#include "osg_viewer_widget.h"
#include "file_system_model.h"
#include "osg_utility.h"
#include "color_map.h"
#include "registrator.h"

Registrator::Registrator(void)
  :pivot_point_(new osg::MatrixTransform),
  normal_point_(new osg::MatrixTransform),
  visualization_(new osg::MatrixTransform),
  pivot_dragger_(new osgManipulator::TranslateAxisDragger),
  normal_dragger_(new osgManipulator::TrackballDragger),
  initilized_(false),
  show_axis_(false),
  show_error_(false),
  error_vertices_(new osg::Vec3Array),
  error_colors_(new osg::Vec4Array),
  source_(new PCLPointCloud),
  target_(new PCLPointCloud)
{
  double point_radius = 2;
  osg::ref_ptr<osg::Sphere> sphere(new osg::Sphere(osg::Vec3(0, 0, 0), point_radius));

  osg::ref_ptr<osg::ShapeDrawable> pivot_point_drawable(new osg::ShapeDrawable(sphere.get()));
  pivot_point_drawable->setColor(osg::Vec4(0.0, 0.0, 0.5, 1.0));
  osg::ref_ptr<osg::Geode> pivot_point_geode = new osg::Geode;
  pivot_point_geode->addDrawable(pivot_point_drawable);
  pivot_point_->addChild(pivot_point_geode);

  osg::ref_ptr<osg::ShapeDrawable> normal_point_drawable(new osg::ShapeDrawable(sphere.get()));
  normal_point_drawable->setColor(osg::Vec4(0.5, 0.0, 0.0, 1.0));
  osg::ref_ptr<osg::Geode> normal_point_geode = new osg::Geode;
  normal_point_geode->addDrawable(normal_point_drawable);
  normal_point_->addChild(normal_point_geode);

  pivot_dragger_->setupDefaultGeometry();
  pivot_dragger_->setHandleEvents(true);
  pivot_dragger_->setActivationModKeyMask(osgGA::GUIEventAdapter::MODKEY_CTRL);
  pivot_dragger_->setActivationKeyEvent('d');

  normal_dragger_->setupDefaultGeometry();
  normal_dragger_->setHandleEvents(true);
  normal_dragger_->setActivationModKeyMask(osgGA::GUIEventAdapter::MODKEY_CTRL);
  normal_dragger_->setActivationKeyEvent('d');

  pivot_dragger_->addTransformUpdating(pivot_point_);
  pivot_dragger_->addTransformUpdating(normal_point_);
  pivot_dragger_->addTransformUpdating(visualization_);
  normal_dragger_->addTransformUpdating(pivot_point_);
  normal_dragger_->addTransformUpdating(normal_point_);
  normal_dragger_->addTransformUpdating(visualization_);

  pivot_dragger_->addTransformUpdating(normal_dragger_);
  normal_dragger_->addTransformUpdating(pivot_dragger_);

  pivot_point_->setMatrix(osg::Matrix::translate(osg::Vec3(0, 0, 0)));
  normal_point_->setMatrix(osg::Matrix::translate(osg::Vec3(0, -1, 0)));
}

Registrator::~Registrator(void)
{
}

osg::Vec3 Registrator::getPivotPoint() const
{
  return pivot_point_->getMatrix().getTrans();
}

osg::Vec3 Registrator::getAxisNormal() const
{
  osg::Vec3 pivot_point = pivot_point_->getMatrix().getTrans();
  osg::Vec3 normal_point = normal_point_->getMatrix().getTrans();
  osg::Vec3 axis_normal = normal_point-pivot_point;
  axis_normal.normalize();

  return axis_normal;
}

void Registrator::setPivotPoint(const osg::Vec3& pivot_point)
{
  osg::Vec3 old_pivot_point = pivot_point_->getMatrix().getTrans();
  osg::Vec3 old_normal_point = normal_point_->getMatrix().getTrans();
  osg::Vec3 axis_normal = old_normal_point-old_pivot_point;

  osg::Vec3 normal_point = pivot_point + axis_normal;

  pivot_point_->setMatrix(osg::Matrix::translate(pivot_point));
  normal_point_->setMatrix(osg::Matrix::translate(normal_point));

  expire();
}

void Registrator::setAxisNormal(const osg::Vec3& axis_normal)
{
  osg::Vec3 pivot_point = pivot_point_->getMatrix().getTrans();
  osg::Vec3 old_normal_point = normal_point_->getMatrix().getTrans();
  osg::Vec3 old_normal = old_normal_point-pivot_point;
  double length = old_normal.length();
  if (old_normal*axis_normal < 0)
    length = -length;

  osg::Vec3 normal = axis_normal;
  normal.normalize();

  osg::Vec3 normal_point = pivot_point + normal*length;
  normal_point_->setMatrix(osg::Matrix::translate(normal_point));

  expire();
}

void Registrator::reset(void)
{
  initilized_ = false;
  clear();

  return;
}


void Registrator::init(void)
{
  if (initilized_)
    return;

  osg::BoundingSphere boundingSphere = MainWindow::getInstance()->getOSGViewerWidget()->getBound();
  double radius = boundingSphere.radius();

  
  pivot_point_->setMatrix(osg::Matrix::translate(boundingSphere.center()));
  normal_point_->setMatrix(osg::Matrix::translate(boundingSphere.center()+osg::Vec3(0, -radius, 0)));


  initilized_ = true;

  return;
}

void Registrator::clear()
{
  Renderable::clear();
  visualization_->setMatrix(osg::Matrix::identity());
  visualization_->removeChildren(0, visualization_->getNumChildren());
}

void Registrator::visualizeError(void)
{
  if (error_vertices_->empty())
    return;

  size_t partition_size = 10000;
  size_t item_num = error_vertices_->size()/2;
  size_t partition_num = (item_num+partition_size-1)/partition_size;
  osg::Geode* geode = new osg::Geode;
  for (size_t i = 0, i_end = partition_num; i < i_end; ++ i) {
    osg::UIntArray* vertices_indices = OSGUtility::generateIndices(partition_size, i, item_num, 2);
    osg::UIntArray* color_indices = OSGUtility::generateIndices(partition_size, i, item_num);

    osg::Geometry* geometry = new osg::Geometry;
    geometry->setUseDisplayList(true);
    geometry->setUseVertexBufferObjects(true);
    geometry->setVertexData(osg::Geometry::ArrayData(error_vertices_, vertices_indices, osg::Geometry::BIND_PER_PRIMITIVE));
    geometry->setColorData(osg::Geometry::ArrayData(error_colors_, color_indices, osg::Geometry::BIND_PER_PRIMITIVE));
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices_indices->size()));

    geode->addDrawable(geometry);
  }

  addChild(geode);
  return;
}

void Registrator::visualizeAxis()
{
  osg::BoundingSphere boundingSphere = MainWindow::getInstance()->getOSGViewerWidget()->getBound();
  double radius = boundingSphere.radius();
  double diameter = radius*2;
  double cone_length = 10;
  double cylinder_thickness = 1;
  double cone_thickness = 2;

  osg::Vec3 pivot_point = pivot_point_->getMatrix().getTrans();
  osg::Vec3 normal_point = normal_point_->getMatrix().getTrans();
  osg::Vec3 normal = normal_point-pivot_point;
  normal.normalize();
                                                
  osg::ref_ptr<osg::LineSegment> cone(new osg::LineSegment(normal_point-normal*cone_length, normal_point));
  visualization_->addChild(OSGUtility::drawCone(*cone, cone_thickness, osg::Vec4(0.6, 0.6, 0.6, 1.0)));

  double disc_radius = std::sqrt(radius*radius-(pivot_point-boundingSphere.center()).length2())/3;
  osg::ref_ptr<osg::LineSegment> disc(new osg::LineSegment(pivot_point-normal*0.01, pivot_point+normal*0.01));
  visualization_->addChild(OSGUtility::drawCylinder(*disc, disc_radius, osg::Vec4(0.79, 1.0, 0.44, 0.8)));
  

  osg::Vec3 axis_y = pivot_dragger_->getMatrix()*osg::Vec3(0,1,0);
  osg::Matrix rotation(osg::Matrix::rotate(axis_y,-normal));

  float t_scale = radius/4;
  float r_scale = radius/6;
  osg::Matrix flip(osg::Matrix::rotate(osg::Vec3(0, 1, 0), osg::Vec3(0, -1, 0)));        
  pivot_dragger_->setMatrix(flip*osg::Matrix::scale(t_scale, t_scale, t_scale)*rotation*osg::Matrix::translate(pivot_point));
  normal_dragger_->setMatrix(osg::Matrix::scale(r_scale, r_scale, r_scale)*rotation*osg::Matrix::translate(pivot_point));

  osg::ref_ptr<osg::LineSegment> cylinder_0(new osg::LineSegment(normal_point-normal*diameter, pivot_point));
  visualization_->addChild(OSGUtility::drawCylinder(*cylinder_0, cylinder_thickness, osg::Vec4(0.6, 0.6, 0.6, 1.0)));
  osg::ref_ptr<osg::LineSegment> cylinder_1(new osg::LineSegment(pivot_point+normal*t_scale, normal_point-normal*cone_length));
  visualization_->addChild(OSGUtility::drawCylinder(*cylinder_1, cylinder_thickness, osg::Vec4(0.6, 0.6, 0.6, 1.0)));

  addChild(pivot_point_);
  addChild(normal_point_);
  addChild(visualization_);
  addChild(pivot_dragger_);
  addChild(normal_dragger_);

  return;
}

void Registrator::updateImpl()
{
  if (show_error_)
    visualizeError();

  if (show_axis_)
    visualizeAxis();

  return;
}

void Registrator::load(const QString& filename)
{
  FILE *file = fopen(filename.toStdString().c_str(),"r");
  if (file == NULL)
    return;

  double x, y, z;
  double nx, ny, nz;
  fscanf(file, "%lf %lf %lf", &x, &y, &z);
  fscanf(file, "%lf %lf %lf", &nx, &ny, &nz);
  fclose(file);

  setPivotPoint(osg::Vec3(x, y, z));
  setAxisNormal(osg::Vec3(nx, ny, nz));

  return;
}

void Registrator::load(void)
{
	const QString& workspace = MainWindow::getInstance()->getWorkspace();
	return load(workspace+"/axis.txt");
}

void Registrator::save(const QString& filename)
{
  FILE *file = fopen(filename.toStdString().c_str(),"w");
  if (file == NULL)
    return;

  osg::Vec3 pivot_point = getPivotPoint();
  osg::Vec3 axis_normal = getAxisNormal();

  fprintf(file, "%f %f %f\n", pivot_point.x(), pivot_point.y(), pivot_point.z());
  fprintf(file, "%f %f %f\n", axis_normal.x(), axis_normal.y(), axis_normal.z());
  fclose(file);

  return;
}

void Registrator::save()
{
	MainWindow* main_window = MainWindow::getInstance();
	QString filename = QFileDialog::getSaveFileName(main_window,
		"Save Registrator", main_window->getWorkspace(), "Registrator (*.txt)");
	if (filename.isEmpty())
		return;

	save(filename);

	return;
}


osg::Matrix Registrator::getRotationMatrix(double angle) const
{
  osg::Vec3 pivot_point = getPivotPoint();
  osg::Vec3 axis_normal = getAxisNormal();

  osg::Matrix matrix = osg::Matrix::identity();
  matrix = matrix*osg::Matrix::translate(-pivot_point);
  matrix = matrix*osg::Matrix::rotate(angle, axis_normal);
  matrix = matrix*osg::Matrix::translate(pivot_point);

  return matrix;
}

void Registrator::saveRegisteredPoints(int frame)
{
  QMutexLocker locker(&mutex_);

  FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();
  int view_number = model->getViewNumber();
  std::string folder = model->getPointsFolder(frame);
  if (folder.empty())
    return;

  PointCloud registered_points;
  for (size_t i = 0; i < view_number; ++ i)
  {
    osg::ref_ptr<PointCloud> point_cloud = model->getPointCloud(frame, i);
	point_cloud->extractByPlane();

	// if I put the denoise code here, the program couldn't run in parallel mode...
	// Delaunay Triangulation Denoise Method
	// point_cloud->denoise(segment_threshold, ParameterManager::getInstance().getTriangleLength());

	// Point Density Denoise Method
	// point_cloud->denoise(segment_threshold);

    if (!point_cloud->isRegistered())
      continue;

    const osg::Matrix& matrix = point_cloud->getMatrix();
    for (size_t j = 0, j_end = point_cloud->size(); j < j_end; ++ j)
    {
      PCLRichPoint registered_point = point_cloud->at(j);

	  if (point_cloud->isNoise(j))
	  continue;

      osg::Vec3 point(registered_point.x, registered_point.y, registered_point.z);
      point = matrix.preMult(point);
      registered_point.x = point.x();
      registered_point.y = point.y();
      registered_point.z = point.z();

      osg::Vec3 normal(registered_point.normal_x, registered_point.normal_y, registered_point.normal_z);
      normal = matrix.preMult(normal);
      registered_point.normal_x = normal.x();
      registered_point.normal_y = normal.y();
      registered_point.normal_z = normal.z();


      registered_points.push_back(registered_point);
    }
  }

  std::string filename = folder+"/points.pcd";
  registered_points.save(filename);

  model->updatePointCloud(frame);

  return;
}

void Registrator::refineAxis(int frame)
{
  QMutexLocker locker(&mutex_);

  FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();
  int view_number = model->getViewNumber();
  std::vector<osg::Matrix> matrices;
  std::vector<double>      angles;
  for (size_t i = 1; i < view_number; ++ i)
  {
    osg::ref_ptr<PointCloud> point_cloud = model->getPointCloud(frame, i);
    if(!point_cloud->isRegistered())
      continue;
    matrices.push_back(point_cloud->getMatrix());
    double angle = -i * 2 * M_PI / view_number;
    angles.push_back(angle);
  }

  if (matrices.empty())
    return;

  // solve the normal of the axis: http://en.wikipedia.org/wiki/Rotation_matrix#Determining_the_axis
  // we can directly solve the null space of A for Ax=0, but gesvd of boost binding of lapack doesn't work
  // so we add a regularization that u+v+w=1, and solve it as least square of Ax=b
  namespace ublas = boost::numeric::ublas;
  ublas::matrix<double> A(3*matrices.size()+1, 3);
  ublas::vector<double> x = ublas::zero_vector<double>(3);
  ublas::vector<double> b = ublas::zero_vector<double>(3*matrices.size()+1);
  for (size_t i = 0, i_end = matrices.size(); i < i_end; ++ i)
    for (size_t j = 0; j < 3; ++ j)
      for (size_t k = 0; k < 3; ++ k)
        A(i*3+j, k) = matrices[i](k, j)-((j==k)?(1.0):(0.0));
  size_t idx = 3*matrices.size();
  A(idx, 0) = 1; A(idx, 1) = 1; A(idx, 2) = 1; b(idx) = 1;
  osg::Vec3 normal = getAxisNormal();
  math_solvers::least_squares(A, b, x);
  normal = osg::Vec3(x(0), x(1), x(2));
  normal.normalize();
  setAxisNormal(normal);

 
  for (size_t i = 0, i_end = matrices.size(); i < i_end; ++ i)
    for (size_t j = 0; j < 3; ++ j)
      b(i*3+j) = -matrices[i](3, j);
  osg::Vec3 pivot_point = getPivotPoint();
  A(idx, 0)=0; A(idx, 1)=1; A(idx, 2)=0; b(idx) = pivot_point.y();

  math_solvers::least_squares(A, b, x);
  pivot_point = osg::Vec3(x(0), x(1), x(2));
  setPivotPoint(pivot_point);

  save((model->getPointsFolder(frame)+"/axis.txt").c_str());

  return;
}

void Registrator::refineAxis(void)
{
  int frame;
  if (!ParameterManager::getInstance().getFrameParameter(frame))
    return;

  refineAxis(frame);
}

void Registrator::computeError(int frame)
{
  error_vertices_->clear();
  error_colors_->clear();

  FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();
  int view_number = model->getViewNumber();
  std::vector<bool> shown_flag(view_number, false);
  shown_flag[0] = true;
  for (size_t i = 1; i < view_number; ++ i)
  {
    osg::ref_ptr<PointCloud> point_cloud = model->getPointCloud(frame, i);
    shown_flag[i] = point_cloud->isShown();
    if (shown_flag[i])
      point_cloud->initRotation();
  }

  const int last_view = view_number - 1;
  std::vector<std::pair<size_t, size_t> > neighbor_pairs;
  for (size_t i = 0; i < view_number-1; ++ i)
    if (shown_flag[i] && shown_flag[i+1])
      neighbor_pairs.push_back(std::make_pair(i, i+1));
  if (shown_flag[0] && shown_flag[last_view])
    neighbor_pairs.push_back(std::make_pair(0, last_view));

  PCLPointCloud::Ptr source(new PCLPointCloud);
  PCLPointCloud::Ptr target(new PCLPointCloud);
  for (size_t i = 0, i_end = neighbor_pairs.size(); i < i_end; ++ i)
  {
    model->getPointCloud(frame, neighbor_pairs[i].first)->getTransformedPoints(*source);
    model->getPointCloud(frame, neighbor_pairs[i].second)->getTransformedPoints(*target);

    pcl::registration::CorrespondenceEstimation<PCLPoint, PCLPoint, float> correspondence_estimation;
    correspondence_estimation.setInputSource(source);
    correspondence_estimation.setInputTarget(target);

    double distance_threshold = ParameterManager::getInstance().getRegistrationMaxDistance();
    pcl::CorrespondencesPtr correspondences (new pcl::Correspondences);
    correspondence_estimation.determineReciprocalCorrespondences(*correspondences, distance_threshold);

    for (size_t i = 0, i_end = correspondences->size(); i < i_end; ++ i)
    {
      const pcl::Correspondence& correspondence = correspondences->at(i);
	  const PCLPoint& source_point = source->at(correspondence.index_query);
	  const PCLPoint& target_point = target->at(correspondence.index_match);
	  osg::Vec3 source_error(source_point.x, source_point.y, source_point.z);
	  osg::Vec3 target_error(target_point.x, target_point.y, target_point.z);
	  error_vertices_->push_back(source_error);
	  error_vertices_->push_back(target_error);
     /* error_vertices_->push_back(source->at(correspondence.index_query).cast<osg::Vec3>());
      error_vertices_->push_back(target->at(correspondence.index_match).cast<osg::Vec3>());*/
      error_colors_->push_back(ColorMap::Instance().getColor(ColorMap::JET, correspondence.distance, 0, distance_threshold));
    }
    }

  return;
}

void Registrator::registrationICP(int max_iterations, double max_distance, int frame, int repeat_times)
{

  for(size_t i = 0; i < repeat_times; i++)
  {
    registrationICP(max_iterations, max_distance, frame);
  }
}

void Registrator::registrationICP(int max_iterations, double max_distance, int frame)
{
  FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();
  int view_number = model->getViewNumber();
  std::vector<osg::ref_ptr<PointCloud> > point_clouds;
  for (size_t i = 1; i < view_number/2; ++ i)
  {
    osg::ref_ptr<PointCloud> front_cloud = model->getPointCloud(frame, i);
    if (front_cloud->isShown())
      point_clouds.push_back(front_cloud);
    osg::ref_ptr<PointCloud> back_cloud = model->getPointCloud(frame, view_number-i);
    if (back_cloud->isShown())
      point_clouds.push_back(back_cloud);
  }

  if (view_number / 2 == 0)
  {
	osg::ref_ptr<PointCloud> center_cloud = model->getPointCloud(frame, view_number / 2);
	if (center_cloud->isShown())
	  point_clouds.push_back(center_cloud);
  }
  else
  {
	osg::ref_ptr<PointCloud> left_center = model->getPointCloud(frame, view_number / 2);
	osg::ref_ptr<PointCloud> right_center = model->getPointCloud(frame, view_number / 2 + 1);
	if (left_center->isShown())
	  point_clouds.push_back(left_center);
	if (right_center->isShown())
	  point_clouds.push_back(right_center);
  }
  
  if (point_clouds.empty())
    return;

  for (size_t i = 0, i_end = point_clouds.size(); i < i_end; ++ i)
    point_clouds[i]->initRotation();

  PCLPointCloud::Ptr source(new PCLPointCloud);
  PCLPointCloud::Ptr target(new PCLPointCloud);

  pcl::IterativeClosestPoint<PCLPoint, PCLPoint> icp;
  icp.setUseReciprocalCorrespondences(true);
  // Set the max correspondence distance (e.g., correspondences with higher distances will be ignored)
  icp.setMaxCorrespondenceDistance(max_distance);
  // Set the maximum number of iterations (criterion 1)
  icp.setMaximumIterations(max_iterations);
  // Set the transformation epsilon (criterion 2)
  icp.setTransformationEpsilon(0.000001);
  // Set the euclidean distance difference epsilon (criterion 3)
  icp.setEuclideanFitnessEpsilon(64);

  model->getPointCloud(frame, 0)->getTransformedPoints(*target);
  for (size_t i = 0, i_end = point_clouds.size(); i < i_end; ++ i)
  {
    point_clouds[i]->getTransformedPoints(*source);
    icp.setInputSource(source);
    icp.setInputTarget(target);
    PCLPointCloud transformed_source;
    icp.align(transformed_source);
   
    osg::Matrix result_matrix = PclMatrixCaster<osg::Matrix>(icp.getFinalTransformation());
    point_clouds[i]->setMatrix(point_clouds[i]->getMatrix()*result_matrix);

    *target += transformed_source;
  }

  if (show_error_)
  {
    QMutexLocker locker(&mutex_);
    computeError(frame);
  }

  expire();

  return;
}

void Registrator::registrationICP(void)
{
  int max_iterations, frame, repeat_times;
  double max_distance;
  if (!ParameterManager::getInstance().getRegistrationICPParameters(max_iterations, max_distance, frame, repeat_times))
    return;

  QFutureWatcher<void>* watcher = new QFutureWatcher<void>(this);
  connect(watcher, SIGNAL(finished()), watcher, SLOT(deleteLater()));

  QString running_message = QString("ICP registration for frame %1 is running!").arg(frame);
  QString finished_message = QString("ICP registration for frame %1 finished!").arg(frame);
  Messenger* messenger = new Messenger(running_message, finished_message, this);
  connect(watcher, SIGNAL(started()), messenger, SLOT(sendRunningMessage()));
  connect(watcher, SIGNAL(finished()), messenger, SLOT(sendFinishedMessage()));

  watcher->setFuture(QtConcurrent::run(this, &Registrator::registrationICP, max_iterations, max_distance, frame, repeat_times));

  return;
}

void Registrator::registrationLUM(int segment_threshold, int max_iterations, double max_distance, int frame)
{
  std::cout << "registrationLUM: frame " << frame << " running..." << std::endl;

  FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();
  int view_number = model->getViewNumber();
  for (size_t view = 0; view < view_number; ++ view)
  {
    osg::ref_ptr<PointCloud> point_cloud = model->getPointCloud(frame, view);
	// Delaunay Triangulation Denoise Method
	// point_cloud->denoise(segment_threshold, ParameterManager::getInstance().getTriangleLength());

	// Point Density Denoise Method
    // point_cloud->denoise(segment_threshold);
    point_cloud->initRotation();
    point_cloud->setRegisterState(true);
  }

  int lum_max_iterations = 16;
  int outer_loop_num = std::max(1, max_iterations/lum_max_iterations);
  for (size_t loop = 0; loop < outer_loop_num; ++ loop)
  {
    pcl::registration::LUM<PCLPoint> lum;
    for (size_t i = 0; i < view_number; ++ i)
    {
      osg::ref_ptr<PointCloud> point_cloud = model->getPointCloud(frame, i);
      point_cloud->initRotation();

      PCLPointCloud::Ptr transformed_cloud(new PCLPointCloud);
      point_cloud->getTransformedPoints(*transformed_cloud);

      lum.addPointCloud(transformed_cloud);
      
    }

    for (size_t i = 0; i < view_number; ++ i)
    {
      int source_idx = i;
      int target_idx = (i==view_number-1)?(0):(i+1);
      pcl::registration::CorrespondenceEstimation<PCLPoint, PCLPoint, float> correspondence_estimation;
      correspondence_estimation.setInputSource(lum.getPointCloud(source_idx));
      correspondence_estimation.setInputTarget(lum.getPointCloud(target_idx));

      pcl::CorrespondencesPtr correspondences(new pcl::Correspondences);
      correspondence_estimation.determineReciprocalCorrespondences (*correspondences, max_distance);
      lum.setCorrespondences(source_idx, target_idx, correspondences);
    }
    
    lum.setMaxIterations(lum_max_iterations);
    lum.compute();
    
    for (size_t i = 0; i < view_number; ++ i)
    {
      Eigen::Affine3f transformation = lum.getTransformation(i);
      osg::Matrix osg_transformation = PclMatrixCaster<osg::Matrix>(Eigen::Matrix4f(transformation.data()));
      osg::ref_ptr<PointCloud> point_cloud = model->getPointCloud(frame, i);
      point_cloud->setMatrix(point_cloud->getMatrix()*osg_transformation);
      point_cloud->setRegisterState(true);
    }
  }

  if (show_error_)
  {
    QMutexLocker locker(&mutex_);
    computeError(frame);
  }

  saveRegisteredPoints(frame);
  refineAxis(frame);

  expire();

  return;
}

void Registrator::registrationLUM(void)
{
  int segment_threshold, max_iterations, frame;
  double max_distance;
  if (!ParameterManager::getInstance().getRegistrationLUMParameters(segment_threshold, max_iterations, max_distance, frame))
    return;

  QFutureWatcher<void>* watcher = new QFutureWatcher<void>(this);
  connect(watcher, SIGNAL(finished()), watcher, SLOT(deleteLater()));

  QString running_message = QString("LUM registration for frame %1 is running!").arg(frame);
  QString finished_message = QString("LUM registration for frame %1 finished!").arg(frame);
  Messenger* messenger = new Messenger(running_message, finished_message, this);
  connect(watcher, SIGNAL(started()), messenger, SLOT(sendRunningMessage()));
  connect(watcher, SIGNAL(finished()), messenger, SLOT(sendFinishedMessage()));

  watcher->setFuture(QtConcurrent::run(this, &Registrator::registrationLUM, segment_threshold, max_iterations, max_distance, frame));

  return;
}

void Registrator::registration(void)
{
  int frame, segment_threshold;
  if (!ParameterManager::getInstance().getRegistrationParameters(frame, segment_threshold))
    return;

  QFutureWatcher<void>* watcher = new QFutureWatcher<void>(this);
  connect(watcher, SIGNAL(finished()), watcher, SLOT(deleteLater()));

  QString running_message = QString("Registration for frame %1 is running!").arg(frame);
  QString finished_message = QString("Registration for frame %1 finished!").arg(frame);
  Messenger* messenger = new Messenger(running_message, finished_message, this);
  connect(watcher, SIGNAL(started()), messenger, SLOT(sendRunningMessage()));
  connect(watcher, SIGNAL(finished()), messenger, SLOT(sendFinishedMessage()));

  watcher->setFuture(QtConcurrent::run(this, &Registrator::registration, frame, segment_threshold));
}

void Registrator::registration(int frame, int segment_threshold)
{
  std::cout << "registration: frame " << frame << " running..." << std::endl;

  FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();
  int view_number = model->getViewNumber();
  for (size_t view = 0; view < view_number; ++ view)
  {
    osg::ref_ptr<PointCloud> point_cloud = model->getPointCloud(frame, view);
	// Delaunay Triangulation Denoise Method
	// point_cloud->denoise(segment_threshold, ParameterManager::getInstance().getTriangleLength());

	// Point Density Denoise Method
	//point_cloud->denoise(segment_threshold);
    point_cloud->initRotation();
    point_cloud->setRegisterState(true);
  }


  if (show_error_)
  {
    QMutexLocker locker(&mutex_);
    computeError(frame);
  }

  saveRegisteredPoints(frame);
  refineAxis(frame);

  expire();

  return;
}

void Registrator::toggleRendering()
{
  if (!show_axis_ && !show_error_)
  {
    hidden_ = false;
    show_axis_ = true;
  }
  else if (show_axis_ && !show_error_)
  {
    show_axis_ = false;
    show_error_ = true;
  }
  else if (!show_axis_ && show_error_)
  {
    hidden_ = true;
    show_error_ = false;
  }

  expire();

  return;
}