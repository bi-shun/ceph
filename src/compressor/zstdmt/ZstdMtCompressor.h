// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2015 Haomai Wang <haomaiwang@gmail.com>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#ifndef CEPH_ZSTDCOMPRESSOR_H
#define CEPH_ZSTDCOMPRESSOR_H

#define ZSTD_STATIC_LINKING_ONLY
#include "zstd/lib/zstd.h"

#include "include/buffer.h"
#include "include/encoding.h"
#include "compressor/Compressor.h"
#include "lib/zstd-mt.h"
#include "platform.h"



#define COMPRESSION_LEVEL 5

#define METHOD   "zstd"
#define PROGNAME "zstd-mt"
#define UNZIP    "unzstd-mt"
#define ZCAT     "zstdcat-mt"
#define SUFFIX   ".zst"

#define LEVEL_DEF          3
#define LEVEL_MIN          ZSTDMT_LEVEL_MIN
#define LEVEL_MAX          ZSTDMT_LEVEL_MAX
#define THREAD_MAX         ZSTDMT_THREAD_MAX

#define MT_isError         ZSTDMT_isError
#define MT_getErrorString  ZSTDMT_getErrorString
#define MT_Buffer          ZSTDMT_Buffer
#define MT_RdWr_t          ZSTDMT_RdWr_t

#define MT_CCtx            ZSTDMT_CCtx
#define MT_createCCtx      ZSTDMT_createCCtx
#define MT_compressCCtx    ZSTDMT_compressCCtx
#define MT_GetFramesCCtx   ZSTDMT_GetFramesCCtx
#define MT_GetInsizeCCtx   ZSTDMT_GetInsizeCCtx
#define MT_GetOutsizeCCtx  ZSTDMT_GetOutsizeCCtx
#define MT_freeCCtx        ZSTDMT_freeCCtx

#define MT_DCtx            ZSTDMT_DCtx
#define MT_createDCtx      ZSTDMT_createDCtx
#define MT_decompressDCtx  ZSTDMT_decompressDCtx
#define MT_GetFramesDCtx   ZSTDMT_GetFramesDCtx
#define MT_GetInsizeDCtx   ZSTDMT_GetInsizeDCtx
#define MT_GetOutsizeDCtx  ZSTDMT_GetOutsizeDCtx
#define MT_freeDCtx        ZSTDMT_freeDCtx
#define MODE_COMPRESS    1	/* -z (default) */
#define MODE_DECOMPRESS  2	/* -d */
#define MODE_LIST        3	/* -l */
#define MODE_TEST        4	/* -t */
static int opt_mode = MODE_COMPRESS;

/* for the -i option */
#define MAX_ITERATIONS   1000
static int opt_iterations = 1;

/* exit codes */
#define E_OK      0
#define E_ERROR   1
#define E_WARNING 2
static int exit_code = E_OK;

static int opt_stdout = 0;
static int opt_level = LEVEL_DEF;
static int opt_force = 0;
static int opt_keep = 0;
static int opt_threads;

/* 0 = quiet | 1 = normal | >1 = verbose */
static int opt_verbose = 1;
static int opt_bufsize = 0;
static int opt_timings = 0;
static int opt_nocrc = 0;

static char *progname;
static char *opt_filename;
static char *opt_suffix = SUFFIX;
static const char *errmsg = 0;

/* pointer to current infile, outfile */
static FILE *fin = NULL;
static FILE *fout = NULL;
static size_t bytes_read = 0;
static size_t bytes_written = 0;

/* when set, do not change fout */
static int global_fout = 0;

static MT_CCtx *cctx = 0;
static MT_DCtx *dctx = 0;

/* for benchmarks */
static struct timeval tms, tme, tm;

/* for -l with verbose > 1 */
static time_t mtime;
static unsigned int crc = 0;
static unsigned int crc32_table[1][256];
static unsigned int crc32(const unsigned char *buf, size_t size,
                          unsigned int crc);

