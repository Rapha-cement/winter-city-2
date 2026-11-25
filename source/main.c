// Winterstad 3D - low-poly open world for the (2)3DS (citro3d). Spidey & Deadpool kerstverhaal.
#include <3ds.h>
#include <citro3d.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shader_shbin.h"

#define DISPLAY_TRANSFER_FLAGS (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
#define CLR(r,g,b) (((u32)(r)<<24)|((u32)(g)<<16)|((u32)(b)<<8)|0xFF)
#define NG 8
#define NBLD (NG*NG)
#define MAXC 40000
#define MAXD 9000

typedef struct { float x, y, z, r, g, b; } V;
typedef struct { float x, z, w, d, h; } Bd;
typedef struct { float x, y, z; int on; } Pk;
typedef struct { float x, z, home, dir; int on; } Sn;

static V *cityV, *dynV, *cur;
static int cityN, dynN, cn;
static Bd B[NBLD + 3];
static int nb;
static Pk M[120];
static int nm;
static Sn S[12];
static unsigned rs = 777u;
static float towerX, towerZ, towerH;
static float px = 31, py = 0, pz = 31.5f, vx, vy, vz, yaw, face, ax, ay, az, rl, bossHp = 8;
static int hero, hp = 5, mugs, kills, state, page, mission, onG, jumps, rope, atk, hurt, dashT, bossCd, tick;
static C3D_Mtx proj;
static int uProj;

static float rnd(float a, float b) { rs = rs * 1664525u + 1013904223u; return a + ((rs >> 8) & 0xFFFF) / 65535.0f * (b - a); }
static void vtx(float x, float y, float z, float r, float g, float b) { V *v = &cur[cn++]; v->x = x; v->y = y; v->z = z; v->r = r; v->g = g; v->b = b; }
static void quad(const float *p, float s, float r, float g, float b) {
  vtx(p[0],p[1],p[2],r*s,g*s,b*s); vtx(p[3],p[4],p[5],r*s,g*s,b*s); vtx(p[6],p[7],p[8],r*s,g*s,b*s);
  vtx(p[0],p[1],p[2],r*s,g*s,b*s); vtx(p[6],p[7],p[8],r*s,g*s,b*s); vtx(p[9],p[10],p[11],r*s,g*s,b*s);
}
// oriented box (rotated around Y)
static void boxr(float cx, float cy, float cz, float sx, float sy, float sz, float ang, float r, float g, float b) {
  float c = cosf(ang), s = sinf(ang), a = sx / 2, h = sy / 2, d = sz / 2, k[8][3];
  for (int i = 0; i < 8; i++) { float lx = (i & 1) ? a : -a, ly = (i & 2) ? h : -h, lz = (i & 4) ? d : -d;
    k[i][0] = cx + lx * c + lz * s; k[i][1] = cy + ly; k[i][2] = cz - lx * s + lz * c; }
  static const int F[5][4] = { {2,3,7,6}, {4,5,7,6}, {0,1,3,2}, {0,4,6,2}, {5,1,3,7} };
  static const float SH[5] = { 1.0f, .8f, .6f, .7f, .7f };
  for (int f = 0; f < 5; f++) { float p[12]; for (int j = 0; j < 4; j++) for (int t = 0; t < 3; t++) p[j * 3 + t] = k[F[f][j]][t]; quad(p, SH[f], r, g, b); }
}
static void box(float cx, float cy, float cz, float sx, float sy, float sz, float r, float g, float b) { boxr(cx, cy, cz, sx, sy, sz, 0, r, g, b); }

