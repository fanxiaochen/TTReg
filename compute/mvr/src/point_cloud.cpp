#include <numeric>

#include <QRegExp>
#include <QFileInfo>
#include <QFileDialog>
#include <QColorDialog>
#include <QFutureWatcher>
#include <QtConcurrentRun>

#include <osg/Geode>
#include <osg/io_utils>
#include <osg/Geometry>
#include <osgManipulator/TrackballDragger>
#include <osgManipulator/TranslateAxisDragger>

#include <CGAL/Delaunay_triangulation_3.h>
#include <CGAL/Triangulation_vertex_base_with_info_3.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>

#include <pcl/io/pcd_io.h>
#include <pcl/io/ply_io.h>


#include "parameter.h"
#include "registrator.h"
#include "main_window.h"
#include "parameter_dialog.h"
#include "file_system_model.h"
#include "parameter_manager.h"
#include "point_cloud.h"
#include "osg_viewer_widget.h"


PointCloud::PointCloud(void)
  :translate_dragger_(new osgManipulator::TranslateAxisDragger),
  trackball_dragger_(new osgManipulator::TrackballDragger),
  show_draggers_(false),
  registered_(false),
  triangulation_(new CGAL::Delaunay()),
  point_graph_(new boost::PointGraph()),
  point_graph_threshold_(-1.0),
  points_num_(0),
  noise_points_num_(0)
{
  translate_dragger_->setupDefaultGeometry();
  translate_dragger_->setHandleEvents(true);
  translate_dragger_->setActivationModKeyMask(osgGA::GUIEventAdapter::MODKEY_CTRL);
  translate_dragger_->setActivationKeyEvent('d');

  trackball_dragger_->setupDefaultGeometry();
  trackball_dragger_->setHandleEvents(true);
  trackball_dragger_->setActivationModKeyMask(osgGA::GUIEventAdapter::MODKEY_CTRL);
  trackball_dragger_->setActivationKeyEvent('d');

  translate_dragger_->addTransformUpdating(this);
  translate_dragger_->addTransformUpdating(trackball_dragger_);

  trackball_dragger_->addTransformUpdating(this);
  trackball_dragger_->addTransformUpdating(translate_dragger_);
}

PointCloud::~PointCloud(void)
{
  delete triangulation_;
  delete point_graph_;
}

void PointCloud::setRegisterState(bool registered)
{
  registered_=registered;
  if (registered_ && getView() != 0)
    saveTransformation();

  return;
}

bool PointCloud::open(const std::string& filename)
{
  clearData();

  QMutexLocker locker(&mutex_);

  if (pcl::io::loadPCDFile(filename, *this) != 0)
    return false;

  filename_ = filename;
  loadTransformation();

  registered_ = (getView() == 0) || (!(getMatrix().isIdentity()));
 
  expire();

  return true;
}

bool PointCloud::save(const std::string& filename)
{
  if (QString(filename.c_str()).right(3) == "ply")
  {
    PCLPointCloud point_cloud;
    osg::Vec3 pivot_point(-13.382786, 50.223461, 917.477600);
    osg::Vec3 axis_normal(-0.054323, -0.814921, -0.577020);
    osg::Matrix transformation = osg::Matrix::translate(-pivot_point)*osg::Matrix::rotate(axis_normal, osg::Vec3(0, 0, 1));
    for (size_t i = 0; i < points_num_; ++ i)
    {
      osg::Vec3 point(at(i).x, at(i).y, at(i).z);
      point = transformation.preMult(point);
      point_cloud.push_back(PCLPoint(point.x(), point.y(), point.z()));     
    }
    pcl::PLYWriter ply_writer;
    if (ply_writer.write<PCLPoint>(filename, point_cloud) != 0)
      return false;
  }
  else
  {
    pcl::PCDWriter pcd_writer;
    if (pcd_writer.writeBinaryCompressed<PCLRichPoint>(filename, *this) != 0)       
    return false;
  }

  return true;
}

