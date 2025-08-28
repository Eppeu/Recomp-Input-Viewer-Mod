#include "modding.h"
#include "global.h"
#include "recompconfig.h"
#include "recomputils.h"
#include "rt64_extended_gbi.h"
#include "libu64/pad.h"
#include "libc64/math64.h"

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

// Layout N64

INCBIN(Notches_Ia8, "src/Assets/analog_ia8.bin");
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

// TODO: Layout GCN 
INCBIN(Notches_GCN_Ia8, "src/Assets/analog_ia8.bin");
INCBIN(Analog_GCN_Ia8, "src/Assets/stick_ia8.bin");
INCBIN(CStick_GCN_Ia8, "src/Assets/button_pressed_ia8.bin");
INCBIN(ABbuttons_GCN_Ia8, "src/Assets/stick_ia8.bin");
INCBIN(ABbuttons_pressed_GCN_Ia8, "src/Assets/button_pressed_ia8.bin");
INCBIN(Xbutton_GCN_Ia8, "src/Assets/x_button_ia8.bin");
INCBIN(Xbutton_pressed_GCN_Ia8, "src/Assets/x_button_pressed_ia8.bin");
INCBIN(Ybutton_GCN_Ia8, "src/Assets/y_button_ia8.bin");
INCBIN(Ybutton_pressed_GCN_Ia8, "src/Assets/y_button_pressed_ia8.bin");
INCBIN(Zbutton_GCN_Ia8, "src/Assets/z_button_ia8.bin");
INCBIN(Zbutton_pressed_GCN_Ia8, "src/Assets/z_button_pressed_ia8.bin");

void Interface_SetOrthoView(InterfaceContext* interfaceCtx) {
    SET_FULLSCREEN_VIEWPORT(&interfaceCtx->view);
    View_ApplyOrthoToOverlay(&interfaceCtx->view);

}

// Options from mod.toml
typedef enum
{
	N64,
	GCN,
} ControllerLayout;

typedef enum
{
	Off,
	On,
} LBUTTON_N64;

typedef enum 
{
	OFF,
	ON,
} DeadzoneViewer;

typedef enum
{
	Left,
	Right,
} OverlayPlacement;

typedef enum
{
	Normal,
	Custom,
} CustomColorButton;

#define CONTROLLER_OVERLAY ((ControllerLayout)recomp_get_config_u32("controller_layout_option"))
#define L_BUTTON_PLACEMENT ((LBUTTON_N64)recomp_get_config_u32("button_placement_option"))
#define DEADZONE_SHOW ((DeadzoneViewer)recomp_get_config_u32("input_display_deadzone"))
#define OVERLAY_PLACEMENT ((OverlayPlacement)recomp_get_config_u32("overlay_placement_option"))
#define CUSTOM_COLOR_BUTTON ((CustomColorButton)recomp_get_config_u32("custom_color_option"))
RECOMP_IMPORT("*", void recomp_get_window_resolution(u32* width, u32* height));
RECOMP_IMPORT("*", void recomp_get_camera_inputs(float* x, float* y));

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

Vtx gNotchesVtx[] = {
    VTX( 0,   0, 0,  0 << 5,  0 << 5, 0, 0, 0, 0xFF), 
    VTX(64,   0, 0, 64 << 5,  0 << 5, 0, 0, 0, 0xFF), 
    VTX(64, -64, 0, 64 << 5, 64 << 5, 0, 0, 0, 0xFF), 
    VTX( 0, -64, 0,  0 << 5, 64 << 5, 0, 0, 0, 0xFF), 
};

Vtx gDpadVtx[] = {
    VTX( 0,   0, 0,  0 << 5,  0 << 5, 0, 0, 0, 0xFF), 
    VTX(64,   0, 0, 64 << 5,  0 << 5, 0, 0, 0, 0xFF), 
    VTX(64, -64, 0, 64 << 5, 64 << 5, 0, 0, 0, 0xFF), 
    VTX( 0, -64, 0,  0 << 5, 64 << 5, 0, 0, 0, 0xFF), 
};


Vtx gButtonVtx[] = {
    VTX( 0,   0, 0,  0 << 5,  0 << 5, 0, 0, 0, 0xFF), 
    VTX(64,   0, 0, 64 << 5,  0 << 5, 0, 0, 0, 0xFF), 
    VTX(64, -64, 0, 64 << 5, 64 << 5, 0, 0, 0, 0xFF), 
    VTX( 0, -64, 0,  0 << 5, 64 << 5, 0, 0, 0, 0xFF), 
};


