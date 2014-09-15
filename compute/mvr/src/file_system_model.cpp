#include <QDir>
#include <QColor>
#include <QMutexLocker>
#include <QColorDialog>
#include <QFutureWatcher>
#include <QtConcurrentRun>

#include <osg/Group>
#include <osgDB/ReadFile>

#include "main_window.h"
#include "point_cloud.h"
#include "parameter_manager.h"
#include "osg_viewer_widget.h"
#include "file_system_model.h"


FileSystemModel::FileSystemModel()
  :start_frame_(-1),
  end_frame_(-1),
  view_number_(-1)
{
  setNameFilterDisables(false);
  QStringList allowed_file_extensions;
  allowed_file_extensions.push_back("*.pcd");
  allowed_file_extensions.push_back("*.obj");     
  setNameFilters(allowed_file_extensions);

  connect(this, SIGNAL(timeToHideAndShowPointCloud(int, int, int, int)), this, SLOT(hideAndShowPointCloud(int, int, int, int)));
  connect(this, SIGNAL(timeToShowPointCloud(int, int)), this, SLOT(showPointCloud(int, int)));
}

FileSystemModel::~FileSystemModel()
{
}

Qt::ItemFlags FileSystemModel::flags(const QModelIndex &index) const
{
  return QFileSystemModel::flags(index) | Qt::ItemIsUserCheckable;
}

QVariant FileSystemModel::data(const QModelIndex &index, int role) const
{
  if(role == Qt::CheckStateRole)
    return computeCheckState(index);
  else
  {
    if(role == Qt::ForegroundRole && checkRegisterState(index))
      return QBrush(QColor(255, 0, 0));
    return QFileSystemModel::data(index, role);
  }
}

bool FileSystemModel::checkRegisterState(const QModelIndex &index) const
{
  PointCloudCacheMap::const_iterator it = point_cloud_cache_map_.find(filePath(index).toStdString());
  if (it != point_cloud_cache_map_.end())
    return it->second.get()->isRegistered();
  else
    return false;
}

Qt::CheckState FileSystemModel::computeCheckState(const QModelIndex &index) const
{
  if(!hasChildren(index))
    return (checked_indexes_.contains(index)) ? (Qt::Checked) : (Qt::Unchecked);

  bool all_checked = true;
  bool all_unchecked = true;
  for(int i = 0, i_end = rowCount(index); i < i_end; i ++)
  {
    QModelIndex child = QFileSystemModel::index(i, 0, index);
    Qt::CheckState check_state = computeCheckState(child);
    if (check_state == Qt::PartiallyChecked)
      return check_state;

    if (check_state == Qt::Checked)
      all_unchecked = false;
    if (check_state == Qt::Unchecked)
      all_checked = false;

    if (!all_checked && !all_unchecked)
      return Qt::PartiallyChecked;
  }

  if (all_unchecked)
    return Qt::Unchecked;

  return Qt::Checked;
}

bool FileSystemModel::isShown(const std::string& filename) const
{
  QModelIndex index = this->index(filename.c_str());
  return (checked_indexes_.contains(index)) ? (true) : (false);
}

bool FileSystemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  bool is_point_cloud = (filePath(index).right(3) == "pcd");
  if(role == Qt::CheckStateRole)
  {
    if (is_point_cloud)
    {
      if(value == Qt::Checked)
        showPointCloud(index);
      else
        hidePointCloud(index);
    }

    if(hasChildren(index) == true)
      recursiveCheck(index, value);

    emit dataChanged(index, index);
    return true;
  }

  return QFileSystemModel::setData(index, value, role);
}

