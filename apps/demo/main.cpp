#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <chrono>
#include <list>

#include <RSCL/RSCL.h>
#include <vrep_driver/vrep_driver.h>

#include "state_machine.h"

using namespace RSCL;
using namespace vrep;

bool _stop = false;

void sigint_handler(int sig) {
	_stop = true;
}

int main(int argc, char const *argv[]) {

	/***				Robot				***/
	auto robot = std::make_shared<Robot>(
		"LBR4p",    // Robot's name, must match V-REP model's name
		7);         // Robot's joint count

	/***				V-REP driver				***/
	VREPDriver driver(
		robot,
		ControlLevel::TCP,
		SAMPLE_TIME,
		"", "127.0.0.1", -1000
		);

	driver.startSimulation();
	auto laser_data = driver.initLaserScanner("Hokuyo");

	/***			Controller configuration			***/
	*robot->controlPointDampingMatrix() *= 500.;
	auto safety_controller = SafetyController(robot);

	auto laser_detector = LaserScannerDetector(
		laser_data,
		270. * M_PI / 180.,
		0.2,
		3.);

	auto state_machine = StateMachine(
		robot,
		safety_controller,
		laser_detector,
		argc > 1); // skip teaching

	Clock clock(SAMPLE_TIME);
	DataLogger logger(
		safety_controller,
		"/mnt/tmpfs/rscl_logs",
		clock.getTime(),
		true,   // create gnuplot files
		true);  // delay disk write
	logger.logRobotData(robot);

	double fsm_states[2];
	logger.logExternalData("fsm state", fsm_states, 2);
	double operator_distance;
	logger.logExternalData("operator distance", &operator_distance, 1);
	double sep_dist_vlim;
	logger.logExternalData("sep dist vlim", &sep_dist_vlim, 1);

	auto update_external_data = [state_machine, &fsm_states, &operator_distance, &sep_dist_vlim]()
								{
									fsm_states[0] = static_cast<double>(state_machine.getTeachState());
									fsm_states[1] = static_cast<double>(state_machine.getReplayState());
									operator_distance = state_machine.getOperatorDistance();
									sep_dist_vlim = state_machine.getSeparationDistanceVelocityLimitation();
								};

	signal(SIGINT, sigint_handler);

	driver.enableSynchonous(true);
	size_t count = 10;
	while(not (driver.getSimulationData(ReferenceFrame::Base, ReferenceFrame::Base) and laser_data->size() == 1080 and count-- == 0 )and not _stop) {
		driver.nextStep();
	}

	laser_detector.init();
	state_machine.init();

	if(not _stop)
		std::cout << "Starting main loop\n";

	double t_avg = 0.;
	while(not _stop) {
		if(driver.getSimulationData(ReferenceFrame::Base, ReferenceFrame::Base)) {

			auto t_start = std::chrono::high_resolution_clock::now();
			_stop |= state_machine.compute();
			safety_controller.compute();
			auto t_end = std::chrono::high_resolution_clock::now();

			t_avg = 0.01*std::chrono::duration<double>(t_end-t_start).count() + 0.99*t_avg;

			if(not driver.sendSimulationData()) {
				std::cerr << "Can'send robot data to V-REP" << std::endl;
			}

			std::cout << "t_avg: " << t_avg << std::endl;


			clock();
			update_external_data();
			logger();
		}
		else {
			std::cerr << "Can't get robot data from V-REP" << std::endl;
		}

		driver.nextStep();
	}

	driver.enableSynchonous(false);
	driver.stopSimulation();

	return 0;
}
