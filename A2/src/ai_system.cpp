// internal
#include "ai_system.hpp"
#include "world_init.hpp"

int timer = 0;
vec2 fish_vel = {};

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

Motion predict_player_motion(Motion& player_motion, float window_width_px, float window_height_px, vec2& bounding_box) {
	Motion predicted_motion = Motion();
	predicted_motion.position = player_motion.position;
	predicted_motion.velocity = player_motion.velocity;
	predicted_motion.acceleration = player_motion.acceleration;
	predicted_motion.scale = player_motion.scale;
	float t = (10 / 1000.f);
	for (int i = 0; i < 100; i++) {
		step_update_position(predicted_motion, t);
		step_update_vel_acc(predicted_motion, t);
		step_handle_player_wall_bb_collision(predicted_motion, t, bounding_box);
	}
	return predicted_motion;
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
	vec2 bounding_box = vec2();
	Motion projected_motion = predict_player_motion(player_motion, window_width_px, window_height_px, bounding_box);
	bool is_player_motion_overlap = false, is_projected_motion_overlap = false;
	ComponentContainer<Motion>& motion_container = registry.motions;
	for (uint i = 0; i < motion_container.components.size(); i++)
	{
		Motion& motion_i = motion_container.components[i];
		Entity entity_i = motion_container.entities[i];
		if (registry.softShells.has(entity_i)) {
			SoftShell& soft_shell = registry.softShells.get(entity_i);
			vec2 soft_shell_bounding_box = get_bounding_box(motion_i);
			switch (soft_shell.state) {
			case SoftShell::NORMAL:
				if (is_bounding_boxes_overlap(player_motion, motion_i, player_range_box, soft_shell_bounding_box) ||
					(debugging.is_advance_ai &&	is_bounding_boxes_overlap(projected_motion, motion_i, player_range_box, soft_shell_bounding_box))) {
					is_player_motion_overlap = is_player_motion_overlap || is_bounding_boxes_overlap(player_motion, motion_i, player_range_box, soft_shell_bounding_box);
					is_projected_motion_overlap = is_projected_motion_overlap || is_bounding_boxes_overlap(projected_motion, motion_i, player_range_box, soft_shell_bounding_box);
					soft_shell.velocity_prev = motion_i.velocity;
					if (soft_shell.update_frame_counter <= 0) {
						soft_shell.state = SoftShell::UPDATING;
					}
					else {
						soft_shell.state = SoftShell::DODGING;
					}
				}
				break;
			case SoftShell::UPDATING: {
				is_player_motion_overlap = is_player_motion_overlap || is_bounding_boxes_overlap(player_motion, motion_i, player_range_box, soft_shell_bounding_box);
				is_projected_motion_overlap = is_projected_motion_overlap || is_bounding_boxes_overlap(projected_motion, motion_i, player_range_box, soft_shell_bounding_box);
				if (motion_i.position.x <= player_motion.position.x && debugging.is_advance_ai) {
					motion_i.velocity.x = -200;
				}
				else {
					motion_i.velocity.x = 0;
				}
				if (player_motion.position.y - epsilon / 2 < soft_shell_bounding_box.y ||
					projected_motion.position.y - epsilon / 2 < soft_shell_bounding_box.y) {
					motion_i.velocity.y = 200;
				}
				else if (player_motion.position.y + epsilon / 2 > window_height_px - soft_shell_bounding_box.y ||
						projected_motion.position.y + epsilon / 2 > window_height_px - soft_shell_bounding_box.y) {
					motion_i.velocity.y = -200;
				}
				else if (motion_i.position.y <= projected_motion.position.y) {
					motion_i.velocity.y = -200;
				}
				else if (motion_i.position.y > projected_motion.position.y) {
					motion_i.velocity.y = 200;
				}
				fish_vel = { motion_i.velocity.x, motion_i.velocity.y };
				if (!debugging.in_freeze_mode) {
					timer = 500;
				}
				soft_shell.update_frame_counter = debugging.ai_update_every_X_frames;
				soft_shell.state = SoftShell::DODGING;
				break;
			}
			case SoftShell::DODGING:
				is_player_motion_overlap = is_player_motion_overlap || is_bounding_boxes_overlap(player_motion, motion_i, player_range_box, soft_shell_bounding_box);
				is_projected_motion_overlap = is_projected_motion_overlap || is_bounding_boxes_overlap(projected_motion, motion_i, player_range_box, soft_shell_bounding_box);
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
		if (is_player_motion_overlap) {
			Entity lineLeft = createLine(player_motion.position - vec2({ epsilon / 2.f, 0 }), { player_motion.scale.x / 30, epsilon });
			Entity lineRight = createLine(player_motion.position + vec2({ epsilon / 2.f, 0 }), { player_motion.scale.x / 30, epsilon });
			Entity lineTop = createLine(player_motion.position - vec2({ 0, epsilon / 2.f }), { epsilon, player_motion.scale.x / 30 });
			Entity lineBot = createLine(player_motion.position + vec2({ 0, epsilon / 2.f }), { epsilon, player_motion.scale.x / 30 });
		}
		if (is_projected_motion_overlap && debugging.is_advance_ai) {
			Entity lineLeft = createLine(projected_motion.position - vec2({ epsilon / 2.f, 0 }), { player_motion.scale.x / 30, epsilon });
			Entity lineRight = createLine(projected_motion.position + vec2({ epsilon / 2.f, 0 }), { player_motion.scale.x / 30, epsilon });
			Entity lineTop = createLine(projected_motion.position - vec2({ 0, epsilon / 2.f }), { epsilon, player_motion.scale.x / 30 });
			Entity lineBot = createLine(projected_motion.position + vec2({ 0, epsilon / 2.f }), { epsilon, player_motion.scale.x / 30 });
		}
		for (uint i = 0; i < motion_container.components.size(); i++)
		{
			Motion& motion_i = motion_container.components[i];
			Entity entity_i = motion_container.entities[i];
			if (registry.softShells.has(entity_i)) {
				SoftShell& soft_shell = registry.softShells.get(entity_i);
				vec2 soft_shell_bounding_box = get_bounding_box(motion_i);
				switch (soft_shell.state) {
				case SoftShell::DODGING: {
					if (fish_vel.y && fish_vel.x) {
						float angle = atan2f(fish_vel.y, fish_vel.x) + M_PI / 2;
						float len = sqrt(fish_vel.y * fish_vel.y + fish_vel.x * fish_vel.x);
						Entity lineBot = createLine(motion_i.position + vec2({ fish_vel.x / 2, fish_vel.y / 2 }),
							{ fish_vel.x / 50, len }, angle);
					}
					else {
						Entity lineBot = createLine(motion_i.position - vec2({ 0, -motion_i.velocity.y / 2 }),
							{ motion_i.scale.x / 30, -motion_i.velocity.y });
					}
					break;
				}
				}
			}
		}
	}
	if (debugging.in_debug_mode && debugging.is_advance_ai) {
		Motion& player_motion = registry.motions.get(player_entity);
		float vel_angle = atan2f(player_motion.velocity.y, player_motion.velocity.x) + M_PI / 2;
		float vel_len = sqrt(player_motion.velocity.y * player_motion.velocity.y + player_motion.velocity.x * player_motion.velocity.x);
		Entity player_vel = createLine(player_motion.position + vec2({ player_motion.velocity.x / 2, player_motion.velocity.y / 2 }),
			{ player_motion.scale.x / 30, vel_len }, vel_angle);
		float acc_angle = atan2f(player_motion.acceleration.y, player_motion.acceleration.x) + M_PI / 2;
		float acc_len = sqrt(player_motion.acceleration.y * player_motion.acceleration.y + player_motion.acceleration.x * player_motion.acceleration.x);
		Entity player_acc = createLine(player_motion.position + vec2({ player_motion.acceleration.x / 2, player_motion.acceleration.y / 2 }),
			{ player_motion.scale.x / 30, acc_len }, acc_angle);
		const vec2 bonding_box = get_bounding_box(projected_motion);
		Entity lineLeft = createLine(projected_motion.position - vec2({ bounding_box.x / 2.f, 0 }), { player_motion.scale.x / 30, bounding_box.y });
		Entity lineRight = createLine(projected_motion.position + vec2({ bounding_box.x / 2.f, 0 }), { player_motion.scale.x / 30, bounding_box.y });
		Entity lineTop = createLine(projected_motion.position - vec2({ 0, bounding_box.y / 2 }), { bounding_box.x, player_motion.scale.x / 30 });
		Entity lineBot = createLine(projected_motion.position + vec2({ 0, bounding_box.y / 2 }), { bounding_box.x, player_motion.scale.x / 30 });
	}
}