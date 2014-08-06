#include <iostream>
#include <QDomElement>
#include <QDomDocument>
#include <QTextStream>

#include "parameter.h"
#include "main_window.h"
#include "parameter_dialog.h"
#include "file_system_model.h"
#include "parameter_manager.h"

ParameterManager::ParameterManager(void)
  :registration_max_iterations_(new IntParameter("Max Iterations", "Max Iterations", std::numeric_limits<int>::max(), 1, std::numeric_limits<int>::max(),1)),
  registration_max_distance_(new DoubleParameter("Max Distance", "Max Distance", 4, 1, 16, 1.0)),
  start_frame_(new IntParameter("Start frame", "Start frame", -1, -1, -1, 1)),
  end_frame_(new IntParameter("End frame", "End frame", -1, -1, -1, 1)),
  current_frame_(new IntParameter("Current frame", "Current frame", -1, -1, -1, 1)),
  repeat_times_(new IntParameter("Repeat times", "Repeat times", 5, 1, 1000, 1)),
  generator_ctr_threshold_(new IntParameter("CTR Threshold", "CTR Threshold", 25, 1, 255, 1)),
  generator_sat_threshold_(new IntParameter("SAT Threshold", "SAT Threshold", 500, 1, 1000, 1)),
  triangle_length_(new DoubleParameter("Triangle Length", "Triangle Length", 2.5, 1.0, 8.0, 0.1)),
  segment_threshold_(new IntParameter("Segment Threshold", "Segment Threshold", 10, 10, 500, 10)),
  transformation_epsilon_(new DoubleParameter("Transformation Epsilon", "Transformation Epsilon", 100, 1e-8, 100, 1)),
  euclidean_fitness_epsilon_(new DoubleParameter("Euclidean Fitness Epsilon", "Euclidean Fitness Epsilon", 50, 1, 100, 1))
{
}

ParameterManager::~ParameterManager(void)
{

  delete registration_max_distance_;
  delete registration_max_iterations_;
  delete start_frame_;
  delete end_frame_;
  delete transformation_epsilon_;
  delete euclidean_fitness_epsilon_;
}

void ParameterManager::initFrameNumbers(void)
{
  int start_frame, end_frame;
  MainWindow::getInstance()->getFileSystemModel()->getFrameRange(start_frame, end_frame);

  start_frame_->setDefaultValue(start_frame);
  start_frame_->setValue(start_frame);
  start_frame_->setLow(start_frame);
  start_frame_->setHigh(end_frame);

  end_frame_->setDefaultValue(end_frame);
  end_frame_->setValue(end_frame);
  end_frame_->setLow(start_frame);
  end_frame_->setHigh(end_frame);

  current_frame_->setDefaultValue(start_frame);
  current_frame_->setValue(start_frame);
  current_frame_->setLow(start_frame);
  current_frame_->setHigh(end_frame);

  return;
}

double ParameterManager::getRegistrationMaxDistance(void) const
{
  return *registration_max_distance_;
}

void ParameterManager::addFrameParameters(ParameterDialog* parameter_dialog, bool with_frames)
{
  if (!with_frames)
    return;

  parameter_dialog->addParameter(start_frame_);
  parameter_dialog->addParameter(end_frame_);

  return;
}

bool ParameterManager::getFrameParameter(int& frame)
{
  ParameterDialog parameter_dialog("frame Parameter", MainWindow::getInstance());
  parameter_dialog.addParameter(current_frame_);
  if (!parameter_dialog.exec() == QDialog::Accepted)
    return false;

  frame = *current_frame_;
  return true;
}

void ParameterManager::getFrameparametersImpl(int& start_frame, int& end_frame, bool with_frames)
{
  if (!with_frames)
    return;

  if ((int)(*start_frame_) > (int)(*end_frame_))
  {
    int temp = *start_frame_;
    start_frame_->setValue(*end_frame_);
    end_frame_->setValue(temp);
  }

  start_frame = *start_frame_;
  end_frame = *end_frame_;

  return;
}

bool ParameterManager::getGenerationParameters(int& ctr_threshold, int& sat_threshold)
{
	int place_holder_1, place_holder_2;
	return getGenerationParameters(ctr_threshold, sat_threshold, place_holder_1, place_holder_2, false);
}
bool ParameterManager::getGenerationParameters(int& ctr_threshold, int& sat_threshold,
	int& start_frame, int& end_frame, bool with_frames)
{
	ParameterDialog parameter_dialog("Points Generation Parameters", MainWindow::getInstance());
	parameter_dialog.addParameter(generator_ctr_threshold_);
	parameter_dialog.addParameter(generator_sat_threshold_);
	addFrameParameters(&parameter_dialog, with_frames);
	if (!parameter_dialog.exec() == QDialog::Accepted)
		return false;

	ctr_threshold = *generator_ctr_threshold_;
	sat_threshold = *generator_sat_threshold_;
	getFrameparametersImpl(start_frame, end_frame, with_frames);

	return true;
}