void PointCloud::save(void)
{
  MainWindow* main_window = MainWindow::getInstance();
  QString filename = QFileDialog::getSaveFileName(main_window, "Save Point Cloud",
    main_window->getWorkspace(), "Point Cloud (*.pcd *.ply)");
  if (filename.isEmpty())
    return;

  save(filename.toStdString());

  return;
}

void PointCloud::reload(void)
{
  clearData();
  open(filename_);

  return;
}

void PointCloud::clearData()
{
  QMutexLocker locker(&mutex_);

  Renderable::clear();
  PCLRichPointCloud::clear();

  return;
}

void PointCloud::visualizePoints(size_t start, size_t end)
{
  osg::ref_ptr<osg::Vec3Array>  vertices = new osg::Vec3Array;
  osg::ref_ptr<osg::Vec3Array>  normals_vertices = new osg::Vec3Array;
  osg::ref_ptr<osg::Vec3Array>  orientations_vertices = new osg::Vec3Array;
  osg::ref_ptr<osg::Vec3Array>  normals = new osg::Vec3Array;
  osg::ref_ptr<osg::Vec4Array>  colors = new osg::Vec4Array;

  for (size_t i=start, i_end = end; i < i_end; i++)
  {
    const PCLRichPoint& point = at(i);
    vertices->push_back(osg::Vec3(point.x, point.y, point.z));
    normals->push_back(osg::Vec3(point.normal_x, point.normal_y, point.normal_z));
    colors->push_back(osg::Vec4(point.r/255.0, point.g/255.0, point.b/255.0, 1.0));
  }

  size_t item_num = vertices->size();

  osg::Geode* geode = new osg::Geode;
 
  osg::Geometry* geometry = new osg::Geometry;
  geometry->setUseDisplayList(true);
  geometry->setUseVertexBufferObjects(true);
  geometry->setVertexData(osg::Geometry::ArrayData(vertices, osg::Geometry::BIND_PER_VERTEX));
  geometry->setNormalData(osg::Geometry::ArrayData(normals, osg::Geometry::BIND_PER_VERTEX));
  geometry->setColorData(osg::Geometry::ArrayData(colors, osg::Geometry::BIND_PER_VERTEX));
  geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, item_num));

  geode->addDrawable(geometry);
   
  
  addChild(geode);

  return;
}

void PointCloud::updateImpl()
{

  visualizePoints(0, size());

  if (show_draggers_)
  {
    osg::BoundingSphere boundingSphere = getBound();
    osg::Matrix trans = osg::Matrix::translate(boundingSphere.center());
    double radius = boundingSphere.radius();
    float t_scale = radius/4;
    float r_scale = radius/8;
    osg::Matrix flip(osg::Matrix::rotate(osg::Vec3(0, 1, 0), osg::Vec3(0, -1, 0)));
    translate_dragger_->setMatrix(flip*osg::Matrix::scale(t_scale, t_scale, t_scale)*trans);
    trackball_dragger_->setMatrix(osg::Matrix::scale(r_scale, r_scale, r_scale)*trans);

    addChild(translate_dragger_);
    addChild(trackball_dragger_);
  }

  return;
}



PointCloud* PointCloud::getPrevFrame(void)
{
  FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();
  return model->getPointCloud(getFrame()-1);
}

PointCloud* PointCloud::getNextFrame(void)
{
  FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();
  return model->getPointCloud(getFrame()+1);
}



int PointCloud::getFrame(void) const      
{
  QRegExp frame("[\\/]frame_([0-9]{5,5})[\\/]");
  frame.indexIn(filename_.c_str());
  QString index = frame.cap(1);

  return index.toInt();
}

int PointCloud::getView(void) const
{
  FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();

  QRegExp frame("[\\/]view_([0-9]{2,2})[\\/]");
  frame.indexIn(filename_.c_str());
  QString index = frame.cap(1);
  if (index.isEmpty())
	  return model->getViewNumber();

  return index.toInt();
}

