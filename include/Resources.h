#pragma once
#include "cinder/CinderResources.h"

#define RES_AREA_STENCIL CINDER_RESOURCE( ../resources/, gfx/area.png, 128, PNG )
#define RES_MAP_BRUSHES CINDER_RESOURCE( ../resources/, gfx/brushes.png, 129, PNG )
#define RES_MAP_COLORS CINDER_RESOURCE( ../resources/, gfx/colors.png, 130, PNG )
#define RES_BACKGROUND CINDER_RESOURCE( ../resources/, gfx/layout.jpg, 131, JPG )
#define RES_SPLASHSCREEN CINDER_RESOURCE( ../resources/, gfx/splash.jpg, 132, JPG )

#define RES_PASSTHROUGH_VERT CINDER_RESOURCE( ../resources/, shaders/PassThrough.vert, 133, GLSL )
#define RES_MIXER_FRAG CINDER_RESOURCE( ../resources/, shaders/Mixer.frag, 134, GLSL )

#define RES_STROKE_VERT CINDER_RESOURCE( ../resources/, shaders/Stroke.vert, 135, GLSL )
#define RES_STROKE_FRAG CINDER_RESOURCE( ../resources/, shaders/Stroke.frag, 136, GLSL )
#define RES_STROKE_GEOM CINDER_RESOURCE( ../resources/, shaders/Stroke.geom, 137, GLSL )

