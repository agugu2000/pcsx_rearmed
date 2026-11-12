/*
 * (C) 2024 - Embedded cheat system for PCSX ReARMed libretro
 *
 * Parses embedded cheat database, generates core options,
 * and applies selections to the PCSX cheat engine.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libretro_cheats.h"
#include "../libpcsxcore/cheat.h"
#include "../libpcsxcore/system.h"

cheat_db_t g_cheat_db = {0};

/* External: generated cheat database */
#include "cheat_db.c"

/* The blob layout changed when the pack became bilingual; a stale
   frontend/cheat_db.c left over from an older build would be read with the
   wrong strides and produce garbage names, so refuse it outright. */
#if CHEAT_DB_FORMAT != 1
#error "frontend/cheat_db.c is stale - regenerate it with tools/gen_cheat_db.py"
#endif

/* One cheat is: cn name, en name, code count, codes. The Chinese name is
   what the emulator's cheat list stores; the English one only ever reaches
   the menu as the option sublabel, so both are read here. */
static int parse_cheats_for_game(const uint8_t *data, size_t data_len,
                                 int max_cheats, int max_codes)
{
  const uint8_t *p = data;
  const uint8_t *end = data + data_len;

  g_cheat_db.n_cheats = 0;
  g_cheat_db.n_codes = 0;

  while (p + 2 <= end) {
    uint16_t name_len = p[0] | (p[1] << 8);
    p += 2;

    if (name_len == 0)
      break; /* end of game marker */

    if (p + name_len + 2 > end)
      return -1;

    /* The generated counts should make these impossible; keep them as a
       guard against a hand-edited blob rather than a silent truncation. */
    if (g_cheat_db.n_cheats >= max_cheats)
      break;

    /* Allocate name copy */
    char *name = (char *)malloc(name_len + 1);
    if (!name) return -1;
    memcpy(name, p, name_len);
    name[name_len] = '\0';
    p += name_len;

    uint16_t name_en_len = p[0] | (p[1] << 8);
    p += 2;

    if (p + name_en_len + 2 > end) {
      free(name);
      return -1;
    }

    char *name_en = (char *)malloc(name_en_len + 1);
    if (!name_en) {
      free(name);
      return -1;
    }
    memcpy(name_en, p, name_en_len);
    name_en[name_en_len] = '\0';
    p += name_en_len;

    uint16_t code_count = p[0] | (p[1] << 8);
    p += 2;

    if (g_cheat_db.n_codes + code_count > max_codes) {
      free(name);
      free(name_en);
      break;
    }
    if (p + code_count * 6 > end) {
      free(name);
      free(name_en);
      return -1;
    }

    g_cheat_db.names[g_cheat_db.n_cheats] = name;
    g_cheat_db.names_en[g_cheat_db.n_cheats] = name_en;
    g_cheat_db.cheat_first_code[g_cheat_db.n_cheats] = g_cheat_db.n_codes;
    g_cheat_db.cheat_num_codes[g_cheat_db.n_cheats] = code_count;

    for (int j = 0; j < code_count; j++) {
      uint32_t addr = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
      p += 4;
      uint16_t val = p[0] | (p[1] << 8);
      p += 2;
      g_cheat_db.code_addrs[g_cheat_db.n_codes] = addr;
      g_cheat_db.code_values[g_cheat_db.n_codes] = val;
      g_cheat_db.n_codes++;
    }

    g_cheat_db.n_cheats++;
  }

  return 0;
}

/* Keep only alnum, upper case: this ends up in a core option key. */
static void make_game_id(const char *cdrom_id, char *out, size_t out_size)
{
  size_t n = 0;

  while (*cdrom_id && n + 1 < out_size) {
    char c = *cdrom_id++;

    if (c >= 'a' && c <= 'z')
      c -= 'a' - 'A';
    if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
      out[n++] = c;
  }
  out[n] = '\0';
}