static void addBld(float x, float z, float w, float d, float h, float r, float g, float b) {
  B[nb].x = x; B[nb].z = z; B[nb].w = w; B[nb].d = d; B[nb].h = h; nb++;
  box(x, h / 2, z, w, h, d, r, g, b); box(x, h + .15f, z, w + .2f, .3f, d + .2f, 1, 1, 1);
  for (float y = 4; y < h - 2; y += 6) {
    float q1[12] = { x-w/2+1,y,z+d/2+.02f, x+w/2-1,y,z+d/2+.02f, x+w/2-1,y+2,z+d/2+.02f, x-w/2+1,y+2,z+d/2+.02f }; quad(q1, 1, 1, .85f, .42f);
    float q2[12] = { x+w/2+.02f,y,z-d/2+1, x+w/2+.02f,y,z+d/2-1, x+w/2+.02f,y+2,z+d/2-1, x+w/2+.02f,y+2,z-d/2+1 }; quad(q2, 1, 1, .85f, .42f);
  }
}
static void genCity(void) {
  cur = cityV; cn = 0; nb = 0; nm = 0; rs = 777u;
  box(0, -.1f, 0, 700, .2f, 700, .9f, .93f, .98f);
  for (int k = -NG / 2; k < NG / 2; k++) { float w = 13 + 26 * k; box(w, .02f, 0, 7.4f, .06f, 700, .17f, .19f, .24f); box(0, .03f, w, 700, .06f, 7.4f, .17f, .19f, .24f); }
  for (int i = -NG / 2; i < NG / 2; i++) for (int j = -NG / 2; j < NG / 2; j++) {
    if (i >= 0 && i <= 1 && j >= 0 && j <= 1) continue;
    int T = (i == 3 && j == 3);
    float w = T ? 14 : rnd(8, 14), d = T ? 14 : rnd(8, 14), h = T ? 58 : (rnd(0, 1) < .4f ? rnd(4, 9) : rnd(14, 40)), x = i * 26 + rnd(-2, 2), z = j * 26 + rnd(-2, 2);
    float cc = rnd(0, 4); float r = cc < 1 ? .78f : cc < 2 ? .04f : cc < 3 ? .95f : .18f, g = cc < 1 ? .13f : cc < 2 ? .37f : cc < 3 ? .76f : .55f, b = cc < 1 ? .18f : cc < 2 ? .7f : cc < 3 ? 0 : .24f;
    if (T) { r = .13f; g = .15f; b = .2f; towerX = x; towerZ = z; towerH = h; }
    addBld(x, z, w, d, h, r, g, b);
    if (!T && nm < 100) { M[nm].x = x; M[nm].y = h + 1.2f; M[nm].z = z; M[nm++].on = 1; }
  }
  // house behind the gas station
  box(24, 1.65f, 34, 6.5f, 3.3f, .25f, .54f, .42f, .32f); box(24, 1.65f, 29, 6.5f, 3.3f, .25f, .54f, .42f, .32f); box(21, 1.65f, 31.5f, .25f, 3.3f, 5, .54f, .42f, .32f);
  box(27, .5f, 31.5f, .25f, 1, 5, .54f, .42f, .32f); box(27, 2.8f, 31.5f, .25f, 1, 5, .54f, .42f, .32f); box(27, 1.65f, 29.85f, .25f, 1.3f, 1.7f, .54f, .42f, .32f); box(27, 1.65f, 33.15f, .25f, 1.3f, 1.7f, .54f, .42f, .32f);
  box(24, 3.45f, 31.5f, 7.2f, .3f, 5.8f, 1, 1, 1); box(22.3f, .3f, 32.3f, .9f, .5f, 3.3f, .35f, .22f, .13f); box(25.2f, .3f, 32.3f, .9f, .5f, 3.3f, .35f, .22f, .13f);
  box(28, 1.6f, 25, 9, 3.2f, 4, .9f, .9f, .9f); box(28, 3.35f, 25, 9.4f, .3f, 4.4f, .78f, .13f, .18f); box(28, 4.4f, 20.5f, 12, .3f, 5, .78f, .13f, .18f);
  for (int i = 0; i < 4; i++) box(i < 2 ? 22.5f : 33.5f, 2.2f, i % 2 ? 22.7f : 18.3f, .4f, 4.4f, .4f, .9f, .9f, .9f);
  B[nb].x = 24; B[nb].z = 31.5f; B[nb].w = 6.5f; B[nb].d = 5.5f; B[nb].h = 3.6f; nb++;
  B[nb].x = 28; B[nb].z = 25; B[nb].w = 9; B[nb].d = 4; B[nb].h = 3.4f; nb++;
  for (int k = 0; k < 14; k++) { M[nm].x = 13 + 26 * (int)rnd(-4, 3); M[nm].y = 1.2f; M[nm].z = rnd(-90, 90); M[nm++].on = 1; }
  for (int i = 0; i < 12; i++) { S[i].x = 13 + 26 * (int)rnd(-4, 3); S[i].z = rnd(-90, 90); S[i].home = S[i].z; S[i].dir = 1; S[i].on = 1; }
  cityN = cn; GSPGPU_FlushDataCache(cityV, sizeof(V) * cityN);
}

