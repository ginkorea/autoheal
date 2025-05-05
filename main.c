// autoheal.c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <getopt.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/stb_image.h"
#include "include/stb_image_write.h"

// K-Means parameters
#define KMEANS_ITER    10
#define KMEANS_SAMPLES 50000
#define MIN_REGION     20

static int   IMG_W    = 512;
static int   IMG_H    = 512;
static int   CLUSTERS = 5;

// Helpers
static inline int inside(int x, int y) {
    return x >= 0 && x < IMG_W && y >= 0 && y < IMG_H;
}

static float dist3(uint8_t r, uint8_t g, uint8_t b, const float *cent) {
    float dr = r - cent[0], dg = g - cent[1], db = b - cent[2];
    return dr*dr + dg*dg + db*db;
}

// Parse command line
static void parse_args(int argc, char **argv) {
    static struct option longopts[] = {
        {"clusters", required_argument, NULL, 'k'},
        {"width",    required_argument, NULL, 'w'},
        {"height",   required_argument, NULL, 'h'},
        {NULL,0,0,0}
    };
    int c;
    while ((c = getopt_long(argc, argv, "k:w:h:", longopts, NULL)) != -1) {
        switch (c) {
            case 'k': CLUSTERS = atoi(optarg); break;
            case 'w': IMG_W    = atoi(optarg); break;
            case 'h': IMG_H    = atoi(optarg); break;
            default:
                fprintf(stderr,
                    "Usage: %s [-k clusters] [-w width] [-h height]\n", argv[0]);
                exit(1);
        }
    }
    if (CLUSTERS < 1) CLUSTERS = 1;
    if (IMG_W<1||IMG_H<1) {
        fprintf(stderr,"Invalid dimensions %dx%d\n",IMG_W,IMG_H);
        exit(1);
    }
}

static void smooth_mask(uint8_t *mask, int w, int h, int radius) {
    uint8_t *tmp = malloc(w * h);
    if (!tmp) {
        fprintf(stderr, "❌ Failed to allocate memory for smoothing.\n");
        return;
    }
    memset(tmp, 0, w * h);

    // Erode
    for (int y = radius; y < h - radius; y++) {
        for (int x = radius; x < w - radius; x++) {
            int solid = 1;
            for (int dy = -radius; dy <= radius && solid; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    if (mask[(y + dy) * w + (x + dx)] == 0) {
                        solid = 0;
                        break;
                    }
                }
            }
            tmp[y * w + x] = solid ? 255 : 0;
        }
    }

    // Dilate
    for (int y = radius; y < h - radius; y++) {
        for (int x = radius; x < w - radius; x++) {
            int hit = 0;
            for (int dy = -radius; dy <= radius && !hit; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    if (tmp[(y + dy) * w + (x + dx)] == 255) {
                        hit = 1;
                        break;
                    }
                }
            }
            mask[y * w + x] = hit ? 255 : 0;
        }
    }

    free(tmp);
}

