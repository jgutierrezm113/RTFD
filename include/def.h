#ifndef DEF_H
#define DEF_H

#include "include.h"
#include "queue.h"

//#define CPU_ONLY

// Size of the convolution box (Don't change)
#define PNET_CONV_SIZE 12

// Limit max amount of threads running PNET stage
#define PNET_MAX_SCALE_COUNT 12

// CAM fps
#define CAM_FPS 30

// Number of stages in the pipeline
#define STAGE_COUNT 6

#define DEBUG_ENABLED (1)

#define MEASURE_TIME (1)

enum Processing_Type { IMG, VID, CAM, DTB, END};

typedef struct {
  // Bounding Box
  cv::Point2f p1;
  cv::Point2f p2;

  // Score
  float score;

  // Bounding Box Regression adjustment
  cv::Point2f dP1;
  cv::Point2f dP2;
} BBox;

typedef struct {
  cv::Point2f LE;
  cv::Point2f RE;
  cv::Point2f N;
  cv::Point2f LM;
  cv::Point2f RM;
} Landmark;

typedef struct conf {
  Processing_Type type;
	bool verbose;
	bool debug;
  bool show_video;
  bool record_video;

  // Only when running
  bool take_snapshot;

  // File name for output writes
	char *full_file_name;
	std::string short_file_name;
} CONF;

extern CONF config;

#endif
