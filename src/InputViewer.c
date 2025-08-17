#include "modding.h"
#include "global.h"
#include "recompconfig.h"
#include "rt64_extended_gbi.h"
#include "libu64/pad.h"

#define INCBIN(identifier, filename)         \
    asm(".pushsection .rodata\n"             \
        "\t.local " #identifier "\n"         \
        "\t.type " #identifier ", @object\n" \
        "\t.balign 8\n" #identifier ":\n"    \
        "\t.incbin \"" filename "\"\n\n"     \
                                             \
        "\t.balign 8\n"                      \
        "\t.popsection\n");                  \
    extern u8 identifier[]

INCBIN(Analog_Ia8, "src/Assets/analog_ia8.bin");
INCBIN(Dpad_Ia8, "src/Assets/dpad_ia8.bin");
INCBIN(Stick_Ia8, "src/Assets/stick_ia8.bin");
INCBIN(Dpad_up_Ia8, "src/Assets/dpad_up_ia8.bin");
INCBIN(Dpad_down_Ia8, "src/Assets/dpad_down_ia8.bin");
INCBIN(Dpad_left_Ia8, "src/Assets/dpad_left_ia8.bin");
INCBIN(Dpad_right_Ia8, "src/Assets/dpad_right_ia8.bin");
INCBIN(Cbuttons_Ia8, "src/Assets/stick_ia8.bin");
INCBIN(Cbuttons_pressed_Ia8, "src/Assets/button_pressed_ia8.bin");
INCBIN(Start_Ia8, "src/Assets/stick_ia8.bin");
INCBIN(Start_pressed_Ia8, "src/Assets/button_pressed_ia8.bin");
INCBIN(Trigger_Ia8, "src/Assets/trigger_ia8.bin");
INCBIN(Trigger_pressed_Ia8, "src/Assets/trigger_pressed_ia8.bin");
INCBIN(ABbuttons_Ia8, "src/Assets/stick_ia8.bin");
INCBIN(ABbuttons_pressed_Ia8, "src/Assets/button_pressed_ia8.bin");

void Interface_SetOrthoView(InterfaceContext* interfaceCtx) {
    SET_FULLSCREEN_VIEWPORT(&interfaceCtx->view);
    View_ApplyOrthoToOverlay(&interfaceCtx->view);

}

// Options from mod.toml

typedef enum 
{
	OFF,
	ON,
} DeadzoneViewer;

typedef enum
{
	Off,
	On,
} LBUTTON;

typedef enum
{
	Left,
	Center,
	Right,
} OverlayPlacement;

#define DEADZONE_SHOW ((DeadzoneViewer)recomp_get_config_u32("input_display_deadzone"))
#define L_BUTTON_PLACEMENT ((LBUTTON)recomp_get_config_u32("button_placement_option"))
#define OVERLAY_CUSTOM ((OverlayPlacement)recomp_get_config_u32("overlay_placement_option"))

/**
 * `x` vertex x
 * `y` vertex y
 * `z` vertex z
 * `s` texture s coordinate
 * `t` texture t coordinate
 * `crnx` red component of color vertex, or x component of normal vertex
 * `cgny` green component of color vertex, or y component of normal vertex
 * `cbnz` blue component of color vertex, or z component of normal vertex
 * `a` alpha
 */
#define VTX(x, y, z, s, t, crnx, cgny, cbnz, a) \
    { { { x, y, z }, 0, { s, t }, { crnx, cgny, cbnz, a } }, }

#define gEXMatrixGroupDecomposedNormal(cmd, push, proj, edit) \

Vtx gStickVtx[] = {
    VTX( 0,   0, 0,  0 << 5,  0 << 5, 0, 0, 0, 0xFF), // top-left vertex
    VTX(64,   0, 0, 64 << 5,  0 << 5, 0, 0, 0, 0xFF), // top-right vertex
    VTX(64, -64, 0, 64 << 5, 64 << 5, 0, 0, 0, 0xFF), // bottom-right vertex
    VTX( 0, -64, 0,  0 << 5, 64 << 5, 0, 0, 0, 0xFF), // bottom-left vertex
};


