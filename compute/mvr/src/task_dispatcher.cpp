﻿#include <fstream>
#include <QProcess>
#include <QMessageBox>
#include <QMutexLocker>
#include <QProgressBar>
#include <QFutureWatcher>
#include <QtConcurrentFilter>
#include <QFileDialog>
#include <QComboBox>

#include "main_window.h"
#include "point_cloud.h"
#include "registrator.h"
#include "parameter_manager.h"
#include "file_system_model.h"
#include "osg_viewer_widget.h"

#include "task_dispatcher.h"

TaskImpl::TaskImpl(int frame, int view)
  :frame_(frame),view_(view)
{}

TaskImpl::~TaskImpl(void)
{}

Task::Task(void)
{
}

Task::Task(TaskImpl* task_impl)
  :task_impl_(task_impl)
{
}

Task::Task(const Task &other)
{
  task_impl_ = other.task_impl_;
}

Task& Task::operator=(const Task &other)
{
  task_impl_ = other.task_impl_;
  return (*this);
}

Task::~Task()
{
}

bool Task::run(void) const
{
  task_impl_->run();

  emit finished(task_impl_->frame_, task_impl_->view_);

  return false;
}


TaskPointsGeneration::TaskPointsGeneration(int frame, int view, int ctr_threshold, int sat_threshold)
  :TaskImpl(frame, view),ctr_threshold_(ctr_threshold), sat_threshold_(sat_threshold)
{}

TaskPointsGeneration::~TaskPointsGeneration(void)
{}

void TaskPointsGeneration::run(void) const
{
  convertImages();

  FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();

  QStringList arguments;
  arguments << model->getPointsFolder(frame_, view_).c_str()
    << QString::number(ctr_threshold_) << QString::number(sat_threshold_);
  QProcess process;
  process.start(getExeFilename(), arguments);
  process.waitForFinished(-1);

  deleteImages();
  colorizePoints();

  model->updatePointCloud(frame_, view_);

  return;
}

void TaskPointsGeneration::convertImages(void) const
{
  FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();

  for (size_t i = 0; i < 30; ++ i)
  {
    QString load_filename = QString("%1/image_%2.jpg")
      .arg(model->getImagesFolder(frame_, view_).c_str()).arg(i, 2, 10, QChar('0'));

    if (!QFile::exists(load_filename))
      continue;

    QImage image;
    bool f = image.load(load_filename);

    QString save_filename = QString("%1/%2.bmp").arg(model->getPointsFolder(frame_, view_).c_str()).arg(i);
    bool flag = image.save(save_filename);
  }

  return;
}

void TaskPointsGeneration::deleteImages(void) const
{
  FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();

  for (size_t i = 0; i < 30; ++ i)
  {
    QString delete_filename = QString("%1/%2.bmp").arg(model->getPointsFolder(frame_, view_).c_str()).arg(i);
    QFile::remove(delete_filename);
  }

  return;
}

void TaskPointsGeneration::colorizePoints(void) const
{
  FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();
  std::string points_folder = model->getPointsFolder(frame_, view_);

  std::string filename = points_folder+"/points.bxyzuv";
  if (!QFile::exists(filename.c_str()))
    return;

  QImage snapshot((points_folder+"/snapshot.jpg").c_str());

  std::ifstream fin(filename, std::ios::binary);
  if (!fin.good())
    return;

  long begin, end;
  fin.seekg(0, std::ios::end);
  end = fin.tellg();
  fin.seekg(0, std::ios::beg);
  begin = fin.tellg();

  PointCloud point_cloud;
  long num_points = (end-begin)/(5*sizeof(double));
  PCLRichPoint point;
  while (num_points)
  {
    double x, y, z;
    fin.read((char*)&x, sizeof(double));
    fin.read((char*)&y, sizeof(double));
    fin.read((char*)&z, sizeof(double));
    point.x = x;
    point.y = y;
    point.z = z;

    osg::Vec3 normal(-x, -y, -z);
    normal.normalize();
    point.normal_x = normal.x();
    point.normal_y = normal.y();
    point.normal_z = normal.z();

    double u, v;
    fin.read((char*)&u, sizeof(double));
    fin.read((char*)&v, sizeof(double));

    int px = std::min(std::max((int)(u), 0), snapshot.width()-1);
    int py = std::min(std::max((int)(v), 0), snapshot.height()-1);
    QColor rgb(snapshot.pixel(px, py));
    point.r = rgb.red();
    point.g = rgb.green();
    point.b = rgb.blue();

    point_cloud.push_back(point);
    num_points --;
  }

  fin.close();
  if (!filename.empty())
    QFile::remove(filename.c_str());

  point_cloud.save(points_folder+"/points.pcd");

  return;
}


