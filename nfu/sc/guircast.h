
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
GLUI_Spinner *timestep_spinner,
             *maxsizeisorange_spinner,
             *maxposisorange_spinner,
             *maxopaisorange_spinner,
             *minsizeisorange_spinner,
             *minposisorange_spinner,
             *minopaisorange_spinner;

GLUI_Panel *obj_panel, *obj_panel1;
int winid;
extern void display(void);
extern int tstep;
extern int numoftsteps;
extern int cycle;
vcbMaterial color = MATERIAL_DEFAULT; 
float intensity = 0.8;
float fzin = 0.95;
float fzout = 1.05;
float isoval = 0.0;

float maxposIsoRange = 5.5f;
float maxsizeIsoRange = 1.5f;
float maxopaIsoRange = 0.8f;
float minposIsoRange = 3.0f;
float minsizeIsoRange = 1.0f;
float minopaIsoRange = 0.05f;

extern projStruct proj;
extern int iwinWidth;
extern int iwinHeight;


extern char setname[80];
extern char datasetname[200];

extern char datasetname[200];
extern char dataname[200];
extern char setname[80];

char oldisovals[80];
char statefile[100];
char gsfile[100];
char opfile[100];
char command[200];
char launchcmd[200];
char removecmd[200];

void control_cb(int);
void creategui(void);