int cheat_db_init(const char *cdrom_id)
{
  char gid[16];
  const cheat_db_index_t *entry;
  int n_cheats, n_codes, ret;

  cheat_db_free();

  if (!cdrom_id || !cdrom_id[0])
    return 0;

  /* the index is keyed on the cleaned id, CdromId itself may be lower case */
  make_game_id(cdrom_id, gid, sizeof(gid));
  entry = cheat_db_find(gid);
  if (!entry)
    return 0; /* no cheats for this game - not an error */

  /* Allocate buffers, sized from what the generator recorded for this game */
  n_cheats = (int)entry->n_cheats;
  n_codes  = (int)entry->n_codes;
  if (n_cheats <= 0 || n_codes <= 0)
    return 0;

  /* Work out where this game's data lives before allocating anything: a
     hand-edited blob with a bad offset should cost nothing and leave the
     previous game's buffers alone rather than strand them here. */
  if (entry->blob_offset >= sizeof(cheat_db_blob)) {
    SysPrintf("cheat database: %s offset past the end of the blob\n", gid);
    return -1;
  }

  const uint8_t *blob_start = cheat_db_blob + entry->blob_offset;
  const uint8_t *blob_end;
  int idx = (int)(entry - cheat_db_index);
  if (idx + 1 < CHEAT_DB_GAME_COUNT)
    blob_end = cheat_db_blob + cheat_db_index[idx + 1].blob_offset;
  else
    blob_end = cheat_db_blob + sizeof(cheat_db_blob);

  if (blob_end <= blob_start || blob_end > cheat_db_blob + sizeof(cheat_db_blob)) {
    SysPrintf("cheat database: bad range for %s\n", gid);
    return -1;
  }

  memset(&g_cheat_db, 0, sizeof(g_cheat_db));
  snprintf(g_cheat_db.game_id, sizeof(g_cheat_db.game_id), "%s", gid);
  g_cheat_db.names            = (char **)calloc(n_cheats, sizeof(char *));
  g_cheat_db.names_en         = (char **)calloc(n_cheats, sizeof(char *));
  g_cheat_db.cheat_first_code = (int *)calloc(n_cheats, sizeof(int));
  g_cheat_db.cheat_num_codes  = (int *)calloc(n_cheats, sizeof(int));
  g_cheat_db.code_addrs       = (uint32_t *)calloc(n_codes, sizeof(uint32_t));
  g_cheat_db.code_values      = (uint16_t *)calloc(n_codes, sizeof(uint16_t));
  g_cheat_db.cheat_map        = (int *)calloc(n_cheats, sizeof(int));

  if (!g_cheat_db.names || !g_cheat_db.names_en ||
      !g_cheat_db.cheat_first_code ||
      !g_cheat_db.cheat_num_codes || !g_cheat_db.code_addrs ||
      !g_cheat_db.code_values || !g_cheat_db.cheat_map) {
    cheat_db_free();
    return -1;
  }

  ret = parse_cheats_for_game(blob_start, (size_t)(blob_end - blob_start),
                              n_cheats, n_codes);
  if (ret != 0)
    SysPrintf("cheat database: damaged entry for %s\n", gid);
  return ret;
}

void cheat_db_free(void)
{
  if (g_cheat_db.names) {
    for (int i = 0; i < g_cheat_db.n_cheats; i++)
      free(g_cheat_db.names[i]);
    free(g_cheat_db.names);
  }
  if (g_cheat_db.names_en) {
    for (int i = 0; i < g_cheat_db.n_cheats; i++)
      free(g_cheat_db.names_en[i]);
    free(g_cheat_db.names_en);
  }
  /* the option keys were allocated one by one */
  if (g_cheat_db.option_defs) {
    for (int i = 0; i < (int)g_cheat_db.option_count; i++)
      free((void *)g_cheat_db.option_defs[i].key);
  }
  free(g_cheat_db.cheat_first_code);
  free(g_cheat_db.cheat_num_codes);
  free(g_cheat_db.code_addrs);
  free(g_cheat_db.code_values);
  free(g_cheat_db.cheat_map);
  free(g_cheat_db.option_defs);
  memset(&g_cheat_db, 0, sizeof(g_cheat_db));
}

static struct retro_core_option_v2_definition *
build_option_defs(void)
{
  size_t n = g_cheat_db.n_cheats;
  if (n == 0) return NULL;

  struct retro_core_option_v2_definition *defs =
    (struct retro_core_option_v2_definition *)
    calloc(n + 1, sizeof(struct retro_core_option_v2_definition));

  if (!defs) return NULL;

  for (size_t i = 0; i < n; i++) {
    struct retro_core_option_v2_definition *opt = &defs[i];

    /* Key: pcsx_rearmed_cheat_<game>_<n>, per game so values don't leak
       into another title that happens to share the index */
    char *key = (char *)malloc(64);
    if (!key) {
      /* a NULL key in the middle would end the array early and hide every
         entry after it, so drop the whole set instead */
      for (size_t j = 0; j < i; j++)
        free((void *)defs[j].key);
      free(defs);
      return NULL;
    }
    snprintf(key, 64, "pcsx_rearmed_cheat_%s_%zu", g_cheat_db.game_id, i);
    opt->key = key;

    /* Chinese name is the option label - the big line, which the menu
       scrolls when it does not fit. The English name goes to the sublabel
       underneath: smaller, dimmer, word-wrapped, never scrolled away.

       Both at once is deliberate. A translated pack is easy to get subtly
       wrong, and having the original on screen lets a player see what a
       cheat really does instead of trusting a single line of Chinese.

       Proper nouns and unit labels ("HP 9999", "Gran Turismo") are spelled
       the same in both, so they get no sublabel rather than the same text
       twice. */
    opt->desc = g_cheat_db.names[i];
    opt->desc_categorized = NULL;
    opt->info = (strcmp(opt->desc, g_cheat_db.names_en[i]) != 0)
                  ? g_cheat_db.names_en[i] : NULL;
    opt->info_categorized = NULL;
    opt->category_key = "cheat";

    /* Boolean: disabled / enabled */
    opt->values[0].value = "disabled";
    opt->values[0].label = NULL;
    opt->values[1].value = "enabled";
    opt->values[1].label = NULL;
    opt->values[2].value = NULL;
    opt->values[2].label = NULL;

    opt->default_value = "disabled";
  }

  /* Sentinel */
  defs[n].key = NULL;
  defs[n].desc = NULL;
  defs[n].category_key = NULL;
  defs[n].values[0].value = NULL;

  return defs;
}

