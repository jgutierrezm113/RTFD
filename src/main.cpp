
#include "../include/include.h"
#include "../include/def.h"
#include "../include/data.h"
#include "../include/queue.h"
#include "../include/auxiliar.h"
#include "../include/pthreads.h"

using namespace cv;
using namespace std;

CONF config;

double fps;

// Shared string with output thread to print information
string output_string;

void usage() {
	fprintf(stderr,"\n");
	fprintf(stderr,"Usage:  face-detector [option]\n");
	fprintf(stderr, "\nOptions:\n");
	fprintf(stderr, "    --help,-h      print this message\n");
	fprintf(stderr, "    --verbose,-b   basic verbosity level\n");
	fprintf(stderr, "    --debug,-d     enhanced verbosity level\n");
	fprintf(stderr, "\nOther:\n");
	fprintf(stderr, "    --image,-i <file_name> Process image file\n");
	fprintf(stderr, "    --video,-v <file_name> Process video file\n");
	fprintf(stderr, "    No -i or -v option used, will process CAM\n");
	fprintf(stderr, "    --nodisp,-nd           Don't display image/video\n");
	fprintf(stderr, "    --record,-r            Record video into output folder\n");
}

void init_conf() {
	config.type = CAM;
	config.show_video = true;
	config.record_video = false;
	config.take_snapshot = false;
	config.verbose = false;
	config.debug = false;
  config.short_file_name = "CAM";
}

int parse_arguments(int argc, char** argv) {

  int i = 1;
	while ( i < argc ) {
		if ( strcmp(argv[i], "-h")==0 || strcmp(argv[i], "--help")==0 ) {
			usage();
			return 0;
		}
		else if ( strcmp(argv[i], "-b")==0 || strcmp(argv[i], "--verbose")==0 ) {
			config.verbose = 1;
		}
		else if ( strcmp(argv[i], "-d")==0 || strcmp(argv[i], "--debug")==0 ) {
			config.debug = 1;
		}
		else if ( strcmp(argv[i], "-i")==0 || strcmp(argv[i], "--image")==0 ) {
			++i;
			config.full_file_name = argv[i];
      config.type = IMG;
      config.take_snapshot = true;
		}
		else if ( strcmp(argv[i], "-v")==0 || strcmp(argv[i], "--video")==0 ) {
			++i;
			config.full_file_name = argv[i];
      config.type = VID;
		}
		else if ( strcmp(argv[i], "-nd")==0 || strcmp(argv[i], "--nodisp")==0 ) {
			config.show_video = false;
		}
		else if ( strcmp(argv[i], "-r")==0 || strcmp(argv[i], "--record")==0 ) {
			config.record_video = true;
		}
		++i;
	}

	if (config.type == VID || config.type == IMG){
	  const std::string text = config.full_file_name;
	  const std::regex slashrm(".*/");
	  std::stringstream result;
	  std::regex_replace(std::ostream_iterator<char>(result), text.begin(), text.end(), slashrm, "");
	  config.short_file_name = result.str();
	  config.short_file_name = config.short_file_name.substr(0, config.short_file_name.find_last_of("."));
	}

	return 1;
}

void print_conf() {
	fprintf(stderr, "\nCONFIGURATION:\n");
	fprintf(stderr, "- Processing Type %d\n", config.type);
	fprintf(stderr, "- Show Video:     %d\n", config.show_video);
  fprintf(stderr, "- Record Video:   %d\n", config.record_video);
  fprintf(stderr, "- Take Snapshot:  %d\n", config.take_snapshot);
  fprintf(stderr, "- File Name:      %s\n", config.short_file_name.c_str());
	if (config.verbose && config.debug) fprintf(stderr, "- verbose mode\n");
 	if (config.debug) fprintf(stderr, "- debug mode\n");
}