void control_cb(int control)
{

  unsigned char * pixels;
  char filename[200];
  char temp[20];

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




  if (control == ISOVAL_ID) {

//Make sed command

    if(strcmp(setname, "vorts")==0) {
        system("/bin/rm gs_vorts.xml");
        memset(command, 0, 200*(sizeof(char)));
        sprintf(command, "sed 's/<pos>%s/<pos>%2.4f/g' %s > %s ", "5.5", maxposIsoRange, "global_stat_vorts.xml", "gs_vorts_tmp1.xml");
        printf("command = %s\n", command);
        system(command);
        memset(command, 0, 200*(sizeof(char)));
        sprintf(command, "sed 's/<pos>%s/<pos>%2.4f/g' %s > %s ", "3.0", minposIsoRange, "gs_vorts_tmp1.xml", "gs_vorts_tmp2.xml");
        printf("command = %s\n", command);
        system(command);
        memset(command, 0, 200*(sizeof(char)));
        sprintf(command, "sed 's/<size>%s/<size>%2.4f/g' %s > %s ", "1.5",maxsizeIsoRange, "gs_vorts_tmp2.xml", "gs_vorts_tmp3.xml");
        printf("command = %s\n", command);
       system(command);
        memset(command, 0, 200*(sizeof(char)));
       sprintf(command, "sed 's/<size>%s/<size>%2.4f/g' %s > %s ", "1.0", minsizeIsoRange, "gs_vorts_tmp3.xml", "gs_vorts_tmp4.xml");
        printf("command = %s\n", command);
        system(command);
        memset(command, 0, 200*(sizeof(char)));
        sprintf(command, "sed 's/<opa>%s/<opa>%2.4f/g' %s > %s ", "0.8", maxopaIsoRange, "gs_vorts_tmp4.xml", "gs_vorts_tmp5.xml");
        printf("command = %s\n", command);
        system(command);
        memset(command, 0, 200*(sizeof(char)));
        sprintf(command, "sed 's/<size>%s/<size>%2.4f/g' %s > %s ", "0.05", minopaIsoRange, "gs_vorts_tmp5.xml", "gs_vorts.xml");
        printf("command = %s\n", command);
        system(command);
        system("/bin/rm gs_vorts_* ");

    }
    if(strcmp(setname, "jet")==0) {
        system("/bin/rm gs_jet.xml");
        memset(command, 0, 200*(sizeof(char)));
        sprintf(command, "sed 's/<pos>%s/<pos>%2.4f/g' %s > %s ", "37.0", maxposIsoRange, "global_stat_jet.xml", "gs_jet_tmp1.xml");
        printf("command = %s\n", command);
        system(command);
        memset(command, 0, 200*(sizeof(char)));
        sprintf(command, "sed 's/<pos>%s/<pos>%2.4f/g' %s > %s ", "120.0", minposIsoRange, "gs_jet_tmp1.xml", "gs_jet_tmp2.xml");
        printf("command = %s\n", command);
        system(command);
        memset(command, 0, 200*(sizeof(char)));
        sprintf(command, "sed 's/<size>%s/<size>%2.4f/g' %s > %s ", "10.0", maxsizeIsoRange, "gs_jet_tmp2.xml", "gs_jet_tmp3.xml");
        printf("command = %s\n", command);
       system(command);
        memset(command, 0, 200*(sizeof(char)));
       sprintf(command, "sed 's/<size>%s/<size>%2.4f/g' %s > %s ", "60.0", minsizeIsoRange, "gs_jet_tmp3.xml", "gs_jet_tmp4.xml");
        printf("command = %s\n", command);
        system(command);
        memset(command, 0, 200*(sizeof(char)));
        sprintf(command, "sed 's/<opa>%s/<opa>%2.4f/g' %s > %s ", "0.1", maxopaIsoRange, "gs_jet_tmp4.xml", "gs_jet_tmp5.xml");
        printf("command = %s\n", command);
        system(command);
        memset(command, 0, 200*(sizeof(char)));
        sprintf(command, "sed 's/<opa>%s/<opa>%2.4f/g' %s > %s ", "0.8", minopaIsoRange, "gs_jet_tmp5.xml", "gs_jet.xml");
        printf("command = %s\n", command);
        system(command);
        system("/bin/rm gs_jet_* ");

    }
    if(strcmp(setname, "tsi")==0) {
        system("/bin/rm gs_tsi.xml");
        memset(command, 0, 200*(sizeof(char)));
        sprintf(command, "sed 's/<pos>%s/<pos>%2.4f/g' %s > %s ", "100.1000", maxposIsoRange, "global_stat_tsi.xml", "gs_tsi_tmp1.xml");
        printf("command = %s\n", command);
        system(command);
        memset(command, 0, 200*(sizeof(char)));
        sprintf(command, "sed 's/<pos>%s/<pos>%2.4f/g' %s > %s ", "200.0000", minposIsoRange, "gs_tsi_tmp1.xml", "gs_tsi_tmp2.xml");
        printf("command = %s\n", command);
        system(command);
        memset(command, 0, 200*(sizeof(char)));
        sprintf(command, "sed 's/<size>%s/<size>%2.4f/g' %s > %s ", "30.0000", maxsizeIsoRange, "gs_tsi_tmp2.xml", "gs_tsi_tmp3.xml");
        printf("command = %s\n", command);
       system(command);
        memset(command, 0, 200*(sizeof(char)));
       sprintf(command, "sed 's/<size>%s/<size>%2.4f/g' %s > %s ", "30.0000", minsizeIsoRange, "gs_tsi_tmp3.xml", "gs_tsi_tmp4.xml");
        printf("command = %s\n", command);
        system(command);
        memset(command, 0, 200*(sizeof(char)));
        sprintf(command, "sed 's/<opa>%s/<opa>%2.4f/g' %s > %s ", "0.0500", maxopaIsoRange, "gs_tsi_tmp4.xml", "gs_tsi_tmp5.xml");
        printf("command = %s\n", command);
        system(command);
        memset(command, 0, 200*(sizeof(char)));
        sprintf(command, "sed 's/<opa>%s/<opa>%2.4f/g' %s > %s ", "0.9000", minopaIsoRange, "gs_tsi_tmp5.xml", "gs_tsi.xml");
        printf("command = %s\n", command);
        system(command);
        system("/bin/rm gs_tsi_* ");

    }


//Finished sed command
//Make remove command
    strcpy(removecmd, "/bin/rm ");
    strncat(removecmd, datasetname, strlen(datasetname));
    strncat(removecmd, "* ", strlen("* "));
//Finished remove command

    system(removecmd);
    printf("removecmd = %s\n", removecmd);

//Make launch command
    strcpy(launchcmd, "./lors_launch ./lors_compute ");
    strcpy(opfile, "rcast_");
    strncat(opfile, setname, strlen(setname));
    strncat(opfile, ".xml ", strlen(".xml "));
    strncat(launchcmd, opfile, strlen(opfile));
    strncat(launchcmd, "rcast 1 &", strlen("rcast 0 &"));
//Finished launch command

    system(launchcmd);
    printf("launchcmd = %s\n", launchcmd);

  } 

    if (control == SCREENSHOT_ID) {

      sprintf(temp,"%.2d",tstep);

        memset(filename, 0, 200*sizeof(char));
        strcpy(filename, setname);
        strncat(filename, temp, strlen(temp));
        strncat(filename, ".bmp", strlen(".bmp")); 
        printf("%s\n", filename);

        pixels = (unsigned char *) malloc(iwinWidth*iwinHeight*4* sizeof(unsigned char));
        glReadPixels(0,0,iwinWidth,iwinHeight,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
        vcbImgWriteBMP(filename,pixels,4, iwinWidth, iwinHeight);
        free(pixels);
    }

    if(control == QUIT_ID) 
      exit(1);
}

void creategui(void) {


  if(strcmp(setname, "jet") == 0) {
    maxposIsoRange = 20.0f;
    minposIsoRange = 230.0f;
    maxsizeIsoRange = 15.0f;
    minsizeIsoRange = 30.0f;
    maxopaIsoRange = 0.2f;
    minopaIsoRange = 1.0f;
  }

  if(strcmp(setname, "tsi") == 0) {
    maxposIsoRange = 100.1000f;
    minposIsoRange = 200.0000f;
    maxsizeIsoRange = 30.0000f;
    minsizeIsoRange = 30.0000f;
    maxopaIsoRange = 0.0500f;
    minopaIsoRange = 0.9000f;
  }

  winid = glutCreateWindow( "Rcastview");
  glutDisplayFunc(display);
  GLUI_Master.set_glutReshapeFunc(reshape);
  GLUI_Master.set_glutKeyboardFunc(keys);
  GLUI_Master.set_glutSpecialFunc( NULL);
  GLUI_Master.set_glutMouseFunc(mouse_handler);
  GLUI_Master.set_glutIdleFunc(display);
  glutMotionFunc(trackMotion);

  glui = GLUI_Master.create_glui_subwindow(winid, GLUI_SUBWINDOW_RIGHT);

  obj_panel = glui->add_rollout("Timestep", false);

  timestep_spinner =
      glui->add_spinner_to_panel(obj_panel, "Timestep", GLUI_SPINNER_INT, &tstep, TIMESTEP_ID, control_cb);
  glui->add_button_to_panel(obj_panel, "Cycle", CYCLE_ID, control_cb);

  timestep_spinner->set_int_limits(1, numoftsteps+1, GLUI_LIMIT_WRAP);

  obj_panel1 = glui->add_rollout("Isoranges", false);

  maxposisorange_spinner = glui->add_spinner_to_panel(obj_panel1, "Position", GLUI_SPINNER_FLOAT, &maxposIsoRange, ISOVAL_ID, control_cb);

  maxposisorange_spinner->set_float_limits(0.0f, 500.0f, GLUI_LIMIT_WRAP);

  minposisorange_spinner = glui->add_spinner_to_panel(obj_panel1, "Position", GLUI_SPINNER_FLOAT, &minposIsoRange, ISOVAL_ID, control_cb);

  minposisorange_spinner->set_float_limits(0.0f, 500.0f, GLUI_LIMIT_WRAP);


  maxsizeisorange_spinner = glui->add_spinner_to_panel(obj_panel1, "Size", GLUI_SPINNER_FLOAT, &maxsizeIsoRange, ISOVAL_ID, control_cb);

  maxsizeisorange_spinner->set_float_limits(0.0f, 500.0f, GLUI_LIMIT_WRAP);

  minsizeisorange_spinner = glui->add_spinner_to_panel(obj_panel1, "Size", GLUI_SPINNER_FLOAT, &minsizeIsoRange, ISOVAL_ID, control_cb);

  minsizeisorange_spinner->set_float_limits(0.0f, 500.0f, GLUI_LIMIT_WRAP);


  maxopaisorange_spinner = glui->add_spinner_to_panel(obj_panel1, "Opacity", GLUI_SPINNER_FLOAT, &maxopaIsoRange, ISOVAL_ID, control_cb);

  maxopaisorange_spinner->set_float_limits(0.0f, 500.0f, GLUI_LIMIT_WRAP);


  minopaisorange_spinner = glui->add_spinner_to_panel(obj_panel1, "Opacity", GLUI_SPINNER_FLOAT, &minopaIsoRange, ISOVAL_ID, control_cb);

  minopaisorange_spinner->set_float_limits(0.0f, 500.0f, GLUI_LIMIT_WRAP);


//  glui->add_button("Zoom In", ZOOMIN_ID, control_cb);
//  glui->add_button("Zoom Out", ZOOMOUT_ID, control_cb);


  glui->add_button("Screenshot", SCREENSHOT_ID, control_cb);

  glui->add_button("Quit", QUIT_ID, control_cb );

  glui->set_main_gfx_window( winid );


}