struct retro_core_option_v2_definition *
cheat_db_get_option_defs(size_t *count)
{
  if (!count)
    return NULL;

  *count = 0;

  if (g_cheat_db.n_cheats == 0)
    return NULL;

  if (!g_cheat_db.option_defs) {
    g_cheat_db.option_defs = build_option_defs();
    /* the caller copies *count entries out of the returned array, so a
       failed allocation has to report zero as well - NULL with a non-zero
       count would be dereferenced right away */
    if (!g_cheat_db.option_defs)
      return NULL;
    g_cheat_db.option_count = g_cheat_db.n_cheats;
  }

  *count = g_cheat_db.option_count;
  return g_cheat_db.option_defs;
}

/* Format a cheat's code lines for PCSX's AddCheat function.
   PCSX expects space-separated addr-value pairs, multi-line codes
   separated by newlines. */
static char *format_codes_for_addcheat(int cheat_idx)
{
  int first = g_cheat_db.cheat_first_code[cheat_idx];
  int num   = g_cheat_db.cheat_num_codes[cheat_idx];
  int bufsz = num * 20 + 1;

  char *buf = (char *)malloc(bufsz);
  if (!buf) return NULL;

  int pos = 0;
  for (int i = 0; i < num; i++) {
    int written = snprintf(buf + pos, bufsz - pos, "%08X %04X",
                           g_cheat_db.code_addrs[first + i],
                           g_cheat_db.code_values[first + i]);
    if (written < 0) { free(buf); return NULL; }
    pos += written;
    if (i < num - 1) {
      if (pos + 1 < bufsz) { buf[pos] = '\n'; pos++; }
    }
  }
  buf[pos] = '\0';
  return buf;
}

/**
 * Add all cheats for the current game into the PCSX cheat engine,
 * all initially disabled. Called once per game load.
 */
void cheat_db_load(void)
{
  int before, ok = 0;

  if (g_cheat_db.n_cheats == 0 || !g_cheat_db.cheat_map)
    return;

  /* emu_on_new_cd() has already reset the list and pulled in any user
     cheats */
  before = NumCheats;

  /* Pre-add all cheats to PCSX engine, disabled */
  for (int i = 0; i < g_cheat_db.n_cheats; i++) {
    char *code = format_codes_for_addcheat(i);

    g_cheat_db.cheat_map[i] = -1;
    if (code) {
      /* AddCheat drops entries whose code lines it cannot use and only
         bumps NumCheats on success, so the slot is only known afterwards */
      if (AddCheat(g_cheat_db.names[i], code) == 0)
        g_cheat_db.cheat_map[i] = ok++;
      free(code);
    }
  }

  /* ApplyCheats() walks the list in order and later entries win, so move
     ours to the head: the user's own cheats then stay behind and win when
     both touch the same address */
  if (MoveCheatsToFront(before, ok) != 0) {
    /* the list kept the old order, we can no longer tell which slots are
       ours, so leave every option off instead of poking the wrong cheat */
    for (int i = 0; i < g_cheat_db.n_cheats; i++)
      g_cheat_db.cheat_map[i] = -1;
    g_cheat_db.embedded_count = 0;
    g_cheat_db.external_base  = NumCheats;
    return;
  }

  g_cheat_db.embedded_count = ok;
  g_cheat_db.external_base  = NumCheats;
}

/**
 * Read cheat option values from frontend and update
 * Cheats[i].Enabled flags. Called initially and on variable change.
 */
void cheat_db_update(retro_environment_t cb)
{
  if (g_cheat_db.n_cheats == 0 || !cb || !g_cheat_db.cheat_map)
    return;

  /* retro_cheat_reset() wipes everything, ours included, and nothing
     tells us afterwards, so rebuild the first block */
  if (NumCheats < g_cheat_db.embedded_count)
    cheat_db_load();

  for (int i = 0; i < g_cheat_db.n_cheats; i++) {
    struct retro_variable var = {0};
    char key[64];
    int idx = g_cheat_db.cheat_map[i];

    snprintf(key, sizeof(key), "pcsx_rearmed_cheat_%s_%d",
             g_cheat_db.game_id, i);
    var.key = key;

    if (!cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || !var.value)
      continue;

    if (idx >= 0 && idx < NumCheats)
      Cheats[idx].Enabled = (strcmp(var.value, "enabled") == 0);
  }
}
