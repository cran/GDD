#include "GDD.h"
#include "GDDtalk.h"
#include <Rversion.h>

#include <gdfonts.h>

/* we need registry and win32 API for finding the system fonts folder */
#ifdef WIN32
#include <windows.h>
#include <winreg.h>
#endif

/* Device Driver Actions */

#if R_VERSION < 0x10900
#error This GDD needs at least R version 1.9.0
#endif

#define R2I(X) ((int)((X)+0.5))
#define CREDC(C) ((C)&0xff)
#define CGREENC(C) (((C)&0xff00)>>8)
#define CBLUEC(C) (((C)&0xff0000)>>16)

static char *ftprefix=0;

static char *font_file[8] = {0,0,0,0,0,0,0,0};

static void GDD_Activate(NewDevDesc *dd);
static void GDD_Circle(double x, double y, double r,
			  R_GE_gcontext *gc,
			  NewDevDesc *dd);
static void GDD_Clip(double x0, double x1, double y0, double y1,
			NewDevDesc *dd);
static void GDD_Close(NewDevDesc *dd);
static void GDD_Deactivate(NewDevDesc *dd);
static void GDD_Hold(NewDevDesc *dd);
static Rboolean GDD_Locator(double *x, double *y, NewDevDesc *dd);
static void GDD_Line(double x1, double y1, double x2, double y2,
			R_GE_gcontext *gc,
			NewDevDesc *dd);
static void GDD_MetricInfo(int c, 
			      R_GE_gcontext *gc,
			      double* ascent, double* descent,
			      double* width, NewDevDesc *dd);
static void GDD_Mode(int mode, NewDevDesc *dd);
static void GDD_NewPage(R_GE_gcontext *gc, NewDevDesc *dd);
static void GDD_Polygon(int n, double *x, double *y,
			   R_GE_gcontext *gc,
			   NewDevDesc *dd);
static void GDD_Polyline(int n, double *x, double *y,
			     R_GE_gcontext *gc,
			     NewDevDesc *dd);
static void GDD_Rect(double x0, double y0, double x1, double y1,
			 R_GE_gcontext *gc,
			 NewDevDesc *dd);
static void GDD_Size(double *left, double *right,
			 double *bottom, double *top,
			 NewDevDesc *dd);
static double GDD_StrWidth(char *str, 
			       R_GE_gcontext *gc,
			       NewDevDesc *dd);
static void GDD_Text(double x, double y, char *str,
			 double rot, double hadj,
			 R_GE_gcontext *gc,
			 NewDevDesc *dd);


static R_GE_gcontext lastGC; /** last graphics context. the API send changes, not the entire context, so we cache it for comparison here */
static char *fallback_font = 0; /** fallback font if no match is found in the lookup table */

#define checkGC(xd,gc) sendGC(xd,gc,0)

