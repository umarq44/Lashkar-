/* LASHKAR OF ASHES - Chapter 1 demo for Game Boy Advance
   Plain C, no libraries. Mode 4 (240x160, 256 colours), double buffered.
   Build with devkitARM:  make   ->  lashkar.gba                              */
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32;
typedef signed char s8; typedef signed short s16; typedef signed int s32;
#define W 240
#define SH 160
#define EWRAM __attribute__((section(".sbss"),aligned(4)))
#ifdef HOSTTEST
#include <string.h>
extern u16 hpal[256],hvram[0xA000],hkeys,hdisp; int hook(int);
#define PALRAM hpal
#define VRAMP(p) (hvram+(p)*0x5000)
#define SETDISP(v) (hdisp=(v))
#define KEYS() (hkeys)
#define VSYNC() ((void)0)
static void dma(void*d,const void*s,int n){memcpy(d,s,n*2);}
#define main game_main
#undef EWRAM
#define EWRAM
#else
#define REG16(a) (*(volatile u16*)(a))
#define REG32(a) (*(volatile u32*)(a))
#define PALRAM ((volatile u16*)0x05000000)
#define VRAMP(p) ((volatile u16*)(0x06000000+(p)*0xA000))
#define SETDISP(v) (REG16(0x04000000)=(v))
#define KEYS() ((~REG16(0x04000130))&0x3FF)
#define VSYNC() {while(REG16(0x04000006)>=160);while(REG16(0x04000006)<160);}
static void dma(void*d,const void*s,int n){REG32(0x040000D4)=(u32)s;REG32(0x040000D8)=(u32)d;REG32(0x040000DC)=(u32)n|0x80000000u;}
#endif
#define KA 1
#define KB 2
#define KSTART 8
#define KRIGHT 0x10
#define KLEFT 0x20
#define KUP 0x40
#define KDOWN 0x80
#define ABS(a) ((a)<0?-(a):(a))
enum{SOL,THI,ARC,CLE,WIZ};
enum{INTRO,BATTLE,WIN,LOSE};
enum{IDLE,MOVE,ACT};
#define N 7
#define NU 8
#define OX 120
#define OY 26
#define TW 14
#define TH 7
#define SD 5

static u8 buf[W*SH] EWRAM;
static u8 ter[W*SH] EWRAM;
static u8 til[3][2][4][392] EWRAM;
static u8 spr[7][324];
static u32 pal[256]; static int npal=1;
static u8 col(u32 c){for(int i=1;i<npal;i++)if(pal[i]==c)return i;if(npal>=256)return 255;pal[npal]=c;return npal++;}
static u8 cBg,cStrip,cText,cWhite,cYel,cShadow,cBarBg,cBarDk,cGrn,cRed,cAsh,cEmb,cPopR,cPopG,cGold,cLose;
static u8 cSL[3],cSR[3],cSLd[3],cSRd[3];

static const u8 H[N][N]={{0,0,0,0,1,1,1},{0,0,0,1,1,1,1},{0,0,0,0,1,1,1},{0,0,0,1,2,0,0},{0,0,0,0,1,0,0},{0,0,0,0,0,0,0},{0,0,0,0,0,0,0}};

/* ---------- drawing helpers ---------- */
static void rect(u8*b,int x,int y,int w,int h,u8 c){for(int j=0;j<h;j++){int yy=y+j;if((unsigned)yy>=SH)continue;for(int i=0;i<w;i++){int xx=x+i;if((unsigned)xx<W)b[yy*W+xx]=c;}}}
static void blit(u8*d,const u8*s,int w,int h,int dx,int dy,int flip,int white,int sc){
 for(int j=0;j<h;j++)for(int i=0;i<w;i++){u8 c=s[j*w+(flip?w-1-i:i)];if(!c)continue;if(white)c=cWhite;rect(d,dx+i*sc,dy+j*sc,sc,sc,c);}}
