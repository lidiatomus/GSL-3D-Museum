#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;

out vec4 fColor;

uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;

uniform vec3 lightDir;
uniform vec3 lightColor;

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D roughnessTexture;

uniform sampler2D normalTexture;
uniform int useNormalMap;

uniform sampler2D opacityTexture;
uniform int useOpacityMap;

uniform int useFlatShading; // 0 smooth, 1 flat
uniform mat4 lightSpaceMatrix;
uniform sampler2D shadowMap;
uniform float fogDensity;
uniform vec3  fogColor;

uniform vec3 windowLightDir;
uniform vec3 windowLightColor;

uniform mat4 windowLightSpaceMatrix;
uniform sampler2D windowShadowMap;

// SPOTLIGHTS 
#define MAX_SPOTS 3
uniform int   numSpots;

uniform vec3  spotPos[MAX_SPOTS];       // world-space positions
uniform vec3  spotDir[MAX_SPOTS];       // world-space directions (pointing where the light aims)
uniform vec3  spotColor[MAX_SPOTS];

uniform float spotCutOff[MAX_SPOTS];    // cos(innerAngle)
uniform float spotOuterCutOff[MAX_SPOTS]; // cos(outerAngle)
uniform float spotIntensity[MAX_SPOTS];

uniform float spotConstant[MAX_SPOTS];
uniform float spotLinear[MAX_SPOTS];
uniform float spotQuadratic[MAX_SPOTS];

uniform int isGlass;
uniform int isOutside;
uniform float glassFactor;

float computeShadow(vec4 worldPos)
{
    vec4 fragLS = lightSpaceMatrix * worldPos;
    vec3 proj = fragLS.xyz / fragLS.w;
    proj = proj * 0.5 + 0.5;

    if (proj.z > 1.0) return 0.0;

    float shadow = 0.0;
    // get the size of a single texel in the shadow map
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    float currentDepth = proj.z;
    float bias = 0.002;

    // loop through a 3x3 neighborhood
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, proj.xy + vec2(x, y) * texelSize).r; 
            shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;        
        }    
    }
    
    // average the 9 samples
    return shadow / 9.0;
}

float computeShadow2(vec4 worldPos, mat4 LS, sampler2D smap) {
    vec4 fragLS = LS * worldPos;
    vec3 proj = fragLS.xyz / fragLS.w;
    proj = proj * 0.5 + 0.5;

    if (proj.z > 1.0)
        return 0.0;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(smap, 0);
    float currentDepth = proj.z;
    float bias = 0.002;

    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(smap, proj.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
        }
    }

    return shadow / 9.0;
}

vec3 getNormalEye(vec3 normalEye, vec3 posEye, vec2 uv) {
    if (useNormalMap == 0)
        return normalize(normalEye);

    vec3 nMap = texture(normalTexture, uv).xyz * 2.0 - 1.0;

    vec3 dp1 = dFdx(posEye);
    vec3 dp2 = dFdy(posEye);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);

    vec3 N = normalize(normalEye);

    float det = duv1.x * duv2.y - duv1.y * duv2.x;
    if (abs(det) < 1e-8)
        return N;

    vec3 T = (dp1 * duv2.y - dp2 * duv1.y) / det;
    T = normalize(T - N * dot(N, T));
    vec3 B = normalize(cross(N, T));

    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * nMap);
}

vec3 evalSpotLight(
    int i,
    vec3 posEye,
    vec3 normalEye,
    vec3 viewDir,
    vec3 albedo,
    vec3 specMap,
    float shininess,
    float specStrength
) {
    vec3 lightPosEye = (view * vec4(spotPos[i], 1.0)).xyz;
    vec3 lightDirEye = normalize((view * vec4(spotDir[i], 0.0)).xyz);

    vec3 L = normalize(lightPosEye - posEye);
    float dist = length(lightPosEye - posEye);

    float theta = dot(L, normalize(-lightDirEye));
    float eps = spotCutOff[i] - spotOuterCutOff[i];
    float cone = clamp((theta - spotOuterCutOff[i]) / max(eps, 1e-6), 0.0, 1.0);

    float att = 1.0 / (spotConstant[i] + spotLinear[i] * dist + spotQuadratic[i] * dist * dist);

    float diff = max(dot(normalEye, L), 0.0);

    vec3 R = reflect(-L, normalEye);
    float specCoeff = pow(max(dot(viewDir, R), 0.0), shininess);

    vec3 diffuse = diff * albedo * spotColor[i];
    vec3 specular = specStrength * specCoeff * specMap * spotColor[i];

    return (diffuse + specular) * att * cone * spotIntensity[i];
}

