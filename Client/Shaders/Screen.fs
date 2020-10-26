#version 330 core
out vec4 fragColour;

in vec2 TexCoords;
uniform float exposure;
uniform sampler2D screenTexSampler;
uniform sampler2D blurTexSampler;

const vec3 visionColour = vec3(.2f, 1.f, .4f);
const float luminanceThreshold = .6f;
const float colourAmplification = 2.f;
uniform bool nightVision;

void main(){
	fragColour = vec4(texture(screenTexSampler, TexCoords).rgb, 1.f);
	fragColour.rgb += texture(blurTexSampler, TexCoords).rgb; //Additive blending

	//FragColor.rgb = FragColor.rgb / (FragColor.rgb + vec3(1.f)); //Reinhard tone mapping alg (evenly balances out colour values, tend to slightly favour bright areas)
    fragColour.rgb = vec3(1.f) - exp(-fragColour.rgb * exposure); //Exposure tone mapping (...)

    if(nightVision){
		float lum = dot(vec3(.42f, .62f, .14f), fragColour.rgb);
		if(lum < luminanceThreshold){
			fragColour.rgb *= colourAmplification; 
		}
		fragColour.rgb = fragColour.rgb * visionColour;
	}
}