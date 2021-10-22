Kevin Cai
41146127
Compiled with VS2019 Community on Windows.

// The 'array' for both the AoS and SoA implementation use the vector of components in the ComponentContainer
// class of the tinyECS framework.
	std::vector<Component> components; // line 40 tiny_ecs.hpp

// Every time a component is inserted into the ComponentContainer, it is added to the 'array'.
	components.push_back(std::move(c)); // line 58 tiny_ecs.hpp

// This is the 'struct' for the AoS implementation. It contains the position and velocity for a single entity.
	struct PVData {
		float pos;
		float vel;
		PVData(float pos, float vel) : pos{ pos }, vel{ vel } {};
	};

// The array of structs is thus declared using a ComponentContainer containing a vector of PVData structures.
	ComponentContainer<PVData> position_velocity_aos;

// This is the 'struct' for the SoA implementation. It contains an 'array' for position and one for velocity, 
// so that it is a struct of arrays.
	struct Motion {
		ComponentContainer<float> position;
		ComponentContainer<float> velocity;
	};

// I create the struct of arrays by declaring a single Motion struct, which contains an array for position and an
// array for velocity.
	Motion position_velocity_soa;

// I push all the ComponentContainers onto the registry so that ECS system can iterate over all components.
	registry_list.push_back(&position_velocity_aos);
	registry_list.push_back(&position_velocity_soa.position);
	registry_list.push_back(&position_velocity_soa.velocity);