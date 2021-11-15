// internal
#include "physics_system.hpp"
#include "world_init.hpp"

RenderSystem* renderer;
float window_width_px;
float window_height_px;
vec2 bounding_box;

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

bool collides_spheres(const Motion& motion1, const Motion& motion2, Physics& physics1, Physics& physics2)
{
	vec2 dp = motion1.position - motion2.position;
	float dist_squared = dot(dp, dp);
	float radius1 = physics1.radius;
	float radius2 = physics2.radius;
	float radius_sum = radius1 + radius2;
	if (radius_sum* radius_sum > dist_squared)
		return true;
	return false;
}

void impulse_collision_resolution(Motion& motion1, Motion& motion2, Physics& physics1, Physics& physics2 ) {
	// Based on math derived here: https://www.randygaul.net/2013/03/27/game-physics-engine-part-1-impulse-resolution/
	// Using impulse to resolve collisions
	vec2 norm = glm::normalize(motion2.position - motion1.position); // collision normal
	vec2 relative_vel = motion2.velocity - motion1.velocity;
	if (glm::dot(norm, relative_vel) > 0) // don't calculate this more than once
		return;
	float coeff_restitution = min(physics1.coefficient_of_resititution, physics2.coefficient_of_resititution);

	// calculated based on "VelocityA + Impulse(Direction) / MassA - VelocityB + Impulse(Direction) / MassB = -Restitution(VelocityRelativeAtoB) * Direction"
	float impulse_magnitude = -(coeff_restitution + 1) * glm::dot(norm, relative_vel) / (1 / physics1.mass + 1 / physics2.mass);
	vec2 impulse = impulse_magnitude * norm; // impulse along direction of collision norm

	// velocity is changed relative to the mass of objects in the collision
	motion1.velocity = motion1.velocity - impulse / physics1.mass;
	motion2.velocity = motion2.velocity + impulse / physics2.mass;
}

void prevent_collision_overlap(Entity entity, Entity other) {
	Motion& motion1 = registry.motions.get(entity);
	Motion& motion2 = registry.motions.get(other);
	const float pen_scaling_factor = 10;
	vec2 box1 = get_bounding_box(motion1);
	vec2 box2 = get_bounding_box(motion2);
	float x2_bound = motion2.position[0] - box2[0] / 2.f;
	float x1_bound = motion1.position[0] + box1[0] / 2.f;
	float x_penetration = x1_bound - x2_bound;
	x_penetration /= pen_scaling_factor;
	float y_rectangle_bound = motion2.position[1] - box2[1] / 2.f;
	float y_circle_bound = motion1.position[1] + box1[1] / 2.f;
	float y_penetration = y_circle_bound - y_rectangle_bound;
	y_penetration /= pen_scaling_factor;

	if (glm::dot(motion2.velocity, motion1.velocity) > 0) {
		vec2 player_nextpos = { 0.0, 0.0 };
		vec2 enemy_nextpos = { 0.0, 0.0 };
		if (motion1.velocity[0] < 0) {
			player_nextpos[0] = motion1.position[0] + x_penetration / 2.f;
			enemy_nextpos[0] = motion2.position[0] - x_penetration / 2.f;
		}
		else {
			player_nextpos[0] = motion1.position[0] - x_penetration / 2.f;
			enemy_nextpos[0] = motion2.position[0] + x_penetration / 2.f;
		}
		if (motion1.velocity[1] > 0) {
			player_nextpos[1] = motion1.position[1] + y_penetration / 2.f;
			enemy_nextpos[1] = motion2.position[1] - y_penetration / 2.f;
		}
		else {
			player_nextpos[1] = motion1.position[1] - y_penetration / 2.f;
			enemy_nextpos[1] = motion2.position[1] + y_penetration / 2.f;
		}
	}
}

void prevent_pebble_wall_penetration(Entity entity, int window_height_px) {
	Motion& motion = registry.motions.get(entity);
	Physics& physics = registry.physics.get(entity);
	float y_penetration = motion.position.y + physics.radius - window_height_px;
	if (y_penetration > 0) {
		motion.position.y = motion.position.y - y_penetration;
		motion.velocity.y = -motion.velocity.y * 0.3;
	}
}

