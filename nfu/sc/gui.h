#include "glui.h"
#include <GL/glut.h>
#include <string.h>

#define STEP1_ID 100
#define TIMESTEP_ID 101
#define CYCLE_ID 102
#define BRONZE_ID 103
#define CHROME_ID 104
#define GOLD_ID 105
#define BRASS_ID 106
#define EMERALD_ID 107
#define PEARL_ID 108
#define RUBY_ID 109
#define TURQUOISE_ID 110
#define INTENSITY_ID 111
#define ZOOMIN_ID 112
#define ZOOMOUT_ID 113
#define SCREENSHOT_ID 114
#define ISOVAL_ID 115
#define QUIT_ID 116

GLUI *glui;
GLUI_Panel *obj_panel, *obj_panel2;
GLUI_Spinner *timestep_spinner, *light_spinner, *isoval_spinner;
int winid;
extern void display(void);
extern int tstep;
extern int numoftsteps;
extern int cycle;
vcbMaterial color = MATERIAL_DEFAULT; 
float intensity = 0.8;
float fzin = 0.95;
float fzout = 1.05;
float isoval = 37.0;

extern projStruct proj;
extern int iwinWidth;
extern int iwinHeight;

extern char setname[80];
char isovalue[80];
char oldisovalue[80];
char statefile[100];
char gsfile[100];
char opfile[100];
char command[200];
char launchcmd[200];
char removecmd[200];
extern char datasetname[200];
extern void cleanup(void);

void control_cb(int);
void creategui(void);

