﻿#pragma once
#ifndef TASK_DISPATCHER_H_
#define TASK_DISPATCHER_H_

#include <vector>
#include <QMutex>
#include <QObject>
#include <boost/shared_ptr.hpp>

#include "point_cloud.h"
#include "types.h"

class TaskImpl
{
public:
  TaskImpl(int frame, int view);
  virtual ~TaskImpl(void);

  virtual void run(void) const = 0;

protected:
  friend class Task;
  int frame_;
  int view_;
};

class Task : public QObject
{
  Q_OBJECT

public:
  Task(void);
  Task(TaskImpl* task_impl);
  Task(const Task &other);
  Task& operator=(const Task &other);
  virtual ~Task(void);

  bool run(void) const;

signals:
  void finished(int frame, int view) const;

private:
  boost::shared_ptr<TaskImpl> task_impl_;
};


class TaskPointsGeneration : public TaskImpl
{
public:
  TaskPointsGeneration(int frame, int view, int ctr_threshold, int sat_threshold);
  virtual ~TaskPointsGeneration();

  static QString getExeFilename(void);
  virtual void run(void) const;

private:
  int ctr_threshold_;
  int sat_threshold_;

  void convertImages(void) const;
  void deleteImages(void) const;
  void colorizePoints(void) const;
};

class TaskRegistration : public TaskImpl
{
public:
  TaskRegistration(int frame, int segment_threshold, int max_iterations, double max_distance);
  virtual ~TaskRegistration();

  virtual void run(void) const;

private:
  int segment_threshold_;
  int max_iterations_;
  double max_distance_;
};

class TaskDispatcher : public QObject
{
  Q_OBJECT

public:
  TaskDispatcher(QObject* parent=0);
  virtual ~TaskDispatcher(void);

  bool isRunning(void) const;

public slots:
  void cancelRunningTasks(bool wait=false);
  void dispatchTaskPointsGeneration(void);
  void dispatchTaskRegistration(void);
  void updateDisplayQueue(int frame, int view);
  void clearDisplayQueue(void);
  void removeFinishedWatchers(void);

protected:
  void runTasks(QList<Task>& tasks, const QString& task_name, bool display = true);

protected slots:

private:
  QList<Task>                         points_generation_tasks_;
  QList<Task>                         registration_tasks_;

  std::vector<QObject*>               active_watchers_;
  typedef std::list<std::pair<int, int> > DisplayQueue;
  DisplayQueue                        display_queue_;

  int                              start_frame_;
  int                              end_frame_;
  int                              frame_delta_;
  int                              frame_multiple_;
  int                              camera_number_;
  int                              stop_delta_;
  int                              camera_delta_;

  QString                          root_folder_;

  mutable QMutex                      mutex_;
};

#endif /*TASK_DISPATCHER_H_*/