static const u16 FONT[45]={0,0x2BED,0x6BAE,0x3923,0x6B6E,0x79A7,0x79A4,0x396B,0x5BED,0x7497,0x126A,0x5BAD,0x4927,0x5FED,0x6B6D,0x2B6A,0x6BA4,0x2B73,0x6BAD,0x388E,0x7492,0x5B6F,0x5B6A,0x5BFD,0x5AAD,0x5A92,0x72A7,
 0x7B6F,0x2C97,0x62A7,0x638E,0x5BC9,0x798E,0x39EF,0x7292,0x7BEF,0x7BCE,0x0002,0x0410,0x01C0,0x05D0,0x12A4,0x2482,0x0014,0x6282};
static int gi(char c){if(c>='a'&&c<='z')c-=32;if(c>='A'&&c<='Z')return c-'A'+1;if(c>='0'&&c<='9')return c-'0'+27;
 switch(c){case '.':return 37;case ':':return 38;case '-':return 39;case '+':return 40;case '/':return 41;case '!':return 42;case ',':return 43;case '?':return 44;}return 0;}
static int slen(const char*s){int n=0;while(s[n])n++;return n;}
static void text(int x,int y,const char*s,u8 c,int sc){for(;*s;s++,x+=4*sc){u16 g=FONT[gi(*s)];for(int r=0;r<5;r++)for(int k=0;k<3;k++)if((g>>(14-r*3-k))&1)rect(buf,x+k*sc,y+r*sc,sc,sc,c);}}
static void ctext(int y,const char*s,u8 c,int sc){text(120-slen(s)*2*sc,y,s,c,sc);}
static void cpy(char*d,const char*s){while((*d++=*s++));}
static void cat(char*d,const char*s){while(*d)d++;cpy(d,s);}
static void numcat(char*d,int n){char t[6];int k=0;if(n<=0)t[k++]='0';else while(n&&k<5){t[k++]='0'+n%10;n/=10;}while(*d)d++;while(k)*d++=t[--k];*d=0;}

/* ---------- art generation ---------- */
static const char*Bs[16]={"....oooo....","...ohhhho...","..ohhhhhho..","..ohsssseo..","..ohssssso..","....osso....","..oaaaaaao..",".oaaaaaaaao.",".ocaabbaaco.",".ocaaaaaaco.",".osaaaaaaso.","..obbbbbbo..","..obb..bbo..","..obb..bbo..","..okk..kko..","..ooo..ooo.."};
static void mkSpr(int id,int cls,u32 ph,u32 pa,u32 pb,u32 pc){
 u8*s=spr[id];for(int i=0;i<324;i++)s[i]=0;
 u8 co=col(0x1a0f0a),ch=col(ph),cs=col(0xe8b88a),ce=col(0x201010),ca=col(pa),cb=col(pb),cc=col(pc),ck=col(0x2a1a12);
 for(int j=0;j<16;j++)for(int i=0;i<12;i++){u8 c=0;switch(Bs[j][i]){case 'o':c=co;break;case 'h':c=ch;break;case 's':c=cs;break;case 'e':c=ce;break;case 'a':c=ca;break;case 'b':c=cb;break;case 'c':c=cc;break;case 'k':c=ck;break;}if(c)s[(j+2)*18+i+3]=c;}
#define RC(c,x,y,w,h) for(int _j=0;_j<(h);_j++)for(int _i=0;_i<(w);_i++)s[((y)+_j)*18+(x)+_i]=(c)
 u8 steel=col(0xd8dce0),gold=col(0xc9a227),wood=col(0x5a3a1a),bow=col(0x8a5a2a);
 if(cls==SOL){RC(steel,15,5,1,8);RC(gold,14,12,3,1);RC(wood,15,13,1,2);RC(co,0,9,3,5);RC(cc,1,10,1,3);}
 if(cls==THI){RC(steel,15,9,1,4);RC(ck,14,13,3,1);RC(cc,7,7,4,1);}
 if(cls==ARC){RC(bow,16,4,1,10);RC(bow,15,3,1,1);RC(bow,15,14,1,1);RC(col(0xe0e0e0),15,4,1,10);RC(col(0x6a3a1a),2,6,2,6);RC(col(0xdddddd),2,5,2,1);}
 if(cls==CLE||cls==WIZ){RC(ca,5,14,9,3);RC(cb,5,17,9,1);}
 if(cls==CLE){RC(gold,16,4,1,14);RC(col(0xfff6b0),15,1,3,3);}
 if(cls==WIZ){RC(ch,8,0,2,1);RC(ch,7,1,4,1);RC(wood,16,5,1,13);RC(cc,15,2,3,3);}
 if(id==0){RC(col(0xaa0000),7,10,3,1);}
}
static u32 tc(u32 c,int t){static const u32 K[4]={0,0x3a7bd5,0xe04040,0xffffff};if(!t)return c;u32 k=K[t];
 u32 r=(((c>>16)&255)+((k>>16)&255))/2,g=(((c>>8)&255)+((k>>8)&255))/2,b=((c&255)+(k&255))/2;return (r<<16)|(g<<8)|b;}
