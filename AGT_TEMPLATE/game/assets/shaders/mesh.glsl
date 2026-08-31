// -----------------------------
// -- Hazel Engine PBR shader --
// -----------------------------
// Note: this shader is still very much in progress. There are likely many bugs and future additions that will go in.
//       Currently heavily updated. 
//
// References upon which this is based:
// - Unreal Engine 4 PBR notes (https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)
// - Frostbite's SIGGRAPH 2014 paper (https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/)
// - Michał Siejak's PBR project (https://github.com/Nadrin)
// - My implementation from years ago in the Sparky engine (https://github.com/TheCherno/Sparky)

#type vertex
#version 430 core                                                               
                                                                                    
layout (location = 0) in vec3 a_position;                                             
layout (location = 1) in vec3 a_normal;                                             
layout (location = 2) in vec2 a_tex_coord;                                               
layout (location = 3) in ivec4 bone_ids;
layout (location = 4) in vec4 weights;

const int MAX_BONES = 100;

uniform mat4 u_view_projection;
uniform mat4 u_transform;
uniform mat4 gBones[MAX_BONES];
uniform int num_bones;
// Same view-projection idea as u_view_projection but from the shadow-casting light's point of
// view instead of the camera's - lets the fragment stage look up this fragment's depth in the
// shadow map (see shadow_depth.glsl, which renders that map in the first place).
uniform mat4 u_light_view_projection;

out vec2 v_tex_coord;
out vec3 v_normal;
out vec3 v_position;
out vec4 v_pos;
out vec4 v_light_space_pos;

void main()
{
  vec4 pos_l;
	vec4 normal_l;
	if(num_bones>0){

		mat4 bone_transform;

		if(weights[0] <= 0)
		{
			 bone_transform = mat4(1.0f);
			 bone_transform += gBones[bone_ids[0]] * weights[0];
		}
		else
		{
			bone_transform      = gBones[bone_ids[0]] * weights[0];
		}
		bone_transform     += gBones[bone_ids[1]] * weights[1];
		bone_transform     += gBones[bone_ids[2]] * weights[2];
		bone_transform     += gBones[bone_ids[3]] * weights[3];
		pos_l = bone_transform * vec4(a_position, 1.0);
		normal_l = bone_transform * vec4(a_normal, 0.0);
		gl_Position  = u_view_projection * u_transform * pos_l;
		v_pos = gl_Position;
		v_tex_coord    = a_tex_coord;
		v_normal      = (u_transform * normal_l).xyz;
		v_position    = (u_transform * pos_l).xyz;
		v_light_space_pos = u_light_view_projection * u_transform * pos_l;
	} else {
		v_pos = u_view_projection * u_transform * vec4(a_position, 1.0);
		gl_Position = v_pos;
		v_tex_coord = a_tex_coord;
		v_normal = mat3(transpose(inverse(u_transform))) * a_normal;
		v_position = vec3(u_transform * vec4(a_position, 1.0));
		v_light_space_pos = u_light_view_projection * u_transform * vec4(a_position, 1.0);
	}
}

#type fragment
#version 430 core

layout(location = 0) out vec4 o_color;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
	float transparency;
}; 

const int MAX_POINT_LIGHTS = 2;
const int MAX_SPOT_LIGHTS = 2;

in vec2 v_tex_coord;
in vec3 v_normal;                                                                   
in vec3 v_position;                                                                 

struct VSOutput
{
    vec2 TexCoord;
    vec3 Normal;                                                                   
    vec3 WorldPos;                                                                 
};

struct BaseLight
{
    vec3 Color;
    float AmbientIntensity;
    float DiffuseIntensity;
};

struct DirectionalLight
{
    BaseLight Base;
    vec3 Direction;
};
                                                                                    
struct Attenuation                                                                  
{                                                                                   
    float Constant;                                                                 
    float Linear;                                                                   
    float Exp;                                                                      
};                                                                                  
                                                                                    
struct PointLight                                                                           
{                                                                                           
    BaseLight Base;                                                                  
    vec3 Position;                                                                          
    Attenuation Atten;                                                                      
};                                                                                          
                                                                                            
struct SpotLight                                                                            
{                                                                                           
    PointLight Base;                                                                 
    vec3 Direction;                                                                         
    float Cutoff;                                                                           
};                                                                                          
                                                                                            
