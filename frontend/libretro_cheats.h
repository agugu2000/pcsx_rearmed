/*
 * (C) 2024 - Embedded cheat system for PCSX ReARMed libretro
 *
 * Dynamically generates core options from embedded cheat database.
 * Patterned after FBNeo's cheat core option system.
 */

#ifndef __LIBRETRO_CHEATS_H__
#define __LIBRETRO_CHEATS_H__

#include <stdint.h>
#include <stddef.h>
#include "libretro.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Parsed cheat data for a single game.

   All buffers are sized from the counts stored in the generated index
   (cheat_db_index_t::n_cheats / n_codes), so a different source pack needs
   no matching constants on this side. */
typedef struct {
  struct retro_core_option_v2_definition *option_defs; /* dynamic core options */
  size_t option_count;

  char   game_id[16];    /* cleaned CdromId, keeps core option keys per game */
  int    embedded_count; /* our entries that actually made it into the list */
  int    external_base;  /* first Cheats[] slot the frontend may use */
  int   *cheat_map;      /* option index -> Cheats[] index, -1 when rejected */

  /* Raw data for applying cheats */
  char   **names;        /* cheat names, Chinese: the option label */
  char   **names_en;     /* original names, English: the option sublabel */
  int      n_cheats;
  int      n_codes;
  int     *cheat_first_code;  /* first code index for each cheat */
  int     *cheat_num_codes;   /* num codes for each cheat */
  uint32_t *code_addrs;       /* all code addresses */
  uint16_t *code_values;      /* all code values */
} cheat_db_t;

/* Global cheat state */
extern cheat_db_t g_cheat_db;

/**
 * Initialize cheat system for a game.
 * @param cdrom_id  The PS1 game CdromId (e.g. "SLUS12345")
 * @return 0 on success (even if no cheats found), -1 on error
 */
int cheat_db_init(const char *cdrom_id);

/**
 * Free all cheat resources.
 */
void cheat_db_free(void);

/**
 * Generate core option definitions for the current game's cheats.
 * The returned pointer is owned by g_cheat_db, do not free.
 * @param count  Output: number of option definitions
 * @return Array of retro_core_option_v2_definition, or NULL
 */
struct retro_core_option_v2_definition *
cheat_db_get_option_defs(size_t *count);

/**
 * Add all cheats for the current game into the PCSX cheat engine.
 * All cheats are initially disabled.
 * Should be called once per game load, after cheat_db_init().
 */
void cheat_db_load(void);

/**
 * Read cheat variable values from frontend and update
 * PCSX Cheats[i].Enabled flags accordingly.
 * Should be called after cheat_db_load() and on every variable change.
 */
void cheat_db_update(retro_environment_t cb);

#ifdef __cplusplus
}
#endif

#endif /* __LIBRETRO_CHEATS_H__ */