static int ins(int x,int y){int a=2*x+1-28,b=2*y+1-14;if(a<0)a=-a;if(b<0)b=-b;return a+2*b<=28;}
static void mkTile(u8*o,u32 b,u32 d,u32 l,u32 seed,int t){
 u8 cb=col(tc(b,t)),cd=col(tc(d,t)),cl=col(tc(l,t));
 for(int y=0;y<14;y++)for(int x=0;x<28;x++)o[y*28+x]=ins(x,y)?cb:0;
 u32 r=seed;
 for(int i=0;i<44;i++){r=r*1664525u+1013904223u;int x=(r>>8)%28,y=(r>>16)%14;if(ins(x,y))o[y*28+x]=((r>>24)&1)?cd:cl;if(((r>>25)&7)<2&&x<27&&ins(x+1,y))o[y*28+x+1]=o[y*28+x];}
 for(int i=0;i<14;i++){o[(6-(i>>1))*28+i]=cl;o[(7+(i>>1))*28+i]=cd;o[(7+(i>>1))*28+27-i]=cd;}
}
static void mkTerrain(void){
 rect(ter,0,0,W,SH,cBg);
 for(int s=0;s<=2*N-2;s++)for(int x=0;x<N;x++){int y=s-x;if(y<0||y>=N)continue;
  int h=H[y][x],sx=OX+(x-y)*TW,sy=OY+(x+y)*TH-h*SD,sh=SD*(h+1);
  for(int i=0;i<TW;i++){int yt=sy+TH+(i>>1),yr=sy+2*TH-(i>>1);
   rect(ter,sx-TW+i,yt,1,sh,cSL[h]);rect(ter,sx+i,yr,1,sh,cSR[h]);
   for(int k=1;k<=h;k++){rect(ter,sx-TW+i,yt+SD*k,1,1,cSLd[h]);rect(ter,sx+i,yr+SD*k,1,1,cSRd[h]);}}
  blit(ter,til[h][(x+y)&1][0],28,14,sx-TW,sy,0,0,1);}
}
static void setup(void){
 cBg=col(0x1a0f12);cStrip=col(0x0b0607);cText=col(0xead9b5);cWhite=col(0xffffff);cYel=col(0xffd040);cShadow=col(0x1a1408);cBarBg=col(0x1a0808);cBarDk=col(0x401818);
 cGrn=col(0x58d058);cRed=col(0xd85050);cAsh=col(0xc8bfb4);cEmb=col(0xff8232);cPopR=col(0xff9a9a);cPopG=col(0x77ff77);cGold=col(0xe0c060);cLose=col(0xd05050);
 mkSpr(0,SOL,0x2a1a12,0xb8902a,0x7a5a1a,0x8a2a1a);mkSpr(1,THI,0x4a3020,0x2f8a8a,0x1f5a5a,0xc09040);
 mkSpr(2,ARC,0x6a4a20,0x4a8a3a,0x2f5a24,0x8a6a30);mkSpr(3,CLE,0xf0e8d0,0xece6dc,0xb8b0a0,0xc9a227);
 mkSpr(4,SOL,0x505058,0x8a2a2a,0x5a1a1a,0x2a2a2a);mkSpr(5,WIZ,0x3a1a4a,0x6a2a8a,0x3a1250,0xc040c0);
 mkSpr(6,ARC,0x505058,0x8a2a2a,0x5a1a1a,0x2a2a2a);
 static const u32 TB[3]={0x5d6b3b,0x7a5a3a,0x8a8478},TD[3]={0x4b5830,0x634a2e,0x6c675d},TLt[3]={0x74854a,0x8f6c46,0xa39d90};
 static const u32 SLc[3]={0x4d3826,0x684830,0x6a665c},SRc[3]={0x3a2a1c,0x4e3622,0x504c44};
 for(int h=0;h<3;h++){for(int a=0;a<2;a++)for(int t=0;t<4;t++)mkTile(til[h][a][t],TB[h],TD[h],TLt[h],11+h+a*20,t);
  cSL[h]=col(SLc[h]);cSR[h]=col(SRc[h]);cSLd[h]=col((SLc[h]>>1)&0x7f7f7f);cSRd[h]=col((SRc[h]>>1)&0x7f7f7f);}
 mkTerrain();
 for(int i=0;i<256;i++){u32 c=pal[i];PALRAM[i]=((c>>19)&31)|(((c>>11)&31)<<5)|(((c>>3)&31)<<10);}
}

