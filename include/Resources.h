#pragma once
#include "cinder/CinderResources.h"

#define RES_AREA_STENCIL CINDER_RESOURCE( ../resources/, gfx/area.png, 128, PNG )
#define RES_MAP_BRUSHES CINDER_RESOURCE( ../resources/, gfx/brushes.png, 129, PNG )
#define RES_MAP_COLORS CINDER_RESOURCE( ../resources/, gfx/colors.png, 130, PNG )
#define RES_BACKGROUND CINDER_RESOURCE( ../resources/, gfx/layout.jpg, 131, JPG )
#define RES_SPLASHSCREEN CINDER_RESOURCE( ../resources/, gfx/splash.jpg, 132, JPG )
#define RES_BRUSHES_STENCIL CINDER_RESOURCE( ../resources/, gfx/brusharea.png, 138, PNG )

#define RES_PASSTHROUGH_VERT CINDER_RESOURCE( ../resources/, shaders/PassThrough.vert, 133, GLSL )
#define RES_MIXER_FRAG CINDER_RESOURCE( ../resources/, shaders/Mixer.frag, 134, GLSL )

#define RES_STROKE_VERT CINDER_RESOURCE( ../resources/, shaders/Stroke.vert, 135, GLSL )
#define RES_STROKE_FRAG CINDER_RESOURCE( ../resources/, shaders/Stroke.frag, 136, GLSL )
#define RES_STROKE_GEOM CINDER_RESOURCE( ../resources/, shaders/Stroke.geom, 137, GLSL )

#define RES_MENU_BACKGROUND CINDER_RESOURCE( ../resources/, gfx/menu/background.png, 139, PNG )
#define RES_MENU_HU_CLEAR_ON CINDER_RESOURCE( ../resources/, gfx/menu/hu_clear_on.png, 140, PNG )
#define RES_MENU_HU_CLEAR_OFF CINDER_RESOURCE( ../resources/, gfx/menu/hu_clear_off.png, 141, PNG )
#define RES_MENU_HU_SAVE_ON CINDER_RESOURCE( ../resources/, gfx/menu/hu_save_on.png, 142, PNG )
#define RES_MENU_HU_SAVE_OFF CINDER_RESOURCE( ../resources/, gfx/menu/hu_save_off.png, 143, PNG )

#define LICENSE_XML CINDER_RESOURCE( ../resources/, license/license.xml, 144, XML )
#define LICENSE_KEY CINDER_RESOURCE( ../resources/, license/public.pem, 145, PEM )