/** check changes in GC and issue corresponding commands if necessary */
static void sendGC(GDDDesc *xd, R_GE_gcontext *gc, int sendAll) {
	gdImagePtr img = xd->img;
	int updateLty = 0;

    if (sendAll || gc->col != lastGC.col) {
		xd->gd_draw = ((gc->col >> 24)==0)?-1:gdTrueColor(CREDC(gc->col), CGREENC(gc->col), CBLUEC(gc->col));
		/* force lty update on color change since the gd style array needs to be re-build using the new color */
		if (gc->lty!=0 && gc->lty!=-1) updateLty=1;
#ifdef JGD_DEBUG
		printf("gd_draw = %08x\n", xd->gd_draw);
#endif
    }

    if (sendAll || gc->fill != lastGC.fill)  {
		xd->gd_fill = ((gc->fill >> 24)==0)?-1:gdTrueColor(CREDC(gc->fill), CGREENC(gc->fill), CBLUEC(gc->fill));
#ifdef JGD_DEBUG
		printf("gd_fill = %08x\n", xd->gd_fill);
#endif
    }

    if (sendAll || gc->lwd != lastGC.lwd || gc->lty != lastGC.lty || updateLty) {
		int ith=(int)gc->lwd;
		if (ith<0) ith=1;
		gdImageSetThickness(img, ith);
#ifdef JGD_DEBUG
		printf("gdImageSetThickness: %d\n", ith);
#endif
		if (gc->lty!=lastGC.lty) {
#ifdef JGD_DEBUG
		  printf("lty=%x (was %x)\n", gc->lty, lastGC.lty);
#endif
		  if (gc->lty!=-1 && gc->lty!=0) {
		    int ls[16]; /* max 16x4=64 bit */
		    int l=0, i=0, gof=0, dt=gc->lty, lsum=0;
		    int gdl[64];
		    int *gdp=gdl;
		    while (dt>0) {
		      lsum+=ls[l]=dt&15;
		      dt>>=4;
		      l++;
		    }
#ifdef JGD_DEBUG
		    printf("pattern length: %d (in %d blocks)\n", lsum, l);
#endif
		    if (lsum>63)
		      gdp=(int*) malloc(lsum);
		    while (i<l) {
		      while (ls[i]>0) {
			ls[i]--;
#ifdef JGD_DEBUG
			printf("%c", ((i&1)==0)?'.':'X');
#endif
			gdp[gof++]=((i&1)==0)?gdTransparent:xd->gd_draw;
		      }
		      i++;
		    }
#ifdef JGD_DEBUG
		    printf("\nsetting style (len=%d, color=%d)\n", gof, xd->gd_draw);
#endif
		    gdImageSetStyle(img, gdp, gof);
		    if (gdp!=gdl)
		      free(gdp);
		  }
		}
    }

    if (sendAll || gc->cex!=lastGC.cex || gc->ps!=lastGC.ps || gc->lineheight!=lastGC.lineheight || gc->fontface!=lastGC.fontface || strcmp(gc->fontfamily, lastGC.fontfamily)) {
		xd->gd_font = gdFontGetSmall();
		xd->gd_ftfont=0;
		xd->gd_ftm_width=-1.0; /* <0 means invalid, needs re-calc */
		
		if (!fallback_font && ftprefix) {
			char fc=ftprefix[strlen(ftprefix)-1];
			fallback_font = (char*) malloc(strlen(ftprefix)+32);
			strcpy(fallback_font, ftprefix);
			if (fc!='/' && fc!='\\') strcat(fallback_font, "/");
			strcat(fallback_font, "blue highway free.ttf");
		}
		if (gc->fontface>0 && gc->fontface<=5 && font_file[gc->fontface-1])
			xd->gd_ftfont = font_file[gc->fontface-1];
		else {
			if (font_file[0])
				xd->gd_ftfont = font_file[0];
			else {
				if (fallback_font)
					xd->gd_ftfont = fallback_font;
				else
					xd->gd_ftfont = "blue highway free.ttf";
			}
		}
		xd->gd_ftsize=gc->ps*gc->cex;
#ifdef JGD_DEBUG
		printf("Using font file: %s\n", xd->gd_ftfont);
#endif
    }
    memcpy(&lastGC, gc, sizeof(lastGC));
}

/*------- the R callbacks begin here ... ------------------------*/

static void GDD_Activate(NewDevDesc *dd)
{
    GDDDesc *xd = (GDDDesc *) dd->deviceSpecific;
    if(!xd || !xd->img) return;
}

