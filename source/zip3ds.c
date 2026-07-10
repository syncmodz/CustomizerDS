#include "zip3ds.h"
#include "miniz.h"
#include <string.h>
#include <strings.h>
#include <stdlib.h>

/* ultimo componente do path (basename). */
static const char* baseName(const char* p) {
    const char* b = p;
    for (const char* c = p; *c; c++) if (*c == '/' || *c == '\\') b = c + 1;
    return b;
}

static bool endsWithPng(const char* name) {
    size_t l = strlen(name);
    return l > 4 && name[l-4] == '.' &&
           (name[l-3]|32) == 'p' && (name[l-2]|32) == 'n' && (name[l-1]|32) == 'g';
}

/* localiza o indice da entrada com basename == want (case-insensitive). */
static int findByBasename(mz_zip_archive* z, const char* want) {
    mz_uint n = mz_zip_reader_get_num_files(z);
    for (mz_uint i = 0; i < n; i++) {
        mz_zip_archive_file_stat st;
        if (!mz_zip_reader_file_stat(z, i, &st)) continue;
        if (st.m_is_directory) continue;
        if (strcasecmp(baseName(st.m_filename), want) == 0) return (int)i;
    }
    return -1;
}

bool zipExtract(const char* zipPath, const char* wantBasename, u8** out, u32* outSize) {
    mz_zip_archive z;
    mz_zip_zero_struct(&z);
    if (!mz_zip_reader_init_file(&z, zipPath, 0)) return false;
    int idx = findByBasename(&z, wantBasename);
    if (idx < 0) { mz_zip_reader_end(&z); return false; }
    size_t sz = 0;
    void* p = mz_zip_reader_extract_to_heap(&z, (mz_uint)idx, &sz, 0);
    mz_zip_reader_end(&z);
    if (!p) return false;
    /* miniz usa malloc por padrao -> free() normal serve pro chamador. */
    *out = (u8*)p;
    *outSize = (u32)sz;
    return true;
}

bool zipHas(const char* zipPath, const char* wantBasename) {
    mz_zip_archive z;
    mz_zip_zero_struct(&z);
    if (!mz_zip_reader_init_file(&z, zipPath, 0)) return false;
    int idx = findByBasename(&z, wantBasename);
    mz_zip_reader_end(&z);
    return idx >= 0;
}

int zipCountPng(const char* zipPath) {
    mz_zip_archive z;
    mz_zip_zero_struct(&z);
    if (!mz_zip_reader_init_file(&z, zipPath, 0)) return 0;
    int c = 0;
    mz_uint n = mz_zip_reader_get_num_files(&z);
    for (mz_uint i = 0; i < n; i++) {
        mz_zip_archive_file_stat st;
        if (!mz_zip_reader_file_stat(&z, i, &st) || st.m_is_directory) continue;
        const char* bn = baseName(st.m_filename);
        if (bn[0] != '.' && endsWithPng(bn)) c++;
    }
    mz_zip_reader_end(&z);
    return c;
}

bool zipExtractFirstPng(const char* zipPath, u8** out, u32* outSize) {
    mz_zip_archive z;
    mz_zip_zero_struct(&z);
    if (!mz_zip_reader_init_file(&z, zipPath, 0)) return false;
    bool ok = false;
    mz_uint n = mz_zip_reader_get_num_files(&z);
    for (mz_uint i = 0; i < n; i++) {
        mz_zip_archive_file_stat st;
        if (!mz_zip_reader_file_stat(&z, i, &st) || st.m_is_directory) continue;
        const char* bn = baseName(st.m_filename);
        if (bn[0] == '.' || !endsWithPng(bn)) continue;
        size_t sz = 0;
        void* p = mz_zip_reader_extract_to_heap(&z, i, &sz, 0);
        if (p) { *out = (u8*)p; *outSize = (u32)sz; ok = true; }
        break;
    }
    mz_zip_reader_end(&z);
    return ok;
}

int zipForEachPng(const char* zipPath, ZipPngCb cb, void* ud) {
    mz_zip_archive z;
    mz_zip_zero_struct(&z);
    if (!mz_zip_reader_init_file(&z, zipPath, 0)) return 0;
    int done = 0;
    mz_uint n = mz_zip_reader_get_num_files(&z);
    for (mz_uint i = 0; i < n; i++) {
        mz_zip_archive_file_stat st;
        if (!mz_zip_reader_file_stat(&z, i, &st)) continue;
        if (st.m_is_directory) continue;
        const char* bn = baseName(st.m_filename);
        if (bn[0] == '.' || !endsWithPng(bn)) continue;
        size_t sz = 0;
        void* p = mz_zip_reader_extract_to_heap(&z, i, &sz, 0);
        if (!p) continue;
        cb(bn, (const u8*)p, (u32)sz, ud);
        free(p);
        done++;
    }
    mz_zip_reader_end(&z);
    return done;
}
