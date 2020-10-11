/****************************************************************************/
/* KEYS.H - keystroke codes for the IBM PC                                  */
/*                                                                          */
/* (C) COPYRIGHT 1984  Marc Adler      All Rights Reserved                  */
/****************************************************************************/

/************************** UNSHIFTED FUNCTION KEYS ************************/
#define  VK_F1           187
#define  VK_F2           188
#define  VK_F3           189
#define  VK_F4           190
#define  VK_F5           191
#define  VK_F6           192
#define  VK_F7           193
#define  VK_F8           194
#define  VK_F9           195
#define  VK_F10          196
#define  VK_F11          181
#define  VK_F12          182

/************************** SHIFTED FUNCTION KEYS **************************/
#define  VK_SH_F1        212
#define  VK_SH_F2        213
#define  VK_SH_F3        214
#define  VK_SH_F4        215
#define  VK_SH_F5        216
#define  VK_SH_F6        217
#define  VK_SH_F7        218
#define  VK_SH_F8        219
#define  VK_SH_F9        220
#define  VK_SH_F10       221
#define  VK_SH_F11       179
#define  VK_SH_F12       180

/**************************** ALT FUNCTION KEYS *****************************/
#define  VK_ALT_F1       232
#define  VK_ALT_F2       233
#define  VK_ALT_F3       234
#define  VK_ALT_F4       235
#define  VK_ALT_F5       236
#define  VK_ALT_F6       237
#define  VK_ALT_F7       238
#define  VK_ALT_F8       239
#define  VK_ALT_F9       240
#define  VK_ALT_F10      241
#define  VK_ALT_F11      185
#define  VK_ALT_F12      186

/**************************** CTRL FUNCTION KEYS *****************************/
#define  VK_CTRL_F1      222
#define  VK_CTRL_F2      223
#define  VK_CTRL_F3      224
#define  VK_CTRL_F4      225
#define  VK_CTRL_F5      226
#define  VK_CTRL_F6      227
#define  VK_CTRL_F7      228
#define  VK_CTRL_F8      229
#define  VK_CTRL_F9      230
#define  VK_CTRL_F10     231
#define  VK_CTRL_F11     183
#define  VK_CTRL_F12     184

/**************************** KEYPAD FUNCTION KEYS ***************************/
#define  VK_ESC           27
#define  VK_UP            200
#define  VK_DOWN          208
#define  VK_LEFT          203
#define  VK_RIGHT         205
#define  VK_INS           210
#define  VK_DEL           211
#define  VK_BACKSPACE     8
#define  VK_TAB           9
#define  VK_RETURN        '\r'
#define  VK_NEWLINE       '\n'
#define  VK_BACKTAB       143

/************************* CONTROL FUNCTION KEYS ************************/
#define  VK_HOME          199
#define  VK_CTRL_HOME     247
#define  VK_END           207
#define  VK_CTRL_END      245
#define  VK_PGUP          201
  #define VK_PRIOR        VK_PGUP
#define  VK_PGDN          209
  #define VK_NEXT         VK_PGDN
#define  VK_CTRL_PGUP     132
#define  VK_CTRL_PGDN     246
#define  VK_CTRL_LEFT     243
#define  VK_CTRL_RIGHT    244

/******************** SHIFTED KEYPAD KEYS and PRTSCR KEY *********************/
#define  VK_SH_HOME       133
#define  VK_SH_UP         134
#define  VK_SH_PGUP       135
#define  VK_SH_MINUS      136
#define  VK_SH_LEFT       137
#define  VK_SH_FIVE       138
#define  VK_SH_RIGHT      139
#define  VK_SH_PLUS       140
#define  VK_SH_END        141
#define  VK_SH_DOWN       142
#define  VK_SH_PGDN       154
#define  VK_SH_INS        155
#define  VK_SH_DEL        156
#define  VK_PRTSCREEN     157
#define  VK_CTRL_PRTSCREEN 242

/**************************** CONTROL KEYS < ' ' *****************************/
#define  VK_CTRL_A        1
#define  VK_CTRL_B        2
#define  VK_CTRL_C        3
#define  VK_CTRL_D        4
#define  VK_CTRL_E        5
#define  VK_CTRL_F        6
#define  VK_CTRL_G        7
#define  VK_CTRL_H        8
#define  VK_CTRL_I        9
#define  VK_CTRL_J        10
#define  VK_CTRL_K        11
#define  VK_CTRL_L        12
#define  VK_CTRL_M        13
#define  VK_CTRL_N        14
#define  VK_CTRL_O        15
#define  VK_CTRL_P        16
#define  VK_CTRL_Q        17
#define  VK_CTRL_R        18
#define  VK_CTRL_S        19
#define  VK_CTRL_T        20
#define  VK_CTRL_U        21
#define  VK_CTRL_V        22
#define  VK_CTRL_W        23
#define  VK_CTRL_X        24
#define  VK_CTRL_Y        25
#define  VK_CTRL_Z        26

/********************************* ALT KEYS *********************************/
#define  VK_ALT_A         158
#define  VK_ALT_B         176
#define  VK_ALT_C         174
#define  VK_ALT_D         160
#define  VK_ALT_E         146
#define  VK_ALT_F         161
#define  VK_ALT_G         162
#define  VK_ALT_H         163
#define  VK_ALT_I         151
#define  VK_ALT_J         164
#define  VK_ALT_K         165
#define  VK_ALT_L         166
#define  VK_ALT_M         178
#define  VK_ALT_N         177
#define  VK_ALT_O         152
#define  VK_ALT_P         153
#define  VK_ALT_Q         144
#define  VK_ALT_R         147
#define  VK_ALT_S         159
#define  VK_ALT_T         148
#define  VK_ALT_U         150
#define  VK_ALT_V         175
#define  VK_ALT_W         145
#define  VK_ALT_X         173
#define  VK_ALT_Y         149
#define  VK_ALT_Z         172

#define  VK_ALT_1         248
#define  VK_ALT_2         249
#define  VK_ALT_3         250
#define  VK_ALT_4         251
#define  VK_ALT_5         252
#define  VK_ALT_6         253
#define  VK_ALT_7         254
#define  VK_ALT_8         255
#define  VK_ALT_9         128
#define  VK_ALT_0         129
#define  VK_ALT_MINUS     130
#define  VK_ALT_PLUS      131

#define  VK_CTRL_BREAK    256
#define  VK_FOREIGN       257