uniform int gNumPointLights;                                                                
uniform int gNumSpotLights;                                                                 
uniform DirectionalLight gDirectionalLight;                                                 
uniform PointLight gPointLights[MAX_POINT_LIGHTS];                                          
uniform SpotLight gSpotLights[MAX_SPOT_LIGHTS];                                             
uniform sampler2D gColorMap;                                                                
uniform vec3 gEyeWorldPos;                                                                  
uniform float gMatSpecularIntensity;                                                        
uniform float gSpecularPower;
uniform bool has_texture = false;
uniform Material material;
uniform bool skybox_rendering = false;
uniform float transparency;
uniform bool lighting_on = true;
uniform bool fog_on = false;
uniform vec3 fog_colour;
uniform int fog_factor_type;
// Were hardcoded locals (rho=0.15, fog_start=3, fog_end=15) tuned against clip-space
// distance below rather than real world units - exposed as uniforms with sensible
// world-unit defaults now that the distance calculation is fixed to use v_position.
uniform float fog_density = 0.01f;
uniform float fog_start = 60.0f;
uniform float fog_end = 250.0f;
in vec4 v_pos;

uniform bool colouring_on = false;
uniform vec3 in_colour = vec3(1,0,0);

// SHADOW MAPPING: a single directional-light shadow map (see engine::shadow_map / the
// ShadowRenderer class that fills it). shadows_on defaults false so any shader instance that
// never gets these uniforms set (or whose shadow map failed to initialise) just renders as if
// there were no shadows, rather than sampling an unbound texture.
uniform sampler2D shadow_map;
uniform bool shadows_on = false;
in vec4 v_light_space_pos;

// Returns 0 (fully lit) to 1 (fully in shadow) for this fragment under the directional light.
// 3x3 PCF (percentage-closer filtering) softens the shadow edge instead of a single hard-edged
// sample per fragment - cheap given this is only evaluated once per fragment, not per light.
float calculate_shadow(vec3 normal)
{
	vec3 proj_coords = v_light_space_pos.xyz / v_light_space_pos.w;
	proj_coords = proj_coords * 0.5 + 0.5; // NDC [-1,1] -> texture/depth [0,1]

	// Beyond the shadow frustum's far plane - nothing there to be in shadow from.
	if (proj_coords.z > 1.0) return 0.0;

	float current_depth = proj_coords.z;
	// Slope-scaled bias: faces nearly edge-on to the light need a bigger push than faces
	// square-on to it, or they self-shadow in a moire pattern ("shadow acne").
	float slope = 1.0 - max(dot(normal, -gDirectionalLight.Direction), 0.0);
	float bias = max(0.0025 * slope, 0.0008);

	float shadow = 0.0;
	vec2 texel_size = 1.0 / vec2(textureSize(shadow_map, 0));
	for (int x = -1; x <= 1; x++) {
		for (int y = -1; y <= 1; y++) {
			float pcf_depth = texture(shadow_map, proj_coords.xy + vec2(x, y) * texel_size).r;
			shadow += (current_depth - bias > pcf_depth) ? 1.0 : 0.0;
		}
	}
	return shadow / 9.0;
}


vec4 CalcLightInternal(BaseLight Light, vec3 LightDirection, VSOutput In, float ShadowFactor)
{
	vec4 AmbientColor;
	if(has_texture)
		AmbientColor = vec4(Light.Color * Light.AmbientIntensity, 1.0f);
    else
		AmbientColor = vec4(Light.Color * Light.AmbientIntensity * material.ambient, 1.0f);
    float DiffuseFactor = dot(In.Normal, -LightDirection);

    vec4 DiffuseColor  = vec4(0, 0, 0, 0);
    vec4 SpecularColor = vec4(0, 0, 0, 0);

    if (DiffuseFactor > 0.0) {
        if(has_texture)
			DiffuseColor = vec4(Light.Color * Light.DiffuseIntensity * DiffuseFactor, 1.0f);
        else
			DiffuseColor = vec4(Light.Color * Light.DiffuseIntensity * DiffuseFactor * material.diffuse, 1.0f);
        vec3 VertexToEye = normalize(gEyeWorldPos - In.WorldPos);
        vec3 LightReflect = normalize(reflect(LightDirection, In.Normal));
        float SpecularFactor = dot(VertexToEye, LightReflect);
        if (SpecularFactor > 0.0) {
			SpecularFactor = pow(SpecularFactor, gSpecularPower);
			if(has_texture)
				SpecularColor = vec4(Light.Color * gMatSpecularIntensity * SpecularFactor, 1.0f);
			else
				SpecularColor = vec4(Light.Color * gMatSpecularIntensity * SpecularFactor * material.specular * material.shininess, 1.0f);
        }
    }

    // Shadow only dims the diffuse/specular contribution, not ambient - a fully-shadowed
    // surface should still read as dimly ambient-lit, not pure black.
    return (AmbientColor + (DiffuseColor + SpecularColor) * (1.0 - ShadowFactor));
}

