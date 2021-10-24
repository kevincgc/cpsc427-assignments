#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "components.hpp"
#include "tiny_ecs_registry.hpp"
#include "render_system.hpp"

vec2 get_bounding_box(const Motion& motion);

// A simple physics system that moves rigid bodies and checks for collision
class PhysicsSystem
{
public:
	void step(float elapsed_ms, float window_width_px, float window_height_px, RenderSystem* renderer);

	PhysicsSystem()
	{
	}
};