vec3 getFlatNormal(vec3 posEye) {
    vec3 dx = dFdx(posEye);
    vec3 dy = dFdy(posEye);
    return normalize(cross(dx, dy));
}

void main() {
    // fetch Albedo and handle Alpha discarding
    vec4 albedo4 = texture(diffuseTexture, fTexCoords);
    vec3 albedo  = albedo4.rgb;
    float alpha  = albedo4.a;

    if (alpha < 0.1) discard;

    // early return for sky (no lighting/shadows)
    if (isOutside == 1) {
        fColor = vec4(albedo, 1.0);
        return;
    }

    vec3 posEye = (view * model * vec4(fPosition, 1.0)).xyz;
    vec3 viewDir = normalize(-posEye);
    vec4 worldPos = model * vec4(fPosition, 1.0);

    // normal calculation
    vec3 normalEye;
    if (useFlatShading == 1) {
        normalEye = getFlatNormal(posEye);
    } else {
        normalEye = normalize(normalMatrix * fNormal);
        normalEye = getNormalEye(normalEye, posEye, fTexCoords);
    }

    float rough = texture(roughnessTexture, fTexCoords).r;
    float shininess = mix(128.0, 8.0, rough);
    float specStrength = mix(1.0, 0.1, rough);
    vec3 specMap = texture(specularTexture, fTexCoords).rgb;

    // directional light
    vec3 sunDirEye = normalize(vec3(view * vec4(lightDir, 0.0)));
    float shadowSun = computeShadow(worldPos); 

    vec3 sunDiffuse = max(dot(normalEye, sunDirEye), 0.0) * lightColor * albedo;
    vec3 sunReflect = reflect(-sunDirEye, normalEye);
    float sunSpecCoeff = pow(max(dot(viewDir, sunReflect), 0.0), shininess);
    vec3 sunSpecular = specStrength * sunSpecCoeff * lightColor * specMap;

    // window light
    vec3 winDirEye = normalize(vec3(view * vec4(windowLightDir, 0.0)));
    float shadowWin = computeShadow2(worldPos, windowLightSpaceMatrix, windowShadowMap);

    vec3 winDiffuse = max(dot(normalEye, winDirEye), 0.0) * windowLightColor * albedo;
    vec3 winReflect = reflect(-winDirEye, normalEye);
    float winSpecCoeff = pow(max(dot(viewDir, winReflect), 0.0), shininess);
    vec3 winSpecular = specStrength * winSpecCoeff * windowLightColor * specMap;

    // light volume
    float zWindowPosition = -8.0;
    float distToWindow = abs(worldPos.z - zWindowPosition);
    float windowGlow = clamp(1.0 - (distToWindow / 5.0), 0.0, 1.0);
    windowGlow = pow(windowGlow, 2.0);
    vec3 volumeColor = windowLightColor * windowGlow * 0.25;

    // ambient light
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * albedo;

    vec3 color = ambient + volumeColor
               + (1.0 - shadowSun) * (sunDiffuse + sunSpecular)
               + (1.0 - shadowWin) * (winDiffuse + winSpecular);

    // spotlights
    for (int i = 0; i < numSpots; i++) {
        color += evalSpotLight(i, posEye, normalEye, viewDir, albedo, specMap, shininess, specStrength);
    }

    // glass transparency
    if (isGlass == 1) {
        float glassAlpha = 0.25;
        if (useOpacityMap == 1) {
            float m = texture(opacityTexture, fTexCoords).r;
            glassAlpha = mix(0.25, 1.0, m);
        }
        alpha = glassAlpha;
        color *= glassFactor;
    }

    // fog
    float fragmentDistance = length(posEye);
    float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    // final output
    fColor = mix(vec4(fogColor, 1.0), vec4(color, alpha), fogFactor);
}