void FileSystemModel::limitPointCloudCacheSize(void)
{
  size_t threshold = 64;

  if (point_cloud_cache_map_.size() <= threshold)
    return;

  std::set<osg::ref_ptr<PointCloud> > in_use_clouds;
  std::vector<osg::ref_ptr<PointCloud> > freeable_clouds;

  for (PointCloudMap::const_iterator it = point_cloud_map_.begin(); it != point_cloud_map_.end(); ++ it)
    in_use_clouds.insert(*it);

  for (PointCloudCacheMap::const_iterator it = point_cloud_cache_map_.begin(); it != point_cloud_cache_map_.end(); ++ it)
    if (in_use_clouds.find(it->second) == in_use_clouds.end() && it->second->referenceCount() == 1)
      freeable_clouds.push_back(it->second);

  for (size_t i = 0, i_end = freeable_clouds.size(); i < i_end; ++ i)
    point_cloud_cache_map_.erase(freeable_clouds[i]->getFilename());

  return;
}

std::string FileSystemModel::getImagesFolder(int frame)
{
	std::string points_folder = getPointsFolder(frame);
	if (points_folder.empty())
		return points_folder;

	QString image_folder = QString(points_folder.c_str()).replace("/points/", "/images/");
	if (!QDir(image_folder).exists())
		return std::string();

	return image_folder.toStdString();
}

std::string FileSystemModel::getImagesFolder(int frame, int view)
{
	std::string frame_folder = getImagesFolder(frame);
	if (frame_folder.empty())
		return frame_folder;

	QString view_folder = QString("%1/slice_%2").arg(frame_folder.c_str()).arg(view, 2, 10, QChar('0'));
	if (!QDir(view_folder).exists())
		view_folder = QString("%1/view_%2").arg(frame_folder.c_str()).arg(view, 2, 10, QChar('0'));

	return view_folder.toStdString();
}

osg::ref_ptr<PointCloud> FileSystemModel::getPointCloud(const std::string& filename)
{
  QMutexLocker locker(&mutex_);

  limitPointCloudCacheSize();

  QFileInfo fileinfo(filename.c_str());
  if (!fileinfo.exists() || !fileinfo.isFile())
    return NULL;

  PointCloudCacheMap::iterator it = point_cloud_cache_map_.find(filename);
  if (it != point_cloud_cache_map_.end())
    return it->second.get();

  osg::ref_ptr<PointCloud> point_cloud(new PointCloud());
  if (!point_cloud->open(filename))
    return NULL;

  if (point_cloud->thread() != qApp->thread())
    point_cloud->moveToThread(qApp->thread());

  point_cloud_cache_map_[filename] = point_cloud;

  return point_cloud;
}

QModelIndex FileSystemModel::setRootPath ( const QString & newPath )
{
  point_cloud_cache_map_.clear();
  point_cloud_map_.clear();
  checked_indexes_.clear();

  QModelIndex index = QFileSystemModel::setRootPath(newPath);
  computeFrameRange();
  computeViewNumber();
  if (start_frame_ != -1)
  {
    if (getPointCloud(start_frame_) != NULL)
      showPointCloud(start_frame_, view_number_);
    else if (getPointCloud(start_frame_, 0) != NULL)
      showPointCloud(start_frame_, 0);

    MainWindow::getInstance()->getOSGViewerWidget()->centerScene();
  }

  return index;
}

static void extractStartEndFrame(const QStringList& entries, int& start_frame, int& end_frame)
{
  start_frame = std::numeric_limits<int>::max();
  end_frame = std::numeric_limits<int>::min();

  for (QStringList::const_iterator entries_it = entries.begin();
    entries_it != entries.end(); ++ entries_it)
  {
    if (!entries_it->contains("frame_"))
      continue;


    int index = entries_it->right(4).toInt();
    if (start_frame > index)
      start_frame = index;
    if (end_frame < index)
      end_frame = index;
  }

  return;
}

static void extractViews(const QStringList& entries, int& view_number)
{
	int view_count = 0;
	for (QStringList::const_iterator entries_it = entries.begin();
		entries_it != entries.end(); ++ entries_it)
	{
		if (entries_it->contains("view_"))
			view_count++;
	}
	view_number = view_count;

	return;
}