/* ---------- game state ---------- */
typedef struct{const char*n;u8 cls,team,sp,done,moved,atk,mov,rng,fl,lt,dy,sd,mv;s8 x,y,face;s16 hp,mhp,ldx,ldy;s32 ax,ay,ah;}Unit;
static const struct{const char*n;u8 cls,team;s8 x,y;s16 hp;u8 atk,mov,rng,sp;}UD[NU]={
{"DASTAN",SOL,0,1,5,32,9,3,1,0},{"NOMAD",THI,0,2,6,24,8,4,1,1},{"RAFIQ",ARC,0,0,4,20,7,3,3,2},{"ZARA",CLE,0,0,6,22,4,3,2,3},
{"ASHGUARD",SOL,1,5,1,26,8,3,1,4},{"ASHGUARD",SOL,1,6,2,26,8,3,1,4},{"CULTIST",WIZ,1,4,0,18,9,3,2,5},{"ASHGUARD",ARC,1,6,0,20,7,3,3,6}};
static const char*CN[5]={"SOLDIER","THIEF","ARCHER","CLERIC","WIZARD"};
static const char*LOOT[6]={"IRON SWORD","HUNTER BOW","ASH CHARM","POTION","SILK ROBE","STEEL DAGGER"};
static Unit U[NU];
static u8 rch[N][N],dd[N][N],tg[NU];
static int scene=INTRO,pg=0,mode=IDLE,sel=-1,cx=1,cy=5,enemyTurn=0,ei=0,estage=0,et=0,frame=0,back=1;
static u32 keys,prev,kp,rs=12345;
static char m1[64],m2[64];
static struct{int x,y,t;char s[8];u8 c;}pops[8];
static int ashx[28],ashy[28];
static int rnd(int n){rs=rs*1664525u+1013904223u;return (rs>>16)%n;}
static void setm(const char*a,const char*b){cpy(m1,a);cpy(m2,b);}
static int at(int x,int y){for(int i=0;i<NU;i++)if(U[i].hp>0&&U[i].x==x&&U[i].y==y)return i;return -1;}
static int dist(Unit*a,Unit*b){return ABS(a->x-b->x)+ABS(a->y-b->y);}
static int moving(void){for(int i=0;i<NU;i++)if(U[i].mv)return 1;return 0;}
static void scr(Unit*u,int*x,int*y){*x=OX+(((u->ax-u->ay)*TW)>>8);*y=OY+(((u->ax+u->ay)*TH)>>8)+TH-((u->ah*SD)>>8);}
static void bfs(Unit*u){
 for(int y=0;y<N;y++)for(int x=0;x<N;x++){rch[y][x]=0;dd[y][x]=0;}
 static s8 q[N*N][2];int h=0,t=0;q[t][0]=u->x;q[t][1]=u->y;t++;rch[u->y][u->x]=1;
 while(h<t){int x=q[h][0],y=q[h][1];h++;if(dd[y][x]>=u->mov)continue;
  static const s8 D[4][2]={{1,0},{-1,0},{0,1},{0,-1}};
  for(int k=0;k<4;k++){int a=x+D[k][0],b=y+D[k][1];if(a<0||b<0||a>=N||b>=N||rch[b][a]||at(a,b)>=0)continue;
   int dh=H[b][a]-H[y][x];if(dh>1||dh<-1)continue;rch[b][a]=1;dd[b][a]=dd[y][x]+1;q[t][0]=a;q[t][1]=b;t++;}}
}
static int canT(Unit*u,Unit*v){if(u==v||v->hp<=0||dist(u,v)>u->rng)return 0;return u->cls==CLE?(v->team==u->team&&v->hp<v->mhp):(v->team!=u->team);}
static void clr(void){for(int y=0;y<N;y++)for(int x=0;x<N;x++)rch[y][x]=0;for(int i=0;i<NU;i++)tg[i]=0;}
static void addpop(int x,int y,const char*s,int n,u8 c){for(int i=0;i<8;i++)if(pops[i].t>=40){pops[i].x=x;pops[i].y=y;pops[i].t=0;pops[i].c=c;cpy(pops[i].s,s);numcat(pops[i].s,n);return;}}
static void checkEnd(void){int e=0;for(int i=0;i<NU;i++)if(U[i].team==1&&U[i].hp>0)e=1;if(!e){scene=WIN;return;}if(U[0].hp<=0)scene=LOSE;}
static void act(int ui,int vi){
 Unit*u=&U[ui],*v=&U[vi];
 int sx=((v->x-v->y)-(u->x-u->y))*TW,sy=((v->x+v->y)-(u->x+u->y))*TH,ax=ABS(sx),ay=ABS(sy),l=ax>ay?ax+ay/2:ay+ax/2;if(!l)l=1;
 u->ldx=sx*256/l;u->ldy=sy*256/l;u->lt=10;if(sx)u->face=sx>0?1:-1;
 int px,py;scr(v,&px,&py);m1[0]=0;m2[0]=0;
 if(u->cls==CLE){int h=10;if(v->mhp-v->hp<h)h=v->mhp-v->hp;v->hp+=h;cpy(m1,u->n);cat(m1," HEALS ");cat(m1,v->n);cat(m1," +");numcat(m1,h);addpop(px,py-26,"+",h,cPopG);}
 else{int d=u->atk+rnd(4)+(H[u->y][u->x]>H[v->y][v->x]?2:0);v->hp-=d;v->fl=10;
  cpy(m1,u->n);cat(m1," HITS ");cat(m1,v->n);cat(m1," FOR ");numcat(m1,d);addpop(px,py-26,"-",d,cPopR);
  if(v->hp<=0){v->hp=0;v->dy=24;cpy(m2,v->n);if(v->team==1){cat(m2," FALLS, DROPS ");cat(m2,LOOT[rnd(6)]);}else cat(m2," IS LOST FOREVER");}}
 u->done=1;checkEnd();
}
static void infoMsg(int i){cpy(m1,U[i].n);cat(m1," THE ");cat(m1,CN[U[i].cls]);cat(m1,"  HP ");numcat(m1,U[i].hp);cat(m1,"/");numcat(m1,U[i].mhp);
 cpy(m2,U[i].team==0?(U[i].done?"ALREADY ACTED":"A: SELECT"):"ENEMY");}