void PointCloud::setRotation(void)
{
  FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();
  int view_number = model->getViewNumber();
  float delta_angle = 360.0 / view_number;

  ParameterDialog parameter_dialog("Rotation Parameters", MainWindow::getInstance());
  int view = getView();
  DoubleParameter rotation_angle("Rotation Angle", "Rotation Angle", -view * delta_angle, -360, 0, delta_angle);
  parameter_dialog.addParameter(&rotation_angle);
  if (!parameter_dialog.exec() == QDialog::Accepted)
    return;

  double angle = rotation_angle*M_PI/180.0;
  setMatrix(MainWindow::getInstance()->getRegistrator()->getRotationMatrix(angle));

  if (angle == 0.0)
    deleteTransformation();

  return;
}

void PointCloud::toggleDraggers(void)
{
  QMutexLocker locker(&mutex_);
  show_draggers_ = !show_draggers_;
  expire();

  return;
}

void PointCloud::toggleRegisterState(void)
{
  registered_ = !registered_;
  if (registered_)
    saveTransformation();   
  else
    deleteTransformation();

  return;
}


void PointCloud::getTransformedPoints(PCLPointCloud& points)
{
  points.clear();

  const osg::Matrix& matrix = getMatrix();
  for (size_t i = 0, i_end = size(); i < i_end; ++ i)
  {
    osg::Vec3 point(at(i).x, at(i).y, at(i).z);
    point = matrix.preMult(point);
    points.push_back(PCLPoint(point.x(), point.y(), point.z()));
  }

  return;
}

void PointCloud::loadTransformation(void)
{
  std::string filename = (QFileInfo(filename_.c_str()).path()+"/transformation.txt").toStdString();
  FILE *file = fopen(filename.c_str(),"r");
  if (file == NULL)
    return;

  osg::Matrix matrix;
  for (int i = 0; i < 4; ++ i)
  {
    for (int j = 0; j < 4; ++ j)
    {
      double element;
      fscanf(file, "%lf", &element);
      matrix(j, i) = element;
    }
  }
  setMatrix(matrix);
  fclose(file);

  return;
}

void PointCloud::saveTransformation(void)
{
  std::string filename = (QFileInfo(filename_.c_str()).path()+"/transformation.txt").toStdString();
  FILE *file = fopen(filename.c_str(),"w");
  if (file == NULL)
    return;

  osg::Matrix matrix = getMatrix();
  for (int i = 0; i < 4; ++ i)
  {
    for (int j = 0; j < 4; ++ j)
    {
      fprintf(file, "%lf ", matrix(j, i));
    }
    fprintf(file, "\n");
  }
  fclose(file);

  return;
}

void PointCloud::deleteTransformation(void)
{
  std::string filename = (QFileInfo(filename_.c_str()).path()+"/transformation.txt").toStdString();
  std::remove(filename.c_str());
}


void PointCloud::registration(int segment_threshold, int max_iterations, double max_distance)
{
  FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();
  int view_number = model->getViewNumber();

  if (getView() != view_number)
    return;

  Registrator* registrator = MainWindow::getInstance()->getRegistrator();
  registrator->registrationLUM(segment_threshold, max_iterations, max_distance, getFrame());

  expire();

  return;
}

void PointCloud::registration(void)
{
  int segment_threshold, max_iterations;
  double max_distance;
  if (!ParameterManager::getInstance().getRegistrationLUMParameters(segment_threshold, max_iterations, max_distance))
    return;

  QFutureWatcher<void>* watcher = new QFutureWatcher<void>(this);
  connect(watcher, SIGNAL(finished()), watcher, SLOT(deleteLater()));

  int frame = getFrame();
  QString running_message = QString("Computing registration for frame %1!").arg(frame);
  QString finished_message = QString("Registration for frame %1 computed!").arg(frame);
  Messenger* messenger = new Messenger(running_message, finished_message, this);
  connect(watcher, SIGNAL(started()), messenger, SLOT(sendRunningMessage()));
  connect(watcher, SIGNAL(finished()), messenger, SLOT(sendFinishedMessage()));

  watcher->setFuture(QtConcurrent::run(this, &PointCloud::registration, segment_threshold, max_iterations, max_distance));

  return;
}