#define BUTTONS_W 32
#define BUTTONS_H 32

#define BUTTONS_IMG_W 64
#define BUTTONS_IMG_H 64

#define BUTTONS_DSDX (s32)(2048.0f * (float)(BUTTONS_IMG_W) / (BUTTONS_W))
#define BUTTONS_DTDY (s32)(2048.0f * (float)(BUTTONS_IMG_H) / (BUTTONS_H))

#define top_left_y (int) recomp_get_config_double("pos_y_display")
float scale = 1.0f;
s32 margin_reduction = 8;

//Store of the inputs button
Input g_ControllerInput;

RECOMP_CALLBACK("*", recomp_on_play_main)
void KeepInputs(PlayState* play)
{
	g_ControllerInput = *CONTROLLER1(&play->state);
}

// Majora's mask runs at 320x240
// Note: The center of the screen is at 160, 120

RECOMP_HOOK("Interface_Draw") // Draws ALL the inputs (pressed or not pressed)
void DrawInput(PlayState* play)
{
	int top_left_x = 0;
	if (OVERLAY_CUSTOM == Left)
	{
		top_left_x = top_left_x + 145;
	}
	else if (OVERLAY_CUSTOM == Center)
	{
		top_left_x = top_left_x + 86;
	}
	else if (OVERLAY_CUSTOM == Right)
	{
		top_left_x = top_left_x + 75;
	}
	int buttom_right_x = top_left_x + 20;
	int buttom_right_y =  top_left_y + 20;
	
	InterfaceContext* interfaceCtx = &play->interfaceCtx;

	OPEN_DISPS(play->state.gfxCtx);

    gEXPushScissor(OVERLAY_DISP++);
    gEXSetScissor(OVERLAY_DISP++, G_SC_NON_INTERLACE, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_RIGHT, 0, 0, 0, SCREEN_HEIGHT);
    gDPPipeSync(OVERLAY_DISP++);
    gDPSetCycleType(OVERLAY_DISP++, G_CYC_1CYCLE);
    gDPSetRenderMode(OVERLAY_DISP++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode(OVERLAY_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);

	
			// Analog 

	if (DEADZONE_SHOW == ON)
	{
		float Radius = sqrt(g_ControllerInput.cur.stick_x*g_ControllerInput.cur.stick_x + g_ControllerInput.cur.stick_y* g_ControllerInput.cur.stick_y);
		if (Radius <= 15)
		{
		gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 0, 0, 255);
		}
		else if (Radius <= 28)
		{
		gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 255, 0, 255);
		}
		else
		{
		gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 0, 0, 255);	
		}
	}
	else
	{
	gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, 255);
	}
	gDPLoadTextureBlock(OVERLAY_DISP++, Analog_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,BUTTONS_IMG_H,0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
    if (OVERLAY_CUSTOM == Left)
	{
	gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, (top_left_x - 140 ) <<2, (top_left_y + 70) <<2, (buttom_right_x - 140 ) <<2, (buttom_right_y + 70) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale - 0.1), BUTTONS_DTDY* (scale - 0.1));		
	}
	else if (OVERLAY_CUSTOM == Center)
	{
	gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER, (top_left_x - 140 ) <<2, (top_left_y + 70) <<2, (buttom_right_x - 140 ) <<2, (buttom_right_y + 70) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale - 0.1), BUTTONS_DTDY* (scale - 0.1));		
	}
	else if (OVERLAY_CUSTOM == Right)
	{
	gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT, (top_left_x - 140 ) <<2, (top_left_y + 70) <<2, (buttom_right_x - 140 ) <<2, (buttom_right_y + 70) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale - 0.1), BUTTONS_DTDY* (scale - 0.1));	
	}
	gDPPipeSync(OVERLAY_DISP++);

			//D-PAD + Direction
	
	
	gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 128, 128, 128, 255); 
	gDPLoadTextureBlock(OVERLAY_DISP++, Dpad_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,BUTTONS_IMG_H,0
			,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);		

	if (g_ControllerInput.cur.button & BTN_DUP)
	{
	gDPLoadTextureBlock(OVERLAY_DISP++, Dpad_up_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,BUTTONS_IMG_H,0
			,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);		
	}
	if (g_ControllerInput.cur.button & BTN_DDOWN)
	{
	gDPLoadTextureBlock(OVERLAY_DISP++, Dpad_down_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,BUTTONS_IMG_H,0
			,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);	
	}
	if (g_ControllerInput.cur.button & BTN_DLEFT)
	{
	gDPLoadTextureBlock(OVERLAY_DISP++, Dpad_left_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,BUTTONS_IMG_H,0
			,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);	
	}
	if (g_ControllerInput.cur.button & BTN_DRIGHT)
	{
	gDPLoadTextureBlock(OVERLAY_DISP++, Dpad_right_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,BUTTONS_IMG_H,0
			,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);	
	}
    if (OVERLAY_CUSTOM == Left)
	{
	gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, (top_left_x - 124 ) <<2, (top_left_y + 81) <<2, (buttom_right_x - 124 ) <<2, (buttom_right_y + 81) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale + 0.2), BUTTONS_DTDY * (scale + 0.2));
	}
	else if (OVERLAY_CUSTOM == Center)
	{
	gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER, (top_left_x - 124 ) <<2, (top_left_y + 81) <<2, (buttom_right_x - 124 ) <<2, (buttom_right_y + 81) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale + 0.2), BUTTONS_DTDY * (scale + 0.2));
	}
	else if (OVERLAY_CUSTOM == Right)
	{
	gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT, (top_left_x - 124 ) <<2, (top_left_y + 81) <<2, (buttom_right_x - 124 ) <<2, (buttom_right_y + 81) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale + 0.2), BUTTONS_DTDY * (scale + 0.2));		
	}
	gDPPipeSync(OVERLAY_DISP++);


			// C-Buttons Up 


	gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 0, 255); 
	gDPLoadTextureBlock(OVERLAY_DISP++, Cbuttons_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,BUTTONS_IMG_H,0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
	if (g_ControllerInput.cur.button & BTN_CUP)
	{
	gDPLoadTextureBlock(OVERLAY_DISP++, Cbuttons_pressed_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,BUTTONS_IMG_H,0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
	}
    if (OVERLAY_CUSTOM == Left)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, (top_left_x - 98) <<2, (top_left_y + 66) <<2, (buttom_right_x - 98) <<2, (buttom_right_y + 66) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX *(scale + 0.2), BUTTONS_DTDY * (scale + 0.2));
	}
	else if (OVERLAY_CUSTOM == Center)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER, (top_left_x - 98) <<2, (top_left_y + 66) <<2, (buttom_right_x - 98) <<2, (buttom_right_y + 66) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX *(scale + 0.2), BUTTONS_DTDY * (scale + 0.2));
	}
	else if (OVERLAY_CUSTOM == Right)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT, (top_left_x - 98) <<2, (top_left_y + 66) <<2, (buttom_right_x - 98) <<2, (buttom_right_y + 66) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX *(scale + 0.2), BUTTONS_DTDY * (scale + 0.2));	
	}
	gDPPipeSync(OVERLAY_DISP++);
	

			// C-Buttons Down


	gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 0, 255); 
	gDPLoadTextureBlock(OVERLAY_DISP++, Cbuttons_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,BUTTONS_IMG_H,0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
	if (g_ControllerInput.cur.button & BTN_CDOWN)
	{
	gDPLoadTextureBlock(OVERLAY_DISP++, Cbuttons_pressed_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,BUTTONS_IMG_H,0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
	}
    if (OVERLAY_CUSTOM == Left)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, (top_left_x- 98) <<2, (top_left_y + 80) <<2, (buttom_right_x- 98) <<2, (buttom_right_y+ 80) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX* (scale + 0.2), BUTTONS_DTDY* (scale + 0.2));
	}
	else if (OVERLAY_CUSTOM == Center)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER, (top_left_x- 98) <<2, (top_left_y + 80) <<2, (buttom_right_x- 98) <<2, (buttom_right_y+ 80) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX* (scale + 0.2), BUTTONS_DTDY* (scale + 0.2));
	}
	else if (OVERLAY_CUSTOM == Right)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT, (top_left_x- 98) <<2, (top_left_y + 80) <<2, (buttom_right_x- 98) <<2, (buttom_right_y+ 80) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX* (scale + 0.2), BUTTONS_DTDY* (scale + 0.2));		
	}
	gDPPipeSync(OVERLAY_DISP++);
	

			// C-Buttons Left


	gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 0, 255); 
	gDPLoadTextureBlock(OVERLAY_DISP++, Cbuttons_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,BUTTONS_IMG_H,0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
	if (g_ControllerInput.cur.button & BTN_CLEFT)
	{
	gDPLoadTextureBlock(OVERLAY_DISP++, Cbuttons_pressed_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,BUTTONS_IMG_H,0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
	}
    if (OVERLAY_CUSTOM == Left)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, (top_left_x - 105) <<2, (top_left_y + 73) <<2, (buttom_right_x - 105) <<2, (buttom_right_y+ 73) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX*(scale + 0.2), BUTTONS_DTDY* (scale + 0.2));
	}
	else if (OVERLAY_CUSTOM == Center)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER, (top_left_x - 105) <<2, (top_left_y + 73) <<2, (buttom_right_x - 105) <<2, (buttom_right_y+ 73) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX*(scale + 0.2), BUTTONS_DTDY* (scale + 0.2));
	}
	else if (OVERLAY_CUSTOM == Right)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT, (top_left_x - 105) <<2, (top_left_y + 73) <<2, (buttom_right_x - 105) <<2, (buttom_right_y+ 73) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX*(scale + 0.2), BUTTONS_DTDY* (scale + 0.2));	
	}
	gDPPipeSync(OVERLAY_DISP++);

			// C-Buttons Right


	gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 0, 255); 
	gDPLoadTextureBlock(OVERLAY_DISP++, Cbuttons_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,BUTTONS_IMG_H,0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
	if (g_ControllerInput.cur.button & BTN_CRIGHT)
	{
	gDPLoadTextureBlock(OVERLAY_DISP++, Cbuttons_pressed_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,BUTTONS_IMG_H,0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
	}
    if (OVERLAY_CUSTOM == Left)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, (top_left_x -91) <<2, (top_left_y+ 73) <<2, (buttom_right_x -91) <<2, (buttom_right_y+ 73) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX*(scale + 0.2), BUTTONS_DTDY*(scale + 0.2));
	}
	else if (OVERLAY_CUSTOM == Center)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER, (top_left_x -91) <<2, (top_left_y+ 73) <<2, (buttom_right_x -91) <<2, (buttom_right_y+ 73) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX*(scale + 0.2), BUTTONS_DTDY*(scale + 0.2));
	}
	else if (OVERLAY_CUSTOM == Right)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT, (top_left_x -91) <<2, (top_left_y+ 73) <<2, (buttom_right_x -91) <<2, (buttom_right_y+ 73) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX*(scale + 0.2), BUTTONS_DTDY*(scale + 0.2));
	}
	gDPPipeSync(OVERLAY_DISP++);
	
	if (L_BUTTON_PLACEMENT == On)
	{
			// Start (On Option)
		
	gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 0, 0, 255); 
	gDPLoadTextureBlock(OVERLAY_DISP++, Start_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,BUTTONS_IMG_H,0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
	if (g_ControllerInput.cur.button & BTN_START)
	{
	gDPLoadTextureBlock(OVERLAY_DISP++, Start_pressed_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,BUTTONS_IMG_H,0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);		
	}
    if (OVERLAY_CUSTOM == Left)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, (top_left_x - 114 ) <<2, (top_left_y + 57) <<2, (buttom_right_x - 114 ) <<2, (buttom_right_y + 57) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale + 0.82), BUTTONS_DTDY * (scale + 0.82));
	}
	else if (OVERLAY_CUSTOM == Center)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER, (top_left_x - 114 ) <<2, (top_left_y + 57) <<2, (buttom_right_x - 114 ) <<2, (buttom_right_y + 57) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale + 0.82), BUTTONS_DTDY * (scale + 0.82));
	}
	else if (OVERLAY_CUSTOM == Right)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT, (top_left_x - 114 ) <<2, (top_left_y + 57) <<2, (buttom_right_x - 114 ) <<2, (buttom_right_y + 57) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale + 0.82), BUTTONS_DTDY * (scale + 0.82));
	}
	gDPPipeSync(OVERLAY_DISP++);			
			


			// Trigger R (On Option)


	gDPSetPrimColor(OVERLAY_DISP++, 0, 0,255, 255, 255, 255); 
	gDPLoadTextureBlock(OVERLAY_DISP++, Trigger_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,(BUTTONS_IMG_H - 32),0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
	if (g_ControllerInput.cur.button & BTN_R)
	{
	gDPLoadTextureBlock(OVERLAY_DISP++, Trigger_pressed_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,(BUTTONS_IMG_W - 32),0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
	}
    if (OVERLAY_CUSTOM == Left)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, (top_left_x - 100 ) <<2, (top_left_y + 58) <<2, (buttom_right_x - 100 ) <<2, (buttom_right_y + 58) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale - 0.2), BUTTONS_DTDY * (scale));
	}
	else if (OVERLAY_CUSTOM == Center)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER, (top_left_x - 100 ) <<2, (top_left_y + 58) <<2, (buttom_right_x - 100 ) <<2, (buttom_right_y + 58) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale - 0.2), BUTTONS_DTDY * (scale));
	}
	else if (OVERLAY_CUSTOM == Right)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT, (top_left_x - 100 ) <<2, (top_left_y + 58) <<2, (buttom_right_x - 100 ) <<2, (buttom_right_y + 58) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale - 0.2), BUTTONS_DTDY * (scale));	
	}
	gDPPipeSync(OVERLAY_DISP++);

			// Trigger Z (On Option)


	gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, 255); 
	gDPLoadTextureBlock(OVERLAY_DISP++, Trigger_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,(BUTTONS_IMG_H - 32),0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
	if (g_ControllerInput.cur.button & BTN_Z)
	{
	gDPLoadTextureBlock(OVERLAY_DISP++, Trigger_pressed_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,(BUTTONS_IMG_H - 32),0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
	}
    if (OVERLAY_CUSTOM == Left)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, (top_left_x - 120 ) <<2, (top_left_y + 63) <<2, (buttom_right_x - 120 ) <<2, (buttom_right_y + 63) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale - 0.2), BUTTONS_DTDY * (scale));
	}
	else if (OVERLAY_CUSTOM == Center)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER, (top_left_x - 120 ) <<2, (top_left_y + 63) <<2, (buttom_right_x - 120 ) <<2, (buttom_right_y + 63) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale - 0.2), BUTTONS_DTDY * (scale));
	}
	else if (OVERLAY_CUSTOM == Right)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT, (top_left_x - 120 ) <<2, (top_left_y + 63) <<2, (buttom_right_x - 120 ) <<2, (buttom_right_y + 63) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale - 0.2), BUTTONS_DTDY * (scale));	
	}
	gDPPipeSync(OVERLAY_DISP++);	

			// Trigger L (On Option)

	gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, 255); 
	gDPLoadTextureBlock(OVERLAY_DISP++, Trigger_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,(BUTTONS_IMG_H - 32),0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
	if (g_ControllerInput.cur.button & BTN_L)
	{
	gDPLoadTextureBlock(OVERLAY_DISP++, Trigger_pressed_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,(BUTTONS_IMG_H - 32),0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
	}
    if (OVERLAY_CUSTOM == Left)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, (top_left_x - 140 ) <<2, (top_left_y + 58) <<2, (buttom_right_x - 140 ) <<2, (buttom_right_y + 58) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale - 0.2), BUTTONS_DTDY * (scale));
	}
	else if (OVERLAY_CUSTOM == Center)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER, (top_left_x - 140 ) <<2, (top_left_y + 58) <<2, (buttom_right_x - 140 ) <<2, (buttom_right_y + 58) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale - 0.2), BUTTONS_DTDY * (scale));
	}
	else if (OVERLAY_CUSTOM == Right)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT, (top_left_x - 140 ) <<2, (top_left_y + 58) <<2, (buttom_right_x - 140 ) <<2, (buttom_right_y + 58) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale - 0.2), BUTTONS_DTDY * (scale));
	}
	gDPPipeSync(OVERLAY_DISP++);	
	
	}
	else
	{
			// Start (Off Option)
		
	gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 0, 0, 255); 
	gDPLoadTextureBlock(OVERLAY_DISP++, Start_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,BUTTONS_IMG_H,0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
	if (g_ControllerInput.cur.button & BTN_START)
	{
	gDPLoadTextureBlock(OVERLAY_DISP++, Start_pressed_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,BUTTONS_IMG_H,0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);		
	}
    if (OVERLAY_CUSTOM == Left)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, (top_left_x - 114 ) <<2, (top_left_y + 57) <<2, (buttom_right_x - 114 ) <<2, (buttom_right_y + 57) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale + 0.82), BUTTONS_DTDY * (scale + 0.82));
	}
	else if (OVERLAY_CUSTOM == Center)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER, (top_left_x - 114 ) <<2, (top_left_y + 57) <<2, (buttom_right_x - 114 ) <<2, (buttom_right_y + 57) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale + 0.82), BUTTONS_DTDY * (scale + 0.82));
	}
	else if (OVERLAY_CUSTOM == Right)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT, (top_left_x - 114 ) <<2, (top_left_y + 57) <<2, (buttom_right_x - 114 ) <<2, (buttom_right_y + 57) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale + 0.82), BUTTONS_DTDY * (scale + 0.82));
	}
	gDPPipeSync(OVERLAY_DISP++);		
		


			// Trigger R (Off Option)


	gDPSetPrimColor(OVERLAY_DISP++, 0, 0,255, 255, 255, 255); 
	gDPLoadTextureBlock(OVERLAY_DISP++, Trigger_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,(BUTTONS_IMG_H - 32),0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
	if (g_ControllerInput.cur.button & BTN_R)
	{
	gDPLoadTextureBlock(OVERLAY_DISP++, Trigger_pressed_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,(BUTTONS_IMG_W - 32),0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
	}
    if (OVERLAY_CUSTOM == Left)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, (top_left_x - 100 ) <<2, (top_left_y + 58) <<2, (buttom_right_x - 100 ) <<2, (buttom_right_y + 58) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale - 0.2), BUTTONS_DTDY * (scale));
	}
	else if (OVERLAY_CUSTOM == Center)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER, (top_left_x - 100 ) <<2, (top_left_y + 58) <<2, (buttom_right_x - 100 ) <<2, (buttom_right_y + 58) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale - 0.2), BUTTONS_DTDY * (scale));
	}
	else if (OVERLAY_CUSTOM == Right)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT, (top_left_x - 100 ) <<2, (top_left_y + 58) <<2, (buttom_right_x - 100 ) <<2, (buttom_right_y + 58) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale - 0.2), BUTTONS_DTDY * (scale));
	}
	gDPPipeSync(OVERLAY_DISP++);

			// Trigger Z (Off Option)


	gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, 255); 
	gDPLoadTextureBlock(OVERLAY_DISP++, Trigger_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,(BUTTONS_IMG_H - 32),0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
	if (g_ControllerInput.cur.button & BTN_Z)
	{
	gDPLoadTextureBlock(OVERLAY_DISP++, Trigger_pressed_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,(BUTTONS_IMG_H - 32),0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
	}
    if (OVERLAY_CUSTOM == Left)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, (top_left_x - 140 ) <<2, (top_left_y + 58) <<2, (buttom_right_x - 140 ) <<2, (buttom_right_y + 58) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale - 0.2), BUTTONS_DTDY * (scale));
	}
	else if (OVERLAY_CUSTOM == Center)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER, (top_left_x - 140 ) <<2, (top_left_y + 58) <<2, (buttom_right_x - 140 ) <<2, (buttom_right_y + 58) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale - 0.2), BUTTONS_DTDY * (scale));
	}
	else if (OVERLAY_CUSTOM == Right)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT, (top_left_x - 140 ) <<2, (top_left_y + 58) <<2, (buttom_right_x - 140 ) <<2, (buttom_right_y + 58) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX * (scale - 0.2), BUTTONS_DTDY * (scale));	
	}
	gDPPipeSync(OVERLAY_DISP++);

	}

	

			//A Button

	gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 0, 255, 255); 
	gDPLoadTextureBlock(OVERLAY_DISP++, ABbuttons_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,BUTTONS_IMG_H,0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
	if (g_ControllerInput.cur.button & BTN_A)
	{
	gDPLoadTextureBlock(OVERLAY_DISP++, ABbuttons_pressed_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,BUTTONS_IMG_H,0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);	
	}
    if (OVERLAY_CUSTOM == Left)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, (top_left_x - 112) <<2, ( 80 + top_left_y) <<2, (buttom_right_x - 112) <<2, (80 + buttom_right_y) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX* (scale + 0.1), BUTTONS_DTDY* (scale + 0.1));
	}
	else if (OVERLAY_CUSTOM == Center)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER, (top_left_x - 112) <<2, ( 80 + top_left_y) <<2, (buttom_right_x - 112) <<2, (80 + buttom_right_y) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX* (scale + 0.1), BUTTONS_DTDY* (scale + 0.1));
	}
	else if (OVERLAY_CUSTOM == Right)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT, (top_left_x - 112) <<2, ( 80 + top_left_y) <<2, (buttom_right_x - 112) <<2, (80 + buttom_right_y) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX* (scale + 0.1), BUTTONS_DTDY* (scale + 0.1));
	}
	gDPPipeSync(OVERLAY_DISP++);
	

			// B Buttons

	gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 255, 0, 255); 
	gDPLoadTextureBlock(OVERLAY_DISP++, ABbuttons_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,BUTTONS_IMG_H,0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);
	if (g_ControllerInput.cur.button & BTN_B)
	{
	gDPLoadTextureBlock(OVERLAY_DISP++, ABbuttons_pressed_Ia8,G_IM_FMT_IA,G_IM_SIZ_8b,BUTTONS_IMG_W,BUTTONS_IMG_H,0
            ,G_TX_NOMIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,G_TX_NOLOD);	
	}
    if (OVERLAY_CUSTOM == Left)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, (top_left_x - 120) <<2, (top_left_y + 70) <<2, (buttom_right_x - 120) <<2, (buttom_right_y + 70) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX* (scale + 0.1), BUTTONS_DTDY* (scale + 0.1));
	}
	else if (OVERLAY_CUSTOM == Center)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER, (top_left_x - 120) <<2, (top_left_y + 70) <<2, (buttom_right_x - 120) <<2, (buttom_right_y + 70) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX* (scale + 0.1), BUTTONS_DTDY* (scale + 0.1));
	}
	else if (OVERLAY_CUSTOM == Right)
	{
    gEXTextureRectangle(OVERLAY_DISP++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT, (top_left_x - 120) <<2, (top_left_y + 70) <<2, (buttom_right_x - 120) <<2, (buttom_right_y + 70) <<2,G_TX_RENDERTILE,0 << 5, 0 << 5,BUTTONS_DSDX* (scale + 0.1), BUTTONS_DTDY* (scale + 0.1));
	}
	gDPPipeSync(OVERLAY_DISP++);


			//Stick movement


	if (g_ControllerInput.cur.stick_x != -100 || g_ControllerInput.cur.stick_y != -100) // Stick active
	{
		float Stick_X = 0.0f;
		float Stick_Y = (top_left_y - 50.55f) - (g_ControllerInput.cur.stick_y * (scale -0.96f));
		if (OVERLAY_CUSTOM == Left)
		{
		Stick_X = (Stick_X + top_left_x- 292.80f) + (g_ControllerInput.cur.stick_x * (scale - 0.96f)); // UP = -X | DOWN = +X
		gEXSetViewportAlign(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, -margin_reduction * 4, margin_reduction * 4);
        gEXSetScissorAlign(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_RIGHT, 0, -margin_reduction, -SCREEN_WIDTH, margin_reduction, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		}
		else if (OVERLAY_CUSTOM == Center)
		{
		Stick_X = (Stick_X +top_left_x- 292.80f) + (g_ControllerInput.cur.stick_x * (scale - 0.96f));
		gEXSetViewportAlign(OVERLAY_DISP++, G_EX_ORIGIN_CENTER, -margin_reduction * 4, margin_reduction * 4);
        gEXSetScissorAlign(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_RIGHT, 0, -margin_reduction, -SCREEN_WIDTH, margin_reduction, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		}
		else if (OVERLAY_CUSTOM == Right)
		{
		Stick_X = (Stick_X + top_left_x - 292.80f) + (g_ControllerInput.cur.stick_x * (scale - 0.96f));
		gEXSetViewportAlign(OVERLAY_DISP++, G_EX_ORIGIN_RIGHT, -margin_reduction * 4, margin_reduction * 4);
        gEXSetScissorAlign(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_RIGHT, 0, -margin_reduction, -SCREEN_WIDTH, margin_reduction, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);		}

		InterfaceContext* interfaceCtx = &play->interfaceCtx;

		Interface_SetOrthoView(interfaceCtx);
		Gfx_SetupDL39_Opa(play->state.gfxCtx);

		if (DEADZONE_SHOW == ON)
		{
			float Radius = sqrt(g_ControllerInput.cur.stick_x*g_ControllerInput.cur.stick_x + g_ControllerInput.cur.stick_y* g_ControllerInput.cur.stick_y);
			if (Radius <= 15)
			{
			gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 0, 0, 255);
			}
			else if (Radius <= 28)
			{
			gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 255, 0, 255);
			}
			else
			{
			gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 0, 0, 255);	
			}
		}
		else
		{
		gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, 255);
		}
		gDPLoadTextureTile(OVERLAY_DISP++, Stick_Ia8, G_IM_FMT_IA, G_IM_SIZ_8b,BUTTONS_IMG_W, BUTTONS_IMG_H,0, 0, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, 0,
						G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP,G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
		gSPTexture(OVERLAY_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
		gSPClearGeometryMode(OVERLAY_DISP++, G_CULL_BACK);
		

		Matrix_Push();
		Matrix_Translate(Stick_X, 8- Stick_Y, 1.0f, MTXMODE_NEW);
		Matrix_Scale(0.30f, 0.30f, 0, MTXMODE_APPLY);
		MATRIX_FINALIZE_AND_LOAD(OVERLAY_DISP++, play->state.gfxCtx);
		gSPClearGeometryMode(OVERLAY_DISP++, G_ZBUFFER | G_SHADE | G_CULL_BOTH | G_FOG | G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR | G_LOD);
		gSPSetGeometryMode(OVERLAY_DISP++, G_SHADE | G_SHADING_SMOOTH);
		gSPVertex(OVERLAY_DISP++, gStickVtx, 4, 0);
		gSP1Quadrangle(OVERLAY_DISP++, 0, 1, 2, 3, 0);

		Matrix_Pop();

        gEXPopScissor(OVERLAY_DISP++);
		CLOSE_DISPS(play->state.gfxCtx);
	}
}