bool ParameterManager::getRegistrationLUMParameters(int& segment_threshold, int& max_iterations, double& max_distance)
{
  int place_holder_1, place_holder_2;
  return getRegistrationLUMParameters(segment_threshold, max_iterations, max_distance, place_holder_1, place_holder_2, false);

  return true;
}

bool ParameterManager::getRegistrationLUMParameters(int& segment_threshold, int& max_iterations, double& max_distance,
                                                 int& start_frame, int& end_frame, bool with_frames)
{
  ParameterDialog parameter_dialog("Registration Parameters", MainWindow::getInstance());
  parameter_dialog.addParameter(registration_max_iterations_);
  parameter_dialog.addParameter(registration_max_distance_);
  addFrameParameters(&parameter_dialog, with_frames);
  if (!parameter_dialog.exec() == QDialog::Accepted)
    return false;

  max_iterations = *registration_max_iterations_;
  max_distance = *registration_max_distance_;
  getFrameparametersImpl(start_frame, end_frame, with_frames);

  return true;
}

bool ParameterManager::getRegistrationLUMParameters(int& segment_threshold, int& max_iterations, double& max_distance, int& frame)
{
  ParameterDialog parameter_dialog("Registration Parameters", MainWindow::getInstance());
  parameter_dialog.addParameter(registration_max_iterations_);
  parameter_dialog.addParameter(registration_max_distance_);
  parameter_dialog.addParameter(current_frame_);
  if (!parameter_dialog.exec() == QDialog::Accepted)
    return false;

  max_iterations = *registration_max_iterations_;
  max_distance = *registration_max_distance_;
  frame = *current_frame_;

  return true;
}

bool ParameterManager::getRegistrationICPParameters(int& max_iterations, double& max_distance, int& frame, int& repeat_times)
{
  ParameterDialog parameter_dialog("Registration Parameters", MainWindow::getInstance());
  parameter_dialog.addParameter(registration_max_iterations_);
  parameter_dialog.addParameter(registration_max_distance_);
  parameter_dialog.addParameter(current_frame_);
  parameter_dialog.addParameter(repeat_times_);
  if (!parameter_dialog.exec() == QDialog::Accepted)
    return false;

  max_iterations = *registration_max_iterations_;
  max_distance = *registration_max_distance_;
  frame = *current_frame_;
  repeat_times = *repeat_times_;

  return true;
}

double ParameterManager::getTriangleLength(void) const
{
  return *triangle_length_;
}

bool ParameterManager::getRegistrationParameters(int& frame, int& segment_threshold)
{
  ParameterDialog parameter_dialog("Registration Parameters", MainWindow::getInstance());
  parameter_dialog.addParameter(current_frame_);
  parameter_dialog.addParameter(segment_threshold_);

  if (!parameter_dialog.exec() == QDialog::Accepted)
    return false;

  segment_threshold = *segment_threshold_;
  frame = *current_frame_;

  return true;
}


bool ParameterManager::getAutomaticRegistrationParameters(int& frame, int& segment_threshold, int& max_iterations, int& repeat_times, 
  double& max_distance, double& transformation_epsilon, double& euclidean_fitness_epsilon)

{
  ParameterDialog parameter_dialog("Registration Parameters", MainWindow::getInstance());
  parameter_dialog.addParameter(current_frame_);
  parameter_dialog.addParameter(segment_threshold_);
  parameter_dialog.addParameter(registration_max_iterations_);
  parameter_dialog.addParameter(registration_max_distance_);
  parameter_dialog.addParameter(repeat_times_);
  parameter_dialog.addParameter(transformation_epsilon_);
  parameter_dialog.addParameter(euclidean_fitness_epsilon_);

  if (!parameter_dialog.exec() == QDialog::Accepted)
    return false;

  segment_threshold = *segment_threshold_;
  frame = *current_frame_;
  max_iterations = *registration_max_iterations_;
  max_distance = *registration_max_distance_;
  repeat_times = *repeat_times_;
  transformation_epsilon = *transformation_epsilon_;
  euclidean_fitness_epsilon = *euclidean_fitness_epsilon_;

  return true;
}
