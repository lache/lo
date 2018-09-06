#pragma once

#include "lwmacro.h"
#include "lwshader.h"

typedef enum _LW_VBO_TYPE
{
	LVT_BATTLE_BOWL_OUTER,
	LVT_BATTLE_BOWL_INNER,
	LVT_ENEMY_SCOPE,
	LVT_LEFT_TOP_ANCHORED_SQUARE,		LVT_CENTER_TOP_ANCHORED_SQUARE,		LVT_RIGHT_TOP_ANCHORED_SQUARE,
	LVT_LEFT_CENTER_ANCHORED_SQUARE,	LVT_CENTER_CENTER_ANCHORED_SQUARE,	LVT_RIGHT_CENTER_ANCHORED_SQUARE,
	LVT_LEFT_BOTTOM_ANCHORED_SQUARE,	LVT_CENTER_BOTTOM_ANCHORED_SQUARE,	LVT_RIGHT_BOTTOM_ANCHORED_SQUARE,
	LVT_PLAYER,
	LVT_CUBE_WALL,
	LVT_HOME,
	LVT_TRAIL,
	LVT_FLOOR,
	LVT_FLOOR2,
	LVT_SPHERE,
	LVT_APT,
	LVT_BEAM,
	LVT_PUMP,
	LVT_OIL_TRUCK,
	LVT_ROOM,
	LVT_BATTLEGROUND_FLOOR,
	LVT_BATTLEGROUND_WALL,
	LVT_CROSSBOW_ARROW,
	LVT_CATAPULT_BALL,
	LVT_DEVIL,
	LVT_CRYSTAL,
	LVT_SPIRAL,
	LVT_PUCK, // puck itself
    LVT_PUCK_PLAYER, // player and target
    LVT_TOWER_BASE,
    LVT_TOWER_1,
    LVT_TOWER_2,
    LVT_TOWER_3,
    LVT_TOWER_4,
    LVT_TOWER_5,
    LVT_RINGGAUGE,
    LVT_RINGGAUGETHICK,
    LVT_RADIALWAVE,
    LVT_PHYSICS_MENU,
	LVT_UI_SCRAP_BG,
	LVT_UI_TOWER_BUTTON_BG,
	LVT_UI_LEFT_BUTTON_BG,
	LVT_UI_FULL_PANEL_BG,
	LVT_UI_BUTTON_BG,
    LVT_PUCK_FLOOR_COVER,
    LVT_PUCK_FLOOR_COVER_OCTAGON,
    LVT_TOWER_BASE_2,
    LVT_FOOTBALL_GROUND,
    LVT_PLAYER_CAPSULE,
	LVT_SHIP,
    LVT_SHIP1,
    LVT_PORT,
    LVT_SEA_CITY,
    LVT_LAND_CELL,
    LVT_SEAPORT_ICON,
    LVT_EARTH,
    LVT_SEA_CELL_DEBUG,
    LVT_CELL_PIXEL_SELECTOR,
    LVT_OCTAGON_PLANE, // programmed
    LVT_TILE_SEL_0_0,
    LVT_TILE_SEL_1_0,
    LVT_TILE_SEL_0_1,
    LVT_TILE_SEL_1_1,
    LVT_SUZANNE,

	LVT_COUNT,
    LVT_DONTCARE,
} LW_VBO_TYPE;
typedef struct _LWVBOFILENAME {
    LW_VBO_TYPE lvt;
    const char* filename;
    LW_SHADER_TYPE shader_index;
} LWVBOFILENAME;
static const LWVBOFILENAME vbo_filename[] = {
    { LVT_BATTLE_BOWL_OUTER, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Bowl_Outer.vbo", LWST_DEFAULT, },
    { LVT_BATTLE_BOWL_INNER, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Bowl_Inner.vbo", LWST_DEFAULT, },
    { LVT_ENEMY_SCOPE, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "EnemyScope.vbo", LWST_DEFAULT, },
    { LVT_LEFT_TOP_ANCHORED_SQUARE, "", LWST_DEFAULT, },
    { LVT_CENTER_TOP_ANCHORED_SQUARE, "", LWST_DEFAULT, },
    { LVT_RIGHT_TOP_ANCHORED_SQUARE, "", LWST_DEFAULT, },
    { LVT_LEFT_CENTER_ANCHORED_SQUARE, "", LWST_DEFAULT, },
    { LVT_CENTER_CENTER_ANCHORED_SQUARE, "", LWST_DEFAULT, },
    { LVT_RIGHT_CENTER_ANCHORED_SQUARE, "", LWST_DEFAULT, },
    { LVT_LEFT_BOTTOM_ANCHORED_SQUARE, "", LWST_DEFAULT, },
    { LVT_CENTER_BOTTOM_ANCHORED_SQUARE, "", LWST_DEFAULT, },
    { LVT_RIGHT_BOTTOM_ANCHORED_SQUARE, "", LWST_DEFAULT, },
    { LVT_PLAYER, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Player.vbo", LWST_DEFAULT, },
    { LVT_CUBE_WALL, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "CubeWall.vbo", LWST_DEFAULT, },
    { LVT_HOME, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Home.vbo", LWST_DEFAULT, },
    { LVT_TRAIL, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Trail.vbo", LWST_DEFAULT, },
    { LVT_FLOOR, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Floor.vbo", LWST_DEFAULT, },
    { LVT_FLOOR2, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Floor2.vbo", LWST_DEFAULT, },
    { LVT_SPHERE, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Sphere.vbo", LWST_DEFAULT, },
    { LVT_APT, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Apt.vbo", LWST_DEFAULT, },
    { LVT_BEAM, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Beam.vbo", LWST_DEFAULT, },
    { LVT_PUMP, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Pump.vbo", LWST_DEFAULT, },
    { LVT_OIL_TRUCK, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "OilTruck.vbo", LWST_DEFAULT, },
    { LVT_ROOM, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "room.vbo", LWST_DEFAULT, },
    { LVT_BATTLEGROUND_FLOOR, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "battleground-floor.vbo", LWST_DEFAULT, },
    { LVT_BATTLEGROUND_WALL, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "battleground-wall.vbo", LWST_DEFAULT, },
    { LVT_CROSSBOW_ARROW, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "crossbow-arrow.vbo", LWST_DEFAULT, },
    { LVT_CATAPULT_BALL, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "catapult-ball.vbo", LWST_DEFAULT, },
    { LVT_DEVIL, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "devil.vbo", LWST_DEFAULT, },
    { LVT_CRYSTAL, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "crystal.vbo", LWST_DEFAULT, },
    { LVT_SPIRAL, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "spiral.vbo", LWST_DEFAULT, },
    { LVT_PUCK, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "puck.vbo", LWST_DEFAULT, }, // puck itself
    { LVT_PUCK_PLAYER, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "puck-player.vbo", LWST_DEFAULT, }, // player and target
    { LVT_TOWER_BASE, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "tower-base.vbo", LWST_DEFAULT_NORMAL, },
    { LVT_TOWER_1, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "tower-1.vbo", LWST_DEFAULT_NORMAL, },
    { LVT_TOWER_2, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "tower-2.vbo", LWST_DEFAULT_NORMAL, },
    { LVT_TOWER_3, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "tower-3.vbo", LWST_DEFAULT_NORMAL, },
    { LVT_TOWER_4, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "tower-4.vbo", LWST_DEFAULT_NORMAL, },
    { LVT_TOWER_5, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "tower-5.vbo", LWST_DEFAULT_NORMAL, },
    { LVT_RINGGAUGE, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "ringgauge.vbo", LWST_DEFAULT, },
    { LVT_RINGGAUGETHICK, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "ringgaugethick.vbo", LWST_DEFAULT, },
    { LVT_RADIALWAVE, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "radialwave.vbo", LWST_DEFAULT, },
    { LVT_PHYSICS_MENU, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "physics-menu.vbo", LWST_DEFAULT, },
    { LVT_UI_SCRAP_BG, "", LWST_COLOR, },
    { LVT_UI_TOWER_BUTTON_BG, "", LWST_COLOR, },
    { LVT_UI_LEFT_BUTTON_BG, "", LWST_COLOR, },
    { LVT_UI_FULL_PANEL_BG, "", LWST_COLOR, },
    { LVT_UI_BUTTON_BG, "", LWST_COLOR, },
    { LVT_PUCK_FLOOR_COVER, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "puck-floor-cover.vbo", LWST_DEFAULT, },
    { LVT_PUCK_FLOOR_COVER_OCTAGON, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "puck-floor-cover-octagon.vbo", LWST_DEFAULT, },
    { LVT_TOWER_BASE_2, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "tower-base-2.vbo", LWST_DEFAULT, },
    { LVT_FOOTBALL_GROUND, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "football-ground.vbo", LWST_DEFAULT, },
    { LVT_PLAYER_CAPSULE, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "player-capsule.vbo", LWST_DEFAULT, },
	{ LVT_SHIP, ASSETS_BASE_PATH "cvbo" PATH_SEPARATOR "ship.cvbo", LWST_DEFAULT_NORMAL_COLOR, },
    { LVT_SHIP1, ASSETS_BASE_PATH "cvbo" PATH_SEPARATOR "ship1.cvbo", LWST_DEFAULT_NORMAL_COLOR, },
    { LVT_PORT, ASSETS_BASE_PATH "cvbo" PATH_SEPARATOR "port.cvbo", LWST_DEFAULT_NORMAL_COLOR, },
    { LVT_SEA_CITY, ASSETS_BASE_PATH "cvbo" PATH_SEPARATOR "sea-city.cvbo", LWST_DEFAULT_NORMAL_COLOR, },
    { LVT_LAND_CELL, ASSETS_BASE_PATH "cvbo" PATH_SEPARATOR "land-cell.cvbo", LWST_DEFAULT_NORMAL_COLOR, },
    { LVT_SEAPORT_ICON, ASSETS_BASE_PATH "cvbo" PATH_SEPARATOR "seaport-icon.cvbo", LWST_DEFAULT_NORMAL_COLOR, },
    { LVT_EARTH, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "earth.vbo", LWST_DEFAULT, },
    { LVT_SEA_CELL_DEBUG, ASSETS_BASE_PATH "cvbo" PATH_SEPARATOR "sea-cell-debug.cvbo", LWST_DEFAULT_NORMAL_COLOR, },
    { LVT_CELL_PIXEL_SELECTOR, ASSETS_BASE_PATH "cvbo" PATH_SEPARATOR "cell-pixel-selector.cvbo", LWST_DEFAULT_NORMAL_COLOR, },
    { LVT_OCTAGON_PLANE, "", LWST_DEFAULT, },
    { LVT_TILE_SEL_0_0, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "tile-sel-0-0.vbo", LWST_DEFAULT, },
    { LVT_TILE_SEL_1_0, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "tile-sel-1-0.vbo", LWST_DEFAULT, },
    { LVT_TILE_SEL_0_1, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "tile-sel-0-1.vbo", LWST_DEFAULT, },
    { LVT_TILE_SEL_1_1, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "tile-sel-1-1.vbo", LWST_DEFAULT, },
    { LVT_SUZANNE, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "suzanne.vbo", LWST_DEFAULT, },
};
LwStaticAssert(ARRAY_SIZE(vbo_filename) == LVT_COUNT, "LVT_COUNT error");

typedef enum _LW_SKIN_VBO_TYPE {
	LSVT_TRIANGLE,
	LSVT_TREEPLANE,
	LSVT_HUMAN,
	LSVT_DETACHPLANE,
	LSVT_GUNTOWER,
	LSVT_TURRET,
	LSVT_CROSSBOW,
	LSVT_CATAPULT,
	LSVT_PYRO,

	LSVT_COUNT,
} LW_SKIN_VBO_TYPE;

typedef enum _LW_FAN_VBO_TYPE {
	LFVT_DEFAULT,

	LFVT_COUNT,
} LW_FAN_VBO_TYPE;

typedef enum _LW_PS_VBO_TYPE {
	LPVT_DEFAULT,

	LPVT_COUNT,
} LW_PS_VBO_TYPE;

typedef enum _LW_PS0_VBO_TYPE {
	LP0VT_DEFAULT, // rose particle emitter

	LP0VT_COUNT,
} LW_PS0_VBO_TYPE;

typedef enum _LW_MORPH_VBO_TYPE {
    LMVT_EARTH,

    LMVT_COUNT,
} LW_MORPH_VBO_TYPE;