static int ReadData(void *arg, MT_Buffer * in)
{

  const bufferlist *bin = (const bufferlist *)arg;
  size_t length = bin->length();
  bufferptr ptr = buffer::create_page_aligned(length);
  size_t offset = 0;
  for (auto i = bin->buffers().begin(); i != bin->buffers().end();i++) {
    const uint8_t* next_in = (uint8_t*)i->c_str();
    size_t len = i->length();
    uint8_t* next_out = (uint8_t*)ptr.c_str();
    memcpy(next_out+offset, next_in,len);
    offset += len;
  }
  in->size = length;
  in->buf = ptr.c_str();
  if (opt_mode == MODE_LIST && opt_verbose)
    //bytes_read += doe;

  return 0;
}

static int WriteData(void *arg, MT_Buffer * out)
{
  bufferlist *ob = (bufferlist *)arg;
  bufferptr ptr = buffer::create_page_aligned(out->size);
  ob->append(ptr, 0, out->size);
  return 0;
}


static const char *do_compress(const bufferlist &in, bufferlist &out)
{
  static int first = 1;
  MT_RdWr_t rdwr;
  size_t ret;

  if (first) {
//    headline();
    first = 0;
  }

  if (opt_timings && opt_verbose && opt_mode == MODE_COMPRESS)
    gettimeofday(&tms, NULL);

  /* 1) setup read/write functions */
  rdwr.fn_read = ReadData;
  rdwr.fn_write = WriteData;
  rdwr.arg_read = (void *)&in;
  rdwr.arg_write = (void *)&out;

  /* 2) create compression context */
  cctx = MT_createCCtx(opt_threads, opt_level, opt_bufsize);
  if (!cctx)
    return "Allocating compression context failed!";

  /* 3) compress */
  ret = MT_compressCCtx(cctx, &rdwr);
  if (MT_isError(ret))
    return MT_getErrorString(ret);

  /* 4) get compression statistic */
  if (opt_timings && opt_verbose && opt_mode == MODE_COMPRESS)
    fprintf(stderr, "%d;%d;%lu;%lu;%lu\n",
            opt_level, opt_threads,
            (unsigned long)MT_GetInsizeCCtx(cctx),
            (unsigned long)MT_GetOutsizeCCtx(cctx),
            (unsigned long)MT_GetFramesCCtx(cctx));

  MT_freeCCtx(cctx);

  return 0;
}

class ZstdMtCompressor : public Compressor {
 public:
    ZstdMtCompressor() : Compressor(COMP_ALG_ZSTD, "zstdmt") {}




  int compress(const bufferlist &src, bufferlist &dst) override {
    do_compress(src,dst);
    return 0;
  }

  int decompress(const bufferlist &src, bufferlist &dst) override {
    bufferlist::iterator i = const_cast<bufferlist&>(src).begin();
    return decompress(i, src.length(), dst);
  }

  int decompress(bufferlist::iterator &p,
		 size_t compressed_len,
		 bufferlist &dst) override {
    if (compressed_len < 4) {
      return -1;
    }
    compressed_len -= 4;
    uint32_t dst_len;
    decode(dst_len, p);

    bufferptr dstptr(dst_len);
    ZSTD_outBuffer_s outbuf;
    outbuf.dst = dstptr.c_str();
    outbuf.size = dstptr.length();
    outbuf.pos = 0;
    ZSTD_DStream *s = ZSTD_createDStream();
    ZSTD_initDStream(s);
    while (compressed_len > 0) {
      if (p.end()) {
	return -1;
      }
      ZSTD_inBuffer_s inbuf;
      inbuf.pos = 0;
      inbuf.size = p.get_ptr_and_advance(compressed_len,
					 (const char**)&inbuf.src);
      ZSTD_decompressStream(s, &outbuf, &inbuf);
      compressed_len -= inbuf.size;
    }
    ZSTD_freeDStream(s);

    dst.append(dstptr, 0, outbuf.pos);
    return 0;
  }
};

#endif