static void initBattle(void){
 for(int i=0;i<NU;i++){Unit*u=&U[i];u->n=UD[i].n;u->cls=UD[i].cls;u->team=UD[i].team;u->sp=UD[i].sp;u->x=UD[i].x;u->y=UD[i].y;u->hp=u->mhp=UD[i].hp;u->atk=UD[i].atk;u->mov=UD[i].mov;u->rng=UD[i].rng;
  u->done=u->moved=0;u->ax=u->x<<8;u->ay=u->y<<8;u->ah=H[u->y][u->x]<<8;u->face=u->team?-1:1;u->fl=u->lt=0;u->dy=24;u->sd=i&3;u->mv=0;u->ldx=u->ldy=0;}
 for(int i=0;i<8;i++)pops[i].t=40;
 sel=-1;mode=IDLE;enemyTurn=0;cx=1;cy=5;clr();rs+=frame;setm("CHAPTER 1: EMBERS","DEFEAT THE ASH CULT. START ENDS TURN.");
}
static void toAct(void){mode=ACT;for(int y=0;y<N;y++)for(int x=0;x<N;x++)rch[y][x]=0;int n=0;for(int i=0;i<NU;i++){tg[i]=canT(&U[sel],&U[i]);n+=tg[i];}
 if(n)setm("PICK A TARGET WITH A","B: WAIT");else setm("NO TARGETS IN RANGE","B: WAIT");}