static void GDD_Circle(double x, double y, double r,  R_GE_gcontext *gc,  NewDevDesc *dd)
{
    GDDDesc *xd = (GDDDesc *) dd->deviceSpecific;
    if(!xd || !xd->img) return;
#ifdef JGD_DEBUG
	printf("circle %x %d/%d %d/%d %x\n",xd->img, R2I(x), R2I(y), R2I(r*2.0), R2I(r*2.0), gdAntiAliased);
	printf(" [cex=%f, bfs=%d, fs=%d]\n", xd->cex, xd->basefontsize, xd->fontsize);
#endif
    checkGC(xd, gc);
	if (xd->gd_fill!=-1)
		gdImageFilledEllipse(xd->img, R2I(x), R2I(y), R2I(r*2.0)+1, R2I(r*2.0)+1, xd->gd_fill);
	if (xd->gd_draw!=-1) {
		/* gdImageSetAntiAliased (xd->img, xd->gd_draw); */
		gdImageArc(xd->img, R2I(x), R2I(y), R2I(r*2.0)+1, R2I(r*2.0)+1, 0, 360, xd->gd_draw);
	}
}

static void GDD_Clip(double x0, double x1, double y0, double y1,  NewDevDesc *dd)
{
    GDDDesc *xd = (GDDDesc *) dd->deviceSpecific;
    if(!xd || !xd->img) return;
	if (x1<x0) { double h=x1; x1=x0; x0=h; };
	if (y1<y0) { double h=y1; y1=y0; y0=h; };
	gdImageSetClip(xd->img, R2I(x0), R2I(y0), R2I(x1), R2I(y1));
#ifdef JGD_DEBUG
	printf("clipping %d/%d %d/%d\n", R2I(x0), R2I(y0), R2I(x1), R2I(y1));
#endif
}

/** note that saveActiveImage doesn't increase the sequence number to avoid confusion */
static void saveActiveImage(GDDDesc * xd)
{
	char *it = xd->img_type;
	char *fn;
	FILE *out;
	int   nl;
	
	fn=(char*) malloc(strlen(xd->img_name)+16);
	strcpy(fn, xd->img_name);
	if (xd->img_seq>0)
		sprintf(fn+strlen(fn),"%d",xd->img_seq);
	nl = strlen(fn);
	
	if (!strcmp(it, "png") || !strcmp(it, "png24")) {
		if (nl>3 && strcmp(fn+nl-4,".png")) strcat(fn, ".png");
		out = fopen (fn, "wb");
		if (out) {
			gdImagePng (xd->img, out);
			fclose (out);
		}
		return;
	}

	if (!strcmp(it, "png8")) {
		if (nl>3 && strcmp(fn+nl-4,".png")) strcat(fn, ".png");
		out = fopen (fn, "wb");
		if (out) {
			gdImagePng (xd->img, out);
			fclose (out);
		}
		return;
	}

	if (!strcmp(it, "gif")) {
		if (nl>3 && strcmp(fn+nl-4,".gif")) strcat(fn, ".gif");
		out = fopen (fn, "wb");
		if (out) {
			gdImageGif (xd->img, out);
			fclose (out);
		}
		return;
	}
	
	if (!strcmp(it, "jpeg") || !strcmp(it, "jpg")) {
		if (nl>3 && strcmp(fn+nl-4,".jpg")) strcat(fn, ".jpg");
		out = fopen (fn, "wb");
		if (out) {
			gdImageJpeg (xd->img, out, 80);
			fclose (out);
		}
		return;
	}
	
	error("Unsupported image type (%s).", it);
}

static void GDD_Close(NewDevDesc *dd)
{
    GDDDesc *xd = (GDDDesc *) dd->deviceSpecific;
    if(!xd || !xd->img) return;

	saveActiveImage(xd);
	gdImageDestroy (xd->img);
	xd->img=0;
}

static void GDD_Deactivate(NewDevDesc *dd)
{
    GDDDesc *xd = (GDDDesc *) dd->deviceSpecific;
    if(!xd || !xd->img) return;
}

static void GDD_Hold(NewDevDesc *dd)
{
    GDDDesc *xd = (GDDDesc *) dd->deviceSpecific;
    if(!xd || !xd->img) return;
}

static Rboolean GDD_Locator(double *x, double *y, NewDevDesc *dd)
{
    GDDDesc *xd = (GDDDesc *) dd->deviceSpecific;
    if(!xd || !xd->img) return FALSE;
    return FALSE;
}