QString TaskPointsGeneration::getExeFilename(void)
{
  FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();

  QString exe_filename;
  QStringList root_entries = QDir(model->rootPath()).entryList();
  for (QStringList::const_iterator root_entries_it = root_entries.begin();
    root_entries_it != root_entries.end(); ++ root_entries_it)
  {
    if (root_entries_it->startsWith("TTReg-Convert") && root_entries_it->endsWith(".exe"))
    {
      exe_filename = model->rootPath()+"/"+*root_entries_it;
      break;
    }
  }

  return exe_filename;
}

TaskDispatcher::TaskDispatcher(QObject* parent)
  :QObject(parent)
{
  
}

TaskDispatcher::~TaskDispatcher(void)
{
  cancelRunningTasks(true);

  return;
}

bool TaskDispatcher::isRunning(void) const
{
  QMutexLocker locker(&mutex_);

  return (!active_watchers_.empty());
}

void TaskDispatcher::cancelRunningTasks(bool wait)
{
  QMutexLocker locker(&mutex_);

  for (size_t i = 0, i_end = active_watchers_.size(); i < i_end; ++ i)
  {
    QFutureWatcher<void>* watcher = dynamic_cast<QFutureWatcher<void>*>(active_watchers_[i]);
    watcher->cancel();
    if (wait)
      watcher->waitForFinished();
  }

  return;
}

void TaskDispatcher::removeFinishedWatchers(void)
{
  QMutexLocker locker(&mutex_);

  std::vector<QObject*> temp;
  for (size_t i = 0, i_end = active_watchers_.size(); i < i_end; ++ i)
  {
    QFutureWatcher<void>* watcher = dynamic_cast<QFutureWatcher<void>*>(active_watchers_[i]);
    if (!watcher->isFinished())
      temp.push_back(active_watchers_[i]);
    else
      watcher->deleteLater();
  }

  active_watchers_ = temp;

  return;
}

void TaskDispatcher::clearDisplayQueue(void)
{
  QMutexLocker locker(&mutex_);

  FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();
  for (DisplayQueue::iterator it = display_queue_.begin(); it != display_queue_.end(); ++ it)
  {
    int hide_frame = it->first;
    int hide_view = it->second;
    model->hidePointCloud(hide_frame, hide_view);
  }
  display_queue_.clear();

  return;
}

void TaskDispatcher::updateDisplayQueue(int frame, int view)
{
  QMutexLocker locker(&mutex_);

  FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();
  int queue_size_threshold = QThread::idealThreadCount();

  if (display_queue_.size() < queue_size_threshold)
    model->showPointCloud(frame, view);
  else
  {
    int hide_frame = display_queue_.front().first;
    int hide_view = display_queue_.front().second;
    model->hideAndShowPointCloud(hide_frame, hide_view, frame, view);
    display_queue_.pop_front();
  }

  display_queue_.push_back(std::make_pair(frame, view));

  return;
}

void TaskDispatcher::runTasks(QList<Task>& tasks, const QString& task_name, bool display)
{
  QProgressBar* progress_bar = new QProgressBar(MainWindow::getInstance());
  progress_bar->setRange(0, tasks.size());
  progress_bar->setValue(0);
  progress_bar->setFormat(QString("%1: %p% completed").arg(task_name));
  progress_bar->setTextVisible(true);
  MainWindow::getInstance()->statusBar()->addPermanentWidget(progress_bar);

  QFutureWatcher<void>* watcher = new QFutureWatcher<void>(this);
  active_watchers_.push_back(watcher);
  
  connect(watcher, SIGNAL(progressValueChanged(int)), progress_bar, SLOT(setValue(int)));
  connect(watcher, SIGNAL(finished()), progress_bar, SLOT(deleteLater()));
  connect(watcher, SIGNAL(finished()), this, SLOT(removeFinishedWatchers()));

  if (display)
  {
    connect(watcher, SIGNAL(finished()), this, SLOT(clearDisplayQueue()));
    for (QList<Task>::const_iterator it = tasks.begin(); it != tasks.end(); ++ it)
      connect(&(*it), SIGNAL(finished(int, int)), this, SLOT(updateDisplayQueue(int, int)));
  }

  watcher->setFuture(QtConcurrent::filter(tasks, &Task::run));

  return;
}