vec4 CalcDirectionalLight(VSOutput In)
{
    float shadow = (shadows_on) ? calculate_shadow(In.Normal) : 0.0;
    return CalcLightInternal(gDirectionalLight.Base, gDirectionalLight.Direction, In, shadow);
}

vec4 CalcPointLight(PointLight l, VSOutput In)
{
    vec3 LightDirection = In.WorldPos - l.Position;
    float Distance = length(LightDirection);
    LightDirection = normalize(LightDirection);

    // Point/spot lights aren't tested against the (single, directional-light) shadow map -
    // only the directional light's own contribution above is.
    vec4 Color = CalcLightInternal(l.Base, LightDirection, In, 0.0);
    float Attenuation =  l.Atten.Constant +
                         l.Atten.Linear * Distance +
                         l.Atten.Exp * Distance * Distance;

    return Color / Attenuation;
}                                                                                           
                                                                                            
vec4 CalcSpotLight(SpotLight l, VSOutput In)                                         
{                                                                                           
    vec3 LightToPixel = normalize(In.WorldPos - l.Base.Position);                             
    float SpotFactor = dot(LightToPixel, l.Direction);                                      
                                                                                            
    if (SpotFactor > l.Cutoff) {                                                            
        vec4 Color = CalcPointLight(l.Base, In);                                        
        return Color * (1.0 - (1.0 - SpotFactor) * 1.0/(1.0 - l.Cutoff));                   
    }                                                                                       
    else {                                                                                  
        return vec4(0,0,0,0);                                                               
    }                                                                                       
}

void main()
{                                    
    VSOutput In;
    In.TexCoord = v_tex_coord;
    In.Normal   = normalize(v_normal);
    In.WorldPos = v_position;

	vec4 result;

	if(skybox_rendering || !lighting_on)
	{
	  if(has_texture) {
	      result = texture(gColorMap, In.TexCoord.xy);
        result.w *= transparency;
	  }
	  else
	      result = vec4(material.ambient, material.transparency);

		// uncomment the following to affect the skybox with the light colour
		//result = light.colour * texture(gColorMap, In.TexCoord.xy);
	}
	else
	{
		vec4 TotalLight = CalcDirectionalLight(In);                                         
                                                                                            
		for (int i = 0 ; i < gNumPointLights ; i++) {                                           
			TotalLight += CalcPointLight(gPointLights[i], In);                              
		}                                                                                       
                                                                                            
		for (int i = 0 ; i < gNumSpotLights ; i++) {                                            
			TotalLight += CalcSpotLight(gSpotLights[i], In);                                
		}

		if(has_texture) {
		    result = texture(gColorMap, In.TexCoord.xy) * TotalLight;
		    result.w *= transparency;
		}
		
		else {
			if(material.transparency<1.0)
			    TotalLight.w = material.transparency;
			result = TotalLight;
        }
	}

	if(fog_on)
	{
		// True world-space distance from the eye, not the clip-space vector length this used
		// to measure (which isn't a real distance at all pre-perspective-divide) - this is why
		// fog_start/fog_end can now use plausible world-unit values instead of ~3-15.
		float d = length(v_position - gEyeWorldPos);
		float w;
		if(fog_factor_type == 0) {
			w = clamp((fog_end - d) / (fog_end - fog_start), 0.0, 1.0);
		} else if (fog_factor_type == 1) {
			w = exp(-(fog_density*d));
		} else {
			w = exp(-(fog_density*d)*(fog_density*d));
		}
		result.rgb = mix(fog_colour, result.rgb, w);
	}

	if(colouring_on)
	{
		result = result * vec4(in_colour, transparency);
	}
	
    o_color = result;
}
