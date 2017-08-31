/* 	File: external_force.h
*	This file is part of the program open-phri
*  	Program description : OpenPHRI: a generic framework to easily and safely control robots in interactions with humans
*  	Copyright (C) 2017 -  Benjamin Navarro (LIRMM). All Right reserved.
*
*	This software is free software: you can redistribute it and/or modify
*	it under the terms of the LGPL license as published by
*	the Free Software Foundation, either version 3
*	of the License, or (at your option) any later version.
*	This software is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*	LGPL License for more details.
*
*	You should have received a copy of the GNU Lesser General Public License version 3 and the
*	General Public License version 3 along with this program.
*	If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file external_force.h
 * @author Benjamin Navarro
 * @brief Definition of the ExternalForce class
 * @date June 2017
 * @ingroup OpenPHRI
 */

#pragma once

#include <OpenPHRI/force_generators/force_generator.h>
#include <OpenPHRI/robot.h>
#include <OpenPHRI/definitions.h>

namespace phri {

/** @brief Generates a force based on an externally managed one.
 *  @details Can be useful to add a velocity generated by an external library or a force sensor.
 */
class ExternalForce : public ForceGenerator {
public:
	/** @brief Construct an external fworce generator based on a given robot
	 */
	ExternalForce(RobotConstPtr robot);
	~ExternalForce() = default;

	virtual Vector6d compute() override;

private:
	Vector6dConstPtr external_force_;
};

using ExternalForcePtr = std::shared_ptr<ExternalForce>;
using ExternalForceConstPtr = std::shared_ptr<const ExternalForce>;

} // namespace phri