Vtx gTriggerVtx[] = {
    VTX( 0,   0, 0,  0 << 5,  0 << 5, 0, 0, 0, 0xFF), 
    VTX(64,   0, 0, 64 << 5,  0 << 5, 0, 0, 0, 0xFF), 
    VTX(64, -32, 0, 64 << 5, 32 << 5, 0, 0, 0, 0xFF), 
    VTX( 0, -32, 0,  0 << 5, 32 << 5, 0, 0, 0, 0xFF), 
};


#define BUTTONS_IMG_W 64
#define BUTTONS_IMG_H 64

#define top_left_y (int) recomp_get_config_double("pos_y_display")
#define top_left_x (int) recomp_get_config_double("pos_x_display")
#define alpha_user (int) recomp_get_config_double("alpha_slider")
float scale = 1.0f;
#define scale_overlay (float) recomp_get_config_double("overlay_scale")
s32 margin_reduction = 8;

// Colors - Credits to Neirn

// Convert char to its numeric value
// Returns 0xFF if invalid character
u8 cToNum(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }

    return 0xFF;
}

u8 sToU8(const char *s) {
    return (cToNum(s[0]) << 4) | cToNum(s[1]);
}

bool isValidHexString(const char *s) {
    bool isValid = true;

    size_t len = 0;

    while (*s != '\0' && isValid) {
        if (len > 6 || cToNum(*s) == 0xFF) {
            isValid = false;
        }
        s++;
        len++;
    }

    if (len < 6) {
        isValid = false;
    }

    return isValid;
}



//Store of the inputs button
Input g_ControllerInput;

RECOMP_CALLBACK("*", recomp_on_play_main)
void KeepInputs(PlayState* play)
{
	g_ControllerInput = *CONTROLLER1(&play->state);

}

// Initialization of the repeat function to not repeat the same lines over and over

void DrawInputFunctionStick(Gfx** pkt, PlayState* play, void* timg, int img_w, int img_h, float input_x, float input_y, float scale_x, float scale_y, Vtx* n)
{
	gDPLoadTextureTile((*pkt)++, timg, G_IM_FMT_IA, G_IM_SIZ_8b,BUTTONS_IMG_W, BUTTONS_IMG_H,0, 0, img_w, img_h, 0,
					G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP,G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);	
	gSPTexture((*pkt)++, 0xFFFF, 0xFFFF, -10, G_TX_RENDERTILE, G_ON);
	gSPClearGeometryMode((*pkt)++, G_CULL_BACK);
	Matrix_Push();
	Matrix_Translate(input_x,input_y, 0.0f, MTXMODE_NEW);
	Matrix_Scale(scale_x, scale_y , 0, MTXMODE_APPLY);
	MATRIX_FINALIZE_AND_LOAD((*pkt)++, play->state.gfxCtx);
	gSPVertex((*pkt)++, n, 4, 0);
	gSP1Quadrangle((*pkt)++, 0, 1, 2, 3, 0);

	Matrix_Pop();
	gDPPipeSync((*pkt)++);
}