void TaskDispatcher::dispatchTaskPointsGeneration(void)
{
  FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();
  int view_number = model->getViewNumber();

  if (!points_generation_tasks_.isEmpty())
  {
    QMessageBox::warning(MainWindow::getInstance(), "Points Generation Task Warning",
      "Run points generation task after the previous one has finished");
    return;
  }

  int ctr_threshold, sat_threshold, start_frame, end_frame;
  if (!ParameterManager::getInstance()
    .getGenerationParameters(ctr_threshold, sat_threshold, start_frame, end_frame))
    return;

  QString exe_filename = TaskPointsGeneration::getExeFilename();
  if (!QFile::exists(exe_filename))
  {
    QMessageBox::warning(MainWindow::getInstance(), "Point Cloud Generator Warning",
      "There's no EvoGeoConvert.exe in the root folder");
    return;
  }

  for (int frame = start_frame; frame <= end_frame; frame ++)
    for (int view = 0; view < view_number; ++ view)
      points_generation_tasks_.push_back(Task(new TaskPointsGeneration(frame, view, ctr_threshold, sat_threshold)));

  runTasks(points_generation_tasks_, "Points Generation");

  return;
}

TaskRegistration::TaskRegistration(int frame, int segment_threshold, int max_iterations, double max_distance)
  :TaskImpl(frame, -1), segment_threshold_(segment_threshold), max_iterations_(max_iterations), max_distance_(max_distance)
{}

TaskRegistration::~TaskRegistration(void)
{}

void TaskRegistration::run(void) const
{
  Registrator* registrator = MainWindow::getInstance()->getRegistrator();
  registrator->registrationLUM(segment_threshold_, max_iterations_, max_distance_, frame_);

  return;
}

void TaskDispatcher::dispatchTaskRegistration(void)
{
  if (!registration_tasks_.isEmpty())
  {
    QMessageBox::warning(MainWindow::getInstance(), "Registration Task Warning",
      "Run registration task after the previous one has finished");
    return;
  }

  int segment_threshold, max_iteration, start_frame, end_frame;
  double max_distance;
  if (!ParameterManager::getInstance().getRegistrationLUMParameters(segment_threshold, max_iteration, max_distance, start_frame, end_frame))
    return;

  for (int frame = start_frame; frame <= end_frame; frame ++)
    registration_tasks_.push_back(Task(new TaskRegistration(frame, segment_threshold, max_iteration, max_distance)));

  runTasks(registration_tasks_, "Register Frames");

  return;
}

TaskDenoise::TaskDenoise(int frame, int segment_threshold)
	:TaskImpl(frame, -1), segment_threshold_(segment_threshold)
{}

TaskDenoise::~TaskDenoise(void)
{}

void TaskDenoise::run(void) const
{
	FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();
	osg::ref_ptr<PointCloud> point_cloud = model->getPointCloud(frame_);
	// Delaunay Triangulation Denoise Method
	// point_cloud->denoise(segment_threshold_, ParameterManager::getInstance().getTriangleLength());

	// Point Density Denoise Method
    point_cloud->denoise(segment_threshold_);
	point_cloud->removeNoise();

	return;
}

void TaskDispatcher::dispatchTaskDenoise(void)
{
	if (!denoise_tasks_.isEmpty())
	{
		QMessageBox::warning(MainWindow::getInstance(), "Denoise Task Warning",
			"Run denoise task after the previous one has finished");
		return;
	}

	int segment_threshold, start_frame, end_frame;
	if (!ParameterManager::getInstance().getDenoiseParameters(segment_threshold, start_frame, end_frame))
		return;

	for (int frame = start_frame; frame <= end_frame; frame ++)
		denoise_tasks_.push_back(Task(new TaskDenoise(frame, segment_threshold)));

	runTasks(denoise_tasks_, "Denoise Frames");

	return;
}

void TaskDispatcher::dispatchTaskDenoiseBySerialOrder()
{
	int segment_threshold, start_frame, end_frame;
	if (!ParameterManager::getInstance().getDenoiseParameters(segment_threshold, start_frame, end_frame))
		return;

	for (int frame = start_frame; frame <= end_frame; frame ++)
	{
		FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();
		PointCloud* point_cloud = model->getPointCloud(frame);
		// Delaunay Triangulation Denoise Method
		//point_cloud->denoise(segment_threshold, ParameterManager::getInstance().getTriangleLength());

		// Point Density Denoise Method
		point_cloud->denoise(segment_threshold);
		point_cloud->removeNoise();
	}
	std::cout << "finished!" << std::endl;

	return;
}


