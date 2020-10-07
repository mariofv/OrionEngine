const float gamma = 2.2;
const float W = 11.2;

#if ENABLE_MSAA
uniform sampler2DMS screen_texture;
uniform sampler2DMS normalMap; // in view space
uniform sampler2DMS positionMap; // in view space
uniform sampler2DMS ssrValuesMap;
#else
uniform sampler2D screen_texture;
//Reflections
uniform sampler2D normalMap; // in view space
uniform sampler2D positionMap; // in view space
uniform sampler2D ssrValuesMap;
#endif
uniform sampler2D brightness_texture;

uniform float exposure;
uniform float zNear;
uniform float zFar;

in vec2 texCoord;
layout (location = 0) out vec4 FragColor;

vec3 ToneMapping(vec3 color);
vec3 Uncharted2Tonemap(vec3 x);
vec3 Reflections(float reflection_strength);


layout (std140) uniform Matrices
{
  mat4 model;
  mat4 proj;
	mat4 view;
} matrices;


const int MAX_BINARY_SEARCH_COUNT = 10;
const int MAX_NUMBER_INCREMENTS = 100; // 60
const float MAX_Z_VALUE = -0.1; // 0.025
const float MIN_Z_VALUE = -0.001; // 0.025
const float MIN_RAY_STEP = 0.2;

#if ENABLE_MSAA
vec4 GetTexture(sampler2DMS texture_uniform,vec2 coordinates)
#else
vec4 GetTexture(sampler2D texture_uniform,vec2 coordinates)
#endif
{
	#if ENABLE_MSAA
		ivec2 vp = ivec2(vec2(textureSize(texture_uniform)) * coordinates);
		vec4 sample1 = texelFetch(texture_uniform, vp, 0);
		vec4 sample2 = texelFetch(texture_uniform, vp, 1);
		vec4 sample3 = texelFetch(texture_uniform, vp, 2);
		vec4 sample4 = texelFetch(texture_uniform, vp, 3);

		return (sample1 + sample2 + sample3 + sample4) / 4.0f;
	#else
		return texture(texture_uniform, coordinates);
	#endif
}
// Transform hit_coordinate to screen coordinate
vec4 GetCoordinatesInScreenSpace(vec3 in_coordinates)
{
    vec4 out_coordinates = matrices.proj* vec4(in_coordinates, 1.0);
    out_coordinates.xy /= out_coordinates.w;
    out_coordinates.xy = out_coordinates.xy *0.5 + 0.5;
	return out_coordinates;
}

float linearize_depth(float d)
{
    return zNear * zFar / (zFar + abs(d) * (zNear - zFar)); // abs because z is negative in right handed system
}

vec3 BinarySearch(inout vec3 direction, inout vec3 hit_coordinate)
{
    float depth;

    vec4 projectedCoord;
 
    for(int i = 0; i < MAX_BINARY_SEARCH_COUNT; i++)
    {

		projectedCoord = GetCoordinatesInScreenSpace(hit_coordinate);
 
        float depth = linearize_depth(GetTexture(positionMap,projectedCoord.xy).z);

 
        float depth_diff =linearize_depth(hit_coordinate.z) - depth;

        direction *= 0.2;
        if(depth_diff > 0.0)
            hit_coordinate += direction;
        else
            hit_coordinate -= direction;    
    }

	projectedCoord = GetCoordinatesInScreenSpace(hit_coordinate);
 
    return vec3(projectedCoord.xy, depth);
}
vec4 RayCast(vec3 direction,vec3 hit_coordinate) {
    //Increase ray lenght until we find something
	direction*=MIN_RAY_STEP;
	vec4 projectedCoord ;
	float original_depth = linearize_depth(hit_coordinate.z);
   for (int i = 0; i < MAX_NUMBER_INCREMENTS; i++) {

        hit_coordinate += direction;
		projectedCoord = GetCoordinatesInScreenSpace(hit_coordinate);

		float depth = linearize_depth(GetTexture(positionMap,projectedCoord.xy).z);
		float depth_diff = depth - linearize_depth(hit_coordinate.z);
		//Avoid going over the depth diferent threshold  and check if we have hit something
        if (depth_diff < MIN_Z_VALUE && depth_diff > MAX_Z_VALUE && original_depth < depth)
		{		
			return  vec4(BinarySearch(direction,hit_coordinate),1.0f);
		}
    }

	return vec4(0,0,0,0.0);
}

void main()
{
  vec4 fragment_color = GetTexture( screen_texture,texCoord);

#if ENABLE_BLOOM
  vec4 brightness_color = texture(brightness_texture, texCoord);
  fragment_color += brightness_color;
#endif

#if ENABLE_HDR
  fragment_color.rgb = ToneMapping(fragment_color.rgb);
#endif

	float reflection_strength = GetTexture(ssrValuesMap, texCoord).r;

	vec3 reflection_texture ;

	if(reflection_strength > 0.0)
	{
		reflection_texture = Reflections(reflection_strength);
	}
  	FragColor.rgb =  reflection_texture +  fragment_color.rgb;
	FragColor.rgb = pow(FragColor.rgb, vec3(1 / gamma));
	FragColor.a = 1.0;

}

vec3 ToneMapping(vec3 color)
{
#if ENABLE_REINHARD
  return color.rgb / (color.rgb + vec3(1.0));

#elif ENABLE_FILMIC
  vec3 hdr = color * exposure;
  vec3 curr = Uncharted2Tonemap(2.0 * hdr);
  vec3 whiteScale = 1.0/Uncharted2Tonemap(vec3(W));
  return curr*whiteScale;

#elif ENABLE_EXPOSURE
  return vec3(1.0) - exp(-color * exposure);

#else
  return color;
#endif
}

vec3 Uncharted2Tonemap(vec3 x)
{
   const float A = 0.22;
   const float B = 0.3;
   const float C = 0.1;
   const float D = 0.2;
   const float E = 0.01;
   const float F = 0.30;
   return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 Reflections(float reflection_strength)
{
	vec3 view_normal = vec3(GetTexture(normalMap, texCoord) * inverse(matrices.view));
	vec3 view_position = vec3(GetTexture(positionMap, texCoord));

	//Reflection
	vec3 reflected = normalize( reflect(normalize(view_position), normalize(view_normal))) ;
   
	vec3 hit_position = view_position;
	vec4 coords = RayCast(reflected , hit_position);

	vec2 dCoords = smoothstep(0.2, 0.6, abs(vec2(0.5, 0.5) - coords.xy));
	float screenEdgefactor = clamp(1.0 - (dCoords.x + dCoords.y), 0.0, 1.0) * reflection_strength;

	if(coords.w > 0)
	{
		vec3 SSR = GetTexture(screen_texture,coords.xy).xyz * screenEdgefactor;
		return SSR;	
	}else
	{
		return vec3(coords.x,coords.y,0);
	}

}