static void drawHero(float x, float y, float z, float ang, int h, float ph) {
  float sw = sinf(ph) * .35f, wr = h ? .77f : .82f, wg = h ? .08f : .12f, wb = h ? .11f : .16f;
  float hr = wr, hg = wg, hb = wb, lr = h ? .09f : .11f, lg = h ? .09f : .31f, lb = h ? .09f : .72f;
  float c = cosf(ang), s = sinf(ang);
#define P(ox,oy,oz,sx,sy,sz,r,g,b) boxr(x+(ox)*c+(oz)*s, y+(oy), z-(ox)*s+(oz)*c, sx, sy, sz, ang, r, g, b)
  P(-.3f, .6f, sw, .6f, 1.2f, .6f, lr, lg, lb); P(.3f, .6f, -sw, .6f, 1.2f, .6f, lr, lg, lb);
  P(0, 1.8f, 0, 1.2f, 1.2f, .6f, h ? .77f : .11f, h ? .08f : .31f, h ? .11f : .72f);
  P(-.9f, 1.8f, -sw, .6f, 1.2f, .6f, hr, hg, hb); P(.9f, 1.8f, sw, .6f, 1.2f, .6f, hr, hg, hb);
  P(0, 2.79f, 0, .8f, .78f, .8f, hr, hg, hb);
  if (h) { P(0, 1.2f, .31f, 1.2f, .2f, .05f, .42f, .29f, .2f); P(-.2f, 2.85f, .41f, .26f, .2f, .05f, .05f, .05f, .05f); P(.2f, 2.85f, .41f, .26f, .2f, .05f, .05f, .05f, .05f);
    P(-.2f, 2.85f, .44f, .14f, .1f, .03f, 1, 1, 1); P(.2f, 2.85f, .44f, .14f, .1f, .03f, 1, 1, 1); P(-.25f, 2.1f, -.35f, .08f, 2.4f, .08f, .08f, .08f, .08f); P(.25f, 2.1f, -.35f, .08f, 2.4f, .08f, .08f, .08f, .08f); }
  else { P(-.2f, 2.85f, .41f, .3f, .2f, .05f, 1, 1, 1); P(.2f, 2.85f, .41f, .3f, .2f, .05f, 1, 1, 1); P(0, 1.8f, .31f, .2f, .5f, .04f, .08f, .05f, .05f); }
#undef P
}

