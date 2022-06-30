#include "../core.h"

// #undef SINGELI

#if SINGELI
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "../singeli/gen/squeeze.c"
#pragma GCC diagnostic pop
#endif

NOINLINE B num_squeezeF(B x, usz ia) {
  u32 or = 0;
  SGetU(x)
  for (usz i = 0; i < ia; i++) {
    B cr = GetU(x,i);
    if (RARE(!q_i32(cr))) {
      while (i<ia) if (!isF64(GetU(x,i++))) return x;
      return taga(cpyF64Arr(x));
    }
    i32 c = o2iu(cr);
    or|= ((u32)c & ~1) ^ (u32)(c>>31);
  }
  
  if      (or==0)            return taga(cpyBitArr(x));
  else if (or<=(u32)I8_MAX ) return taga(cpyI8Arr (x));
  else if (or<=(u32)I16_MAX) return taga(cpyI16Arr(x));
  else                       return taga(cpyI32Arr(x));
}
B num_squeeze(B x) {
  usz ia = a(x)->ia;
  u8 xe = TI(x,elType);
  
  #if !SINGELI
  usz i = 0;
  #endif
  
  u32 or = 0; // using bitwise or as an approximate ⌈´
  switch (xe) { default: UD;
    case el_bit: goto r_x;
    #if SINGELI
      case el_i8:  { or = avx2_squeeze_i8 ((u8*)i8any_ptr (x), ia); if(or>       1) goto r_x; else goto mostBit; }
      case el_i16: { or = avx2_squeeze_i16((u8*)i16any_ptr(x), ia); if(or>  I8_MAX) goto r_x; else goto mostI8; }
      case el_i32: { or = avx2_squeeze_i32((u8*)i32any_ptr(x), ia); if(or> I16_MAX) goto r_x; else goto mostI16; }
      case el_f64: { or = avx2_squeeze_f64((u8*)f64any_ptr(x), ia); if(-1==(u32)or) goto r_x; else goto mostI32; }
    #else
      case el_i8:  { i8*  xp = i8any_ptr (x); for (; i < ia; i++) { i32 c = xp[i]; or|= (u8)c;                        } if(or>      1) goto r_x; goto mostBit; }
      case el_i16: { i16* xp = i16any_ptr(x); for (; i < ia; i++) { i32 c = xp[i]; or|= ((u32)c & ~1) ^ (u32)(c>>31); } if(or> I8_MAX) goto r_x; goto mostI8; }
      case el_i32: { i32* xp = i32any_ptr(x); for (; i < ia; i++) { i32 c = xp[i]; or|= ((u32)c & ~1) ^ (u32)(c>>31); } if(or>I16_MAX) goto r_x; goto mostI16; }
      case el_f64: {
        f64* xp = f64any_ptr(x);
        for (; i < ia; i++) {
          f64 cf = xp[i];
          i32 c = (i32)cf;
          if (c!=cf) goto r_x; // already f64
          or|= ((u32)c & ~1) ^ (u32)(c>>31);
        }
        goto mostI32;
      }
    #endif
    case el_B: case el_c8: case el_c16: case el_c32:; /*fallthrough*/
  }
  
  B* xp = arr_bptr(x);
  if (xp==NULL) goto r_f;
  
  #if SINGELI
    or = avx2_squeeze_numB((u8*)xp, ia);
    if (-2==(i32)or) goto r_x;
    if (-1==(i32)or) goto r_f64;
    goto mostI32;
  #else
    for (; i < ia; i++) {
      if (RARE(!q_i32(xp[i]))) {
        while (i<ia) if (!isF64(xp[i++])) goto r_x;
        goto r_f64;
      }
      i32 c = o2iu(xp[i]);
      or|= ((u32)c & ~1) ^ (u32)(c>>31);
    }
    goto mostI32;
  #endif
  
  mostI32: if(or>I16_MAX  ) goto r_i32;
  mostI16: if(or>I8_MAX   ) goto r_i16;
  mostI8:  if(or>0        ) goto r_i8;
  mostBit: goto r_bit;
  
  B res;
  r_f:   res = num_squeezeF(x,ia); goto retn;
  r_x:   res =                x;   goto retn;
  r_f64: res = taga(cpyF64Arr(x)); goto retn;
  r_i32: res = taga(cpyI32Arr(x)); goto retn;
  r_i16: res = taga(cpyI16Arr(x)); goto retn;
  r_i8:  res = taga(cpyI8Arr (x)); goto retn;
  r_bit: res = taga(cpyBitArr(x)); goto retn;
  retn: return FL_SET(res, fl_squoze);
}
B chr_squeeze(B x) {
  usz ia = a(x)->ia;
  u8 xe = TI(x,elType);
  usz i = 0;
  i32 or = 0;
  switch(xe) { default: UD;
    case el_c8: goto r_x;
    #if SINGELI
    case el_c16: { u32 t = avx2_squeeze_c16((u8*)c16any_ptr(x), ia); if (t==0) goto r_c8; else goto r_x; }
    case el_c32: { u32 t = avx2_squeeze_c32((u8*)c32any_ptr(x), ia); if (t==0) goto r_c8; else if (t==1) goto r_c16; else if (t==2) goto r_x; else UD; }
    #else
    case el_c16: {
      u16* xp = c16any_ptr(x);
      for (; i < ia; i++) if (xp[i] != (u8)xp[i]) goto r_x;
      goto r_c8;
    }
    case el_c32: {
      u32* xp = c32any_ptr(x);
      bool c8 = true;
      for (; i < ia; i++) {
        if (xp[i] != (u16)xp[i]) goto r_x;
        if (xp[i] != (u8 )xp[i]) c8 = false;
      }
      if (c8) goto r_c8;
      else    goto r_c16;
    }
    #endif
    case el_bit: case el_i8: case el_i16: case el_i32: case el_f64: case el_B:; /*fallthrough*/
  }
  
  B* xp = arr_bptr(x);
  if (xp!=NULL) {
    #if SINGELI
    u32 t = avx2_squeeze_chrB((u8*)xp, ia);
    if      (t==0) goto r_c8;
    else if (t==1) goto r_c16;
    else if (t==2) goto r_c32;
    else if (t==3) goto r_x;
    else UD;
    #else
    for (; i < ia; i++) {
      if (!isC32(xp[i])) goto r_x;
      or|= o2cu(xp[i]);
    }
    #endif
  } else {
    SGetU(x)
    for (; i < ia; i++) {
      B cr = GetU(x,i);
      if (!isC32(cr)) goto r_x;
      or|= o2cu(cr);
    }
  }
  if      (or<=U8_MAX ) r_c8:  return FL_SET(toC8Any(x), fl_squoze);
  else if (or<=U16_MAX) r_c16: return FL_SET(toC16Any(x), fl_squoze);
  else goto r_c32;      r_c32: return FL_SET(toC32Any(x), fl_squoze);
  /*when known typed:*/ r_x:   return FL_SET(x, fl_squoze);
}

B any_squeeze(B x) {
  assert(isArr(x));
  if (FL_HAS(x,fl_squoze)) return x;
  if (a(x)->ia==0) return FL_SET(x, fl_squoze); // TODO return a version of the smallest type
  B x0 = IGetU(x, 0);
  if (isNum(x0)) return num_squeeze(x);
  else if (isC32(x0)) return chr_squeeze(x);
  return FL_SET(x, fl_squoze);
}

B squeeze_deep(B x) {
  if (!isArr(x)) return x;
  x = any_squeeze(x);
  if (TI(x,elType)!=el_B) return x;
  usz ia = a(x)->ia;
  M_HARR(r, ia)
  B* xp = arr_bptr(x);
  B xf = getFillQ(x);
  if (xp!=NULL) {
    for (usz i=0; i<ia; i++) { HARR_ADD(r, i, squeeze_deep(inc(xp[i]))); }
  } else {
    SGet(x);
    for (usz i=0; i<ia; i++) { HARR_ADD(r, i, squeeze_deep(Get(x,i))); }
  }
  return any_squeeze(qWithFill(HARR_FCD(r, x), xf));
}