void FileSystemModel::computeFrameRange(void)
{
  start_frame_ = end_frame_ = -1;

  QString root_path = rootPath();
  QModelIndex root_index = index(root_path);

  if (root_path.contains("frame_")) {
    start_frame_ = end_frame_ = root_path.right(4).toInt();
    return;
  }

  if (root_path.compare("points") == 0)
  {
    QStringList points_entries = QDir(root_path).entryList();
    extractStartEndFrame(points_entries, start_frame_, end_frame_);
    return;
  }

  QStringList root_entries = QDir(root_path).entryList();
  for (QStringList::const_iterator root_entries_it = root_entries.begin();
    root_entries_it != root_entries.end(); ++ root_entries_it)
  {
    if (root_entries_it->compare("points") != 0)
      continue;

    QStringList points_entries = QDir(root_path+"/"+*root_entries_it).entryList();
    extractStartEndFrame(points_entries, start_frame_, end_frame_);
    return;
  }

  return;
}

void FileSystemModel::computeViewNumber(void)
{
	QString root_path = rootPath();
	QModelIndex root_index = index(root_path);

	QString init_frame("frame_00000");

	if (root_path.contains("points"))
	{
		QString view_root = root_path + "/" + init_frame;
		QStringList views_entries = QDir(view_root).entryList();
		extractViews(views_entries, view_number_);
		return;
	}

	QStringList root_entries = QDir(root_path).entryList();
	for (QStringList::const_iterator root_entries_it = root_entries.begin();
		root_entries_it != root_entries.end(); ++ root_entries_it)
	{
		if (root_entries_it->compare("points") != 0)
			continue;
		
		QString view_root = root_path + "/points/" + init_frame;
		QStringList views_entries = QDir(view_root).entryList();
		extractViews(views_entries, view_number_);
		return;
	}

}

void FileSystemModel::getFrameRange(int &start, int &end)
{
  start = start_frame_;
  end = end_frame_;
}

osg::ref_ptr<PointCloud> FileSystemModel::getPointCloud(int frame)
{
  return getPointCloud(getPointsFilename(frame));
}

std::string FileSystemModel::getPointsFolder(int frame)
{
  QModelIndex root_index = index(rootPath());

  int start, end;
  getFrameRange(start, end);
  if (start < 0 || end < 0)
    return std::string();

  std::string folder;

  QString root_path = rootPath();
  if (root_path.contains("frame_"))
  {
    folder =  root_path.toStdString();
  }
  else if (root_path.compare("points") == 0)
  {
    QModelIndex frame_index = index(frame-start, 0, root_index);
    folder = filePath(frame_index).toStdString();
  }
  else
  {
    QStringList root_entries = QDir(root_path).entryList();
    for (QStringList::const_iterator root_entries_it = root_entries.begin();
      root_entries_it != root_entries.end(); ++ root_entries_it)
    {
      if (root_entries_it->compare("points") != 0)
        continue;

      folder = (root_path+QString("/%1/frame_%2").arg(*root_entries_it).arg(frame, 5, 10, QChar('0'))).toStdString();
      break;
    }
  }

  return folder;
}

std::string FileSystemModel::getPointsFolder(int frame, int view)
{
  std::string frame_folder = getPointsFolder(frame);
  if (frame_folder.empty() || view < 0 || view >= view_number_)
    return frame_folder;

  QString view_folder = QString("%1/view_%2").arg(frame_folder.c_str()).arg(view, 2, 10, QChar('0'));
  if (!QDir(view_folder).exists())
    view_folder = QString("%1/slice_%2").arg(frame_folder.c_str()).arg(view, 2, 10, QChar('0'));

  return view_folder.toStdString();
}

std::string FileSystemModel::getPointsFilename(int frame, int view)
{
  std::string folder = getPointsFolder(frame, view);
  if (folder.empty())
    return folder;

  return folder+"/points.pcd";
}

std::string FileSystemModel::getPointsFilename(int frame)
{
  return getPointsFilename(frame, view_number_);
}

osg::ref_ptr<PointCloud> FileSystemModel::getPointCloud(int frame, int view)
{
  return getPointCloud(getPointsFilename(frame, view));
}