static void GDD_Line(double x1, double y1, double x2, double y2,  R_GE_gcontext *gc,  NewDevDesc *dd)
{
    GDDDesc *xd = (GDDDesc *) dd->deviceSpecific;
    if(!xd || !xd->img) return;
    
#ifdef JGD_DEBUG
	printf("line %d/%d %d/%d %x\n", R2I(x1), R2I(y1), R2I(x2), R2I(y2), xd->gd_draw);
#endif

    checkGC(xd, gc);
    if (xd->gd_draw!=-1 && gc->lty!=-1) {
		if (gc->lty==0) {
			gdImageSetAntiAliased (xd->img, xd->gd_draw);
			gdImageLine(xd->img, R2I(x1), R2I(y1), R2I(x2), R2I(y2), gdAntiAliased);
		} else {
#ifdef JGD_DEBUG
			printf("lty=%x, using gdStyled\n", gc->lty);
#endif
			gdImageLine(xd->img, R2I(x1), R2I(y1), R2I(x2), R2I(y2), gdStyled);
		}
	}
}

static void GDD_MetricInfo(int c,  R_GE_gcontext *gc,  double* ascent, double* descent,  double* width, NewDevDesc *dd)
{
    GDDDesc *xd = (GDDDesc *) dd->deviceSpecific;
    if(!xd || !xd->img) return;
    
    checkGC(xd, gc);

#ifdef HAS_FTL
	if (xd->gd_ftm_width<0 || xd->gd_ftm_char!=c) {
		int br[8];
		char str[3];
		str[0]=(char)c; str[1]=0;
		if (!c) { str[0]='M'; str[1]='g'; str[2]=0; /* this should give us reasonable descent (g) and almost max width (M) */ }
		gdImageStringFT(0, br, xd->gd_draw, xd->gd_ftfont, xd->gd_ftsize, 0.0, 0, 0, str);
#ifdef JGD_DEBUG
		{ int i=0; printf("metric %x [%c]: ",c, (char)c); while (i<8) printf("%d ", br[i++]); printf("\n"); }
#endif
		xd->gd_ftm_ascent=*ascent=-br[5]; xd->gd_ftm_descent=*descent=br[1];
		xd->gd_ftm_width=*width=(c)?((double)br[2]):(0.5*((double)br[2])); xd->gd_ftm_char=c;
	} else {
		*ascent=xd->gd_ftm_ascent; *descent=xd->gd_ftm_descent;
		*width=xd->gd_ftm_width;
	}
#else
    *ascent=xd->gd_font->h; *descent=0; *width=xd->gd_font->w;
#ifdef JGD_DEBUG
	printf("FM> ascent=%f, descent=%f, width=%f\n", *ascent, *descent, *width);
#endif
#endif
}

static void GDD_Mode(int mode, NewDevDesc *dd)
{
    GDDDesc *xd = (GDDDesc *) dd->deviceSpecific;
    if(!xd || !xd->img) return;
}

static void GDD_NewPage(R_GE_gcontext *gc, NewDevDesc *dd)
{
    GDDDesc *xd = (GDDDesc *) dd->deviceSpecific;
	int devNr, white;
    if(!xd || !xd->img) return;
    
    devNr = devNumber((DevDesc*) dd);

	if (xd->img_seq!=-1) /* first request is not saved as this is part of the init */
		saveActiveImage(xd);
	xd->img_seq++;

	white = gdTrueColor (255, 255, 255); /* FIXME: we don't respect canvas color here */
	gdImageFilledRectangle(xd->img, 0, 0, gdImageSX(xd->img), gdImageSY(xd->img), white);
	
    /* this is an exception - we send all GC attributes just after the NewPage command */
    sendGC(xd, gc, 1);
}