float add_drag_to_acceleration(const Motion& motion, float area = 0.03235840433, float mass = 8) { // Assume salmon has radius 3" and mass of 8kg
	const float dragCoefficient = 0.4;
	const float density = 1000; // density of water is 1000 kg/m^2
	float velocity = sqrtf(motion.velocity[0] * motion.velocity[0] / 250 + motion.velocity[1] * motion.velocity[1] / 250); // scaled as m/s with 50 units = 1m
	float dragF = 0.5 * density * area * dragCoefficient * velocity * velocity; // force in N
	return dragF / mass;
}

float add_flowing_water_force_to_acc(const Motion& motion, Physics& physics) { 
	float area = M_PI * physics.radius * physics.radius;
	float mass = physics.mass;
	const float dragCoefficient = 0.5; //sphere
	const float density = 1000; //water
	float velocity = 0.3;
	float dragF = 0.5 * density * area * dragCoefficient * velocity * velocity; // force in N
	return dragF / mass;
}

void step_update_position(Motion& motion, float step_seconds) {
	motion.position[0] = motion.position[0] + step_seconds * motion.velocity[0];
	motion.position[1] = motion.position[1] + step_seconds * motion.velocity[1];
}

void step_update_velocity(Motion& motion, float step_seconds) {
	motion.velocity[0] = motion.velocity[0] + motion.acceleration[0] * step_seconds;
	motion.velocity[1] = motion.velocity[1] + motion.acceleration[1] * step_seconds;
}

void step_update_swimming_acceleration(Motion& motion, float step_seconds) {
	const float swimming_acceleration = 250;
	float velVectorLength = sqrt((motion.velocity[0] * motion.velocity[0]) + (motion.velocity[1] * motion.velocity[1]));
	float dragDecel = add_drag_to_acceleration(motion);
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
	step_update_velocity(motion, step_seconds);
}

void step_handle_player_wall_bb_collision(Motion& motion, float step_seconds, vec2& bb) {
	Entity& player_entity = registry.players.entities[0];
	bb = bounding_box;
	float left_bound = motion.position.x - bounding_box[0] / 2;
	float right_bound = motion.position.x + bounding_box[0] / 2;
	float top_bound = motion.position.y - bounding_box[1] / 2;
	float bot_bound = motion.position.y + bounding_box[1] / 2;
	if ((left_bound <= 0 && motion.velocity[0] < 0) || (right_bound >= window_width_px && motion.velocity[0] > 0)) {
		motion.velocity[0] = -motion.velocity[0];
	}
	if ((top_bound <= 0 && motion.velocity[1] < 0) || (bot_bound >= window_height_px && motion.velocity[1] > 0)) {
		motion.velocity[1] = -motion.velocity[1];
	}
	if (left_bound < 0) {
		motion.position.x += abs(left_bound);
	}
	if (right_bound > window_width_px) {
		motion.position.x -= (right_bound - window_width_px);
	}
	if (top_bound < 0) {
		motion.position.y += abs(top_bound);
	}
	if (bot_bound > window_height_px) {
		motion.position.y -= (bot_bound - window_height_px);
	}
}

