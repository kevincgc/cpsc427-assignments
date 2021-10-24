// internal
#include "ai_system.hpp"

bool is_bounding_boxes_overlap(Motion& m1, Motion& m2, vec2 box1, vec2 box2) {
	vec2 center1 = m1.position;
	vec2 center2 = m2.position;
	float x_min1 = center1.x - box1.x / 2;
	float x_max1 = center1.x + box1.x / 2;
	float x_min2 = center2.x - box2.x / 2;
	float x_max2 = center2.x + box2.x / 2;
	float y_min1 = center1.y - box1.y / 2;
	float y_max1 = center1.y + box1.y / 2;
	float y_min2 = center2.y - box2.y / 2;
	float y_max2 = center2.y + box2.y / 2;
	if (x_min1 < x_max2 && x_max1 > x_min2 && y_min1 < y_max2 && y_max1 > y_min2) {
		return true;
	}
	return false;
}

void AISystem::step(float elapsed_ms)
{
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A2: HANDLE FISH AI HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
	// You will likely want to write new functions and need to create
	// new data structures to implement a more sophisticated Fish AI.
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	Entity& player_entity = registry.players.entities[0];
	Motion& player_motion = registry.motions.get(player_entity);
	vec2 player_range_box = { epsilon, epsilon };
	ComponentContainer<Motion>& motion_container = registry.motions;
	for (uint i = 0; i < motion_container.components.size(); i++)
	{
		Motion& motion_i = motion_container.components[i];
		Entity entity_i = motion_container.entities[i];
		if (registry.softShells.has(entity_i)) {
			SoftShell& soft_shell = registry.softShells.get(entity_i);
			vec2 soft_shell_bounding_box = get_bounding_box(motion_i);
			printf("%i\n", is_bounding_boxes_overlap(player_motion, motion_i, player_range_box, soft_shell_bounding_box));
			switch (soft_shell.state) {
			case SoftShell::NORMAL:
				if (is_bounding_boxes_overlap(player_motion, motion_i, player_range_box, soft_shell_bounding_box)) {
					soft_shell.velocity_prev = motion_i.velocity;
					motion_i.velocity = { 50, 200 };
					soft_shell.state = SoftShell::DODGING;
				}
			case SoftShell::DODGING:
				if (!is_bounding_boxes_overlap(player_motion, motion_i, player_range_box, soft_shell_bounding_box)) {
					motion_i.velocity = soft_shell.velocity_prev;
					soft_shell.state = SoftShell::NORMAL;
				}
			}
			
		}
	}
	//(void)elapsed_ms; // placeholder to silence unused warning until implemented
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A2: DRAW DEBUG INFO HERE on AI path
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
	// You will want to use the createLine from world_init.hpp
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	if (debugging.in_debug_mode) {
		printf("%i\n", epsilon);
		//Entity lineLeft1 = createLine(player_motion.position - vec2({ epsilon / 2.f, 0 }), { player_motion.scale.x / 30, epsilon });
		//Entity lineRight1 = createLine(player_motion.position + vec2({ epsilon / 2.f, 0 }), { player_motion.scale.x / 30, epsilon });
		//Entity lineTop1 = createLine(player_motion.position - vec2({ 0, epsilon / 2.f }), { epsilon, player_motion.scale.x / 30 });
		//Entity lineBot1 = createLine(player_motion.position + vec2({ 0, epsilon / 2.f }), { epsilon, player_motion.scale.x / 30 });
		Entity lineBot1 = createLine({ 600, 400 }, { epsilon, epsilon });
	}
}