void FileSystemModel::showPointCloud(int frame, int view)
{
  showPointCloud(getPointsFilename(frame, view));
}

void FileSystemModel::showPointCloud(const std::string& filename)
{
  QModelIndex index = this->index(QString(filename.c_str()));
  if (!index.isValid())
    return;

  showPointCloud(index);

  return;
}

void FileSystemModel::showPointCloud(const QPersistentModelIndex& index)
{
  checked_indexes_.insert(index);

  osg::ref_ptr<PointCloud> point_cloud(getPointCloud(filePath(index).toStdString()));
  if (!point_cloud.valid())
    return;

  PointCloudMap::iterator point_cloud_map_it = point_cloud_map_.find(index);
  if (point_cloud_map_it != point_cloud_map_.end())
    return;

  MainWindow::getInstance()->getOSGViewerWidget()->addChild(point_cloud);
  point_cloud_map_[index] = point_cloud;

  showPointCloudSceneInformation();

  return;
}

void FileSystemModel::hidePointCloud(const std::string& filename)
{
  QModelIndex index = this->index(QString(filename.c_str()));
  if (!index.isValid())
    return;

  hidePointCloud(index);
}

void FileSystemModel::hidePointCloud(int frame, int view)
{
  hidePointCloud(getPointsFilename(frame, view));
}

void FileSystemModel::hidePointCloud(const QPersistentModelIndex& index)
{
  checked_indexes_.remove(index);

  PointCloudMap::iterator point_cloud_map_it = point_cloud_map_.find(index);
  if (point_cloud_map_it == point_cloud_map_.end())
    return;

  MainWindow::getInstance()->getOSGViewerWidget()->removeChild(point_cloud_map_it.value().get());
  point_cloud_map_.erase(point_cloud_map_it);

  showPointCloudSceneInformation();

  return;
}

void FileSystemModel::hideAndShowPointCloud(int hide_frame, int hide_view, int show_frame, int show_view)
{
  bool to_hide = true;
  bool to_show = true;

  osg::ref_ptr<PointCloud> show_cloud = getPointCloud(show_frame, show_view);
  osg::ref_ptr<PointCloud> hide_cloud = getPointCloud(hide_frame, hide_view);

  QModelIndex show_index = this->index(QString(getPointsFilename(show_frame, show_view).c_str()));
  checked_indexes_.insert(show_index);
  PointCloudMap::iterator point_cloud_map_it = point_cloud_map_.find(show_index);
  if (point_cloud_map_it == point_cloud_map_.end())
  {
    if (show_cloud != NULL)
      point_cloud_map_[show_index] = show_cloud;
    else
      to_show = false;
  }
  else
    to_show = false;

  QModelIndex hide_index = this->index(QString(getPointsFilename(hide_frame, hide_view).c_str()));
  checked_indexes_.remove(hide_index);
  point_cloud_map_it = point_cloud_map_.find(hide_index);
  if (point_cloud_map_it != point_cloud_map_.end())
    point_cloud_map_.erase(point_cloud_map_it);
  else
    to_hide = false;

  OSGViewerWidget* osg_viewer_widget = MainWindow::getInstance()->getOSGViewerWidget();
  if (to_hide && to_show)
    osg_viewer_widget->replaceChild(hide_cloud, show_cloud, true);
  else if (to_hide)
    osg_viewer_widget->removeChild(getPointCloud(hide_frame, hide_view), true);
  else if (to_show)
    osg_viewer_widget->addChild(getPointCloud(show_frame, show_view), true);

  showPointCloudSceneInformation();

  return;
}