// ---- muziek: WAV (mono 22050 Hz, 16-bit) via RomFS + ndsp
#define ABUF 8192
static FILE *mf; static int track = 0, mon = 1, mok = 0; static ndspWaveBuf wb[2]; static s16 *abuf;
static void musicOpen(int t) {
  char p[96]; if (mf) fclose(mf); snprintf(p, sizeof p, "romfs:/music/track%02d.wav", t + 1);
  mf = fopen(p, "rb"); if (mf) fseek(mf, 44, SEEK_SET);
}
static void musicInit(void) {
  if (R_FAILED(ndspInit())) return;
  ndspSetOutputMode(NDSP_OUTPUT_STEREO); ndspChnSetInterp(0, NDSP_INTERP_LINEAR); ndspChnSetRate(0, 22050); ndspChnSetFormat(0, NDSP_FORMAT_MONO_PCM16);
  float mix[12] = { 0 }; mix[0] = 0.8f; mix[1] = 0.8f; ndspChnSetMix(0, mix);
  abuf = (s16 *)linearAlloc(sizeof(s16) * ABUF * 2); if (!abuf) return;
  memset(wb, 0, sizeof wb); for (int i = 0; i < 2; i++) { wb[i].data_vaddr = abuf + i * ABUF; wb[i].status = NDSP_WBUF_DONE; }
  musicOpen(track); mok = 1;
}
static void musicUpdate(void) {
  if (!mok) return;
  for (int i = 0; i < 2; i++) {
    if (wb[i].status != NDSP_WBUF_DONE) continue;
    s16 *p = abuf + i * ABUF; size_t n = mf ? fread(p, sizeof(s16), ABUF, mf) : 0;
    if (n < ABUF) { track = (track + 1) % 3; musicOpen(track); if (mf && n == 0) n = fread(p, sizeof(s16), ABUF, mf); }
    if (n == 0) continue;
    wb[i].nsamples = n; DSP_FlushDataCache(p, n * sizeof(s16)); ndspChnWaveBufAdd(0, &wb[i]);
  }
}
static void say(const char *m) { printf("\x1b[24;1H%-39.39s", m); }
static void hud(void) {
  const char *o[5] = { "Verzamel 8 chocomelk", "Versla 4 sneeuwmannen (Deadpool: B)", "Klim naar de grote toren (Spidey)", "Versla Koning Frostbite", "Kerstmis is gered!" };
  printf("\x1b[1;1HWinterstad 3D                          \n");
  printf("Levens: %d  Chocomelk: %2d  Sneeuwm.: %d   \nHeld: %-9s                         \n", hp, mugs, kills, hero ? "Deadpool" : "Spidey");
  printf("Missie: %-31s\n", o[state == 2 ? 4 : mission]);
  printf("\nCirkelpad lopen  L/R camera  A spring\nY wissel held  B aanval  X web/dash\nSpidey klimt: tegen muur + vooruit/A\nSELECT = volgend lied  aanraken = pauze\n");
}

