#pragma once
#ifndef FILE_SYSTEM_MODEL_H
#define FILE_SYSTEM_MODEL_H

#include <unordered_map>

#include <QSet>
#include <QHash>
#include <QMutex>
#include <QFileSystemModel>
#include <QPersistentModelIndex>

#include <osg/ref_ptr>
#include <osg/Vec4>

class PointCloud;

class FileSystemModel : public QFileSystemModel
{
  Q_OBJECT

public:
  FileSystemModel();
  virtual ~FileSystemModel();

  QModelIndex setRootPath ( const QString & newPath );

  Qt::ItemFlags flags(const QModelIndex &index) const;
  QVariant data(const QModelIndex &index, int role) const;
  bool setData(const QModelIndex &index, const QVariant &value, int role);
  bool isShown(const std::string& filename) const;

  osg::ref_ptr<PointCloud> getPointCloud(const std::string& filename);
  osg::ref_ptr<PointCloud> getPointCloud(int frame);   
  osg::ref_ptr<PointCloud> getPointCloud(int frame, int view);
  void getFrameRange(int &start, int &end);
  std::string getPointsFolder(int frame);
  std::string getPointsFolder(int frame, int view);
  std::string getImagesFolder(int frame);
  std::string getImagesFolder(int frame, int view);

  std::string getPointsFilename(int frame, int view);
  std::string getPointsFilename(int frame);

  void showPointCloud(const std::string& filename);

  void hidePointCloud(int frame, int view);
  void hidePointCloud(const std::string& filename);

  void updatePointCloud(int frame, int view);
  void updatePointCloud(int frame);
  const osg::Vec4& getColor(void) const {return color_;}

  int getStartFrame(void) const {return start_frame_;}
  int getEndFrame(void) const {return end_frame_;}
  int getViewNumber(void) const {return view_number_;}
  PointCloud* getDisplayFirstFrame(void);

  enum NavigationType
  {
    SWITCH,
    APPEND,
    ERASE
  };
  void navigateToPreviousFrame(NavigationType type);
  void navigateToNextFrame(NavigationType type);
  void navigateToPreviousView(NavigationType type);
  void navigateToNextView(NavigationType type);

  public slots:

    void showPointCloud(int frame, int view);
    void hideAndShowPointCloud(int hide_frame, int hide_view, int show_frame, int show_view);

signals:
    void progressValueChanged(int value);
    void timeToHideAndShowPointCloud(int hide_frame, int hide_view, int show_frame, int show_view);
    void timeToShowPointCloud(int show_frame, int show_view);

private:
  
  Qt::CheckState computeCheckState(const QModelIndex &index) const;
  bool checkRegisterState(const QModelIndex &index) const;
  
  
  void showPointCloud(const QPersistentModelIndex& index);
  void hidePointCloud(const QPersistentModelIndex& index);
  void showPointCloudSceneInformation(void) const;
  
  
  void limitPointCloudCacheSize(void);

  void getDisplayFirstFrameFirstView(int& frame, int& view);
  void getDisplayFirstFrameLastView(int& frame, int& view);
  void getDisplayLastFrameLastView(int& frame, int& view);

  bool recursiveCheck(const QModelIndex &index, const QVariant &value);
  void computeFrameRange(void);

private:

  typedef QHash<QPersistentModelIndex, osg::ref_ptr<PointCloud> > PointCloudMap;
  typedef std::unordered_map<std::string, osg::ref_ptr<PointCloud> > PointCloudCacheMap;

  QSet<QPersistentModelIndex>     checked_indexes_;
  PointCloudMap                   point_cloud_map_;
  PointCloudCacheMap              point_cloud_cache_map_;
  int                             start_frame_;
  int                             end_frame_;
  int							  view_number_;
  osg::Vec4                       color_;
  QMutex                          mutex_;
};
#endif // FILE_SYSTEM_MODEL_H
