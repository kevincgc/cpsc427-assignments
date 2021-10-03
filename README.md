Kevin Cai
41146127
Compiled with VS2019 Community on Windows.

3.2.a) WorldSystem::step() updates the position of entities based on their velocity.
3.2.b) Pressing direction keys moves the salmon. Implemented so that the last pressed key has priority for improved user experience (separate for vertical and horizontal movement).
		Eg. Pressing Left will move the salmon left, but pressing Right while Left is still held will move the salmon right. 
			Releasing Left afterwards while the Right key is still held will not stop the salmon.
	   Also prevents user input during the dying sequence.
3.2.c) Turtles and fish properly spawn slightly outside of the screen.
3.2.d) Instead of rotating by an arbitrary angle when the cursor is moved left or right, the salmon has been programmed to always face the cursor using the atan2 function.
3.2.e) Nothing was done as the fish flipping upside down and moving slowly downwards was already implemented in the template. https://piazza.com/class/krpu7s953e6wt?cid=64
3.3.a.i) The salmon becomes red after colliding with a turtle. The color changes back upon resetting.
3.3.a.ii) The salmon lights up for 0.5 seconds after eating a fish. The score showing in the title had already been implemented, but I fixed a bug where it doesn't reset to 0 upon resetting.
3.3.b) The image distorts based on the game time and location of the pixel to simulate a water effect. The entire screen is color-shifted to blue to simulate an underwater effect.

I implemented advanced feature (a). When the 'A' key is pressed, the salmon no longer responds to the direction keys. Instead, it will accelerate in the direction of the cursor
when the spacebar is held. I calculated drag using the drag equation from fluid dynamics, F_drag = 0.5CƒÏAv^2, where C is the drag coefficient, ƒÏ is the density of the fluid, 
A is the cross-sectional area and v is velocity. I calculated the drag on the salmon by assuming that it had a radius of 3 inches, weighed 8 kg, and that its drag coefficient was 0.4. 
I estimated that 50 units in game is equal to 1 meter and I assumed that the salmon accelerates with a constant acceleration of 5 m/s^2. Based on these assumptions, the swimming acceleration
balances out the deceleration due to drag at a terminal velocity of about 5 m/s, which seems reasonably close to real life. It would be more accurate to also model skin resistance, eddy resistance,
and stamina of the fish.

An issue I ran across was that the salmon only updated its direction if I moved the mouse. It would constantly swim out of bounds if I held the acceleration key, so I fixed it by constantly
updating the direction of the salmon to face the cursor. Another issue was that the momentum of the salmon persisted upon death, so I fixed it to always drift slowly downwards. I also had
the advanced controls options as an attribute of the Player component, but that reset upon the game restarting, so I made it a global variable which persists across resets.