TaskExtractImages::TaskExtractImages(int frame, int view_number, std::string folder)
	:TaskImpl(frame, -1), view_number_(view_number), folder_(folder)
{}

TaskExtractImages::~TaskExtractImages(void)
{}

void TaskExtractImages::run(void) const
{

	FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();
	std::string images_folder = model->getPointsFolder(frame_);

	QString image_dir = QString("%1/view_%2/snapshot.jpg")
		.arg(images_folder.c_str()).arg(view_number_, 2, 10, QChar('0'));

	QString save_dir = QString("%1/snapshot_%2_%3.jpg")
		.arg(folder_.c_str()).arg(view_number_).arg(frame_);

	QImage image;
	bool flag = image.load(image_dir);
	
	if (flag)
		image.save(save_dir);

	return;
}

void TaskDispatcher::dispatchTaskExtractImages(void)
{
	QString save_directory;
	save_directory = QFileDialog::getExistingDirectory(MainWindow::getInstance(),QString("Extract Images"));

	if (save_directory.isEmpty())
		return;

	if (!extract_images_tasks_.isEmpty())
	{
		QMessageBox::warning(MainWindow::getInstance(), "Extract Images Task Warning",
			"Run extract images task after the previous one has finished");
		return;
	}

	int view_number, start_frame, end_frame;
	if (!ParameterManager::getInstance().getExtractImagesParameters(view_number, start_frame, end_frame))
		return;

	for (int frame = start_frame; frame <= end_frame; frame ++)
		extract_images_tasks_.push_back(Task(new TaskExtractImages(frame, view_number, save_directory.toStdString())));

	runTasks(extract_images_tasks_, "Extract Images");

	return;
}


TaskDataCut::TaskDataCut(int frame)
	:TaskImpl(frame, -1)
{}

TaskDataCut::~TaskDataCut(void)
{}

void TaskDataCut::run(void) const
{

	FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();
	osg::ref_ptr<PointCloud> point_cloud = model->getPointCloud(frame_);

	point_cloud->extractByPlane();
	point_cloud->removeNoise();

	return;

}

void TaskDispatcher::dispatchTaskDataCut()
{
	if (!data_cut_tasks_.isEmpty())
	{
		QMessageBox::warning(MainWindow::getInstance(), "Data Cut Task Warning",
			"Run data cut task after the previous one has finished");
		return;
	}

	int start_frame, end_frame;
	if (!ParameterManager::getInstance().getDataCutParameters(start_frame, end_frame))
		return;

	for (int frame = start_frame; frame <= end_frame; frame ++)
		data_cut_tasks_.push_back(Task(new TaskDataCut(frame)));

	runTasks(data_cut_tasks_, "Data Cut");

	return;
}

TaskRemoveOutliers::TaskRemoveOutliers(int frame)
	:TaskImpl(frame, -1)
{}

TaskRemoveOutliers::~TaskRemoveOutliers(void)
{}

void TaskRemoveOutliers::run(void) const
{
	FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();
	osg::ref_ptr<PointCloud> point_cloud = model->getPointCloud(frame_);
	point_cloud->extractBySphere();
	point_cloud->removeNoise();

	return;

}

void TaskDispatcher::dispatchTaskRemoveOutliers()
{
	if (!remove_outliers_tasks_.isEmpty())
	{
		QMessageBox::warning(MainWindow::getInstance(), "Remove Outliers Task Warning",
			"Run remove outliers task after the previous one has finished");
		return;
	}

	int start_frame, end_frame;
	if (!ParameterManager::getInstance().getRemoveOutliersParameters(start_frame, end_frame))
		return;

	for (int frame = start_frame; frame <= end_frame; frame ++)
		remove_outliers_tasks_.push_back(Task(new TaskRemoveOutliers(frame)));

	runTasks(remove_outliers_tasks_, "Remove Outliers");

	return;
}


TaskDownsampling::TaskDownsampling(int frame, int sample_ratio, std::string folder)
	:TaskImpl(frame, -1), folder_(folder), sample_ratio_(sample_ratio)
{}

TaskDownsampling::~TaskDownsampling(void)
{}