Rboolean GDD_Open(NewDevDesc *dd, GDDDesc *xd,  char *type, char *file, double w, double h, int bgcolor)
{
	int white;
    
    xd->fill = 0xffffffff; /* transparent, was R_RGB(255, 255, 255); */
    xd->col = R_RGB(0, 0, 0);
    xd->canvas = R_RGB(255, 255, 255);
    xd->windowWidth = w;
    xd->windowHeight = h;
    
	xd->img_type[7]=0;
	strncpy(xd->img_type, type, 7);
	xd->img_name=(char*) malloc(strlen(file)+1);
	strcpy(xd->img_name, file);
	xd->img_seq=-1;	
	
	xd->img = gdImageCreateTrueColor(R2I(w), R2I(h));
	white = gdTrueColor (255, 255, 255);
	gdImageFilledRectangle(xd->img, 0, 0, gdImageSX(xd->img), gdImageSY(xd->img), white);
	gdImageColorTransparent(xd->img, (CONVERT_COLOR(bgcolor)==0x00ffffff)?white:-1); /* if `bg'=="transparent" then white is the transparent color */
#ifdef JGD_DEBUG
	printf("open %dx%d\n", R2I(w), R2I(h));
#endif

#ifndef HAS_FTL
	warning("GDD uses libgd without freetype support. Falling back to raster fonts (untested!).\nPlease consider installing freetype and gd with freetype support to improve the quality of the GDD output.");
#endif
	
    return TRUE;
}

static void GDD_Polygon(int n, double *x, double *y,  R_GE_gcontext *gc,  NewDevDesc *dd)
{
	gdPoint *pt;
	int i=0;
    GDDDesc *xd = (GDDDesc *) dd->deviceSpecific;
    if(!xd || !xd->img) return;

#ifdef JGD_DEBUG
	printf("polygon %d points\n", n);
#endif
    checkGC(xd, gc);
	pt = (gdPoint*) malloc(sizeof(gdPoint)*(n+1));
	while (i<n) {
		pt[i].x=R2I(x[i]);
		pt[i].y=R2I(y[i]);
		i++;
	}
    if (xd->gd_fill!=-1) {
		gdImageSetAntiAliased (xd->img, xd->gd_fill);
		gdImageFilledPolygon(xd->img, pt, n, gdAntiAliased);
	}
    if (xd->gd_draw!=-1) {
		gdImageSetAntiAliased (xd->img, xd->gd_draw);
		gdImagePolygon(xd->img, pt, n, gdAntiAliased);
	}
	free(pt);
}

static void GDD_Polyline(int n, double *x, double *y,  R_GE_gcontext *gc,  NewDevDesc *dd)
{
	gdPoint *pt;
	int i=0;
    GDDDesc *xd = (GDDDesc *) dd->deviceSpecific;
    if(!xd || !xd->img) return;
	
#ifdef JGD_DEBUG
	printf("polyline %d points\n", n);
#endif
    checkGC(xd, gc);
	pt = (gdPoint*) malloc(sizeof(gdPoint)*(n+1));
	while (i<n) {
		pt[i].x=R2I(x[i]);
		pt[i].y=R2I(y[i]);
		i++;
	}
    if (xd->gd_draw!=-1) {
		gdImageSetAntiAliased (xd->img, xd->gd_draw);
		gdImageOpenPolygon(xd->img, pt, n, gdAntiAliased);
	}
	free(pt);
}

static void GDD_Rect(double x0, double y0, double x1, double y1,  R_GE_gcontext *gc,  NewDevDesc *dd)
{
    GDDDesc *xd = (GDDDesc *) dd->deviceSpecific;
    if(!xd || !xd->img) return;
	
    checkGC(xd, gc);
	if (x1<x0) { double h=x1; x1=x0; x0=h; }
	if (y1<y0) { double h=y1; y1=y0; y0=h; }
	if (x0<0) x0=0; if (y0<0) y0=0;
#ifdef JGD_DEBUG
	printf("gdRect: %x %d/%d %d/%d %08x\n", xd->img, R2I(x0), R2I(y0), R2I(x1), R2I(y1), xd->gd_draw);
#endif
	if (xd->gd_fill!=-1)
		gdImageFilledRectangle(xd->img, R2I(x0), R2I(y0), R2I(x1), R2I(y1), xd->gd_fill);
	if (xd->gd_draw!=-1)
		gdImageRectangle(xd->img, R2I(x0), R2I(y0), R2I(x1), R2I(y1), xd->gd_draw);
}

