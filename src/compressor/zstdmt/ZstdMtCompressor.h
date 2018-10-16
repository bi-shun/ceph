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

#ifndef CEPH_ZSTDMTCOMPRESSOR_H
#define CEPH_ZSTDMTCOMPRESSOR_H
#define ZSTD_STATIC_LINKING_ONLY
#include "zstd/lib/zstd.h"
//
#include "include/buffer.h"
#include "include/encoding.h"
#include "compressor/Compressor.h"
#include "lib/zstd-mt.h"
#include <thread>
#include "platform.h"
class ZstdMtCompressor : public Compressor {
 public:
  ZstdMtCompressor() : Compressor(COMP_ALG_ZSTDMT, "zstdmt") {}
  int compress(const bufferlist &src, bufferlist &dst) override {
    ZSTDMT_CCtx *cctx = ZSTDMT_createCCtx(4, 5, 0);
    if (cctx == nullptr) {
      return -1;
    }
    ZSTDMT_RdWr_t rdwr;
    auto i = src.begin();
    rdwr.fn_read = [](void *arg, ZSTDMT_Buffer *in){
      bufferlist::const_iterator* src = static_cast<bufferlist::const_iterator*>(arg);
      size_t remain = std::min<size_t>(in->size,(size_t)src->get_remaining());
      bufferptr new_ptr = buffer::create_small_page_aligned(remain);
      unsigned int offset = 0;
      do {
        const void* tmp;
        size_t size = src->get_ptr_and_advance(remain,(const char**)&tmp);
        if (size == 0)
          break;
        new_ptr.copy_in(offset, size,(const char*)tmp);
        offset += size;
        remain -= size;
      } while (remain != 0);
      new_ptr.copy_out(0,new_ptr.length(),(char *)in->buf);
      in->size = new_ptr.length();
      return 0;
    };
    rdwr.fn_write = [](void *arg, ZSTDMT_Buffer * out){
      bufferlist *dst = static_cast<bufferlist *>(arg);
      bufferptr outptr = buffer::create_small_page_aligned(out->size);
      outptr.copy_in(0,out->size,(const char *)out->buf);
      dst->append(outptr, 0, out->size);
      return 0;
    };
    rdwr.arg_read = const_cast<bufferlist::const_iterator*>(&i);
    rdwr.arg_write = &dst;
    auto ret = ZSTDMT_compressCCtx(cctx, &rdwr);
    if (ZSTDMT_isError(ret))
      return -1;

    ZSTDMT_freeCCtx(cctx);
    return 0;
  }

  int decompress(const bufferlist &src, bufferlist &dst) override {
    auto i = std::cbegin(src);
    return decompress(i, src.length(), dst);
  }
  typedef struct {
    bufferlist::const_iterator *p;
    size_t compressed_len;
  } DeReadArg;

  int decompress(bufferlist::const_iterator &p,
                 size_t compressed_len,
                 bufferlist &dst) override {

    auto dctx = ZSTDMT_createDCtx(4, 0);
    if (!dctx)
      return -1;
    ZSTDMT_RdWr_t rdwr;
    DeReadArg args;
    args.p = const_cast<bufferlist::const_iterator*>(&p);
    args.compressed_len = compressed_len;
    rdwr.fn_read = [](void *arg, ZSTDMT_Buffer *in){
      DeReadArg *args = static_cast<DeReadArg*>(arg);
      bufferlist::const_iterator* src = args->p;
      size_t remain = std::min<size_t>(in->size,(size_t)args->compressed_len);
      args->compressed_len -= remain;
      bufferptr new_ptr = buffer::create_small_page_aligned(remain);
      unsigned int offset = 0;
      do {
        const void* tmp;
        size_t size = src->get_ptr_and_advance(remain,(const char**)&tmp);
        if (size == 0)
          break;
        new_ptr.copy_in(offset, size,(const char*)tmp);
        offset += size;
        remain -= size;
      } while (remain != 0);
      new_ptr.copy_out(0,new_ptr.length(),(char *)in->buf);
      in->size = new_ptr.length();
      return 0;
    };
    rdwr.fn_write =  [](void *arg, ZSTDMT_Buffer * out){
      bufferlist *dst = static_cast<bufferlist *>(arg);
      bufferptr outptr = buffer::create_small_page_aligned(out->size);
      outptr.copy_in(0,out->size,(const char *)out->buf);
      dst->append(outptr, 0, out->size);
      return 0;
    };
    rdwr.arg_read = &args;
    rdwr.arg_write = &dst;
    auto ret = ZSTDMT_decompressDCtx(dctx, &rdwr);
    if (ZSTDMT_isError(ret))
      return -1;
    return 0;

  }
};


#endif