static void step(float dt, circlePosition cp, u32 kd, u32 kh, u32 ku) {
  float fx = cp.dy / 156.0f, sx = cp.dx / 156.0f; if (fabsf(fx) < .15f) fx = 0; if (fabsf(sx) < .15f) sx = 0;
  if (kh & KEY_L) yaw += 1.8f * dt; if (kh & KEY_R) yaw -= 1.8f * dt;
  float dx = -sinf(yaw) * fx + cosf(yaw) * sx, dz = -cosf(yaw) * fx - sinf(yaw) * sx, dl = sqrtf(dx * dx + dz * dz);
  if (dl > 1) { dx /= dl; dz /= dl; dl = 1; } if (dl > .1f) face = atan2f(dx, dz);
  if (kd & KEY_Y) { hero ^= 1; rope = 0; say(hero ? "Deadpool: X dash, B zwaardslag" : "Spidey: houd X om te zwaaien"); }
  if (kd & KEY_A) { rope = 0; if (jumps < 2) { vy = 13; jumps++; } }
  if (kd & KEY_B) atk = 18;
  if (kd & KEY_X) {
    if (hero == 0) {
      float bd = 1e9f; int best = -1; float ux = sinf(face), uz = cosf(face);
      for (int i = 0; i < nb; i++) {
        float cx = fminf(fmaxf(px, B[i].x - B[i].w / 2), B[i].x + B[i].w / 2), cz = fminf(fmaxf(pz, B[i].z - B[i].d / 2), B[i].z + B[i].d / 2), ddx = cx - px, ddz = cz - pz, d = sqrtf(ddx * ddx + ddz * ddz);
        if (d < 5 || d > 55 || (ddx * ux + ddz * uz) / d < .3f) continue;
        float yy = fminf(B[i].h - .5f, py + d * .75f + 10); if (yy < py + 6) continue;
        if (d < bd) { bd = d; best = i; ax = cx; ay = yy; az = cz; }
      }
      if (best >= 0) { rope = 1; rl = sqrtf((ax - px) * (ax - px) + (ay - py - 1) * (ay - py - 1) + (az - pz) * (az - pz)); } else say("Geen gebouw dichtbij genoeg");
    } else if (dashT <= 0) { vx = sinf(face) * 22; vz = cosf(face) * 22; if (vy < 7) vy = 7; dashT = 50; }
  }
  if (ku & KEY_X) { if (rope) { vx *= 1.18f; vz *= 1.18f; vy += 5; } rope = 0; }
  if (atk > 0) atk--; if (dashT > 0) dashT--; if (hurt > 0) hurt--; if (bossCd > 0) bossCd--;
  if (rope) { float h0 = sqrtf(vx * vx + vz * vz) + .001f; vx += (dx * 20 + vx / h0 * 16) * dt; vz += (dz * 20 + vz / h0 * 16) * dt; }
  else { float k = onG ? .2f : .06f; vx += (dx * 9 - vx) * k; vz += (dz * 9 - vz) * k; }
  vy -= (rope ? 18 : 26) * dt; float oy = py; px += vx * dt; py += vy * dt; pz += vz * dt;
  if (rope) {
    float ex = px - ax, ey = py + 1 - ay, ez = pz - az, d = sqrtf(ex * ex + ey * ey + ez * ez);
    if (d > rl) { float nx = ex / d, ny = ey / d, nz = ez / d; px = ax + nx * rl; py = ay + ny * rl - 1; pz = az + nz * rl; float vr = vx * nx + vy * ny + vz * nz; if (vr > 0) { vx -= vr * nx; vy -= vr * ny; vz -= vr * nz; }
      float h2 = sqrtf(vx * vx + vz * vz); if (h2 > 42) { vx *= 42 / h2; vz *= 42 / h2; } }
  }
  onG = 0; int wall = 0;
  for (int i = 0; i < nb; i++) {
    float hx = B[i].w / 2 + .4f, hz = B[i].d / 2 + .4f, ex = px - B[i].x, ez = pz - B[i].z;
    if (fabsf(ex) < hx && fabsf(ez) < hz) {
      if (oy >= B[i].h - .02f && py <= B[i].h && vy <= 0) { py = B[i].h; vy = 0; onG = 1; }
      else if (py < B[i].h - .2f) { wall = 1; if (hx - fabsf(ex) < hz - fabsf(ez)) { px = B[i].x + (ex < 0 ? -hx : hx); vx = 0; } else { pz = B[i].z + (ez < 0 ? -hz : hz); vz = 0; } }
    }
  }
  if (py <= 0) { py = 0; if (vy < 0) vy = 0; onG = 1; }
  if (onG) jumps = 0;
  if (wall) { if (hero == 0 && ((kh & KEY_A) || fx > .3f)) { vy = 11; rope = 0; } else if (hero == 1 && (kh & KEY_A)) vy = 4; }
  if (px < -120) px = -120; if (px > 120) px = 120; if (pz < -120) pz = -120; if (pz > 120) pz = 120;
  float rg = hero ? 4.5f : 8, sf = sinf(face), cf = cosf(face);
  for (int i = 0; i < 12; i++) { if (!S[i].on) continue;
    float ex = S[i].x - px, ez = S[i].z - pz, hd = sqrtf(ex * ex + ez * ez);
    if (hd < 24 && py < 8) { if (hd > 1.6f) { S[i].x -= ex / hd * 6 * dt; S[i].z -= ez / hd * 6 * dt; } } else { S[i].z += S[i].dir * 2 * dt; if (fabsf(S[i].z - S[i].home) > 6) S[i].dir = -S[i].dir; }
    if (atk > 8 && hd < rg && ex * sf + ez * cf > 0 && py < 4) { S[i].on = 0; kills++; continue; }
    if (hd < 1.8f && py < 3.4f) { if (vy < -1 && py > 1.8f) { S[i].on = 0; kills++; vy = 11; jumps = 1; } else if (!hurt) { hp--; hurt = 60; vx = -ex / hd * 10; vz = -ez / hd * 10; vy = 6; } }
  }
  for (int i = 0; i < nm; i++) if (M[i].on && fabsf(M[i].x - px) < 1.9f && fabsf(M[i].z - pz) < 1.9f && fabsf(M[i].y - py - 1) < 2.4f) { M[i].on = 0; mugs++; if (mugs % 3 == 0 && hp < 5) hp++; }
  if (mission == 0 && mugs >= 8) { mission = 1; say("Deadpool: hak 4 sneeuwmannen om!"); }
  else if (mission == 1 && kills >= 4) { mission = 2; say("Spidey: klim naar de grote toren!"); }
  else if (mission == 2 && fabsf(px - towerX) < 8 && fabsf(pz - towerZ) < 8 && py > towerH - 2) { mission = 3; say("Frostbite: jullie komen te laat!"); }
  else if (mission == 3) {
    float ex = towerX - px, ez = towerZ - pz, hd = sqrtf(ex * ex + ez * ez);
    if (atk > 8 && hd < rg + 3.4f && bossCd == 0 && py > towerH - 3) { bossHp--; bossCd = 20; }
    if (hd < 4.6f && py > towerH - 3 && !hurt) { hp -= 2; hurt = 60; vx = -ex / hd * 16; vz = -ez / hd * 16; vy = 8; }
    if (bossHp <= 0) state = 2;
  }
  if (hp <= 0) state = 3;
}

