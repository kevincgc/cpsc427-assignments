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
	float dist_squared = dot(dp, dp);
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

void PhysicsSystem::step(float elapsed_ms, float window_width_px, float window_height_px, RenderSystem* renderer)
{
	Entity& player_entity = registry.players.entities[0];
	// Move fish based on how much time has passed, this is to (partially) avoid
	// having entities move at different speed based on the machine.
	auto& motion_registry = registry.motions;
	float step_seconds = 1.0f * (elapsed_ms / 1000.f);
	for (uint i = 0; i < motion_registry.size(); i++)
	{
		// !!! DONE A1: update motion.position based on step_seconds and motion.velocity
		Motion& motion = motion_registry.components[i];
		// Entity entity = motion_registry.entities[i];
		motion.position[0] = motion.position[0] + step_seconds * motion.velocity[0];
		motion.position[1] = motion.position[1] + step_seconds * motion.velocity[1];
		// printf("pos = %f, vel = %f\n", motion.position[0], motion.velocity[0]);
		// (void)elapsed_ms; // placeholder to silence unused warning until implemented
	}
	// Simulate acceleration and drag using advanced controls, only for salmon
	if (debugging.is_advanced_controls && !registry.deathTimers.has(player_entity)) {
		Motion& motion = registry.motions.get(player_entity);
		const float swimming_acceleration = 250;
		float velVectorLength = sqrt((motion.velocity[0] * motion.velocity[0]) + (motion.velocity[1] * motion.velocity[1]));
		float dragDecel = calcDragDeceleration(motion);
		motion.acceleration[0] = motion.facing[0] * swimming_acceleration * motion.is_swimming;
		motion.acceleration[1] = motion.facing[1] * swimming_acceleration * motion.is_swimming;
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
	ComponentContainer<Motion>& motion_container = registry.motions;
	for (uint i = 0; i < motion_container.components.size(); i++)
	{
		Motion& motion_i = motion_container.components[i];
		Entity entity_i = motion_container.entities[i];
		for (uint j = 0; j < motion_container.components.size(); j++) // i+1
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
				registry.collisions.emplace_with_duplicates(entity_j, entity_i);
			}
		}
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A2: HANDLE SALMON - WALL collisions HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	for (uint i = 0; i < motion_container.components.size(); i++)
	{
		Motion& motion = motion_container.components[i];
		Entity entity = motion_container.entities[i];
		if (entity != player_entity) {
			float top_bound = motion.position.y - (get_bounding_box(motion) / 2.f)[1];
			float bot_bound = motion.position.y + (get_bounding_box(motion) / 2.f)[1];
			if (top_bound <= 0 || bot_bound >= window_height_px) {
				motion.velocity[1] = -motion.velocity[1];
			}
		}
		else {
			vec2 bounding_box = get_bounding_box(motion);
			float top_bound = motion.position.y - bounding_box[1];
			float bot_bound = motion.position.y + bounding_box[1];
			float left_bound = motion.position.x - bounding_box[0];
			float right_bound = motion.position.x + bounding_box[0];
			if (left_bound <= 0 || right_bound >= window_width_px || top_bound <= 0 || bot_bound >= window_height_px) {
				Transform transform;
				transform.translate(motion.position);
				transform.rotate(motion.angle);
				transform.scale(motion.scale);
				mat3 projection = renderer->createProjectionMatrix();
				Mesh& mesh = *(registry.meshPtrs.get(player_entity));
				for (const ColoredVertex& v : mesh.vertices) {
					glm::vec3 transformed_vertex = projection * transform.mat * vec3({ v.position.x, v.position.y, 1.0f });
					float vertex_x = transformed_vertex.x;
					float vertex_y = transformed_vertex.y;
					if ((vertex_x <= -1 && motion.velocity[0] < 0) || (vertex_x >= 1 && motion.velocity[0] > 0)) {
						motion.velocity[0] = -motion.velocity[0];
					}
					if ((vertex_y <= -1 && motion.velocity[1] > 0) || (vertex_y >= 1 && motion.velocity[1] < 0)) {
						motion.velocity[1] = -motion.velocity[1];
					}
					if (vertex_x < -1) {
						motion.position.x += (-1 - vertex_x) / 2.f * window_width_px;
					}
					if (vertex_x > 1) {
						motion.position.x -= (vertex_x - 1) / 2.f * window_width_px;
					}
					if (vertex_y < -1) {
						motion.position.y -= (-1 - vertex_y) / 2.f * window_height_px;
					}
					if (vertex_y > 1) {
						motion.position.y += (vertex_y - 1) / 2.f * window_height_px;
					}
				}
			}
		}
	}
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
			if (entity_i != player_entity) {
				Entity lineLeft = createLine(motion_i.position - vec2({ bonding_box.x / 2, 0 }), vec2({ motion_i.scale.x / 30, bonding_box.y }));
				Entity lineRight = createLine(motion_i.position + vec2({ bonding_box.x / 2, 0 }), vec2({ motion_i.scale.x / 30, bonding_box.y }));
				Entity lineTop = createLine(motion_i.position - vec2({ 0, bonding_box.y / 2 }), vec2({ bonding_box.x, motion_i.scale.x / 30 }));
				Entity lineBot = createLine(motion_i.position + vec2({ 0, bonding_box.y / 2 }), vec2({ bonding_box.x, motion_i.scale.x / 30 }));
			}
			else {
				Transform transform;
				transform.translate(motion_i.position);
				transform.rotate(motion_i.angle);
				transform.scale(motion_i.scale);
				mat3 projection = renderer->createProjectionMatrix();
				Mesh& mesh = *(registry.meshPtrs.get(entity_i));
				float leftBound = 1, rightBound = -1, topBound = -1, botBound = 1;
				for (const ColoredVertex& v : mesh.vertices) {
					glm::vec3 transformed_vertex = projection * transform.mat * vec3({ v.position.x, v.position.y, 1.0f });
					float vertex_x = transformed_vertex.x;
					float vertex_y = transformed_vertex.y;
					Entity vertex = createLine(vec2({ ((vertex_x + 1) / 2.f) * window_width_px, (1 - ((vertex_y + 1) / 2.f)) * window_height_px }),
						vec2({ motion_i.scale.x / 25, motion_i.scale.x / 25 }));
					if (vertex_x < leftBound) {
						leftBound = vertex_x;
					}
					if (vertex_x > rightBound) {
						rightBound = vertex_x;
					}
					if (vertex_y < botBound) {
						botBound = vertex_y;
					}
					if (vertex_y > topBound) {
						topBound = vertex_y;
					}
				}
				float center_x = (((leftBound + rightBound) / 2.f) + 1) / 2.f * window_width_px;
				float center_y = (1 - (((botBound + topBound) / 2.f) + 1) / 2.f) * window_height_px;
				vec2 center = { center_x, center_y };
				float bounding_box_width = ((rightBound - leftBound) / 2.f) * window_width_px;
				float bounding_box_height = ((topBound - botBound) / 2.f) * window_height_px;
				vec2 bounding_box = { bounding_box_width, bounding_box_height };
				Entity lineLeft = createLine(center - vec2({ bounding_box.x / 2.f, 0 }), { motion_i.scale.x / 30, bounding_box.y });
				Entity lineRight = createLine(center + vec2({ bounding_box.x / 2.f, 0 }), { motion_i.scale.x / 30, bounding_box.y });
				Entity lineTop = createLine(center - vec2({ 0, bounding_box.y / 2 }), { bounding_box.x, motion_i.scale.x / 30 });
				Entity lineBot = createLine(center + vec2({ 0, bounding_box.y / 2 }), { bounding_box.x, motion_i.scale.x / 30 });
			}
			/*const vec2 bonding_box = get_bounding_box(motion_i);
			float radius = sqrt(dot(bonding_box/2.f, bonding_box/2.f));
			vec2 line_scale1 = { motion_i.scale.x / 10, 2*radius };
			Entity line1 = createLine(motion_i.position, line_scale1);
			vec2 line_scale2 = { 2*radius, motion_i.scale.x / 10};
			Entity line2 = createLine(motion_i.position, line_scale2);*/

			// !!! TODO A2: implement debugging of bounding boxes and mesh
		}
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A3: HANDLE PEBBLE collisions HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 3
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}