void step_handle_player_wall_collision(Motion& motion, float step_seconds) {
	Entity& player_entity = registry.players.entities[0];
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

void set_vars(float w, float h, RenderSystem* r) {
	window_width_px = w;
	window_height_px = h;
	renderer = r;
}

void PhysicsSystem::step(float elapsed_ms, float window_width_px, float window_height_px, RenderSystem* renderer)
{
	set_vars(window_width_px, window_height_px, renderer);
	Entity& player_entity = registry.players.entities[0];
	// Move fish based on how much time has passed, this is to (partially) avoid
	// having entities move at different speed based on the machine.
	auto& motion_registry = registry.motions;
	auto& physics_registry = registry.physics;
	auto& gravity_registry = registry.gravity;
	ComponentContainer<Motion>& motion_container = registry.motions;
	float step_seconds = 1.0f * (elapsed_ms / 1000.f);
	for (uint i = 0; i < motion_registry.size(); i++)
	{
		// !!! DONE A1: update motion.position based on step_seconds and motion.velocity
		Motion& motion = motion_registry.components[i];
		// Entity entity = motion_registry.entities[i];
		step_update_position(motion, step_seconds);
		// printf("pos = %f, vel = %f\n", motion.position[0], motion.velocity[0]);
		// (void)elapsed_ms; // placeholder to silence unused warning until implemented
	}
	// Simulate acceleration and drag using advanced controls, only for salmon
	if (debugging.is_advanced_controls && !registry.deathTimers.has(player_entity)) {
		Motion& motion = registry.motions.get(player_entity);
		step_update_swimming_acceleration(motion, step_seconds);
		// printf("motion.acceleration[0]: %f, motion.acceleration[1]: %f\n", motion.acceleration[0], motion.acceleration[1]);
		// printf("velVectorLength: %f\n", velVectorLength);
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A3: HANDLE PEBBLE UPDATES HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 3
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	for (uint i = 0; i < motion_container.components.size(); i++)
	{
		Entity entity = motion_container.entities[i];
		if (!gravity_registry.has(entity))
			continue;
		Physics& physics = physics_registry.get(entity);
		Motion& motion = motion_container.components[i];
		FeelsGravity& gravity = gravity_registry.get(entity);
		if (gravity_registry.has(entity)) {
			if (abs(motion.velocity.y) < 1 && motion.position.y + physics.radius >= window_height_px -1) {
				gravity.is_free_fall = false;
			}
			else {
				gravity.is_free_fall = true;
			}
			if (gravity.is_free_fall || !debugging.is_advance_physics) {
				motion.acceleration.y = 9.8 * 50; // ~1/10 of normal gravity or rocks sink really
			}
			else {
				motion.acceleration.y = 0;
			}
			if (debugging.is_advance_physics) {
				float water_drag_force = add_drag_to_acceleration(motion, M_PI * pow((motion.scale.x / 100), 2), physics_registry.get(entity).mass);
				float flowing_water_force = add_flowing_water_force_to_acc(motion, physics);
				float velVectorLength = sqrt((motion.velocity[0] * motion.velocity[0]) + (motion.velocity[1] * motion.velocity[1]));
				motion.acceleration[0] = -1 * water_drag_force * motion.velocity[0] / velVectorLength - flowing_water_force;
				motion.acceleration[1] += -1 * water_drag_force * motion.velocity[1] / velVectorLength;
				prevent_pebble_wall_penetration(entity, window_height_px);
			}
			step_update_velocity(motion, step_seconds);
		}
	}

	// Check for collisions between all moving entities
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
		ComponentContainer<SoftShell>& softshell_container = registry.softShells;
		if (entity != player_entity && softshell_container.has(entity)) {
			float top_bound = motion.position.y - (get_bounding_box(motion) / 2.f)[1];
			float bot_bound = motion.position.y + (get_bounding_box(motion) / 2.f)[1];
			if (top_bound <= 0 || bot_bound >= window_height_px) {
				motion.velocity[1] = -motion.velocity[1];
			}
		}
		else if (entity == player_entity) {
			step_handle_player_wall_collision(motion, step_seconds);
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
	uint size_before_adding_new = (uint)motion_container.components.size();
	for (uint i = 0; i < size_before_adding_new; i++)
	{
		Motion& motion_i = motion_container.components[i];
		Entity entity_i = motion_container.entities[i];
		// visualize the radius with two axis-aligned lines
		const vec2 bonding_box = get_bounding_box(motion_i);
		if (entity_i != player_entity) {
			if (debugging.in_debug_mode)
			{
				Entity lineLeft = createLine(motion_i.position - vec2({ bonding_box.x / 2, 0 }), vec2({ motion_i.scale.x / 30, bonding_box.y }));
				Entity lineRight = createLine(motion_i.position + vec2({ bonding_box.x / 2, 0 }), vec2({ motion_i.scale.x / 30, bonding_box.y }));
				Entity lineTop = createLine(motion_i.position - vec2({ 0, bonding_box.y / 2 }), vec2({ bonding_box.x, motion_i.scale.x / 30 }));
				Entity lineBot = createLine(motion_i.position + vec2({ 0, bonding_box.y / 2 }), vec2({ bonding_box.x, motion_i.scale.x / 30 }));
			}
		}
		else {
			if (!registry.deathTimers.has(player_entity)) {
				Transform transform;
				transform.translate(motion_i.position);
				transform.rotate(motion_i.angle);
				transform.scale(motion_i.scale);
				mat3 projection = renderer->createProjectionMatrix();
				Mesh& mesh = *(registry.meshPtrs.get(entity_i));
				float left_vertex_bound = 1, right_vertex_bound = -1, top_vertex_bound = -1, bot_vertex_bound = 1;
				for (const ColoredVertex& v : mesh.vertices) {
					glm::vec3 transformed_vertex = projection * transform.mat * vec3({ v.position.x, v.position.y, 1.0f });
					float vertex_x = transformed_vertex.x;
					float vertex_y = transformed_vertex.y;
					if (debugging.in_debug_mode)
					{
						Entity vertex = createLine(vec2({ ((vertex_x + 1) / 2.f) * window_width_px, (1 - ((vertex_y + 1) / 2.f)) * window_height_px }),
							vec2({ motion_i.scale.x / 25, motion_i.scale.x / 25 }));
					}
					if (vertex_x < left_vertex_bound) {
						left_vertex_bound = vertex_x;
					}
					if (vertex_x > right_vertex_bound) {
						right_vertex_bound = vertex_x;
					}
					if (vertex_y < bot_vertex_bound) {
						bot_vertex_bound = vertex_y;
					}
					if (vertex_y > top_vertex_bound) {
						top_vertex_bound = vertex_y;
					}
				}
				float center_x = (((left_vertex_bound + right_vertex_bound) / 2.f) + 1) / 2.f * window_width_px;
				float center_y = (1 - (((bot_vertex_bound + top_vertex_bound) / 2.f) + 1) / 2.f) * window_height_px;
				vec2 center = { center_x, center_y };
				float bounding_box_width = ((right_vertex_bound - left_vertex_bound) / 2.f) * window_width_px;
				float bounding_box_height = ((top_vertex_bound - bot_vertex_bound) / 2.f) * window_height_px;
				bounding_box = { bounding_box_width, bounding_box_height };
				if (debugging.in_debug_mode)
				{
					Entity lineLeft = createLine(center - vec2({ bounding_box.x / 2.f, 0 }), { motion_i.scale.x / 30, bounding_box.y });
					Entity lineRight = createLine(center + vec2({ bounding_box.x / 2.f, 0 }), { motion_i.scale.x / 30, bounding_box.y });
					Entity lineTop = createLine(center - vec2({ 0, bounding_box.y / 2 }), { bounding_box.x, motion_i.scale.x / 30 });
					Entity lineBot = createLine(center + vec2({ 0, bounding_box.y / 2 }), { bounding_box.x, motion_i.scale.x / 30 });
				}
			}
		}
		/*const vec2 bonding_box = get_bounding_box(motion_i);
		float radius = sqrt(dot(bonding_box/2.f, bonding_box/2.f));
		vec2 line_scale1 = { motion_i.scale.x / 10, 2*radius };
		Entity line1 = createLine(motion_i.position, line_scale1);
		vec2 line_scale2 = { 2*radius, motion_i.scale.x / 10};
		Entity line2 = createLine(motion_i.position, line_scale2);*/
		// !!! TODO A2: implement debugging of bounding boxes and mesh
	}
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A3: HANDLE PEBBLE collisions HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 3
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	for (uint i = 0; i < motion_container.components.size(); i++)
	{
		Motion& motion_i = motion_container.components[i];
		Entity entity_i = motion_container.entities[i];
		if (!physics_registry.has(entity_i))
			continue;
		for (uint j = 0; j < motion_container.components.size(); j++) // i+1
		{
			Entity entity_j = motion_container.entities[j];
			if (i == j || !physics_registry.has(entity_j))
				continue;
			Physics& physics_i = physics_registry.get(entity_i);
			Physics& physics_j = physics_registry.get(entity_j);
			Motion& motion_j = motion_container.components[j];
			if (collides_spheres(motion_i, motion_j, physics_i, physics_j))
			{
				if (gravity_registry.has(entity_i)) {
					FeelsGravity& gravity_i = gravity_registry.get(entity_i);
					gravity_i.is_free_fall = true;
				}
				if (gravity_registry.has(entity_j)) {
					FeelsGravity& gravity_j = gravity_registry.get(entity_j);
					gravity_j.is_free_fall = true;
				}
				impulse_collision_resolution(motion_i, motion_j, physics_i, physics_j);
				prevent_collision_overlap(entity_i, entity_j);
			}
		}
	}
}