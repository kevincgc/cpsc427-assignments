// internal
#include "physics_system.hpp"
#include "world_init.hpp"

// Returns the local bounding coordinates scaled by the current size of the entity
vec2 get_bounding_box(const Motion& motion)
{
	// abs is to avoid negative scale due to the facing direction.
	return { abs(motion.scale.x), abs(motion.scale.y) };
}

// This is a SUPER APPROXIMATE check that puts a circle around the bounding boxes and sees
// if the center point of either object is inside the other's bounding-box-circle. You can
// surely implement a more accurate detection
bool collides(const Motion& motion1, const Motion& motion2)
{
	vec2 dp = motion1.position - motion2.position;
	float dist_squared = dot(dp,dp);
	const vec2 other_bonding_box = get_bounding_box(motion1) / 2.f;
	const float other_r_squared = dot(other_bonding_box, other_bonding_box);
	const vec2 my_bonding_box = get_bounding_box(motion2) / 2.f;
	const float my_r_squared = dot(my_bonding_box, my_bonding_box);
	const float r_squared = max(other_r_squared, my_r_squared);
	if (dist_squared < r_squared)
		return true;
	return false;
}

float calcDragDeceleration(const Motion& motion, float area = 0.03235840433, float mass = 8) { // Assume salmon has radius 3" and mass of 8kg
	const float dragCoefficient = 0.4; // estimated for salmon
	const float density = 1000; // density of water is 1000 kg/m^2
	float velocity = sqrtf(motion.velocity[0] * motion.velocity[0] / 250 + motion.velocity[1] * motion.velocity[1] / 250); // scaled as m/s with 50 units = 1m
	float dragF = 0.5 * density * area * dragCoefficient * velocity * velocity; // force in N
	return dragF / mass;
}

void PhysicsSystem::step(float elapsed_ms, float window_width_px, float window_height_px)
{
	// Move fish based on how much time has passed, this is to (partially) avoid
	// having entities move at different speed based on the machine.
	auto& motion_registry = registry.motions;
	float step_seconds = 1.0f * (elapsed_ms / 1000.f);
	for(uint i = 0; i< motion_registry.size(); i++)
	{
		// !!! DONE A1: update motion.position based on step_seconds and motion.velocity
		Motion& motion = motion_registry.components[i];
		// Entity entity = motion_registry.entities[i];
		motion.position[0] = motion.position[0] + step_seconds * motion.velocity[0];
		motion.position[1] = motion.position[1] + step_seconds * motion.velocity[1];
		// printf("pos = %f, vel = %f\n", motion.position[0], motion.velocity[0]);
		// (void)elapsed_ms; // placeholder to silence unused warning until implemented
	}
	Entity entity = registry.players.entities[0];
	// Simulate acceleration and drag using advanced controls, only for salmon
	if (is_advanced_controls && !registry.deathTimers.has(entity)) {
		Motion& motion = registry.motions.get(registry.players.entities[0]);
		const float swimming_acceleration = 250;
		float velVectorLength = sqrt((motion.velocity[0] * motion.velocity[0]) + (motion.velocity[1] * motion.velocity[1]));
		float dragDecel = calcDragDeceleration(motion);
		if (motion.is_swimming) {
			motion.acceleration[0] = motion.facing[0] * swimming_acceleration;
			motion.acceleration[1] = motion.facing[1] * swimming_acceleration;
		}
		else {
			motion.acceleration[0] = 0;
			motion.acceleration[1] = 0;
		}
		if (velVectorLength) {
			motion.acceleration[0] += -1 * dragDecel * motion.velocity[0] / velVectorLength;
			motion.acceleration[1] += -1 * dragDecel * motion.velocity[1] / velVectorLength;
		}
		if (velVectorLength < 50) { // stop at low speeds instead of floating off at low speeds
			motion.velocity[0] *= 0.98;
			motion.velocity[1] *= 0.98;
		}
		motion.velocity[0] = motion.velocity[0] + motion.acceleration[0] * step_seconds;
		motion.velocity[1] = motion.velocity[1] + motion.acceleration[1] * step_seconds;
		// printf("motion.acceleration[0]: %f, motion.acceleration[1]: %f\n", motion.acceleration[0], motion.acceleration[1]);
		// printf("velVectorLength: %f\n", velVectorLength);
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A3: HANDLE PEBBLE UPDATES HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 3
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// Check for collisions between all moving entities
    ComponentContainer<Motion> &motion_container = registry.motions;
	for(uint i = 0; i<motion_container.components.size(); i++)
	{
		Motion& motion_i = motion_container.components[i];
		Entity entity_i = motion_container.entities[i];
		for(uint j = 0; j<motion_container.components.size(); j++) // i+1
		{
			if (i == j)
				continue;

			Motion& motion_j = motion_container.components[j];
			if (collides(motion_i, motion_j))
			{
				Entity entity_j = motion_container.entities[j];
				// Create a collisions event
				// We are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity
				registry.collisions.emplace_with_duplicates(entity_i, entity_j);
				//registry.collisions.emplace_with_duplicates(entity_j, entity_i);
			}
		}
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A2: HANDLE SALMON - WALL collisions HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// you may need the following quantities to compute wall positions
	(float)window_width_px; (float)window_height_px;

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A2: DRAW DEBUG INFO HERE on Salmon mesh collision
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
	// You will want to use the createLine from world_init.hpp
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// debugging of bounding boxes
	if (debugging.in_debug_mode)
	{
		uint size_before_adding_new = (uint)motion_container.components.size();
		for (uint i = 0; i < size_before_adding_new; i++)
		{
			Motion& motion_i = motion_container.components[i];
			Entity entity_i = motion_container.entities[i];

			// visualize the radius with two axis-aligned lines
			const vec2 bonding_box = get_bounding_box(motion_i);
			float radius = sqrt(dot(bonding_box/2.f, bonding_box/2.f));
			vec2 line_scale1 = { motion_i.scale.x / 10, 2*radius };
			Entity line1 = createLine(motion_i.position, line_scale1);
			vec2 line_scale2 = { 2*radius, motion_i.scale.x / 10};
			Entity line2 = createLine(motion_i.position, line_scale2);

			// !!! TODO A2: implement debugging of bounding boxes and mesh
		}
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A3: HANDLE PEBBLE collisions HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 3
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}