bool PointCloud::isShown(void) const
{
  FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();

  return model->isShown(filename_);
  return false;
}

void PointCloud::initRotation(void)
{
  FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();
  int view_number = model->getViewNumber();

  if (!getMatrix().isIdentity())
    return;

  int view = getView();
  if (view == 0)
    return;

  double angle = -2 * view * M_PI / view_number;
  setMatrix(MainWindow::getInstance()->getRegistrator()->getRotationMatrix(angle));

  return;
}


void PointCloud::denoise(int segment_threshold, double triangle_length)
{
  QMutexLocker locker(&mutex_);
  
  points_num_ = size();
  initPointGraph(ParameterManager::getInstance().getTriangleLength());

  boost::PointGraph& g_point = *point_graph_;
  std::vector<boost::PointGraphTraits::vertex_descriptor> component(boost::num_vertices(g_point));
  size_t component_num = boost::connected_components(g_point, &component[0]);

  std::vector<std::vector<boost::PointGraphTraits::vertex_descriptor> > components(component_num);
  for (size_t i = 0; i < points_num_; ++ i)
    components[component[i]].push_back(i);
  

  for (size_t i = 0, i_end = components.size(); i < i_end; ++ i)
  {
    if (components[i].size() < segment_threshold)
    {	
      for (size_t j = 0, j_end = components[i].size(); j < j_end; ++ j)
		  indicateNoise(components[i][j]);
    }
  }
    
  triangulation_->clear();
  g_point.clear();

  expire();

  return;
}

// this method is based on the paper -- Consolidation of Low-quality Point Clouds from Outdoor Scenes
void PointCloud::denoise(int k)
{
	QMutexLocker locker(&mutex_);

	const float w = 1.25;
	std::vector<std::vector<int> > point_index;
	std::vector<std::vector<float> > point_dist;

	std::vector<float> d_k_vector;
	std::vector<float> D_k_vector;
	std::vector<float> DDF_k_vector;
	std::vector<float> theta_k_vector;

	pcl::KdTreeFLANN<PCLRichPoint> kdtree;
	PointCloud::Ptr cloud(new PointCloud);
	PointCloud::iterator itr = this->begin();
	while (itr != this->end())
	{
		cloud->push_back(*itr);
		itr ++;
	}

	kdtree.setInputCloud (cloud);

	points_num_ = size();

	//K nearest neighbor search
	for (size_t i = 0, i_end = points_num_; i < i_end; i ++)
	{
		const PCLRichPoint& search_point = at(i);

		std::vector<int> pointIdxNKNSearch(k);
		std::vector<float> pointNKNSquaredDistance(k);
		kdtree.nearestKSearch (search_point, k, pointIdxNKNSearch, pointNKNSquaredDistance);

		for (std::vector<float>::iterator itr = pointNKNSquaredDistance.begin(); itr != pointNKNSquaredDistance.end(); itr++)
		{
			*itr = sqrt(*itr); 
		}

		point_index.push_back(pointIdxNKNSearch);
		point_dist.push_back(pointNKNSquaredDistance);


		std::vector<float> dists = point_dist.at(i);
		float sum = 0;
		for (size_t j = 0, j_end = dists.size(); j < j_end; j ++)
		{
			sum += dists[j];
		}
		int num = dists.size();
		d_k_vector.push_back(sum / num);
	}

	for (size_t i = 0, i_end = points_num_; i < i_end; i ++)
	{
		std::vector<int> index = point_index.at(i);
		float sum = 0;

		for (size_t j = 0, j_end = index.size(); j < j_end; j ++)
		{
			sum += d_k_vector.at(index[j]);
		}
		int num = index.size();

		D_k_vector.push_back(sum / num);
		DDF_k_vector.push_back(abs(1-d_k_vector[i]/D_k_vector[i]));
	}

	for (size_t i = 0, i_end = points_num_; i < i_end; i ++)
	{
		std::vector<int> index = point_index.at(i);
		float sum = 0;
		for (size_t j = 0, j_end = index.size(); j < j_end; j ++)
		{
			sum += pow(d_k_vector[index[j]] - D_k_vector[i], 2);
		}
		float theta = sqrt(sum / index.size());
		theta_k_vector.push_back(theta / D_k_vector[i]);

		if (DDF_k_vector[i] > theta_k_vector[i] * w)
			indicateNoise(i);
	}

	expire();

	return;
}