static void GDD_Size(double *left, double *right,  double *bottom, double *top,  NewDevDesc *dd)
{
    GDDDesc *xd = (GDDDesc *) dd->deviceSpecific;
    if(!xd || !xd->img) return;
	
	*left=*top=0.0;
	*right=R2I(gdImageSX(xd->img));
	*bottom=R2I(gdImageSY(xd->img));
}

static double GDD_StrWidth(char *str,  R_GE_gcontext *gc,  NewDevDesc *dd)
{
    GDDDesc *xd = (GDDDesc *) dd->deviceSpecific;
    if(!xd || !xd->img) return strlen(str)*8;

#ifdef HAS_FTL
	{
		int br[8];
		gdImageStringFT(0, br, xd->gd_draw, xd->gd_ftfont, xd->gd_ftsize, 0.0, 0, 0, str);
		return (double)((br[2]<0)?-br[2]:br[2]);
	}
#else
    return (double)(strlen(str)*xd->gd_font->w);
#endif
}

static void GDD_Text(double x, double y, char *str,  double rot, double hadj,  R_GE_gcontext *gc,  NewDevDesc *dd)
{
    GDDDesc *xd = (GDDDesc *) dd->deviceSpecific;
    if(!xd || !xd->img) return;
        
    checkGC(xd, gc);
#ifdef JGD_DEBUG
	printf("text \"%s\" hadj=%f\n", str, hadj);
#endif
	
#ifdef HAS_FTL
	if (xd->gd_draw!=-1) {
		int br[8];
		double rad=rot/180.0*3.141592;
		gdImageStringFT(0, br, xd->gd_draw, xd->gd_ftfont, xd->gd_ftsize, 0.0, 0, 0, str);
		if (hadj!=0.0) {
			double tw=(double)br[2]; /* string width */
			x-=cos(rad)*(tw*hadj);
			y+=sin(rad)*(tw*hadj);
		}
#ifdef JGD_DEBUG
		printf("FT text using font \"%s\", size %.1f\n", xd->gd_ftfont, xd->gd_ftsize);
#endif
		gdImageStringFT(xd->img, br, xd->gd_draw, xd->gd_ftfont, xd->gd_ftsize, rad, R2I(x), R2I(y), str);
	}
#else
#ifdef JGD_DEBUG
	printf("  using simple text output\n");
#endif
    if (xd->gd_draw!=-1) {		
		if (hadj!=0.0) {
			double tw = (double)(xd->gd_font->w*strlen(str));
			x-=tw*hadj;
		}
		gdImageString(xd->img, xd->gd_font, R2I(x), R2I(y)-(xd->gd_font->h), str, xd->gd_draw);
	}
#endif
}

/*-----------------------------------------------------------------------*/