static void pick(int i){sel=i;if(U[i].moved)toAct();else{mode=MOVE;bfs(&U[i]);for(int k=0;k<NU;k++)tg[k]=0;infoMsg(i);cpy(m2,"A: MOVE HERE  B: CANCEL");}}
static void deselect(void){sel=-1;mode=IDLE;clr();}
static void pressA(void){
 int ui=at(cx,cy);
 if(mode==ACT&&sel>=0){if(ui>=0&&tg[ui]){int s=sel;deselect();act(s,ui);return;}if(ui==sel){U[sel].done=1;deselect();setm("WAITING.","");}return;}
 if(mode==MOVE&&sel>=0&&rch[cy][cx]&&(ui<0||ui==sel)){int s=(cx-cy)-(U[sel].x-U[sel].y);if(s)U[sel].face=s>0?1:-1;U[sel].x=cx;U[sel].y=cy;U[sel].moved=1;toAct();return;}
 if(ui>=0&&U[ui].team==0&&!U[ui].done){pick(ui);return;}
 if(ui>=0)infoMsg(ui);
}
static void pressB(void){
 if(mode==MOVE){deselect();setm("CANCELLED.","");}
 else if(mode==ACT&&sel>=0){U[sel].done=1;deselect();setm("WAITING.","");}
}
static void startEnemy(void){if(scene!=BATTLE||enemyTurn)return;deselect();enemyTurn=1;ei=0;estage=0;et=20;setm("ENEMY TURN...","");}
static void enemyStep(void){
 if(moving())return;if(et>0){et--;return;}
 if(scene!=BATTLE){enemyTurn=0;return;}
 if(estage==0){
  while(ei<NU&&!(U[ei].team==1&&U[ei].hp>0))ei++;
  if(ei>=NU){for(int i=0;i<NU;i++)U[i].done=U[i].moved=0;enemyTurn=0;setm("YOUR TURN.","");return;}
  Unit*e=&U[ei];int bp=-1,bd=99;
  for(int j=0;j<NU;j++)if(U[j].team==0&&U[j].hp>0){int d=dist(e,&U[j]);if(d<bd){bd=d;bp=j;}}
  if(bp<0){enemyTurn=0;return;}
  bfs(e);int bx=e->x,by=e->y,bs=99;
  for(int y=0;y<N;y++)for(int x=0;x<N;x++)if(rch[y][x]){int s=ABS(ABS(x-U[bp].x)+ABS(y-U[bp].y)-e->rng);if(s<bs){bs=s;bx=x;by=y;}}
  int sd=(bx-by)-(e->x-e->y);if(sd)e->face=sd>0?1:-1;
  e->x=bx;e->y=by;clr();estage=1;et=8;
 }else{
  Unit*e=&U[ei];int bt=-1;
  for(int j=0;j<NU;j++)if(canT(e,&U[j])&&(bt<0||U[j].hp<U[bt].hp))bt=j;
  if(bt>=0){act(ei,bt);et=45;}else et=8;
  estage=0;ei++;
 }
}
static void upd(void){
 for(int i=0;i<NU;i++){Unit*u=&U[i];int tx=u->x<<8,ty=u->y<<8,dx=tx-u->ax,dy=ty-u->ay,ax=ABS(dx),ay=ABS(dy),d=ax>ay?ax+ay/2:ay+ax/2;
  if(d>8){int s=d<28?d:28;u->ax+=dx*s/d;u->ay+=dy*s/d;u->mv=1;int sd=dx-dy;if(ABS(sd)>3)u->face=sd>0?1:-1;}else{u->ax=tx;u->ay=ty;u->mv=0;}
  u->ah+=((H[u->y][u->x]<<8)-u->ah)/5;if(u->fl>0)u->fl--;if(u->lt>0)u->lt--;if(u->hp<=0&&u->dy>0)u->dy--;}
 for(int i=0;i<8;i++)if(pops[i].t<40)pops[i].t++;
 if(!(frame%3))for(int i=0;i<28;i++){ashy[i]++;if(ashy[i]>SH){ashy[i]=-2;ashx[i]=rnd(W);}}
}

