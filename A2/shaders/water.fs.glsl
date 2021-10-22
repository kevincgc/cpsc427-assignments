#version 330

uniform sampler2D screen_texture;
uniform float time;
uniform float darken_screen_factor;

in vec2 texcoord;

layout(location = 0) out vec4 color;

vec2 distort(vec2 uv) 
{
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// DONE A1: HANDLE THE WATER WAVE DISTORTION HERE (you may want to try sin/cos)
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//uv[0] = uv[0] * ((sin(time / 8.f) + 13.f) / 14.f) * ((cos(texcoord[0] / 200.f) + 24.f) / 25.f);
	//uv[1] = uv[1] * ((sin(time / 8.f) + 10.f) / 11.f) * ((sin(texcoord[1] / 200.f) + 24.f) / 25.f);
	//bool time_mod = false;
//	float a = time;
//	float b = 300.f;
//	int d = int(a/b) % 2;
//	float c = d == 0 ? a - (b * floor(a/b)) : 300 - (a - (b * floor(a/b)));
//	uv[0] = uv[0] * ((cos((c + 6) / 8.f * (texcoord[0] + 7.f)/8.f) + 32.f) / 33.f);
//	uv[1] = uv[1] * ((sin((c + 6) / 8.f * (texcoord[1] + 7.f)/8.f) + 27.f) / 28.f);
	return uv;
}

vec4 color_shift(vec4 in_color) 
{
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// DONE A1: HANDLE THE COLOR SHIFTING HERE (you may want to make it blue-ish)
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//in_color[3] = in_color[3] + in_color[3] * 0.3 + 1;
	//in_color[3] = in_color[3] > 1 ? 1 : in_color[3];
	in_color = vec4(in_color.xy,in_color.z + 0.45, 1.0);
	return in_color;
}

vec4 fade_color(vec4 in_color) 
{
	return in_color;
}

void main()
{
	vec2 coord = distort(texcoord);

    vec4 in_color = texture(screen_texture, coord);
    color = color_shift(in_color);
    color = fade_color(color);
}