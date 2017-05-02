#include <iostream>
#include <unistd.h>
#include <signal.h>

#include <RSCL/RSCL.h>
#include <vrep_driver/vrep_driver.h>

using namespace std;
using namespace RSCL;
using namespace vrep;

constexpr double SAMPLE_TIME = 0.010;

bool _stop = false;

void sigint_handler(int sig) {
	_stop = true;
}

int main(int argc, char const *argv[]) {

	/***			Controller configuration			***/
	auto damping_matrix = make_shared<Matrix6d>(Matrix6d::Identity() * 500.);
	auto ext_force = make_shared<Vector6d>(Vector6d::Zero());
	auto activation_force_threshold = make_shared<double>(25.);
	auto deactivation_force_threshold = make_shared<double>(5.);

	auto safety_controller = SafetyController(damping_matrix);
	auto tcp_velocity = safety_controller.getTCPVelocity();

	auto stop_constraint = make_shared<StopConstraint>(ext_force, activation_force_threshold, deactivation_force_threshold);

	auto constant_vel = make_shared<Vector6d>(Vector6d::Zero());
	auto constant_velocity_generator = make_shared<VelocityProxy>(constant_vel);
	auto external_force_generator = make_shared<ForceProxy>(ext_force);

	safety_controller.addConstraint("stop constraint", stop_constraint);
	safety_controller.addVelocityGenerator("vel proxy", constant_velocity_generator);
	safety_controller.addForceGenerator("force proxy", external_force_generator);

	/***				V-REP driver				***/
	VREPDriver driver(
		SAMPLE_TIME,
		"LBR4p_");      // Robot prefix


	driver.enableSynchonous(true);
	driver.startSimulation();

	signal(SIGINT, sigint_handler);

	double t = 0.;
	while(not _stop) {
		driver.readTCPWrench(ext_force);
		safety_controller.updateTCPVelocity();
		driver.sendTCPtargetVelocity(tcp_velocity, ReferenceFrame::TCP);

		if(t < 5.) {
			(*constant_vel)(0) = 0.05;
		}
		else if(t < 10.) {
			(*constant_vel)(0) = -0.05;
		}
		else {
			t = 0.;
		}

		t += SAMPLE_TIME;

		driver.nextStep();
	}

	driver.enableSynchonous(false);
	driver.stopSimulation();

	return 0;
}