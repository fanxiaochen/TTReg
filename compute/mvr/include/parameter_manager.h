#pragma once
#ifndef PARAMETER_MANAGER_H_
#define PARAMETER_MANAGER_H_

#include <QString>

class IntParameter;
class DoubleParameter;
class BoolParameter;
class ParameterDialog;

class ParameterManager
{
public:
  static ParameterManager& getInstance() {
    static ParameterManager theSingleton;
    return theSingleton;
  }

  void initFrameNumbers(void);


  double getRegistrationMaxDistance(void) const;
  double getTriangleLength(void) const;

  bool getFrameParameter(int& frame);
  bool getFrameParameters(int& start_frame, int& end_frame, int& downsampling);

  bool getGenerationParameters(int& ctr_threshold, int& sat_threshold);
  bool getGenerationParameters(int& ctr_threshold, int& sat_threshold,
	  int& start_frame, int& end_frame, bool with_frames=true);

  bool getRegistrationLUMParameters(int& segment_threshold, int& max_iterations, double& max_distance);
  bool getRegistrationLUMParameters(int& segment_threshold, int& max_iterations, double& max_distance,
    int& start_frame, int& end_frame, bool with_frames=true);
  bool getRegistrationLUMParameters(int& segment_threshold, int& max_iterations, double& max_distance, int& frame);
  bool getRegistrationICPParameters(int& max_iterations, double& max_distance, int& frame, int& repeat_times);
  bool getRegistrationParameters(int& frame, int& segment_threshold);
  
  bool getDenoiseParameters(int& segment_threshold, int& start_frame, int& end_frame, bool with_frames=true);
  bool getDataCutParameters(int& start_frame, int& end_frame, bool with_frames=true);
  bool getExtractImagesParameters(int& view_number, int& start_frame, int& end_frame, bool with_frames=true);
  bool getDownsamplingParameters(int& sample_ratio, int& start_frame, int& end_frame, bool with_frames=true);
  bool getExtractPointsParameters(int& interval, int& start_frame, int& end_frame, bool with_frames=true);

protected:
  void addFrameParameters(ParameterDialog* parameter_dialog, bool with_frames);
  void getFrameparametersImpl(int& start_frame, int& end_frame, bool with_frames);
private:
  ParameterManager(void);
  ParameterManager(const ParameterManager &) {}            // copy ctor hidden
  ParameterManager& operator=(const ParameterManager &) {return (*this);}   // assign op. hidden
  virtual ~ParameterManager();

  IntParameter*                                       registration_max_iterations_;
  DoubleParameter*                                    registration_max_distance_;

  IntParameter*                                       start_frame_;
  IntParameter*                                       end_frame_;
  IntParameter*                                       current_frame_;

  IntParameter*                                       repeat_times_;

  DoubleParameter*                                    triangle_length_;
  IntParameter*                                       segment_threshold_;
  IntParameter*										  view_number_;
  IntParameter*										  sample_ratio_;
  IntParameter*										  interval_;

  DoubleParameter*                                    transformation_epsilon_;
  DoubleParameter*                                    euclidean_fitness_epsilon_;

  IntParameter*                                       generator_ctr_threshold_;
  IntParameter*                                       generator_sat_threshold_;

};

#endif // PARAMETER_MANAGER_H_
