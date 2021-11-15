Kevin Cai
41146127
Compiled with VS2019 Community on Windows.

Part 3:
1.e) The salmon always faces the mouse pointer. I fixed the bug where the salmon's facing direction does not update if the mouse does not move. The up/down keys 
increase acceleration along the direction that the salmon is facing. I implemented acceleration and drag for A1, so the arrow keys don't update velocity directly.

2.a) The fish bounce off the walls based on their bounding boxes. The salmon collides based on its mesh vertices and penetration into the wall is prevented by
offsetting its location. This is hard to see with the distortion when the water effect fragment shader is turned on . There was a piazza post that said we didn't 
have to apply the water distortion to the mesh to calculate collisions.

2.b) The debug mode is activated with 'D' and I made it a toggle for convenience. The vertices of the salmon and the exact bounding boxes of the fish/turtle are 
drawn using line entities.

3.a) The fish swim away vertically from the salmon based on the location of the salmon relative to the fish. In order to prevent the fish from getting stuck when 
cornered close to the top or bottom wall, the fish will swim away from the wall if there isn't enough space to pass between the salmon and the wall. X, the number 
of frames before the fish's path is updated, can be changed with the 'MINUS' and the 'EQUAL' keys (don't need to hold SHIFT + EQUAL for the PLUS key).

3.b) The velocity of the fish is updated by the AI and is drawn as a line originating from the fish. The avoidance detection range is an epsilon by epsilon square 
centered around the salmon. The game freezes for 500 ms after each time the AI recalculates the fish's path.

4) I chose option 1, which is to make the fish even smarter by accounting not just for the salmonÅfs current position, but also for its anticipated motion 
trajectory. Pressing 'A' will activate the advance AI of the fish. I calculate the projected position of the salmon 1 second into the future based on its current
position, velocity and acceleration. This predicted position will also account for projected collision within the timespan. The velocity, acceleration and 
projected position of the salmon are drawn while in debugging mode. The fish will dodge based on the combination of these two positions, and it will also dodge 
along the x-axis, instead of just dodging vertically.