void gddSetFTFontPath(char **ftfp) {
	int fpl=0;
	int fty=-1;
	if (ftprefix) free(ftprefix);
	ftprefix=(char*) malloc(strlen(*ftfp)+2);
	strcpy(ftprefix, *ftfp);
	if (ftprefix[strlen(ftprefix)-1]!='/') strcat(ftprefix, "/");												
	fpl=strlen(ftprefix);

#ifdef WIN32
	/* In Windows we need to find the Fonts path and set WINDOWSFONTS */
	{
		char rbuf[512];
		DWORD t,s=511;
		HKEY k;
		char *key="Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";
		char *fpath=0;

		rbuf[511]=0;
		if (RegOpenKeyEx(HKEY_CURRENT_USER,key,0,KEY_QUERY_VALUE,&k)==ERROR_SUCCESS) {
			if (RegQueryValueEx(k,"Fonts",0,&t,rbuf,&s)==ERROR_SUCCESS) {
				if (strchr(rbuf,'%')) {
					char b2[512];
					int r = ExpandEnvironmentStrings(rbuf, b2, 512);
					if (r>0 && r<512) strcpy(rbuf, b2);
				}
				fpath=rbuf;
			}
			RegCloseKey(k);
		}
		if (!fpath) {
			int l = GetWindowsDirectory(rbuf, 480);
			if (l>0 && l<480) {
				strcat(rbuf, "\\Fonts");
				if (GetFileAttributes(rbuf)!=0xffffffff)
					fpath=rbuf;
			}
		}
		if (fpath && *fpath) {
			char pe[530];
			strcpy(pe, "WINDOWSFONTS=");
			strcat(pe, fpath);
			putenv(pe);
#ifdef JGD_DEBUG
			printf("setting WINDOWSFONTS=%s\n", fpath);
#endif
		}
	}
#endif
	
	{ /* read mapping from fontface (1..5 -> n,b,i,bi,sym) to TTF files */
		FILE *f;
		int matched=0;
		char *buf = (char*) malloc(strlen(ftprefix)+512);
		strcpy(buf, ftprefix);
		strcat(buf, "basefont.mapping");
		memset(font_file, 0, sizeof(char*)*8);
		f = fopen(buf, "r");
#ifdef JGD_DEBUG
		printf("Trying to read mappings from \"%s\"\n", buf);
#endif
		if (f) {
			buf[255]=0;
			while (matched<5 && fgets(buf, 256, f)) {
				int j=strlen(buf);
				while (j>0 && (buf[j-1]=='\r' || buf[j-1]=='\n')) j--;
				buf[j]=0;
				fty=-1;
				if (!strncmp(buf,"base.norm:",10)) fty=0;
				else if (!strncmp(buf,"base.bold:",10)) fty=1;
				else if (!strncmp(buf,"base.ital:",10)) fty=2;
				else if (!strncmp(buf,"base.bita:",10)) fty=3;
				else if (!strncmp(buf,"symbol:",7)) fty=4;
				if (fty!=-1 && j>10) {
					FILE *ff=0;
					char *fb=strchr(buf,':')+1;
					while (*fb=='\t' || *fb==' ') fb++;
					j=strlen(fb);
					
					if (*fb=='<') { /* if a font name is given, we ask gd to find the font via fontconfig */
						gdFTStringExtra se;
						char *fbe=fb;
						memset(&se, 0, sizeof(se));
						while (*fbe && *fbe!='>') fbe++;
						*fbe=0;
#ifdef HAS_FTL
						se.flags = gdFTEX_RETURNFONTPATHNAME | gdFTEX_FONTCONFIG;
						gdImageStringFTEx(0, 0, 0, fb, 10.0, 0.0, 10, 10, "bla", &se);
						if (se.fontpath) {
#ifdef JGD_DEBUG
							printf("font path found: \"%s\"\n", se.fontpath);
#endif
							if (strlen(se.fontpath)>511) {
								warning("Font path for font <%s> is too long, ignoring.", fb);
								*fb=0;
							} else strcpy(fb, se.fontpath);
							gdFree(se.fontpath);
						} else *fb=0;
#else
						*fb=0;
#endif
					}

#ifdef WIN32
					/* in Windows we expand env variables */
					if (strchr(fb, '%')) {
						char exp[512];
						char *c;
						strcpy(exp, fb);
#ifdef JGD_DEBUG
						printf("  (Win32) font path in need to be expanded: \"%s\"\n", fb);
#endif
						c = strchr(exp, '%');
						while (c) {
							char *e = strchr(c+1, '%');
							if (e) {
								char *v;
								*e=0;
								v = getenv(c+1);
#ifdef JGD_DEBUG
								printf("  (Win32) expand \"%s\" to \"%s\"\n", c+1, v?v:"<not set>");
#endif
								if (v) { /* env var found */
									int l = strlen(v);
									if (l>500-(c-exp)) *e='%'; /* expansion is too long */
									else {
										memmove(c+l, e+1, strlen(e)); /* incl. tariling \0 */
										memcpy(c, v, l);
										e = c + l - 1; /* make sure e is just at the end of the new string */
									}
								} else *e='%';
							}
							c = e;
						}
						strcpy(fb, exp);
					}

					if (*fb && *fb!='/' && *fb!='\\' && fb[1]!=':') {
						memmove(fb+fpl, fb, j+1);
						memcpy(fb, ftprefix, fpl);
					}

#else
					if (*fb && *fb!='/') {
						memmove(fb+fpl, fb, j+1);
						memcpy(fb, ftprefix, fpl);
					}
#endif /* WIN32 */

#ifdef JGD_DEBUG
                    printf("- candidate type=%d, file=\"%s\"\n", fty+1, fb);
#endif
                    if (*fb) ff = fopen(fb, "rb"); else ff = 0;
					if (ff) {
						fclose(ff);
						if (!font_file[fty]) {
							font_file[fty]=(char*) malloc(strlen(fb)+1);
							strcpy(font_file[fty], fb);
							matched++;
#ifdef JGD_DEBUG
							printf("TYPE %d ASSIGNED FONT \"%s\"\n", fty, font_file[fty]);
#endif
						}
					}
				}
				buf[255]=0;
			}
			fclose(f);
		}
		free(buf);
	}
}