void FileSystemModel::showPointCloudSceneInformation(void) const
{
  QString information("Displaying Point Cloud:\n");

  if (point_cloud_map_.empty())
  {
 //   MainWindow::getInstance()->getInformation()->setText(information.toStdString(), 20, 20);
    return;
  }

  std::vector<std::pair<int, int> > sorted_scene_info;
  for (PointCloudMap::const_iterator it = point_cloud_map_.begin(); it != point_cloud_map_.end(); ++ it)
  {
    int frame = (*it)->getFrame();
    int view = (*it)->getView();
    sorted_scene_info.push_back(std::make_pair(frame, view));
  }

  std::sort(sorted_scene_info.begin(), sorted_scene_info.end());
  for (size_t i = 0, i_end = sorted_scene_info.size(); i < i_end; ++ i)
  {
    int frame = sorted_scene_info[i].first;
    int view = sorted_scene_info[i].second;
    if (view < view_number_)
      information += QString("frame %1 View %2\n").arg(frame, 5, 10, QChar('0')).arg(view, 2, 10, QChar('0'));
    else
      information += QString("frame %1\n").arg(frame, 5, 10, QChar('0'));
  }
//  MainWindow::getInstance()->getInformation()->setText(information.toStdString(), 20, 20);

  return;
}

PointCloud* FileSystemModel::getDisplayFirstFrame(void)
{
  if (point_cloud_map_.empty())
    return NULL;

  osg::ref_ptr<PointCloud> first_cloud = *point_cloud_map_.begin();
  int frame = first_cloud->getFrame();
  int view = first_cloud->getView();

  for (PointCloudMap::const_iterator it = point_cloud_map_.begin(); it != point_cloud_map_.end(); ++ it)
  {
    osg::ref_ptr<PointCloud> point_cloud = *it;
    int this_frame = point_cloud->getFrame();
    int this_view = point_cloud->getView();
    if (this_frame < frame)
    {
      frame = this_frame;
      view = this_view;
    }
    else if(this_frame == frame && this_view > view)
    {
      frame = this_frame;
      view = this_view;
    }
  }

  if (view != view_number_)
    return NULL;

  return getPointCloud(frame);
}

void FileSystemModel::getDisplayFirstFrameFirstView(int& frame, int& view)
{
  if (point_cloud_map_.empty())
  {
    frame = -1;
    view = -1;
    return;
  }

  osg::ref_ptr<PointCloud> first_cloud = *point_cloud_map_.begin();
  frame = first_cloud->getFrame();
  view = first_cloud->getView();

  for (PointCloudMap::const_iterator it = point_cloud_map_.begin(); it != point_cloud_map_.end(); ++ it)
  {
    osg::ref_ptr<PointCloud> point_cloud = *it;
    int this_frame = point_cloud->getFrame();
    int this_view = point_cloud->getView();
    if (this_frame < frame)
    {
      frame = this_frame;
      view = this_view;
    }
    else if(this_frame == frame && this_view < view)
    {
      frame = this_frame;
      view = this_view;
    }
  }
  return;
}

void FileSystemModel::getDisplayFirstFrameLastView(int& frame, int& view)
{
  if (point_cloud_map_.empty())
  {
    frame = -1;
    view = -1;
    return;
  }

  std::vector<std::pair<int, int> > display_items;
  for (PointCloudMap::const_iterator it = point_cloud_map_.begin(); it != point_cloud_map_.end(); ++ it)
  {
    osg::ref_ptr<PointCloud> point_cloud = *it;
    display_items.push_back(std::make_pair(point_cloud->getFrame(), point_cloud->getView()));
  }

  frame = display_items[0].first;
  view = display_items[0].second;
  for (size_t i = 1, i_end = display_items.size(); i < i_end; ++ i)
  {
    int this_frame = display_items[i].first;
    if (this_frame > frame)
      frame = this_frame;
  }
  for (size_t i = 1, i_end = display_items.size(); i < i_end; ++ i)
  {
    int this_frame = display_items[i].first;
    int this_view = display_items[i].second;
    if (this_frame == frame && this_view > view)
      view = this_view;
  }
  return;
}

