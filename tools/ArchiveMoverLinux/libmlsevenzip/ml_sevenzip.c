#include <stdio.h>

#include <caml/mlvalues.h>
#include <caml/fail.h>
#include <caml/alloc.h>
#include <caml/memory.h>
#include <caml/callback.h>
#include <caml/custom.h>

#include "libsevenzip/Archive/7z/7zIn.h"
#include "libsevenzip/Archive/7z/7zAlloc.h"
#include "libsevenzip/7zCrc.h"


static ISzAlloc allocImp;
static ISzAlloc allocTempImp;

/* Sevenzip.in_file */

#define sevenzip_in_file_val(v) (*((CSzArEx *) Data_custom_val(v)))

static void finalize_sevenzip_in_file(value sevenzip)
{
  CSzArEx db = sevenzip_in_file_val(sevenzip);
  SzArEx_Free(&db, &allocImp);
}

static struct custom_operations sevenzip_in_file_ops = {
  "org.detrino.sevenzip",
  finalize_sevenzip_in_file,
  custom_compare_default,
  custom_hash_default,
  custom_serialize_default,
  custom_deserialize_default
};

static value alloc_sevenzip_in_file(CSzArEx db)
{
  CAMLparam0 ();
  CAMLlocal1 (sevenzip);
  sevenzip = alloc_custom(&sevenzip_in_file_ops, sizeof(CSzArEx), 0, 1);
  sevenzip_in_file_val(sevenzip) = db;
  CAMLreturn (sevenzip);
}

/* Sevenzip.entry */

static value alloc_sevenzip_entry(CSzArEx db, int i)
{

  CSzFileItem *f = db.db.Files + i;
  CAMLparam0 ();
  CAMLlocal5 (entry, index, name, size, is_directory);

  entry = caml_alloc (4, 0);
  index = Val_int(i);
  name = caml_copy_string(f->Name);
  size = Val_int(f->Size);
  is_directory = Val_bool(f->IsDir);

  Store_field (entry, 0, index);
  Store_field (entry, 1, name);
  Store_field (entry, 2, size);
  Store_field (entry, 3, is_directory);

  CAMLreturn (entry);
}

/* Sevenzip.init */

value ml_sevenzip_init(value unit)
{
  CAMLparam1 (unit);

  CrcGenerateTable();

  allocImp.Alloc = SzAlloc;
  allocImp.Free = SzFree;
  allocTempImp.Alloc = SzAllocTemp;
  allocTempImp.Free = SzFreeTemp;

  CAMLreturn (Val_unit);
}

/* Sevenzip.readable */

typedef struct _CFileInStream {
  ISzInStream funcs;
  value readable;
} CFileInStream;

SRes ml_sevenzip_read(void *object, void **buffer, size_t *size)
{

  CFileInStream *archive_in = (CFileInStream *) object;
  CAMLparam0 ();
  CAMLlocal5 (readable, read, tuple, ml_string, ml_size);

  readable = archive_in->readable;
  read = Field(readable, 0);
  tuple = caml_callback(read, Val_int(*size));
  ml_string = Field(tuple, 0);
  ml_size = Field(tuple, 1);
  *buffer = String_val(ml_string);
  *size = Int_val(ml_size);

  /* The GC might have moved the readable pointer */
  archive_in->readable = readable;

  CAMLreturnT(SRes, SZ_OK);
}

SRes ml_sevenzip_seek(void *object, CFileSize pos, ESzSeek origin)
{
  CFileInStream *archive_in = (CFileInStream *) object;
  CAMLparam0 ();
  CAMLlocal3 (readable, seek, term);

  readable = archive_in->readable;
  seek = Field(readable, 1);

  switch (origin) {

  case SZ_SEEK_SET: term = Val_int(0); break;
  case SZ_SEEK_CUR: term = Val_int(1); break;
  case SZ_SEEK_END: term = Val_int(2); break;

  }

  caml_callback2(seek, Val_int(pos), term);

  /* The GC might have moved the readable pointer */
  archive_in->readable = readable;

  CAMLreturnT(SRes, SZ_OK);
}

/* Sevenzip.open_readable */

value ml_sevenzip_open_readable (value readable)
{
  CAMLparam1 (readable);
  CAMLlocal1 (sevenzip);

  CFileInStream archive_in;
  CSzArEx db;
  SRes res;

  archive_in.funcs.Read = ml_sevenzip_read;
  archive_in.funcs.Seek = ml_sevenzip_seek;
  archive_in.readable = readable;

  SzArEx_Init(&db);
  res = SzArEx_Open(&db, &archive_in.funcs, &allocImp, &allocTempImp);

  switch (res) {

  case SZ_OK: break;
  case SZ_ERROR_NO_ARCHIVE: caml_failwith("NO ARCHIVE");
  case SZ_ERROR_ARCHIVE: caml_failwith("ARCHIVE");
  case SZ_ERROR_UNSUPPORTED: caml_failwith("UNSUPPORTED");
  case SZ_ERROR_MEM: caml_failwith("MEM");
  case SZ_ERROR_CRC: caml_failwith("CRC");
  case SZ_ERROR_INPUT_EOF: caml_failwith("INPUT_EOF");
  case SZ_ERROR_FAIL:
  default: caml_failwith("FAIL");

  }

  sevenzip = alloc_sevenzip_in_file(db);
  CAMLreturn (sevenzip);
}

/* Sevenzip.entries */

value ml_sevenzip_entries(value sevenzip)
{
  CAMLparam1 (sevenzip);
  CAMLlocal2 (entries, entry);
  CSzArEx db = sevenzip_in_file_val(sevenzip);

  entries = caml_alloc (db.db.NumFiles, 0);

  UInt32 i;
  for (i = 0; i < db.db.NumFiles; i++) {
    entry = alloc_sevenzip_entry(db, i);
    Store_field (entries, i, entry);
  }

  CAMLreturn (entries);
}
