Kevin Cai
41146127
Compiled with VS2019 Community on Windows.

2.a + b) Pebbles are generated with random initial direction and velocity at the salmon's location every 0.5s.

2.c) Pebbles have its mass calculated from its volume using 4/3*pi*r^3 and density of 2000kg/m^3, with 50 pixels/m.

2.d) Pebbles have a FeelsGravity component, which gives it a constant downward acceleration of 9.8m/s^2. It also has a Physics component which
makes it collide with other objects that have a Physics component.

3.a) Pebbles bounce off each other using a physical impulse based resolution model.

3.b) Turtles have a Physics component so they will collide with Pebbles and other Turtles. They have a mass of 1000kg (so they don't bounce too
much when hit by a pebble) and a radius of 2m. Collision is accurately detected based off of their radius instead of their bounding box.

3.c) Refer to 2.d.

4) I chose Option 2. Use A and B to toggle between Basic Physics and Advance Physics.
Water is flowing leftwards at 0.3m/s. The force of the flowing water is calculated for the pebbles using the drag equation from
fluid dynamics: F_drag = 0.5CƒÏAv^2. The pebbles also have water drag factored in, so they reach x-axis terminal velocity when the force of the flowing 
water matches the drag force. They reach terminal y-velocity when the force of gravity matches the drag force in the y-axis. Pebbles collide with the 
bottom of the screen. I added a penetration prevention check so the pebbles don't clip into the bottom of the screen or into each other. Pebbles that
have hit the bottom of the screen and have 0 y-axis velocity will not have its gravitational acceleration calculated.
