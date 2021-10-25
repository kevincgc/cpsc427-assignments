// internal
#include "ai_system.hpp"
#include "world_init.hpp"

int timer = 0;

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

void AISystem::step(float elapsed_ms, float window_width_px, float window_height_px)
{
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A2: HANDLE FISH AI HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
	// You will likely want to write new functions and need to create
	// new data structures to implement a more sophisticated Fish AI.
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	Entity& player_entity = registry.players.entities[0];
	Motion& player_motion = registry.motions.get(player_entity);
	int epsilon = 400;
	vec2 player_range_box = { epsilon, epsilon };
	ComponentContainer<Motion>& motion_container = registry.motions;
	for (uint i = 0; i < motion_container.components.size(); i++)
	{
		Motion& motion_i = motion_container.components[i];
		Entity entity_i = motion_container.entities[i];
		if (registry.softShells.has(entity_i)) {
			SoftShell& soft_shell = registry.softShells.get(entity_i);
			vec2 soft_shell_bounding_box = get_bounding_box(motion_i);
			// printf("%i\n", is_bounding_boxes_overlap(player_motion, motion_i, player_range_box, soft_shell_bounding_box));
			switch (soft_shell.state) {
			case SoftShell::NORMAL:
				if (is_bounding_boxes_overlap(player_motion, motion_i, player_range_box, soft_shell_bounding_box)) {
					soft_shell.velocity_prev = motion_i.velocity;
					if (soft_shell.update_frame_counter <= 0) {
						soft_shell.state = SoftShell::UPDATING;
					}
					else {
						soft_shell.state = SoftShell::DODGING;
					}
				}
				break;
			case SoftShell::UPDATING:
				if (player_motion.position.y - epsilon / 2 < soft_shell_bounding_box.y) {
					motion_i.velocity = { 0, 200 };
				} 
				else if (player_motion.position.y + epsilon / 2 > window_height_px - soft_shell_bounding_box.y) {
					motion_i.velocity = { 0, -200 };
				}
				else if (motion_i.position.y <= player_motion.position.y) {
					motion_i.velocity = { 0, -200 };
				}
				else if (motion_i.position.y > player_motion.position.y) {
					motion_i.velocity = { 0, 200 };
				}
				if (!debugging.in_freeze_mode) {
					timer = 500;
				}
				soft_shell.update_frame_counter = debugging.ai_update_every_X_frames;
				soft_shell.state = SoftShell::DODGING;
				break;
			case SoftShell::DODGING:
				if (soft_shell.update_frame_counter <= 0) {
					soft_shell.state = SoftShell::UPDATING;
				}
				if (!is_bounding_boxes_overlap(player_motion, motion_i, player_range_box, soft_shell_bounding_box)) {
					motion_i.velocity = { soft_shell.velocity_prev.x, 0 };
					soft_shell.state = SoftShell::NORMAL;
				}
				break;
			}
			if (!debugging.in_freeze_mode && soft_shell.update_frame_counter > 0) {
				soft_shell.update_frame_counter--;
			}
		}
	}

	if (timer > 0 && debugging.in_debug_mode) {
		debugging.in_freeze_mode = true;
		timer -= elapsed_ms;
	}
	else {
		debugging.in_freeze_mode = false;
	}
	//(void)elapsed_ms; // placeholder to silence unused warning until implemented
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A2: DRAW DEBUG INFO HERE on AI path
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
	// You will want to use the createLine from world_init.hpp
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	if (debugging.in_debug_mode && debugging.in_freeze_mode) {
		Entity lineLeft = createLine(player_motion.position - vec2({ epsilon / 2.f, 0 }), { player_motion.scale.x / 30, epsilon });
		Entity lineRight = createLine(player_motion.position + vec2({ epsilon / 2.f, 0 }), { player_motion.scale.x / 30, epsilon });
		Entity lineTop = createLine(player_motion.position - vec2({ 0, epsilon / 2.f }), { epsilon, player_motion.scale.x / 30 });
		Entity lineBot = createLine(player_motion.position + vec2({ 0, epsilon / 2.f }), { epsilon, player_motion.scale.x / 30 });
		for (uint i = 0; i < motion_container.components.size(); i++)
		{
			Motion& motion_i = motion_container.components[i];
			Entity entity_i = motion_container.entities[i];
			if (registry.softShells.has(entity_i)) {
				SoftShell& soft_shell = registry.softShells.get(entity_i);
				vec2 soft_shell_bounding_box = get_bounding_box(motion_i);
				switch (soft_shell.state) {
				case SoftShell::DODGING:
					Entity lineBot = createLine(motion_i.position - vec2({ 0, -motion_i.velocity.y / 2 }), 
						{ motion_i.scale.x / 30, -motion_i.velocity.y });
					break;
				}
			}
		}
		//if (debugging.is_advance_ai) {
		//	motion.angle = atan2f(y_diff, x_diff);
		//	Entity lineBot = createLine(player_motion.position,
		//		{ motion_i.scale.x / 30, -motion_i.velocity.y });
		//}
	}
}