/* ---------- rendering ---------- */
static void tpos(int x,int y,int*sx,int*sy){*sx=OX+(x-y)*TW;*sy=OY+(x+y)*TH-H[y][x]*SD;}
static void tint(int x,int y,int t){int sx,sy;tpos(x,y,&sx,&sy);blit(buf,til[H[y][x]][(x+y)&1][t],28,14,sx-TW,sy,0,0,1);}
static void outline(int x,int y,u8 c){int sx,sy;tpos(x,y,&sx,&sy);sx-=TW;for(int i=0;i<14;i++){rect(buf,sx+i,sy+6-(i>>1),1,1,c);rect(buf,sx+27-i,sy+6-(i>>1),1,1,c);rect(buf,sx+i,sy+7+(i>>1),1,1,c);rect(buf,sx+27-i,sy+7+(i>>1),1,1,c);}}
static const s8 SINT[10]={0,31,59,81,95,100,95,81,59,31};
static void drawU(int i){
 Unit*u=&U[i];int X,Y;scr(u,&X,&Y);
 if(u->hp<=0&&!(u->dy&2))return;
 int k=u->lt>0?SINT[10-u->lt]:0;X+=u->ldx*k*4/25600;Y+=u->ldy*k*4/25600;
 int f=frame&7,bob=u->mv?-((f<4?f:8-f)*3/4):((((frame>>5)+u->sd)&1)?-1:0);
 for(int dy=-2;dy<=2;dy++){int w=ABS(dy)==2?3:6;for(int dx=-w;dx<w;dx++)if(!((X+dx+Y+dy)&1))rect(buf,X+dx,Y+dy,1,1,cShadow);}
 blit(buf,spr[u->sp],18,18,X-9,Y-18+bob,u->face<0,(u->fl>0&&(u->fl&2))||u->hp<=0,1);
 if(u->hp>0){rect(buf,X-7,Y-25,14,4,cBarBg);rect(buf,X-6,Y-24,12,2,cBarDk);rect(buf,X-6,Y-24,12*u->hp/u->mhp,2,u->team?cRed:cGrn);}
 if(i==sel){int b=((frame>>3)&1);rect(buf,X-2,Y-31+b,5,1,cYel);rect(buf,X-1,Y-30+b,3,1,cYel);rect(buf,X,Y-29+b,1,1,cYel);}
}
static void embers(void){for(int i=0;i<28;i++)rect(buf,ashx[i],ashy[i],1,1,(i%5==0)?cEmb:cAsh);}
static const char*L[3][4]={{"THE VILLAGE OF SARHAD BURNS.","RIDERS IN BLACK TOOK A SCROLL","OF FORBIDDEN WORDS, AND EVERY","SOUL BENEATH ITS ROOFS."},
 {"DASTAN ALONE STILL BREATHES,","A SCAR ACROSS HIS CHEST.","IN THE FOG A VOICE WHISPERS:","ASH MUST RISE FROM YOU."},{"LASHKAR OF ASHES","CHAPTER 1: EMBERS","","PRESS A TO BEGIN"}};