void DrawInputFunction(Gfx** pkt, PlayState* play, void* timg, int img_w, int img_h, float input_x, float input_y, float scale_x, float scale_y, Vtx* n)
{
    gEXMatrixGroup((*pkt)++, G_EX_ID_IGNORE,G_EX_PUSH,G_MTX_MODELVIEW,G_EX_EDIT_NONE,G_EX_COMPONENT_AUTO, G_EX_COMPONENT_AUTO , G_EX_COMPONENT_AUTO , G_EX_COMPONENT_AUTO ,
	G_EX_COMPONENT_AUTO, G_EX_COMPONENT_AUTO , G_EX_COMPONENT_AUTO , G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);
	gDPLoadTextureTile((*pkt)++, timg, G_IM_FMT_IA, G_IM_SIZ_8b,BUTTONS_IMG_W, BUTTONS_IMG_H,0, 0, img_w, img_h, 0,
					G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP,G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);	
	gSPTexture((*pkt)++, 0xFFFF, 0xFFFF, -10, G_TX_RENDERTILE, G_ON);
	gSPClearGeometryMode((*pkt)++, G_CULL_BACK);
	Matrix_Push();
	Matrix_Translate(input_x,input_y, 0.0f, MTXMODE_NEW);
	Matrix_Scale(scale_x, scale_y , 0, MTXMODE_APPLY);
	MATRIX_FINALIZE_AND_LOAD((*pkt)++, play->state.gfxCtx);
	gSPVertex((*pkt)++, n, 4, 0);
	gSP1Quadrangle((*pkt)++, 0, 1, 2, 3, 0);

	gEXPopMatrixGroup((*pkt)++, G_MTX_MODELVIEW);
	Matrix_Pop();
	gDPPipeSync((*pkt)++);
}
// r,g,b are the values of the color if the option is disabled
void CustomColorSet(Gfx** pkt, char* OptionID, int r,int g,int b)
{
	if (CUSTOM_COLOR_BUTTON == Custom)
	{
	char *color = recomp_get_config_string(OptionID);
	if (color)
	{
    	if (isValidHexString(color))
		{
		gDPSetPrimColor((*pkt)++, 0, 0, sToU8(color), sToU8(color + 2), sToU8(color + 4), alpha_user);
    	}
	}
    recomp_free_config_string(color);
	}
	else
	{
		gDPSetPrimColor((*pkt)++, 0, 0, r, g, b, alpha_user);
	}
}
// Majora's mask runs at 320x240
// Note: The center of the screen is at 160, 120


PlayState *cplay;
RECOMP_HOOK("Play_PostWorldDraw")
void pre_drawinput(PlayState *play)
{
	cplay = play;
}