SEXP gdd_look_up_font(SEXP f)
{
	int i=0;
	SEXP rv;
	if (f==R_NilValue) {
		PROTECT(rv = allocVector(STRSXP, 5));
		while (i<5) {
			if (font_file[i])
				SET_STRING_ELT(rv, i, mkChar(font_file[i]));
			else
				SET_STRING_ELT(rv, i, R_NaString);
			i++;
		}
		UNPROTECT(1);
		return rv;		
	}
	if (!isString(f) || LENGTH(f)<1) error("Font name must be a string.");
	PROTECT(rv = allocVector(STRSXP, LENGTH(f)));
	while (i<LENGTH(f)) {
		char *fn = CHAR(STRING_ELT(f, i));
		gdFTStringExtra se;
		memset(&se, 0, sizeof(se));
#ifdef HAS_FTL
		se.flags = gdFTEX_RETURNFONTPATHNAME | gdFTEX_FONTCONFIG;
		gdImageStringFTEx(0, 0, 0, fn, 10.0, 0.0, 10, 10, "bla", &se);
		if (se.fontpath) {
			SET_STRING_ELT(rv, i, mkChar(se.fontpath));
			gdFree(se.fontpath);
		} else
			SET_STRING_ELT(rv, i, R_NaString);
#else
		error("No freetype support compiled in libgd.");
#endif
		i++;
	}
	UNPROTECT(1);
	return rv;
}

/** fill the R device structure with callback functions */
void setupGDDfunctions(NewDevDesc *dd) {
    dd->open = GDD_Open;
    dd->close = GDD_Close;
    dd->activate = GDD_Activate;
    dd->deactivate = GDD_Deactivate;
    dd->size = GDD_Size;
    dd->newPage = GDD_NewPage;
    dd->clip = GDD_Clip;
    dd->strWidth = GDD_StrWidth;
    dd->text = GDD_Text;
    dd->rect = GDD_Rect;
    dd->circle = GDD_Circle;
    dd->line = GDD_Line;
    dd->polyline = GDD_Polyline;
    dd->polygon = GDD_Polygon;
    dd->locator = GDD_Locator;
    dd->mode = GDD_Mode;
    dd->hold = GDD_Hold;
    dd->metricInfo = GDD_MetricInfo;
}
