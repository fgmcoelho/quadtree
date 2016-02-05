#include "orx.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

struct area_st {
    int left, right, top, bottom;
};

struct quad_tree_st {
    struct quad_tree_st *nw, *ne, *sw, *se, *parent;
    struct area_st area;
	int closed:1;
};

#define FILL_AREA(area, l, r, b, t) do{ \
        area.left = l;\
        area.right = r;\
        area.bottom = b; \
        area.top = t;\
    }while(0)

static struct quad_tree_st* root;

int area_intersect(struct area_st* first, struct area_st* second){
    return !(second->left > first->right || second->right < first->left ||
		second->top < first->bottom || second->bottom > first->top);
}

#define DOES_NOT_CONTAINS 0
#define CONTAINS 1
#define PERFECTLY_CONTAINS 2

int area_contains(struct area_st* node_area, struct area_st* object_area){
    if (node_area->left >= object_area->right
            || node_area->right <= object_area->left
            || node_area->top <= object_area->bottom
            || node_area->bottom >= object_area->top){
        return DOES_NOT_CONTAINS;
    }
    if (node_area->left >= object_area->left
            && node_area->right <= object_area->right
            && node_area->bottom >= object_area->bottom
            && node_area->top <= object_area->top){
        return PERFECTLY_CONTAINS;
    }
    return CONTAINS;
}

struct quad_tree_st* quad_tree_create_node(
		struct quad_tree_st* node,
		struct area_st* new_area){

    struct quad_tree_st* new_node = malloc(sizeof(struct quad_tree_st));
    if (new_node == NULL){
        printf("Error, out of memory!\n");
        return NULL;
    }

    new_node->nw = new_node->ne = new_node->sw = new_node->se = NULL;
    memcpy(&new_node->area, new_area, sizeof(struct area_st));
	new_node->closed = 0;
    new_node->parent = node;

    return new_node;
}

static int quad_tree_node_divide(struct quad_tree_st* node){
	struct area_st new_area;
	int midX = node->area.left + \
        ((node->area.right - node->area.left)/2 + \
         ((node->area.right -node->area.left) % 2));
	int midY = node->area.bottom + \
        ((node->area.top - node->area.bottom)/2 - \
         ((node->area.top - node->area.bottom) % 2));

    FILL_AREA(new_area, node->area.left, midX, midY, node->area.top);
	node->nw = quad_tree_create_node(node, &new_area);
 	assert(node->nw != NULL);

    FILL_AREA(new_area, midX, node->area.right, midY, node->area.top);
    node->ne = quad_tree_create_node(node, &new_area);
	assert(node->ne != NULL);

    FILL_AREA(new_area, node->area.left, midX, node->area.bottom, midY);
    node->sw = quad_tree_create_node(node, &new_area);
    assert(node->sw != NULL);

    FILL_AREA(new_area, midX, node->area.right, node->area.bottom, midY);
    node->se = quad_tree_create_node(node, &new_area);
    assert(node->se != NULL);

    return 1;
}

void quad_tree_insert_object(
        struct quad_tree_st* node,
        struct area_st* object){

    int res = area_contains(&node->area, object);
    if (res == DOES_NOT_CONTAINS){
        return;
    }
    else if (res == PERFECTLY_CONTAINS){
		node->closed = 1;
        return;
	}
	else{
        if (node->sw == NULL){
            quad_tree_node_divide(node);
        }
        quad_tree_insert_object(node->nw, object);
        quad_tree_insert_object(node->ne, object);
        quad_tree_insert_object(node->sw, object);
        quad_tree_insert_object(node->se, object);
	}
}

void quad_tree_clear(struct quad_tree_st* node){
    if (node != NULL){
        if (node->sw != NULL){
            quad_tree_clear(node->nw);
            quad_tree_clear(node->ne);
            quad_tree_clear(node->sw);
            quad_tree_clear(node->se);
        }
        free(node);
    }
}

orxVIEWPORT* viewport;
orxU8 blueCount, redCount;

static void plot_quad_tree(struct quad_tree_st* node){
    if (node->sw == NULL){
        orxVECTOR points[4], screenPoints[4];
        orxU32 i;
        orxVector_Set(&points[0], node->area.left, node->area.bottom, orxFLOAT_0);
        orxVector_Set(&points[1], node->area.left, node->area.top, orxFLOAT_0);
        orxVector_Set(&points[2], node->area.right, node->area.top, orxFLOAT_0);
        orxVector_Set(&points[3], node->area.right, node->area.bottom, orxFLOAT_0);

        for (i = 0; i < 4; ++i){
            orxRender_GetScreenPosition(&points[i], viewport, &screenPoints[i]);
        }

        orxRGBA color;
        color.u8A = 0x80;
        if (node->closed){
            color.u8G = blueCount;
            color.u8R = 0x00;
            color.u8B = 0xFF;
            blueCount += 0x20;
        }
        else{
            color.u8G = redCount;
            color.u8R = 0xFF;
            color.u8B = 0x00;
            redCount += 0x20;
        }
        orxDisplay_DrawPolygon(&screenPoints, 4, color, orxTRUE);
    }
    else{
        plot_quad_tree(node->nw);
        plot_quad_tree(node->ne);
        plot_quad_tree(node->sw);
        plot_quad_tree(node->se);
    }
}

orxSTATUS orxFASTCALL Exit(){
    quad_tree_clear(root);
    return orxSTATUS_SUCCESS;
}

orxSTATUS orxFASTCALL Run(){

    orxCAMERA* camera = orxViewport_GetCamera(viewport);
    orxVECTOR pos;
    orxCamera_GetPosition(camera, &pos);

    if(orxInput_IsActive("GoRight")) {
        pos.fX += 3;
        orxCamera_SetPosition(camera, &pos);
    }
    else if(orxInput_IsActive("GoLeft")) {
        pos.fX -= 3;
        orxCamera_SetPosition(camera, &pos);
    }
    else if(orxInput_IsActive("GoUp")) {
        pos.fY -= 3;
        orxCamera_SetPosition(camera, &pos);
    }
    else if(orxInput_IsActive("GoDown")) {
        pos.fY += 3;
        orxCamera_SetPosition(camera, &pos);
    }

    //orxLOG("Camera pos: %f %f", pos.fX, pos.fY);

    return orxSTATUS_SUCCESS;
}

static orxSTATUS orxFASTCALL draw_basic(const orxEVENT* event){
    if (event->eID ==  orxRENDER_EVENT_VIEWPORT_STOP){
        blueCount = 0;
        redCount = 0;
        plot_quad_tree(root);
    }

    return orxSTATUS_SUCCESS;
}

orxSTATUS orxFASTCALL Init(){
    viewport = orxViewport_CreateFromConfig("Viewport");
    orxMouse_ShowCursor(orxTRUE);
    orxEvent_AddHandler(orxEVENT_TYPE_RENDER, draw_basic);

    struct area_st temp_area, object, object2, object3;
    FILL_AREA(temp_area, 200, 400, 200, 400);
    root = quad_tree_create_node(NULL, &temp_area);

    FILL_AREA(object, 270, 300, 280, 300);
    quad_tree_insert_object(root, &object);

    FILL_AREA(object2, 200, 240, 200, 240);
    quad_tree_insert_object(root, &object2);

    FILL_AREA(object3, 350, 400, 350, 400);
    quad_tree_insert_object(root, &object3);

    return orxSTATUS_SUCCESS;
}

int main(int argc, char* argv[]){
    orx_Execute(argc, argv, Init, Run, Exit);
    return 0;
}