RECOMP_HOOK_RETURN("Play_PostWorldDraw") // Draws ALL the inputs (pressed or not pressed)
void post_DrawInput()
{
	PlayState *play = cplay;
	// Initalizarion of the inputs positions and rendering
	float Stick_X = (top_left_x - 292.90f) + (g_ControllerInput.cur.stick_x * (scale - 0.96f)); // UP = -X | DOWN = +X;
	float Stick_Y = (top_left_y - 50.55f) - (g_ControllerInput.cur.stick_y * (scale -0.96f));
	float Button_X = top_left_x - 292.90f;
	float Button_Y = top_left_y - 50.55f;
	float DPAD_Y = top_left_y - 45.55f;
	float BButton_Y = top_left_y - 49.55f;

	// C-Stick
	float Cright_x, Cright_y;
	recomp_get_camera_inputs(&Cright_x, &Cright_y);
	float StickC_X = (top_left_x - 292.80f) + (Cright_x * (scale * 3.3f));
	float StickC_Y = (top_left_y - 57.0f) + (Cright_y * (scale * 3.3f));

	OPEN_DISPS(cplay->state.gfxCtx);

    gDPSetCycleType(OVERLAY_DISP++, G_CYC_1CYCLE);
    gDPSetRenderMode(OVERLAY_DISP++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode(OVERLAY_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gEXPushScissor(OVERLAY_DISP++);
    gEXSetScissor(OVERLAY_DISP++, G_SC_NON_INTERLACE, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_RIGHT, 0, 0, 0, SCREEN_HEIGHT);

	if (OVERLAY_PLACEMENT == Left)
	{
		gEXSetViewportAlign(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, -margin_reduction * 4, margin_reduction * 4);
   		gEXSetScissorAlign(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_RIGHT, 0, -margin_reduction, -SCREEN_WIDTH, margin_reduction, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	}
	else if (OVERLAY_PLACEMENT == Right)
	{
		gEXSetViewportAlign(OVERLAY_DISP++, G_EX_ORIGIN_RIGHT, -margin_reduction * 4, margin_reduction * 4);
   	    gEXSetScissorAlign(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_RIGHT, 0, -margin_reduction, -SCREEN_WIDTH, margin_reduction, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	}	

	//Start of the drawing of the inputs

			// Notches 
	if (CONTROLLER_OVERLAY == N64)
	{
	InterfaceContext* interfaceCtx = &cplay->interfaceCtx;

	gDPSetTexturePersp(OVERLAY_DISP++, G_TP_PERSP);
	Interface_SetOrthoView(interfaceCtx);
	Gfx_SetupDL39_Opa(cplay->state.gfxCtx);

	if (DEADZONE_SHOW == ON)
	{
		float Radius = sqrt(g_ControllerInput.cur.stick_x*g_ControllerInput.cur.stick_x + g_ControllerInput.cur.stick_y* g_ControllerInput.cur.stick_y);
		if (Radius <= 15)
		{
		gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 0, 0, alpha_user);
		}
		else if (Radius <= 28)
		{
		gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 255, 0, alpha_user);
		}
		else
		{
		gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 0, 0, alpha_user);	
		}
	}
	else
	{
	CustomColorSet(&OVERLAY_DISP, "stick_color",255,255,alpha_user);
	}
	DrawInputFunctionStick(&OVERLAY_DISP,cplay, Notches_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X, 8 - Button_Y, 0.30f, 0.30f, gNotchesVtx);

	// Analog

	if (g_ControllerInput.cur.stick_x != -100 || g_ControllerInput.cur.stick_y != -100)
	{

		if (DEADZONE_SHOW == ON)
		{
			float Radius = sqrt(g_ControllerInput.cur.stick_x*g_ControllerInput.cur.stick_x + g_ControllerInput.cur.stick_y* g_ControllerInput.cur.stick_y);
			if (Radius <= 15)
			{
			gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 0, 0, alpha_user);
			}
			else if (Radius <= 28)
			{
			gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 255, 0, alpha_user);
			}
			else
			{
			gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 0, 0, alpha_user);	
			}
		}
		else
		{
		CustomColorSet(&OVERLAY_DISP, "stick_color",255,255,255);
		}
	DrawInputFunctionStick(&OVERLAY_DISP,cplay, Stick_Ia8, BUTTONS_IMG_W, BUTTONS_IMG_H, Stick_X, 8- Stick_Y, 0.30f, 0.30f, gStickVtx);
	}

	// D-PAD

	CustomColorSet(&OVERLAY_DISP, "dpad_color",128,128,128);
	DrawInputFunction(&OVERLAY_DISP,cplay, Dpad_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 16, -DPAD_Y, 0.19f, 0.19f, gDpadVtx);
	if (g_ControllerInput.cur.button & BTN_DUP) // D-PAD Up PRESSED
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, Dpad_up_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 16, -DPAD_Y, 0.19f, 0.19f, gDpadVtx);
	}
	if (g_ControllerInput.cur.button & BTN_DDOWN) // D-PAD Down PRESSED
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, Dpad_down_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 16, -DPAD_Y, 0.19f, 0.19f, gDpadVtx);		
	}
	if (g_ControllerInput.cur.button & BTN_DLEFT) // D-PAD Left PRESSED
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, Dpad_left_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 16, -DPAD_Y, 0.19f, 0.19f, gDpadVtx);		
	}
	if (g_ControllerInput.cur.button & BTN_DRIGHT) // D-PAD Right PRESSED
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, Dpad_right_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 16, -DPAD_Y, 0.19f, 0.19f, gDpadVtx);		
	}

	// C Button (Not Pressed)

	CustomColorSet(&OVERLAY_DISP, "c_button_color",255,255,0);
	DrawInputFunction(&OVERLAY_DISP,cplay, Cbuttons_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 43, 12 - Button_Y, 0.21f, 0.21f, gButtonVtx); //C-UP
	DrawInputFunction(&OVERLAY_DISP,cplay, Cbuttons_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 43, - Button_Y, 0.21f, 0.21f, gButtonVtx); //C-DOWN
	DrawInputFunction(&OVERLAY_DISP,cplay, Cbuttons_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 37, 6 - Button_Y, 0.21f, 0.21f, gButtonVtx); //C-LEFT
	DrawInputFunction(&OVERLAY_DISP,cplay, Cbuttons_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 49, 6 -  Button_Y, 0.21f, 0.21f, gButtonVtx); //C-RIGHT
	if (g_ControllerInput.cur.button & BTN_CUP)
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, Cbuttons_pressed_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 43, 12 - Button_Y, 0.21f, 0.21f, gButtonVtx);
	}
	if (g_ControllerInput.cur.button & BTN_CDOWN)
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, Cbuttons_pressed_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 43, - Button_Y, 0.21f, 0.21f, gButtonVtx);
	}
	if (g_ControllerInput.cur.button & BTN_CLEFT)
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, Cbuttons_pressed_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 37, 6 - Button_Y, 0.21f, 0.21f, gButtonVtx);
	}
	if (g_ControllerInput.cur.button & BTN_CRIGHT)
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, Cbuttons_pressed_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 49, 6 -  Button_Y, 0.21f, 0.21f, gButtonVtx);
	}

	// A Button

	CustomColorSet(&OVERLAY_DISP, "a_button_color",0,0,255);
	DrawInputFunction(&OVERLAY_DISP,cplay, ABbuttons_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 27, -BButton_Y, 0.27f, 0.27f, gButtonVtx);
	if (g_ControllerInput.cur.button & BTN_A)
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, ABbuttons_pressed_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 27, -BButton_Y, 0.27f, 0.27f, gButtonVtx);
	}

	// B Button

	CustomColorSet(&OVERLAY_DISP, "b_button_color",0,255,0);
	DrawInputFunction(&OVERLAY_DISP,cplay, ABbuttons_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 18, 8 - Button_Y, 0.27f, 0.27f, gButtonVtx);
	if (g_ControllerInput.cur.button & BTN_B)
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, ABbuttons_pressed_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 18, 8 - Button_Y, 0.27f, 0.27f, gButtonVtx);
	}

	// Start Button

	CustomColorSet(&OVERLAY_DISP, "start_button_color",255,0,0);
	DrawInputFunction(&OVERLAY_DISP,cplay, Start_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X+ 26, 20 -  Button_Y, 0.11f, 0.11f, gButtonVtx);
	if (g_ControllerInput.cur.button & BTN_START)
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, Start_pressed_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 26, 20 - Button_Y, 0.11f, 0.11f, gButtonVtx);
	}

	// Triggers 

	CustomColorSet(&OVERLAY_DISP, "trigger_color",255,255,255);
	DrawInputFunction(&OVERLAY_DISP,cplay, Trigger_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 33, Button_X + 40, 20 -  Button_Y, 0.30f, 0.20f, gTriggerVtx); // R Button Draw (On & off)
	if (g_ControllerInput.cur.button & BTN_R)
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, Trigger_pressed_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 33, Button_X + 40, 20 - Button_Y, 0.30f, 0.20f, gTriggerVtx); // R Button Press (On & off)
	}
	if (L_BUTTON_PLACEMENT == On)
	{
		DrawInputFunction(&OVERLAY_DISP,cplay, Trigger_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 33, Button_X, 20 -  Button_Y, 0.30f, 0.20f, gTriggerVtx); // L Button Draw (On)
		if (g_ControllerInput.cur.button & BTN_L)
		{
		DrawInputFunction(&OVERLAY_DISP,cplay, Trigger_pressed_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 33, Button_X, 20 - Button_Y, 0.30f, 0.20f, gTriggerVtx); // L Button Press (On)
		}
		DrawInputFunction(&OVERLAY_DISP,cplay, Trigger_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 33, Button_X + 20, 15 -  Button_Y, 0.30f, 0.20f, gTriggerVtx); // Z Button Draw (On)
		if (g_ControllerInput.cur.button & BTN_Z)
		{
		DrawInputFunction(&OVERLAY_DISP,cplay, Trigger_pressed_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 33, Button_X + 20, 15 - Button_Y, 0.30f, 0.20f, gTriggerVtx); // Z Button Press (On)
		}
	}
	else
	{
		DrawInputFunction(&OVERLAY_DISP,cplay, Trigger_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 33, Button_X, 20 -  Button_Y, 0.30f, 0.20f, gTriggerVtx); // Z Button Draw (Off)
		if (g_ControllerInput.cur.button & BTN_Z)
		{
		DrawInputFunction(&OVERLAY_DISP,cplay, Trigger_pressed_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 33, Button_X, 20 -  Button_Y, 0.30f, 0.20f, gTriggerVtx); // Z Button Press (Off)
		}
	}
	}

	// GCN Layout ------------------------------------------------------------------------

	else if (CONTROLLER_OVERLAY == GCN)
	{
	InterfaceContext* interfaceCtx = &cplay->interfaceCtx;

	gDPSetTexturePersp(OVERLAY_DISP++, G_TP_PERSP);
	Interface_SetOrthoView(interfaceCtx);
	Gfx_SetupDL39_Opa(cplay->state.gfxCtx);

	if (Cright_x != -100 || Cright_y != -100)
	{
	CustomColorSet(&OVERLAY_DISP, "c_stick_color_gcn",255,255,0);
	DrawInputFunctionStick(&OVERLAY_DISP,cplay, Notches_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X -3 + 25, -Button_Y + 7, 0.27f, 0.27f, gNotchesVtx);	
	DrawInputFunctionStick(&OVERLAY_DISP,cplay, ABbuttons_pressed_Ia8, BUTTONS_IMG_W -1, BUTTONS_IMG_H - 1, StickC_X + 22.6f ,-StickC_Y, 0.25f, 0.25f, gStickVtx);
	}

	// Notches 
	if (g_ControllerInput.cur.stick_x != -100 || g_ControllerInput.cur.stick_y != -100)
	{
		if (DEADZONE_SHOW == ON)
		{
			float Radius = sqrt(g_ControllerInput.cur.stick_x*g_ControllerInput.cur.stick_x + g_ControllerInput.cur.stick_y* g_ControllerInput.cur.stick_y);
			if (Radius <= 15)
			{
			gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 0, 0, alpha_user);
			}
			else if (Radius <= 28)
			{
			gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 255, 0, alpha_user);
			}
			else
			{
			gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 0, 0, alpha_user);	
			}
		}
		else
		{
		CustomColorSet(&OVERLAY_DISP, "stick_color",255,255,255);
		}
	}
	DrawInputFunctionStick(&OVERLAY_DISP,cplay, Notches_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X, 8 - Button_Y, 0.29f, 0.29f, gNotchesVtx);

	// Analog

	if (g_ControllerInput.cur.stick_x != -100 || g_ControllerInput.cur.stick_y != -100)
	{
		if (DEADZONE_SHOW == ON)
		{
		float Radius = sqrt(g_ControllerInput.cur.stick_x*g_ControllerInput.cur.stick_x + g_ControllerInput.cur.stick_y* g_ControllerInput.cur.stick_y);
			if (Radius <= 15)
			{
			gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 0, 0, alpha_user);
			}
			else if (Radius <= 28)
			{
			gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 255, 0, alpha_user);
			}
			else
			{
			gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 0, 0, alpha_user);	
			}
		}
		else
		{
		CustomColorSet(&OVERLAY_DISP, "stick_color",255,255,255);
		}
	DrawInputFunctionStick(&OVERLAY_DISP,cplay, Stick_Ia8, BUTTONS_IMG_W, BUTTONS_IMG_H, Stick_X, 8- Stick_Y, 0.29f, 0.29f, gStickVtx);
	}
		

		// D-PAD

	CustomColorSet(&OVERLAY_DISP, "dpad_color",128,128,128);
	DrawInputFunction(&OVERLAY_DISP,cplay, Dpad_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 14, -DPAD_Y, 0.19f, 0.19f, gDpadVtx);
	if (g_ControllerInput.cur.button & BTN_DUP) // D-PAD Up PRESSED
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, Dpad_up_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 14, -DPAD_Y, 0.19f, 0.19f, gDpadVtx);
	}
	if (g_ControllerInput.cur.button & BTN_DDOWN) // D-PAD Down PRESSED
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, Dpad_down_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 14, -DPAD_Y, 0.19f, 0.19f, gDpadVtx);		
	}
	if (g_ControllerInput.cur.button & BTN_DLEFT) // D-PAD Left PRESSED
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, Dpad_left_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 14, -DPAD_Y, 0.19f, 0.19f, gDpadVtx);		
	}
	if (g_ControllerInput.cur.button & BTN_DRIGHT) // D-PAD Right PRESSED
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, Dpad_right_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 14, -DPAD_Y, 0.19f, 0.19f, gDpadVtx);		
	}
	if (g_ControllerInput.cur.button & BTN_L) // L Button PRESSED
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, Dpad_up_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 14, -DPAD_Y, 0.19f, 0.19f, gDpadVtx);
	DrawInputFunction(&OVERLAY_DISP,cplay, Dpad_down_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 14, -DPAD_Y, 0.19f, 0.19f, gDpadVtx);		
	DrawInputFunction(&OVERLAY_DISP,cplay, Dpad_left_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 14, -DPAD_Y, 0.19f, 0.19f, gDpadVtx);		
	DrawInputFunction(&OVERLAY_DISP,cplay, Dpad_right_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 14, -DPAD_Y, 0.19f, 0.19f, gDpadVtx);
	}

	// Triggers

	CustomColorSet(&OVERLAY_DISP, "trigger_color",255,255,255);
	DrawInputFunction(&OVERLAY_DISP,cplay, Trigger_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 33, Button_X + 20, 17 -  Button_Y, 0.30f, 0.20f, gTriggerVtx); // R Button Draw (On & off)
	if (g_ControllerInput.cur.button & BTN_R)
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, Trigger_pressed_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 33, Button_X + 20, 17 - Button_Y, 0.30f, 0.20f, gTriggerVtx); // R Button Press (On & off)
	}
	DrawInputFunction(&OVERLAY_DISP,cplay, Trigger_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 33, Button_X, 17 -  Button_Y, 0.30f, 0.20f, gTriggerVtx); // Z Button Draw (Off)
	if (g_ControllerInput.cur.button & BTN_Z)
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, Trigger_pressed_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 33, Button_X, 17 - Button_Y, 0.30f, 0.20f, gTriggerVtx); // Z Button Press (Off)
	}

	// A Button
	CustomColorSet(&OVERLAY_DISP, "a_button_color",0,255,0); 
	DrawInputFunction(&OVERLAY_DISP,cplay, ABbuttons_GCN_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 43, -Button_Y + 12, 0.40f, 0.40f, gButtonVtx);
	if (g_ControllerInput.cur.button & BTN_A)
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, ABbuttons_pressed_GCN_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 43, -Button_Y + 12, 0.40f, 0.40f, gButtonVtx);
	}


	// B Button

	CustomColorSet(&OVERLAY_DISP, "b_button_color",255,0,0); 
	DrawInputFunction(&OVERLAY_DISP,cplay, ABbuttons_GCN_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 38, 0.5f - BButton_Y, 0.20f, 0.20f, gButtonVtx);
	if (g_ControllerInput.cur.button & BTN_B)
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, ABbuttons_pressed_GCN_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 38, 0.5f - BButton_Y, 0.20f, 0.20f, gButtonVtx);
	}

	// Start Button

	CustomColorSet(&OVERLAY_DISP, "start_button_color",255,255,255); 
	DrawInputFunction(&OVERLAY_DISP,cplay, Start_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X+ 40, 8 -  Button_Y, 0.13f, 0.13f, gButtonVtx);
	if (g_ControllerInput.cur.button & BTN_START)
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, Start_pressed_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 40, 8 -Button_Y, 0.13f, 0.13f, gButtonVtx);
	}

	// C Button Left 

	CustomColorSet(&OVERLAY_DISP, "x_button_color_gcn",255,255,255); 
	DrawInputFunction(&OVERLAY_DISP,cplay, Ybutton_GCN_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 47, 14 - Button_Y, 0.21f, 0.21f, gButtonVtx); // C Left (Y)
	if (g_ControllerInput.cur.button & BTN_CLEFT)
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, Ybutton_pressed_GCN_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 47, 14 - Button_Y, 0.21f, 0.21f, gButtonVtx); 
	}

	// C Button Right

	CustomColorSet(&OVERLAY_DISP, "x_button_color_gcn",255,255,255); 
	DrawInputFunction(&OVERLAY_DISP,cplay, Xbutton_GCN_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 58, 8 - Button_Y, 0.21f, 0.21f, gButtonVtx); // C Right (X)
	if (g_ControllerInput.cur.button & BTN_CRIGHT)
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, Xbutton_pressed_GCN_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 58, 8 - Button_Y, 0.21f, 0.21f, gButtonVtx); 
	}

	// C-DOWN
	CustomColorSet(&OVERLAY_DISP, "z_button_color_gcn",160,32,240); 
	DrawInputFunction(&OVERLAY_DISP,cplay, Zbutton_GCN_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 61, 18 - Button_Y, 0.17f, 0.17f, gButtonVtx); // C UP (Z)
	if (g_ControllerInput.cur.button & BTN_CDOWN)
	{
	DrawInputFunction(&OVERLAY_DISP,cplay, Zbutton_pressed_GCN_Ia8, BUTTONS_IMG_W - 1, BUTTONS_IMG_H - 1, Button_X + 61, 18 - Button_Y, 0.17f, 0.17f, gButtonVtx);
	}
	}
	gEXPopScissor(OVERLAY_DISP++);
	CLOSE_DISPS(cplay->state.gfxCtx);
}