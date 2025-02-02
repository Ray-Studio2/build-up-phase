#version 450

layout(location = 0) in vec3 FragPos;
layout(location = 1) in vec3 Normal;
//layout(location = 2) in bool isNormalMode;

layout(location = 0) out vec4 outColor;

vec3 cameraPos = vec3(30.0, 30.0, 30.0);
vec3 lightPos = vec3(10.0, 10.0, 0.0);

vec3 materialColor = vec3(1.0, 1.0, 0.0);
vec3 materialSpecular = vec3(1.0, 1.0, 1.0);
float materialShininess = 10;

vec3 lAmbient = vec3(0.2, 0.2, 0.2);
vec3 lDiffuse = vec3(0.5, 0.5, 0.5);
vec3 lSpecular = vec3(1.0, 1.0, 1.0);

float lightConstant = 1.0;
float lightLinear = 0.09;
float lightQuadratic = 0.032;

void main() {
    /*if (isNormalMode) {
        outColor = vec4(Normal, 1.0);
        return;
    }*/

    // ambient
    vec3 ambient = lAmbient * materialColor;
  	
    // diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = lDiffuse * diff * materialColor;
    
    // specular
    vec3 viewDir = normalize(cameraPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    vec3 specular = lSpecular * spec * materialSpecular;

    float distance    = length(lightPos - FragPos);
    float attenuation = 1.0 / (lightConstant + lightLinear * distance + lightQuadratic * (distance * distance)); 
        
    ambient  *= attenuation; 
    diffuse  *= attenuation;
    specular *= attenuation;

    vec3 result = ambient + diffuse + specular;

    outColor = vec4(result, 1.0);
}