int main(int argc, char **argv) {
  gfxInitDefault(); consoleInit(GFX_BOTTOM, NULL); romfsInit(); C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
  C3D_RenderTarget *top = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
  C3D_RenderTargetSetOutput(top, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
  DVLB_s *dvlb = DVLB_ParseFile((u32 *)shader_shbin, shader_shbin_size); shaderProgram_s prog; shaderProgramInit(&prog); shaderProgramSetVsh(&prog, &dvlb->DVLE[0]); C3D_BindProgram(&prog);
  uProj = shaderInstanceGetUniformLocation(prog.vertexShader, "projection");
  C3D_AttrInfo *ai = C3D_GetAttrInfo(); AttrInfo_Init(ai); AttrInfo_AddLoader(ai, 0, GPU_FLOAT, 3); AttrInfo_AddLoader(ai, 1, GPU_FLOAT, 3);
  C3D_TexEnv *env = C3D_GetTexEnv(0); C3D_TexEnvInit(env); C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0); C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
  C3D_CullFace(GPU_CULL_NONE);
  Mtx_PerspTilt(&proj, C3D_AngleFromDegrees(60.0f), C3D_AspectRatioTop, 0.3f, 300.0f, false);
  cityV = (V *)linearAlloc(sizeof(V) * MAXC); dynV = (V *)linearAlloc(sizeof(V) * MAXD); genCity(); musicInit();
  static const char *P[4] = { "Een klein huisje achter het tankstation.\nSpidey en Deadpool worden wakker en\nklimmen door het raam naar buiten...", "Alles is stil. De kerstbomen zijn weg,\nde chocomelk is gestolen en de grootste\ntoren is donker.", "Koning Frostbite heeft het gedaan!\nVerzamel chocomelk, versla zijn sneeuwmannen\nen klim naar de toren.", "Cirkelpad = lopen, A = springen\nY = wissel held, B = aanval\nX = web (Spidey) of dash (Deadpool)\nSpidey klimt tegen een muur." };
  printf("\x1b[2J");
  while (aptMainLoop()) {
    hidScanInput(); u32 kd = hidKeysDown(), kh = hidKeysHeld(), ku = hidKeysUp(); circlePosition cp; hidCircleRead(&cp);
    if (kd & KEY_START) break; tick++; musicUpdate();
    if (kd & KEY_SELECT) { track = (track + 1) % 3; musicOpen(track); }
    if (kd & KEY_TOUCH) { mon ^= 1; if (mok) ndspChnSetPaused(0, !mon); }
    if (state == 0) { printf("\x1b[1;1H%s\n\nDruk A (%d/4)                  ", P[page], page + 1); if (kd & KEY_A) { page++; printf("\x1b[2J"); if (page >= 4) state = 1; } yaw += .008f; }
    else if (state == 1) { step(1.0f / 60, cp, kd, kh, ku); if (tick % 6 == 0) hud(); }
    else if (state == 2) { printf("\x1b[1;1HKerstmis is gered!\nDe kerstlichtjes branden weer.\nBeste kerstavond ooit!"); }
    else if (kd & KEY_A) { hp = 5; px = 31; py = 0; pz = 31.5f; vx = vy = vz = 0; hurt = 90; state = 1; printf("\x1b[2J"); }
    // camera
    float tx = state == 0 ? 28 : px, ty = state == 0 ? 1.5f : py + 1.6f, tz = state == 0 ? 31 : pz, dist = state == 0 ? 14 : (rope ? 15 : 9), pitch = .35f;
    C3D_FVec eye = FVec3_New(tx + sinf(yaw) * cosf(pitch) * dist, fmaxf(.8f, ty + sinf(pitch) * dist), tz + cosf(yaw) * cosf(pitch) * dist), tg = FVec3_New(tx, ty, tz), up = FVec3_New(0, 1, 0);
    C3D_Mtx view, mvp; Mtx_LookAt(&view, eye, tg, up, false); Mtx_Multiply(&mvp, &proj, &view);
    // dynamic geometry
    cur = dynV; cn = 0; float ph = tick * .25f * (sqrtf(vx * vx + vz * vz) > 1 ? 1 : 0);
    if (!(hurt > 0 && (hurt / 4) % 2)) drawHero(px, py, pz, face, hero, ph);
    if (state == 0) { drawHero(30, 0, 30, 1.5f, 0, 0); drawHero(31.5f, 0, 32, 1.5f, 1, 0); }
    for (int i = 0; i < nm; i++) if (M[i].on) { float yb = M[i].y + sinf(tick * .08f + i) * .25f; box(M[i].x, yb, M[i].z, .6f, .7f, .6f, .42f, .23f, .12f); box(M[i].x, yb + .4f, M[i].z, .64f, .14f, .64f, 1, 1, 1); }
    for (int i = 0; i < 12; i++) if (S[i].on) { box(S[i].x, 1, S[i].z, 1.8f, 2, 1.8f, 1, 1, 1); box(S[i].x, 2.4f, S[i].z, 1.2f, 1.2f, 1.2f, 1, 1, 1); box(S[i].x, 3.1f, S[i].z, 1.1f, .5f, 1.1f, .07f, .07f, .07f); }
    if (mission == 3 && bossHp > 0) { box(towerX, towerH + 3.5f, towerZ, 7, 7, 7, 1, 1, 1); box(towerX, towerH + 9, towerZ, 4.5f, 4.5f, 4.5f, 1, 1, 1); box(towerX, towerH + 12, towerZ, 3.4f, 1.2f, 3.4f, 1, .82f, .25f); }
    if (state == 1 && mission == 2) box(towerX, towerH + 40, towerZ, 2, 80, 2, 1, .82f, .25f);
    dynN = cn; GSPGPU_FlushDataCache(dynV, sizeof(V) * (dynN > 0 ? dynN : 1));
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C3D_RenderTargetClear(top, C3D_CLEAR_ALL, CLR(60, 90, 160), 0); C3D_FrameDrawOn(top);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uProj, &mvp);
    C3D_BufInfo *bi = C3D_GetBufInfo(); BufInfo_Init(bi); BufInfo_Add(bi, cityV, sizeof(V), 2, 0x10); C3D_DrawArrays(GPU_TRIANGLES, 0, cityN);
    if (dynN > 0) { bi = C3D_GetBufInfo(); BufInfo_Init(bi); BufInfo_Add(bi, dynV, sizeof(V), 2, 0x10); C3D_DrawArrays(GPU_TRIANGLES, 0, dynN); }
    C3D_FrameEnd(0);
  }
  if (mok) { ndspChnWaveBufClear(0); ndspExit(); } if (mf) fclose(mf); linearFree(cityV); linearFree(dynV); shaderProgramFree(&prog); DVLB_Free(dvlb); C3D_Fini(); romfsExit(); gfxExit();
  return 0;
}
