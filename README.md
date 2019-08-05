# miyoo_daemon

## Default Button Mappings

| BUTTON | ACTION |
|  --- | --- |
| X, Right | Brightness Up |
| A, Left | Brightness Down |
| B, Down | Volume Down |
| Y, Up | Volume Up |
| Start | Mute |
| Hold Select | Quick Shutdown |

## Config
button config location  
`/mnt/.buttons.conf`  
button config format  
`x:a:y:b:up:down:left:right:select:start:l1:r1:hold-x:hold-a:hold-y:hold-b:hold-up:hold-down:hold-left:hold-right:hold-select:hold-start:hold-l1:hold-r1`  
button config default  
`1:2:3:4:3:4:2:1:0:5:0:0:0:0:0:0:0:0:0:0:21:0:0:0` 

## Actions
1 = backlight up  
2 = backlight down  
3 = volume up  
4 = volume down  
5 = mute  
6 = volume rotate up  
7 = volume rotate down  
8 = backlight rotate up  
9 = backlight rotate down  
10 = backlight min max  
11 = remount /mnt as rw  
12 = remount /mnt as ro  
20 = kill, sync and shutdown  
21 = kill. Does not work on most emulators. do not use  