static void render(void){
 if(scene==INTRO){
  rect(buf,0,0,W,SH,cBg);
  if(pg==2)blit(buf,spr[0],18,18,102,6,0,0,2);
  for(int i=0;i<4;i++)ctext((pg==2?52:40)+i*14,L[pg][i],(pg==2&&i==0)?cGold:cText,2);
  embers();return;}
 dma(buf,ter,W*SH/2);
 if(mode==MOVE)for(int y=0;y<N;y++)for(int x=0;x<N;x++)if(rch[y][x])tint(x,y,1);
 if(mode==ACT)for(int i=0;i<NU;i++)if(tg[i])tint(U[i].x,U[i].y,2);
 if(sel>=0)tint(U[sel].x,U[sel].y,3);
 if(!enemyTurn)outline(cx,cy,cYel);
 int o[NU];for(int i=0;i<NU;i++)o[i]=i;
 for(int i=1;i<NU;i++){int v=o[i],j=i-1;while(j>=0&&U[o[j]].ax+U[o[j]].ay>U[v].ax+U[v].ay){o[j+1]=o[j];j--;}o[j+1]=v;}
 for(int i=0;i<NU;i++)drawU(o[i]);
 for(int i=0;i<8;i++)if(pops[i].t<40){text(pops[i].x-6,pops[i].y-pops[i].t/3+1,pops[i].s,cStrip,2);text(pops[i].x-7,pops[i].y-pops[i].t/3,pops[i].s,pops[i].c,2);}
 embers();
 rect(buf,0,136,W,24,cStrip);text(4,140,m1,cText,1);text(4,149,m2,cText,1);
 text(4,4,"LASHKAR OF ASHES",cText,1);if(enemyTurn)text(180,4,"ENEMY TURN",cRed,1);
 if(scene!=BATTLE){rect(buf,0,52,W,52,cStrip);ctext(62,scene==WIN?"EMBERS ARE LIT":"DASTAN HAS FALLEN",scene==WIN?cGold:cLose,2);
  ctext(84,scene==WIN?"CHAPTER 1 COMPLETE":"THE ASH CLAIMS ALL",cText,1);ctext(94,"PRESS A",cText,1);}
}
static void present(void){dma((void*)VRAMP(back),buf,W*SH/2);VSYNC();SETDISP(0x0404|(back<<4));back^=1;}

int main(void){
 setup();SETDISP(0x0404);back=1;
 for(int i=0;i<28;i++){ashx[i]=rnd(W);ashy[i]=rnd(SH);}
 for(int i=0;i<8;i++)pops[i].t=40;
 for(;;){
  frame++;keys=KEYS();kp=keys&~prev;prev=keys;
  if(scene==INTRO){if(kp&(KA|KSTART)){rs+=frame;if(++pg>=3){scene=BATTLE;initBattle();}}}
  else if(scene==BATTLE){
   if(enemyTurn)enemyStep();
   else if(!moving()){
    int mv=0;
    if((kp&KUP)&&cy>0){cy--;mv=1;}if((kp&KDOWN)&&cy<N-1){cy++;mv=1;}
    if((kp&KLEFT)&&cx>0){cx--;mv=1;}if((kp&KRIGHT)&&cx<N-1){cx++;mv=1;}
    if(mv&&mode==IDLE){int ui=at(cx,cy);if(ui>=0)infoMsg(ui);}
    if(kp&KA)pressA();
    if(kp&KB)pressB();
    if(kp&KSTART)startEnemy();
    if(scene==BATTLE&&!enemyTurn){int all=1;for(int i=0;i<NU;i++)if(U[i].team==0&&U[i].hp>0&&!U[i].done)all=0;if(all)startEnemy();}
   }
  }else if(kp&(KA|KSTART)){scene=INTRO;pg=0;}
  upd();render();present();
#ifdef HOSTTEST
  if(hook(frame))return 0;
#endif
 }
}
