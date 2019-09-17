/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option)any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <linux/ioctl.h>
#include <sys/fcntl.h> 
#include <sys/stat.h>
#include <sys/ioctl.h>

#define MIYOO_VIR_SET_MODE    _IOWR(0x100, 0, unsigned long)
#define MIYOO_VIR_SET_VER     _IOWR(0x101, 0, unsigned long)
#define MIYOO_SND_SET_VOLUME  _IOWR(0x100, 0, unsigned long)
#define MIYOO_KBD_GET_HOTKEY  _IOWR(0x100, 0, unsigned long)
#define MIYOO_KBD_SET_VER     _IOWR(0x101, 0, unsigned long)
#define MIYOO_FB0_PUT_OSD     _IOWR(0x100, 0, unsigned long)
#define MIYOO_FB0_SET_MODE    _IOWR(0x101, 0, unsigned long)
#define MIYOO_FB0_GET_VER     _IOWR(0x102, 0, unsigned long)
#define MIYOO_FB0_SET_FPBP    _IOWR(0x104, 0, unsigned long)
#define MIYOO_FB0_GET_FPBP    _IOWR(0x105, 0, unsigned long)

#define MIYOO_FBP_FILE        "/mnt/.fpbp.conf"
#define MIYOO_LID_FILE        "/mnt/.backlight.conf"
#define MIYOO_VOL_FILE        "/mnt/.volume.conf"
#define MIYOO_LID_CONF        "/sys/devices/platform/backlight/backlight/backlight/brightness"
#define MIYOO_BUTTON_FILE     "/mnt/.buttons.conf"
#define MIYOO_BATTERY    "/sys/class/power_supply/miyoo-battery/voltage_now"
#define MIYOO_BATTERY_FILE    "/mnt/.batterylow.conf"

#define BUTTON_COUNT	10

unsigned char actionmap[BUTTON_COUNT*2]={0,0,0,0,3,4,2,1,0,13,0,0,0,0,0,0,0,0,20,0};


static void create_daemon(void)
{
  int x;

  pid_t pid;

  pid = fork();
  if (pid < 0) {
    exit(EXIT_FAILURE);
  }

  if (pid > 0) {
    exit(EXIT_SUCCESS);
  }

  if (setsid() < 0) {
    exit(EXIT_FAILURE);
  }
  signal(SIGCHLD, SIG_IGN);
  signal(SIGHUP, SIG_IGN);
  pid = fork();
  if (pid < 0) {
    exit(EXIT_FAILURE);
  }

  if (pid > 0) {
    exit(EXIT_SUCCESS);
  }
  umask(0);
  chdir("/");
  
  for (x = sysconf(_SC_OPEN_MAX); x>=0; x--){
    close(x);
  }
}

static int read_int(int fd, int default_val)
{
  int i;
  int val = default_val;
  char buf[10]={0};
  
  if(fd < 0){
    return default_val;
  }
  else{
    read(fd, buf, sizeof(buf));
    for(i=0; i<strlen(buf); i++){
      if(buf[i] == '\r'){
        buf[i] = 0;
      }
      if(buf[i] == '\n'){
        buf[i] = 0;
      }
      if(buf[i] == ' '){
        buf[i] = 0;
      }
    }
    val = atoi(buf);
  }
  return val;
}

static int read_conf(const char *file, int default_val)
{
  int i, fd;
  
  fd = open(file, O_RDWR);
  i = read_int(fd,default_val);
  close(fd);
  return i;
}

static void write_conf(const char *file, int val)
{
  int fd;
  char buf[10]={0};
  
  fd = open(file, O_WRONLY | O_CREAT | O_TRUNC);
  if(fd > 0){
    sprintf(buf, "%d", val);
    write(fd, buf, strlen(buf));
    close(fd);
  }
}

char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        *(result + idx) = 0;
    }

    return result;
}

static void read_button_config(const char *file, char *vals)
{
  
  int i, fd;
  char buf[BUTTON_COUNT * 8 + 12]={0};
  
  fd = open(file, O_RDWR);
  if(fd < 0){
    return;
  }
  else{
    read(fd, buf, sizeof(buf));
    for(i=0; i<strlen(buf); i++){
      if(buf[i] == '\r'){
        buf[i] = 0;
      }
      if(buf[i] == '\n'){
        buf[i] = 0;
      }
      if(buf[i] == ' '){
        buf[i] = 0;
      }
    }
    char** tokens;
    tokens = str_split(buf, ':');
    for(i=0; i<BUTTON_COUNT*2; i++){
      vals[i] = atol(tokens[i]);
    }
    free(tokens);
  }
  close(fd);
  return;
}

static void info_fb0(int fb0, int lid, int vol, int show_osd)
{
  unsigned long val;

  val = (show_osd ? 0x80000000 : 0x00000000) | (vol << 16) | (lid);
  ioctl(fb0, MIYOO_FB0_PUT_OSD, val);
}

void my_handler(int signum)
{
    if (signum == SIGUSR1)
    {
       read_button_config(MIYOO_BUTTON_FILE,actionmap);
    }
}