void control_cb(int control)
{

  unsigned char * pixels;

  if (control == STEP1_ID)
    exit(0);
  if (control == TIMESTEP_ID) {
  //  printf("Timestep = %d", tstep);
  }
  if (control == CYCLE_ID) {
    if (cycle == 0) {
      cycle = 1;
    }
    else {
      cycle = 0; 
    }
  }

  if (control == ISOVAL_ID) {

    memset(oldisovalue, 0, 80*sizeof(char));

//Make sed command
    strcpy(command, "sed 's/<iso_value>");

    if(strcmp(setname, "vorts")==0)
      strcpy(oldisovalue, "5.5");
    if(strcmp(setname, "jet")==0)
      strcpy(oldisovalue, "37.0");
    if(strcmp(setname, "tsi")==0)
      strcpy(oldisovalue, "200.0");

    strncat(command, oldisovalue, strlen(oldisovalue));
    strncat(command, "/<iso_value>", strlen("/<iso_value>"));
    sprintf(isovalue, "%2.4f", isoval);
    strncat(command, isovalue, strlen(isovalue));
    strncat(command, "/g' ", strlen("/g' "));

    if(strcmp(setname, "vorts")) {
      strcpy(statefile, "global_stat_vorts.xml");
      strcpy(gsfile, "gs_vorts.xml");
    }
    if(strcmp(setname, "jet")) {
      strcpy(statefile, "global_stat_jet.xml");
      strcpy(gsfile, "gs_jet.xml");
    }
    if(strcmp(setname, "tsi")) {
      strcpy(statefile, "global_stat_tsi.xml");
      strcpy(gsfile, "gs_tsi.xml");
    }
    strncat(command, statefile, strlen(statefile));
    strncat(command, " > ", strlen(" > "));
    strncat(command, gsfile, strlen(gsfile));
//Finished sed command

    system(command);
    printf("command = %s\n", command);

//Make remove command
    strcpy(removecmd, "/bin/rm ");
    strncat(removecmd, datasetname, strlen(datasetname));
    strncat(removecmd, "* ", strlen("* "));
//Finished remove command

    system(removecmd);
    printf("removecmd = %s\n", removecmd);

//Make launch command
    strcpy(launchcmd, "./lors_launch ./lors_compute ");
    strcpy(opfile, "mcube_");
    strncat(opfile, setname, strlen(setname));
    strncat(opfile, ".xml ", strlen(".xml "));
    strncat(launchcmd, opfile, strlen(opfile));
    strncat(launchcmd, "mcube 1 &", strlen(".xml mcube 0 &"));
//Finished launch command

    system(launchcmd);
    printf("launchcmd = %s\n", launchcmd);

  } 

  if (control == ZOOMIN_ID) {
        proj.xmin *= fzin;
        proj.xmax *= fzin;
        proj.ymin *= fzin;
        proj.ymax *= fzin;
        proj.fov  *= fzin;
        reshape(iwinWidth,iwinHeight);
  } 

  if (control == ZOOMOUT_ID) {
        proj.xmin *= fzout;
        proj.xmax *= fzout;
        proj.ymin *= fzout;
        proj.ymax *= fzout;
        proj.fov  *= fzout;
        reshape(iwinWidth,iwinHeight);
  }

  if (control == SCREENSHOT_ID) {
        pixels = (unsigned char *) malloc(iwinWidth*iwinHeight*4* sizeof(unsigned char));
        glReadPixels(0,0,iwinWidth,iwinHeight,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
        vcbImgWriteBMP("pbufres.bmp",pixels,4, iwinWidth, iwinHeight);
        free(pixels);
  }

  if (control == BRONZE_ID)
    color = MATERIAL_BRONZE;
  if (control == CHROME_ID)
    color = MATERIAL_CHROME;
  if (control == GOLD_ID)
    color = MATERIAL_GOLD;
  if (control == BRASS_ID) 
    color = MATERIAL_BRASS;
  if (control == EMERALD_ID)
    color = MATERIAL_EMERALD;
  if (control == RUBY_ID)
    color = MATERIAL_RUBY;
  if (control == PEARL_ID)
    color = MATERIAL_PEARL;
  if (control == TURQUOISE_ID)
    color = MATERIAL_TURQUOISE;

  if(control == QUIT_ID)
    exit(0);

}

void creategui(void) {

  winid = glutCreateWindow( "Meshview");
  glutDisplayFunc(display);
  GLUI_Master.set_glutReshapeFunc(reshape);
  GLUI_Master.set_glutKeyboardFunc(keys);
  GLUI_Master.set_glutSpecialFunc( NULL);
  GLUI_Master.set_glutMouseFunc(mouse_handler);
  GLUI_Master.set_glutIdleFunc(display);
  glutMotionFunc(trackMotion);

  glui = GLUI_Master.create_glui_subwindow(winid, GLUI_SUBWINDOW_RIGHT);

  obj_panel = glui->add_rollout("Timestep", false);
  timestep_spinner = glui->add_spinner_to_panel(obj_panel, 
						"Timestep", 
						GLUI_SPINNER_INT, 
						&tstep, 
						TIMESTEP_ID, 
						control_cb);

  glui->add_button_to_panel(obj_panel, "Cycle", CYCLE_ID, control_cb);

  timestep_spinner->set_int_limits(1, numoftsteps+1, GLUI_LIMIT_WRAP);

  //printf("\nhey, I am here\n\n");

  if(strcmp(setname, "vorts") == 0)
    isoval = 5.5;

  if(strcmp(setname, "tsi") == 0)
    isoval = 200.0;

  isoval_spinner = glui->add_spinner("Isovalue", GLUI_SPINNER_FLOAT, &isoval, ISOVAL_ID, control_cb);

  isoval_spinner->set_float_limits(0.0, 300.0, GLUI_LIMIT_WRAP);

  light_spinner = glui->add_spinner("Light Intensity", GLUI_SPINNER_FLOAT, &intensity, INTENSITY_ID, control_cb);
 
  light_spinner->set_float_limits(0.0, 1.0, GLUI_LIMIT_WRAP);

  glui->add_button("Zoom In", ZOOMIN_ID, control_cb);
  glui->add_button("Zoom Out", ZOOMOUT_ID, control_cb); 

  obj_panel2 = glui->add_rollout("Color", false);
  glui->add_button_to_panel(obj_panel2, "Bronze", BRONZE_ID, control_cb);
  glui->add_button_to_panel(obj_panel2, "Chrome", CHROME_ID, control_cb);
  glui->add_button_to_panel(obj_panel2, "Gold", GOLD_ID, control_cb);
  glui->add_button_to_panel(obj_panel2, "Brass", BRASS_ID, control_cb);
  glui->add_button_to_panel(obj_panel2, "Emerald", EMERALD_ID, control_cb);
  glui->add_button_to_panel(obj_panel2, "Pearl", PEARL_ID, control_cb);
  glui->add_button_to_panel(obj_panel2, "Ruby", RUBY_ID, control_cb);
  glui->add_button_to_panel(obj_panel2, "Turquoise", TURQUOISE_ID, control_cb);

  glui->add_button("Screenshot", SCREENSHOT_ID, control_cb);

  glui->add_button("Quit", QUIT_ID, control_cb );

  glui->set_main_gfx_window( winid );


}