int main(int argc, char **argv) {
    parse_args(argc, argv);
    srand((unsigned)time(NULL));

    const int PIXELS   = IMG_W * IMG_H;

    // 1) load image
    int w,h,c;
    uint8_t *img = stbi_load("input.png",&w,&h,&c,3);
    if (!img || w!=IMG_W || h!=IMG_H) {
        fprintf(stderr,"Failed to load %dx%d PNG\n",IMG_W,IMG_H);
        return 1;
    }

    // 2) sample for k-means
    int sample_n = KMEANS_SAMPLES;
    if (sample_n > PIXELS) sample_n = PIXELS;
    uint8_t *samples = malloc(sample_n*3);
    for (int i=0;i<sample_n;i++){
        int p = rand()%PIXELS;
        samples[3*i+0] = img[3*p+0];
        samples[3*i+1] = img[3*p+1];
        samples[3*i+2] = img[3*p+2];
    }

    // allocate centroids & palette arrays
    float (*centroids)[3] = malloc(sizeof(*centroids)*CLUSTERS);
    uint8_t *palette_r    = malloc(CLUSTERS);
    uint8_t *palette_g    = malloc(CLUSTERS);
    uint8_t *palette_b    = malloc(CLUSTERS);
    int *labels           = calloc(PIXELS, sizeof(int));

    // 3) K-means
    for (int k=0;k<CLUSTERS;k++){
        int idx = (rand()%sample_n)*3;
        centroids[k][0] = samples[idx+0];
        centroids[k][1] = samples[idx+1];
        centroids[k][2] = samples[idx+2];
    }
    for (int it=0; it<KMEANS_ITER; ++it) {
        float sums[CLUSTERS][3];
        int   counts[CLUSTERS];
        memset(sums,0,sizeof(sums));
        memset(counts,0,sizeof(counts));
        for (int i=0;i<sample_n;i++){
            uint8_t r = samples[3*i+0],
                    g = samples[3*i+1],
                    b = samples[3*i+2];
            float best_d=1e9; int best=0;
            for(int k=0;k<CLUSTERS;k++){
                float d = dist3(r,g,b, centroids[k]);
                if(d<best_d){ best_d=d; best=k; }
            }
            sums[best][0]+=r; sums[best][1]+=g; sums[best][2]+=b;
            counts[best]++;
        }
        for(int k=0;k<CLUSTERS;k++) if(counts[k]){
            centroids[k][0]=sums[k][0]/counts[k];
            centroids[k][1]=sums[k][1]/counts[k];
            centroids[k][2]=sums[k][2]/counts[k];
        }
    }
    for(int k=0;k<CLUSTERS;k++){
        palette_r[k] = (uint8_t)(centroids[k][0]+.5f);
        palette_g[k] = (uint8_t)(centroids[k][1]+.5f);
        palette_b[k] = (uint8_t)(centroids[k][2]+.5f);
    }
    free(samples);

    // 4) quantize every pixel
    uint8_t *idxbuf = malloc(PIXELS);
    for(int i=0;i<PIXELS;i++){
        uint8_t r=img[3*i], g=img[3*i+1], b=img[3*i+2];
        int best=0; int bd=1e9;
        for(int k=0;k<CLUSTERS;k++){
            int dr=r-palette_r[k], dg=g-palette_g[k], db=b-palette_b[k];
            int d=dr*dr+dg*dg+db*db;
            if(d<bd){ bd=d; best=k; }
        }
        idxbuf[i]=best;
    }
    stbi_image_free(img);

    // 5) flood fill / auto-heal identical to your original code
    int *q_x = malloc(sizeof(int)*PIXELS),
        *q_y = malloc(sizeof(int)*PIXELS);
    int  region_id=1;
    for(int y=0;y<IMG_H;y++){
      for(int x=0;x<IMG_W;x++){
        int pos=y*IMG_W+x;
        if(labels[pos]) continue;
        uint8_t col=idxbuf[pos];
        int qt=0, qh=0;
        q_x[qt]=x; q_y[qt++]=y;
        labels[pos]=region_id;
        int rsz=1;
        while(qh<qt){
          int cx=q_x[qh], cy=q_y[qh++];
          const int dirs[4][2]={{1,0},{-1,0},{0,1},{0,-1}};
          for(int d=0;d<4;d++){
            int nx=cx+dirs[d][0], ny=cy+dirs[d][1];
            if(!inside(nx,ny)) continue;
            int np=ny*IMG_W+nx;
            if(labels[np]||idxbuf[np]!=col) continue;
            labels[np]=region_id;
            q_x[qt]=nx; q_y[qt++]=ny;
            rsz++;
          }
        }
        // small-region repaint as before…
        if(rsz<MIN_REGION){
          int hist[CLUSTERS]; memset(hist,0,sizeof(hist));
          for(int i=0;i<qt;i++){
            int rx=q_x[i], ry=q_y[i];
            for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){
              int sx=rx+dx, sy=ry+dy;
              if(!inside(sx,sy)) continue;
              int sp=sy*IMG_W+sx;
              if(labels[sp]!=region_id) hist[idxbuf[sp]]++;
            }
          }
          int dom=0, mc=0;
          for(int k=0;k<CLUSTERS;k++){
            if(hist[k]>mc){ mc=hist[k]; dom=k; }
          }
          for(int i=0;i<qt;i++){
            int rx=q_x[i], ry=q_y[i];
            idxbuf[ry*IMG_W+rx]=dom;
          }
        }
        region_id++;
      }
    }

    // 6) emit layered masks
    uint8_t *mask = malloc(PIXELS);
    for(int k=0;k<CLUSTERS;k++){
      for(int i=0;i<PIXELS;i++){
        mask[i] = (idxbuf[i]==k?255:0);
      }
      // simple 1px smooth
      // (reuse your smooth_mask function here)
      smooth_mask(mask, IMG_W, IMG_H, 1);

      char fn[80];
      snprintf(fn,sizeof(fn),
        "layer_%d_r%d_g%d_b%d.png",
         k,palette_r[k],palette_g[k],palette_b[k]);
      stbi_write_png(fn, IMG_W, IMG_H, 1, mask, IMG_W);
    }

    // 7) write palette.json
    FILE *f = fopen("palette.json","w");
    if(f){
      fprintf(f,"{\n");
      for(int k=0;k<CLUSTERS;k++){
        fprintf(f,"  \"layer_%d\": [%u,%u,%u]%s\n",
          k,
          palette_r[k],palette_g[k],palette_b[k],
          (k<CLUSTERS-1)?",":"");
      }
      fprintf(f,"}\n");
      fclose(f);
    }

    // cleanup
    free(palette_r); free(palette_g); free(palette_b);
    free(labels);   free(idxbuf);
    free(q_x);      free(q_y);
    free(mask);     free(centroids);

    return 0;
}