int main(int argc, char* argv[]) {

	cv::VideoCapture video;
	cv::Mat img;

	// Parse arguments
	init_conf();
	if ( !parse_arguments(argc, argv) ) return 0;
	#if(DEBUG_ENABLED)
		print_conf();
	#endif

	// Start ncurses lib
	// Needed to process input keyboard correctly
	initscr();

	// Create Pthreads
	pthread_t* pthreads = new pthread_t [STAGE_COUNT];
	std::vector<int> pthread_id(STAGE_COUNT);
	for (int i = 0; i < STAGE_COUNT; i++){
	        pthread_id[i] = i;
	}

	pthread_create(&pthreads[0], NULL, preprocess   , (void *)&pthread_id[0]);
	pthread_create(&pthreads[1], NULL, pnet         , (void *)&pthread_id[1]);
	pthread_create(&pthreads[2], NULL, rnet         , (void *)&pthread_id[2]);
	pthread_create(&pthreads[3], NULL, onet         , (void *)&pthread_id[3]);
	pthread_create(&pthreads[4], NULL, postprocess  , (void *)&pthread_id[4]);
	pthread_create(&pthreads[5], NULL, output       , (void *)&pthread_id[5]);

	switch (config.type){
	  case CAM: {
      // open the video file for reading
      // Check if CAM available
      video.open(0);
			if (!video.isOpened()) { // if not success, exit program
				endwin();
				cout << "Cannot open the video" << endl;
				Data* Packet = new Data;
				Packet->type = END;
				ptr_queue[0].Insert(Packet);
				break;
			}

			fps = CAM_FPS;
			
			video.set(CV_CAP_PROP_FPS, fps);
			video.set(CV_CAP_PROP_FRAME_WIDTH,1920);
			video.set(CV_CAP_PROP_FRAME_HEIGHT,1080);

			timeout (100/fps);

			double dWidth   = video.get(CV_CAP_PROP_FRAME_WIDTH); //get the width of frames of the video
			double dHeight  = video.get(CV_CAP_PROP_FRAME_HEIGHT); //get the height of frames of the video

			bool finished = 0;
			while (!finished){
				// Read packet
				Data* Packet = new Data;

				bool bSuccess = video.read(Packet->frame); // read a new frame from video

				if (!bSuccess) {//if not success, break loop
					printw("Cannot read a frame from video stream\n");
					Packet->type = END;
					ptr_queue[0].Insert(Packet);
					break;
				}

				// Control screen
				clear();
				printw("Face Detection by JGM\n");
				printw("---------------------\n");

				#if(DEBUG_ENABLED)
					printw("Frame size : %lf x %lf\n", dWidth, dHeight);
					printw("Frame rate : %lf\n", fps);
				#endif

				printw("[%d] Show Video:    Press 'v' to change.\n", (config.show_video)?1:0);
				printw("[%d] Record Video:  Press 'r' to toggle recording.\n", (config.record_video)?1:0);
				printw("[%d] Take Snapshot: Press 's' to take snapshot.\n", (config.take_snapshot)?1:0);
				if (!output_string.empty())
					printw("%s", output_string.c_str());
				printw("Press 'q' to quit.\n");

				Packet->type = VID;

				// Record time
				#if(MEASURE_TIME)
					Packet->start_time = CLOCK();
				#endif

				ptr_queue[0].Insert(Packet);

				int c = getch();
				switch (c){
					case 113: { // 'q'
						// Quit
						Data* FinishPacket = new Data;
						FinishPacket->type = END;
						ptr_queue[0].Insert(FinishPacket);
						finished = 1;
						break;
					}
					case 114: { // 'r'
						// Record Video
						config.record_video = !config.record_video;
						break;
					}
					case 115: { // 's'
						// Take Snapshot
						config.take_snapshot = !config.take_snapshot;
						break;
					}
					case 118: { // 'v'
						// Show Video
						config.show_video = !config.show_video;
						break;
					}
					default: {
					}
				}
			}

			video.release();

			break;

	  }
	  case VID: {
      // open the video file for reading
      video.open(config.full_file_name);
      if (!video.isOpened()) { // if not success, exit program
	      endwin();
	      cout << "Cannot open the video" << endl;
				Data* Packet = new Data;
	      Packet->type = END;
	      ptr_queue[0].Insert(Packet);
	      break;
      }

      fps = video.get(CV_CAP_PROP_FPS);

      timeout (1000/fps);

      double dWidth   = video.get(CV_CAP_PROP_FRAME_WIDTH); //get the width of frames of the video
      double dHeight  = video.get(CV_CAP_PROP_FRAME_HEIGHT); //get the height of frames of the video

      bool finished = 0;
      while (!finished){
	      // Read packet
	      Data* Packet = new Data;

	      bool bSuccess = video.read(Packet->frame); // read a new frame from video

	      if (!bSuccess) {//if not success, break loop
          printw("Cannot read a frame from video stream\n");
          Packet->type = END;
          ptr_queue[0].Insert(Packet);
          break;
	      }

	      // Control screen
	      clear();
	      printw("Face Detection by JGM\n");
	      printw("---------------------\n");

	      #if(DEBUG_ENABLED)
          printw("Frame size : %lf x %lf\n", dWidth, dHeight);
          printw("Frame rate : %lf\n", fps);
	      #endif

	      printw("[%d] Show Video:    Press 'v' to change.\n", (config.show_video)?1:0);
	      printw("[%d] Record Video:  Press 'r' to toggle recording.\n", (config.record_video)?1:0);
	      printw("[%d] Take Snapshot: Press 's' to take snapshot.\n", (config.take_snapshot)?1:0);
				if (!output_string.empty())
					printw("%s", output_string.c_str());
				printw("Press 'q' to quit.\n");

	      Packet->type = VID;

		    // Record time
		    #if(MEASURE_TIME)
		     	Packet->start_time = CLOCK();
		    #endif

	      ptr_queue[0].Insert(Packet);

	      int c = getch();
	      switch (c){
          case 113: { // 'q'
            // Quit
            Data* FinishPacket = new Data;
            FinishPacket->type = END;
            ptr_queue[0].Insert(FinishPacket);
            finished = 1;
            break;
          }
          case 114: { // 'r'
            // Record Video
            config.record_video = !config.record_video;
            break;
          }
          case 115: { // 's'
            // Take Snapshot
            config.take_snapshot = !config.take_snapshot;
            break;
          }
          case 118: { // 'v'
            // Show Video
            config.show_video = !config.show_video;
            break;
          }
          default: {
          }
	      }
      }

      video.release();

      break;
	  }
	  case IMG: {

      // open image file for reading
			img = cv::imread(config.full_file_name, -1);
			if(img.empty()){
				//endwin();
				cout << "Unable to decode image " << config.full_file_name << endl;
				Data* Packet = new Data;
	      Packet->type = END;
	      ptr_queue[0].Insert(Packet);
	      break;
			}

      Data* Packet = new Data;
      img.copyTo(Packet->frame);
      Packet->type = IMG;

	    // Record time
	    #if(MEASURE_TIME)
	     	Packet->start_time = CLOCK();
	    #endif

			ptr_queue[0].Insert(Packet);

      // Finish program
      Data* FinishPacket = new Data;
      FinishPacket->type = END;
			ptr_queue[0].Insert(FinishPacket);

      break;
	  }
	  default: {
      // Nothing
      break;
	  }
	}

	// Close ncurses
	endwin();

	// Wait for children
	cout << "Waiting for child threads to exit successfully.\n";
	for (int i = 0; i < STAGE_COUNT; i++){
		pthread_join(pthreads[i], NULL);
	}

	// Done
	return 0;
}
