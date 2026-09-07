#include "sdf.glsl"

uniform sampler2D texUnit;
uniform vec2 noiseTextureSize;
uniform vec4 box;
uniform vec4 cornerRadius;

in vec2 vertex;

void main(void)
{
    vec2 uvNoise = vec2(gl_FragCoord.xy / noiseTextureSize);

    // Match the onscreen glass pass even when its geometry spans the full
    // rectangular card. Noise is additively blended, so mask RGB, not alpha.
    float f = sdfRoundedBox(vertex, box.xy, box.zw, cornerRadius);
    float df = fwidth(f);
    float coverage = 1.0 - clamp(0.5 + f / df, 0.0, 1.0);
    fragColor = vec4(texture(texUnit, uvNoise).rrr * coverage, 0);
}
