R"(float rand(vec2 co)
{
    return max(fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453), 0.25);
}

vec3 sRGBToLinear(vec3 color)
{
	if (length(color) <= 0.04045)
    	return color / 12.92;
	else
		return pow((color + vec3(0.055)) / 1.055, vec3(2.4));
})"