void FileSystemModel::getDisplayLastFrameLastView(int& frame, int& view)
{
  if (point_cloud_map_.empty())
  {
    frame = -1;
    view = -1;
    return;
  }

  osg::ref_ptr<PointCloud> first_cloud = *point_cloud_map_.begin();
  frame = first_cloud->getFrame();
  view = first_cloud->getView();

  for (PointCloudMap::const_iterator it = point_cloud_map_.begin(); it != point_cloud_map_.end(); ++ it)
  {
    osg::ref_ptr<PointCloud> point_cloud = *it;
    int this_frame = point_cloud->getFrame();
    int this_view = point_cloud->getView();
    if (this_frame > frame)
    {
      frame = this_frame;
      view = this_view;
    }
    else if(this_frame == frame && this_view > view)
    {
      frame = this_frame;
      view = this_view;
    }
  }
  return;
}

bool FileSystemModel::recursiveCheck(const QModelIndex &index, const QVariant &value)
{
  if(!hasChildren(index))
    return false;

  for(int i = 0, i_end = rowCount(index); i < i_end; i ++)
  {
    QModelIndex child = QFileSystemModel::index(i, 0, index);
    setData(child, value, Qt::CheckStateRole);
  }

  return true;
}

void FileSystemModel::updatePointCloud(int frame, int view)
{
  std::string filename = getPointsFilename(frame, view);
  if (point_cloud_cache_map_.find(filename) == point_cloud_cache_map_.end())
    return;

  getPointCloud(frame, view)->reload();

  return;
}

void FileSystemModel::updatePointCloud(int frame)
{
  std::string filename = getPointsFilename(frame);
  if (point_cloud_cache_map_.find(filename) == point_cloud_cache_map_.end())
    return;

  getPointCloud(frame)->reload();

  return;
}


void FileSystemModel::navigateToPreviousFrame(NavigationType type)
{
  int first_frame, first_view;
  getDisplayFirstFrameFirstView(first_frame, first_view);

  if (first_frame == -1 || first_view == -1)
  {
    showPointCloud(getStartFrame(), view_number_);
    return;
  }

  if (type == ERASE)
  {
    hidePointCloud(first_frame, first_view);
    return;
  }

  int current_frame = first_frame-1;
  if (current_frame < getStartFrame())
    return;

  if (type == APPEND)
    showPointCloud(current_frame, first_view);
  else
    hideAndShowPointCloud(first_frame, first_view, current_frame, first_view);

  return;
}

void FileSystemModel::navigateToNextFrame(NavigationType type)
{
  int last_frame, last_view;
  getDisplayLastFrameLastView(last_frame, last_view);

  if (last_frame == -1 || last_view == -1)
  {
    showPointCloud(getEndFrame(), view_number_);
    return;
  }

  if (type == ERASE)
  {
    hidePointCloud(last_frame, last_view);
    return;
  }

  int current_frame = last_frame+1;
  if (current_frame > getEndFrame())
    return;

  if (type == APPEND)
    showPointCloud(current_frame, last_view);
  else
    hideAndShowPointCloud(last_frame, last_view, current_frame, last_view);

  return;
}

void FileSystemModel::navigateToPreviousView(NavigationType type)
{
  int first_frame, first_view;
  getDisplayFirstFrameFirstView(first_frame, first_view);

  if (first_frame == -1 || first_view == -1)
  {
    showPointCloud(getStartFrame(), 0);
    return;
  }

  if (type == ERASE)
  {
    hidePointCloud(first_frame, first_view);
    return;
  }

  int current_view = first_view-1;
  if (current_view < 0)
    return;

  if (type == APPEND)
    showPointCloud(first_frame, current_view);
  else
    hideAndShowPointCloud(first_frame, first_view, first_frame, current_view);

  return;
}

void FileSystemModel::navigateToNextView(NavigationType type)
{
  int first_frame, last_view;
  getDisplayFirstFrameLastView(first_frame, last_view);

  if (first_frame == -1 || last_view == -1)
  {
    showPointCloud(getStartFrame(), 11);
    return;
  }

  if (type == ERASE)
  {
    hidePointCloud(first_frame, last_view);
    return;
  }

  int current_view = last_view+1;
  if (current_view > view_number_)
    return;

  if (type == APPEND)
    showPointCloud(first_frame, current_view);
  else
    hideAndShowPointCloud(first_frame, last_view, first_frame, current_view);

  return;
}