int main(int argc, char** argv)
{
  int lid=0, vol=0, fbp=0;
  char buf[255]={0};
  unsigned long ret, lastret, version;
  int fb0, kbd, snd, vir;
  int battery_low=3550;
  FILE *battery_file;
  char wstr[100];
  int battery_level; 
  setvbuf (stdout, NULL, _IONBF, 0);

  create_daemon();
  fb0 = open("/dev/miyoo_fb0", O_RDWR);
  kbd = open("/dev/miyoo_kbd", O_RDWR);
  snd = open("/dev/miyoo_snd", O_RDWR);

  // fp, bp
  fbp = read_conf(MIYOO_FBP_FILE, -1);
  if(fbp > 0){
    ioctl(fb0, MIYOO_FB0_SET_FPBP, fbp);
  }

  // backlight
  lid = read_conf(MIYOO_LID_FILE, 5);
  if(lid < 0){
    lid = 5;
    write_conf(MIYOO_LID_FILE, lid);
  }
  sprintf(buf, "echo %d > %s", lid, MIYOO_LID_CONF);
  system(buf);
  
  // volume
  vol = read_conf(MIYOO_VOL_FILE,5);
  if(vol < 0){
    vol = 5;
    write_conf(MIYOO_VOL_FILE, vol);
  }
  ioctl(snd, MIYOO_SND_SET_VOLUME, vol);

  // buttons
  read_button_config(MIYOO_BUTTON_FILE,actionmap);
  signal(SIGUSR1, my_handler);

  // info fb0
  info_fb0(fb0, lid, vol, 0);

  //battery
  battery_low = read_conf(MIYOO_BATTERY_FILE,3550);
  //battery_file = open(MIYOO_BATTERY, O_RDWR);

  // update version
  ioctl(fb0, MIYOO_FB0_GET_VER, &ret);
  version = ret;
  ioctl(kbd, MIYOO_KBD_SET_VER, ret);
  vir = open("/dev/miyoo_vir", O_RDWR);
  ioctl(vir, MIYOO_VIR_SET_VER, ret);
  close(vir);
  lastret = 0;
  unsigned int counter = 0;
  unsigned int actioned = 0;
  unsigned int battery_counter = 0;
  unsigned int battery_flash_counter = 0;
  while(1){
    usleep(40000);
    
    if (battery_counter == 0){
        battery_file = fopen(MIYOO_BATTERY, "r");
        while ( (fgets(wstr,100,battery_file)) != NULL ) {
	  battery_level = atoi(wstr) ;
          //printf("%s\n", wstr);
        }
        fclose(battery_file);
    }

    battery_counter++;
    battery_counter%=750;

      if(battery_level > 0 && battery_level <  battery_low) {
        battery_flash_counter++;
      } else {
        battery_flash_counter = 0;
      }

    battery_flash_counter%=4000;
    
    if ((battery_flash_counter<299)&&(battery_flash_counter>210)) {
          //bright
      if(version<3) {
              sprintf(buf, "echo %d > %s", ((battery_flash_counter % 6) +4), MIYOO_LID_CONF); 
              system(buf); 
      } else if(battery_flash_counter == 211) {
        vir = open("/dev/miyoo_vir", O_RDWR);
          if(vir > 0){
            ioctl(vir, MIYOO_VIR_SET_MODE, 0);
            close(vir);
          }
      }
    } else if(battery_flash_counter==300 && version > 2 ) {
      vir = open("/dev/miyoo_vir", O_RDWR);
       if(vir > 0){
         ioctl(vir, MIYOO_VIR_SET_MODE, 1);
         close(vir);
       }
    }

    ioctl(kbd, MIYOO_KBD_GET_HOTKEY, &ret);
    if(ret == 0 && lastret == 0){
      actioned = 0;
      continue;
    } else if (ret == lastret) {
	    counter++;
	    if (counter > 15) {
        if (lastret != ret + BUTTON_COUNT) {
          lastret = ret + BUTTON_COUNT;
        }
      } else {
        lastret = ret;
      }
      continue;
    } else if(actioned == 0 && ( (ret == 0 && lastret != 0) || (ret + BUTTON_COUNT == lastret) ) ) {
      counter = 0;
      actioned = 1;
      switch(actionmap[lastret-1]){
      case 0:
	  ;
	  break;
      case 1:

        //printf("backlight++\n");
        if(lid < 10){
          lid+= 1;
          write_conf(MIYOO_LID_FILE, lid);
          sprintf(buf, "echo %d > %s", lid, MIYOO_LID_CONF);
          system(buf);
          info_fb0(fb0, lid, vol, 1);
        }
/**/
//    battery_counter++;
        break;
      case 2:
        //printf("backlight--\n");
        if(lid > 1){
          lid-= 1;
          write_conf(MIYOO_LID_FILE, lid);
          sprintf(buf, "echo %d > %s", lid, MIYOO_LID_CONF);
          system(buf);
          info_fb0(fb0, lid, vol, 1);
        }
        break;
      case 3:
        //printf("sound++\n");
        if(vol < 9){
          vol+= 1;
          write_conf(MIYOO_VOL_FILE, vol);
          ioctl(snd, MIYOO_SND_SET_VOLUME, vol);
          info_fb0(fb0, lid, vol, 1);
        }
        break;
      case 4:
        //printf("sound--\n");
        if(vol > 0){
          vol-= 1;
          write_conf(MIYOO_VOL_FILE, vol);
          ioctl(snd, MIYOO_SND_SET_VOLUME, vol);
          info_fb0(fb0, lid, vol, 1);
        }
        break;
      case 5:
        //printf("mute\n");
        if(vol == 0){
          vol = read_conf(MIYOO_VOL_FILE,5);
          if(vol < 1){
            vol = 5;
            write_conf(MIYOO_VOL_FILE, vol);
          }
          ioctl(snd, MIYOO_SND_SET_VOLUME, vol);
        } else {
          vol = 0;
          ioctl(snd, MIYOO_SND_SET_VOLUME, vol);
        }
        break;
      case 6: 
        //printf("volume rotate up\n"); 
        if(vol < 9){ 
          vol+= 1;
          write_conf(MIYOO_VOL_FILE, vol);
          ioctl(snd, MIYOO_SND_SET_VOLUME, vol);
          info_fb0(fb0, lid, vol, 1);
        } else { 
          vol = 0;
          write_conf(MIYOO_VOL_FILE, vol);
          ioctl(snd, MIYOO_SND_SET_VOLUME, vol);
          info_fb0(fb0, lid, vol, 1);
        } 
        break;
      case 7: 
        //printf("volume rotate down\n"); 
        if(vol < 1){ 
          vol = 9;
          write_conf(MIYOO_VOL_FILE, vol);
          ioctl(snd, MIYOO_SND_SET_VOLUME, vol);
          info_fb0(fb0, lid, vol, 1);
        } else { 
          vol -= 1;
          write_conf(MIYOO_VOL_FILE, vol);
          ioctl(snd, MIYOO_SND_SET_VOLUME, vol);
          info_fb0(fb0, lid, vol, 1);
        } 
        break;
      case 8: 
        //printf("backlight rotate up\n"); 
        if(lid < 10){ 
          lid+= 1; 
          write_conf(MIYOO_LID_FILE, lid); 
          sprintf(buf, "echo %d > %s", lid, MIYOO_LID_CONF); 
          system(buf); 
          info_fb0(fb0, lid, vol, 1); 
        } else { 
          lid= 1; 
          write_conf(MIYOO_LID_FILE, lid); 
          sprintf(buf, "echo %d > %s", lid, MIYOO_LID_CONF); 
          system(buf); 
          info_fb0(fb0, lid, vol, 1); 
        } 
        break;
      case 9: 
        //printf("backlight rotate down\n"); 
        if(lid == 1){ 
          lid = 10; 
          write_conf(MIYOO_LID_FILE, lid); 
          sprintf(buf, "echo %d > %s", lid, MIYOO_LID_CONF); 
          system(buf); 
          info_fb0(fb0, lid, vol, 1); 
        } else { 
          lid -= 1; 
          write_conf(MIYOO_LID_FILE, lid); 
          sprintf(buf, "echo %d > %s", lid, MIYOO_LID_CONF); 
          system(buf); 
          info_fb0(fb0, lid, vol, 1); 
        } 
        break;
      case 10: 
        //printf("backlight min max\n"); 
        if(lid != 10){ 
          lid = 10; 
          write_conf(MIYOO_LID_FILE, lid); 
          sprintf(buf, "echo %d > %s", lid, MIYOO_LID_CONF); 
          system(buf); 
          info_fb0(fb0, lid, vol, 1); 
        } else { 
          lid = 2; 
          write_conf(MIYOO_LID_FILE, lid); 
          sprintf(buf, "echo %d > %s", lid, MIYOO_LID_CONF); 
          system(buf); 
          info_fb0(fb0, lid, vol, 1); 
        } 
  	break ;
   	case 11:
      	  system("mount -o remount,rw,utf8 /dev/mmcblk0p4");
          break;
      case 12:
          system("mount -o remount,ro,utf8 /dev/mmcblk0p4");
          break;
      case 13:
          system("sh -c /mnt/apps/fbgrab/screenshot.sh");
          break;
      case 20:
        {
          int status;
          pid_t son = fork();
          if (!son) {
            execlp("sh", "sh", "-c", "kill $(ps -al | grep \"/mnt/\" | grep -v \"/kernel/\" | tr -s [:blank:] | cut -d \" \" -f 2) ; sleep 0.1 ; sync && poweroff",  NULL);
          }
          break;
	        }
      case 21:
        {
          //printf("kill\n"); 
          int status;
          pid_t son = fork();
          if (!son) {
            //execlp("sh", "sh", "/mnt/kernel/killgui.sh", NULL);
            execlp("sh", "sh", "-c", "kill $(ps -al | grep \"/mnt/\" | grep -v \"/kernel/\" | tr -s [:blank:] | cut -d \" \" -f 2)",  NULL);
          }
          break; 
        }
      }
    } 
    lastret = ret;
  }
 
  close(fb0);
  close(kbd);
  close(snd);
//  close(battery_file);

  return EXIT_SUCCESS;
}