void PointCloud::indicateNoise(size_t i)
{
	 // indicate special number to remove 
	PCLRichPoint& point = at(i);
	point.x = 0;
	point.y = 0;
	point.z = 0;

	return;
}

bool PointCloud::isNoise(size_t i)
{
	PCLRichPoint& point = at(i);
	if (point.x == 0 && point.y == 0 && point.z == 0)
		return true;
	return false;
}

void PointCloud::extractByPlane()
{
	QMutexLocker locker(&mutex_);

	Registrator* registrator = MainWindow::getInstance()->getRegistrator();
	osg::Vec3 pivot_point = registrator->getPivotPoint();
	osg::Vec3 axis_normal = registrator->getAxisNormal();
	osg::Vec4 plane = osg::Plane(axis_normal, pivot_point).asVec4();
	double a = plane.x();
	double b = plane.y();
	double c = plane.z();
	double d = plane.w();

	for (size_t i = 0, i_end = size();i < i_end; i++) 
	{
		const PCLRichPoint& point = at(i);
		double check = a*point.x + b*point.y + c*point.z + d;
		if (check <= 0)
			indicateNoise(i);
	}

	locker.unlock();

	return;
}

void PointCloud::initPointGraph(double distance_threshold)
{
  if (distance_threshold == point_graph_threshold_
    && boost::num_edges(*point_graph_) != 0)
    return;

  triangulate();
  point_graph_threshold_ = distance_threshold;

  boost::PointGraph& g_point = *point_graph_;
  g_point = boost::PointGraph(points_num_);

  size_t t = 8;

  for (CGAL::Delaunay::Finite_edges_iterator it = triangulation_->finite_edges_begin();
    it != triangulation_->finite_edges_end(); ++ it)
  {
    const CGAL::Delaunay::Cell_handle& cell_handle  = it->first;
    const CGAL::Delaunay::Vertex_handle& source_handle = cell_handle->vertex(it->second);
    const CGAL::Delaunay::Vertex_handle& target_handle = cell_handle->vertex(it->third);
    double distance_L1 = std::sqrt(CGAL::squared_distance(source_handle->point(), target_handle->point()));
    if (distance_L1 > point_graph_threshold_)
      continue;

    size_t source_id = source_handle->info();
    size_t target_id = target_handle->info();
    assert (source_id < points_num_ && target_id < points_num_);
    WeightedEdge weighted_edge(distance_L1);
    boost::add_edge(source_id, target_id, weighted_edge, g_point);
  }

  return;
}

void PointCloud::triangulate() const
{
  if (triangulation_->number_of_vertices() != 0)
    return;
  
  for (size_t i = 0, i_end = points_num_; i < i_end; ++ i)
  {
    const PCLRichPoint& point = at(i);
    CGAL::Delaunay::Point delaunay_point(point.x, point.y, point.z);
    CGAL::Delaunay::Vertex_handle vertex_handle = triangulation_->insert(delaunay_point);
    vertex_handle->info() = i;
  }
  
  return;
}