void TaskDownsampling::run(void) const
{
	FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();

	osg::ref_ptr<PointCloud> point_cloud = model->getPointCloud(frame_);
	if(point_cloud == NULL || point_cloud->size() == 0)
	{
		std::cout<<"Frame:"<<frame_<<" where is the point cloud?"<<std::endl;
		return;
	}

	std::cout << "Downsampling Frame:" << frame_ << std::endl;

	std::vector<size_t> indices(point_cloud->size());
	for (size_t i = 0, i_end = indices.size(); i < i_end; ++ i)
		indices[i] = i;
	std::random_shuffle(indices.begin(), indices.end());

	PointCloud extract_cloud;  
	size_t extract_point_size = indices.size()/sample_ratio_;
	assert(extract_point_size != 0);
	while(extract_point_size > 0)
	{
		extract_cloud.push_back(point_cloud->at(indices[extract_point_size - 1]));
		extract_point_size --;
	}

	QString root_folder = QString("%1").arg(folder_.c_str());
	QDir root_path(root_folder);
	root_path.mkdir("points");
	
	QString points_folder = QString("%1/points").arg(root_folder);
	QDir points_dir(points_folder);

	QString point_file = QString("frame_%1").arg(frame_, 5, 10, QChar('0'));
	points_dir.mkdir(point_file);
	
	QString path = QString("%1/points/%2/points.pcd").arg(root_folder).arg(point_file);

	extract_cloud.save(path.toStdString());

	return;
}

void TaskDispatcher::dispatchTaskDownsampling(void)
{
	QString save_directory;
	save_directory = QFileDialog::getExistingDirectory(MainWindow::getInstance(),QString("Downsampling"));

	if (save_directory.isEmpty())
		return;

	if (!downsampling_tasks_.isEmpty())
	{
		QMessageBox::warning(MainWindow::getInstance(), "Downsampling Task Warning",
			"Run Downsampling task after the previous one has finished");
		return;
	}

	int start_frame, end_frame, sample_ratio;
	if (!ParameterManager::getInstance().getDownsamplingParameters(sample_ratio, start_frame, end_frame))
		return;

	for (int frame = start_frame; frame <= end_frame; frame ++)
		downsampling_tasks_.push_back(Task(new TaskDownsampling(frame, sample_ratio, save_directory.toStdString())));

	runTasks(downsampling_tasks_, "Downsampling");

	return;
}

TaskExtractPoints::TaskExtractPoints(int frame, int interval, std::string folder)
	:TaskImpl(frame, -1), interval_(interval), folder_(folder)
{}

TaskExtractPoints::~TaskExtractPoints(void)
{}

void TaskExtractPoints::run(void) const
{

	FileSystemModel* model = MainWindow::getInstance()->getFileSystemModel();
	std::string points_folder = model->getPointsFolder(frame_);
	int start_frame = model->getStartFrame();
	int new_index = (frame_ - start_frame) / interval_;

	QString points_dir = QString("%1/points.pcd")
		.arg(points_folder.c_str());

	QDir root_folder = QString(folder_.c_str());
	root_folder.mkdir("points");
	QDir points_path = QString(folder_.c_str()) + "/points/";
	QString frame_folder = QString("frame_%1").arg(new_index, 5, 10, QChar('0'));
	points_path.mkdir(frame_folder);

	QString save_dir = points_path.absolutePath() + "/" + frame_folder + QString("/points.pcd");

	QFile points_file(points_dir);
	if (points_file.exists())
		QFile::copy(points_dir, save_dir);

	return;
}

void TaskDispatcher::dispatchTaskExtractPoints(void)
{
	QString save_directory;
	save_directory = QFileDialog::getExistingDirectory(MainWindow::getInstance(),QString("Extract Points"));

	if (save_directory.isEmpty())
		return;

	if (!extract_points_tasks_.isEmpty())
	{
		QMessageBox::warning(MainWindow::getInstance(), "Extract Points Task Warning",
			"Run extract points task after the previous one has finished");
		return;
	}

	int start_frame, end_frame, interval;
	if (!ParameterManager::getInstance().getExtractPointsParameters(interval, start_frame, end_frame))
		return;

	for (int frame = start_frame; frame <= end_frame; frame += interval)
		extract_points_tasks_.push_back(Task(new TaskExtractPoints(frame, interval, save_directory.toStdString())));

	runTasks(extract_points_tasks_, "Extract Points");

	return;
}
