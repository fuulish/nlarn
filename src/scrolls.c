/*
 * scrolls.c
 * Copyright (C) 2009, 2010 Joachim de Groot <jdegroot@web.de>
 *
 * NLarn is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NLarn is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <stdlib.h>

#include "display.h"
#include "game.h"
#include "nlarn.h"
#include "scrolls.h"

const magic_scroll_data scrolls[ST_MAX] =
{
    /* ID                   name                  effect               price obtainable */
    { ST_NONE,              "",                   ET_NONE,                 0, FALSE },
    { ST_ENCH_ARMOUR,       "enchant armour",     ET_NONE,               100,  TRUE },
    { ST_ENCH_WEAPON,       "enchant weapon",     ET_NONE,               100,  TRUE },
    { ST_ENLIGHTENMENT,     "enlightenment",      ET_ENLIGHTENMENT,      800,  TRUE },
    { ST_BLANK,             "blank paper",        ET_NONE,               100, FALSE },
    { ST_CREATE_MONSTER,    "create monster",     ET_NONE,               100, FALSE },
    { ST_CREATE_ARTIFACT,   "create artifact",    ET_NONE,               400, FALSE },
    { ST_AGGRAVATE_MONSTER, "aggravate monsters", ET_AGGRAVATE_MONSTER,  100, FALSE },
    { ST_TIMEWARP,          "time warp",          ET_NONE,               800, FALSE },
    { ST_TELEPORT,          "teleportation",      ET_NONE,               250,  TRUE },
    { ST_AWARENESS,         "expanded awareness", ET_AWARENESS,          250,  TRUE },
    { ST_SPEED,             "speed",              ET_SPEED,              250, FALSE },
    { ST_HEAL_MONSTER,      "monster healing",    ET_NONE,               100, FALSE },
    { ST_SPIRIT_PROTECTION, "spirit protection",  ET_SPIRIT_PROTECTION,  400,  TRUE },
    { ST_UNDEAD_PROTECTION, "undead protection",  ET_UNDEAD_PROTECTION,  400,  TRUE },
    { ST_STEALTH,           "stealth",            ET_STEALTH,            400,  TRUE },
    { ST_MAPPING,           "magic mapping",      ET_NONE,               250,  TRUE },
    { ST_HOLD_MONSTER,      "hold monsters",      ET_HOLD_MONSTER,       800, FALSE },
    { ST_GEM_PERFECTION,    "gem perfection",     ET_NONE,              3000, FALSE },
    { ST_SPELL_EXTENSION,   "spell extension",    ET_NONE,               800, FALSE },
    { ST_IDENTIFY,          "identify",           ET_NONE,               400,  TRUE },
    { ST_REMOVE_CURSE,      "remove curse",       ET_NONE,               250,  TRUE },
    { ST_ANNIHILATION,      "annihilation",       ET_NONE,              3000, FALSE },
    { ST_PULVERIZATION,     "pulverization",      ET_NONE,               800,  TRUE },
    { ST_LIFE_PROTECTION,   "life protection",    ET_LIFE_PROTECTION,   3000, FALSE },
    { ST_GENOCIDE_MONSTER,  "genocide monster",   ET_NONE,              3000, FALSE },
};

static int scroll_with_effect(player *p, item *scroll);
static int scroll_annihilate(player *p, item *scroll);
static int scroll_create_artefact(player *p, item *scroll);
static int scroll_enchant_armour(player *p, item *scroll);
static int scroll_enchant_weapon(player *p, item *scroll);
static int scroll_gem_perfection(player *p, item *scroll);
static int scroll_genocide_monster(player *p, item *scroll);
static int scroll_heal_monster(player *p, item *scroll);
static int scroll_identify(player *p, item *scroll);
static int scroll_remove_curse(player *p, item *scroll);
static int scroll_spell_extension(player *p, item *scroll);
static int scroll_teleport(player *p, item *scroll);
static int scroll_timewarp(player *p, item *scroll);

static const char *_scroll_desc[ST_MAX - 1] =
{
    "Ssyliir Wyleeum",
    "Etzak Biqolix",
    "Tzaqa Chanim",
    "Lanaj Lanyesaj",
    "Azayal Ixasich",
    "Assossasda",
    "Sondassasta",
    "Mindim Lanak",
    "Sudecke Chadosia",
    "L'sal Chaj Izjen",
    "Assosiala",
    "Lassostisda",
    "Bloerdadarsya",
    "Chadosia",
    "Iskim Namaj",
    "Chamote Ajaqa",
    "Lirtilsa",
    "Undim Jiskistast",
    "Lirtosiala",
    "Frichassaya",
    "Undast Kabich",
    "Fril Ajich Lsosa",
    "Chados Azil Tzos",
    "Ixos Tzek Ajak",
    "Xodil Keterulo",
};

char *scroll_desc(int scroll_id)
{
    assert(scroll_id > ST_NONE && scroll_id < ST_MAX);
    return (char *)_scroll_desc[nlarn->scroll_desc_mapping[scroll_id - 1]];
}

item_usage_result scroll_read(struct player *p, item *scroll)
{
    char description[61];
    item_usage_result result  = { FALSE, FALSE };

    item_describe(scroll, player_item_known(p, scroll),
                  TRUE, scroll->count == 1, description, 60);

    if (player_effect(p, ET_BLINDNESS))
    {
        log_add_entry(nlarn->log, "As you are blind you can't read %s.",
                      description);

        return result;
    }

    if (scroll->cursed && scroll->blessed_known)
    {
        log_add_entry(nlarn->log, "You'd rather not read this cursed scroll.");
        return result;
    }

    log_add_entry(nlarn->log, "You read %s.", description);

    /* try to complete reading the scroll */
    if (!player_make_move(p, 2, TRUE, "reading %s", description))
    {
        /* the action has been aborted */
        return result;
    }

    /* the scroll has successfully been read */
    result.used_up = TRUE;

    /* increase number of scrolls read */
    p->stats.scrolls_read++;

    if (scroll->cursed)
    {
        damage *dam = damage_new(DAM_FIRE, ATT_NONE, rand_1n(p->hp), NULL);
        log_add_entry(nlarn->log, "The scroll explodes!");
        player_damage_take(p, dam, PD_CURSE, scroll->type);

        return result;
    }

    switch (scroll->id)
    {
    case ST_ENCH_ARMOUR:
        result.identified = scroll_enchant_armour(p, scroll);
        break;

    case ST_ENCH_WEAPON:
        result.identified = scroll_enchant_weapon(p, scroll);
        break;

    case ST_BLANK:
        result.used_up = FALSE;
        result.identified = TRUE;
        log_add_entry(nlarn->log, "This scroll is blank.");
        break;

    case ST_CREATE_MONSTER:
        result.identified = spell_create_monster(p);
        break;

    case ST_CREATE_ARTIFACT:
        result.identified = scroll_create_artefact(p, scroll);
        break;

    case ST_TIMEWARP:
        result.identified = scroll_timewarp(p, scroll);
        break;

    case ST_TELEPORT:
        result.identified = scroll_teleport(p, scroll);
        break;

    case ST_HEAL_MONSTER:
        result.identified = scroll_heal_monster(p, scroll);
        break;

    case ST_MAPPING:
        log_add_entry(nlarn->log, "There is a map on the scroll!");
        result.identified = scroll_mapping(p, scroll);
        break;

    case ST_GEM_PERFECTION:
        result.identified = scroll_gem_perfection(p, scroll);
        break;

    case ST_SPELL_EXTENSION:
        result.identified = scroll_spell_extension(p, scroll);
        break;

    case ST_IDENTIFY:
        result.identified = scroll_identify(p, scroll);
        break;

    case ST_REMOVE_CURSE:
        result.identified = scroll_remove_curse(p, scroll);
        break;

    case ST_ANNIHILATION:
        result.identified = scroll_annihilate(p, scroll);
        break;

    case ST_PULVERIZATION:
        if (!p->identified_scrolls[ST_PULVERIZATION])
        {
            log_add_entry(nlarn->log, "This is a scroll of %s. ",
                          scroll_name(scroll));
        }

        if (spell_vaporize_rock(p))
        {
            /* recalc fov if something has been vaporised */
            player_update_fov(p);
        }

        result.identified = TRUE;
        break;

    case ST_GENOCIDE_MONSTER:
        scroll_genocide_monster(p, scroll);
        result.identified = TRUE;
        break;

    default:
        result.identified = scroll_with_effect(p, scroll);
        break;
    }

    if (!result.identified)
    {
        log_add_entry(nlarn->log, "Nothing happens.");
    }

    return result;
}

static int scroll_with_effect(struct player *p, item *scroll)
{
    effect *eff;

    assert(p != NULL && scroll != NULL);

    eff = effect_new(scroll_effect(scroll));
    // Blessed scrolls last longer.
    if (scroll->blessed)
    {
        // Life protection is permanent.
        if (scroll->id == ST_LIFE_PROTECTION)
            eff->turns = 0;
        else
            eff->turns *= 2;
    }

    eff = player_effect_add(p, eff);

    if (eff && !effect_get_msg_start(eff))
    {
        return FALSE;
    }

    return TRUE;
}

/**
 * Scroll "annihilation".
 *
 * @param the player
 * @param the scroll just read
 *
 */
static int scroll_annihilate(struct player *p, item *scroll)
{
    int count = 0;
    area *blast, *obsmap;
    position cursor = p->pos;
    monster *m;
    map *cmap = game_map(nlarn, p->pos.z);

    assert(p != NULL && scroll != NULL);

    obsmap = map_get_obstacles(cmap, p->pos, 2);
    blast = area_new_circle_flooded(p->pos, 2, obsmap);

    for (cursor.y = blast->start_y; cursor.y < blast->start_y + blast->size_y; cursor.y++)
    {
        for (cursor.x = blast->start_x; cursor.x < blast->start_x + blast->size_x; cursor.x++)
        {
            if (area_pos_get(blast, cursor) && (m = map_get_monster_at(cmap, cursor)))
            {
                if (monster_flags(m, MF_DEMON))
                {
                    m = monster_damage_take(m, damage_new(DAM_MAGICAL, ATT_NONE, 2000, p));

                    /* check if the monster has been killed */
                    if (!m) count++;
                }
                else
                {
                    log_add_entry(nlarn->log, "The %s barely escapes being annihilated.",
                                  monster_get_name(m));

                    /* lose half hit points */
                    damage *dam = damage_new(DAM_MAGICAL, ATT_NONE, monster_hp(m) / 2, p);
                    monster_damage_take(m, dam);
                }
            }
        }
    }

    area_destroy(blast);

    if (count)
    {
        log_add_entry(nlarn->log, "You hear loud screams of agony!");
    }

    return count;
}

static int scroll_create_artefact(player *p, item *scroll)
{
    item *it;
    char buf[61];

    assert(p != NULL && scroll != NULL);

    const int magic_item_type[] = { IT_AMULET, IT_BOOK, IT_POTION, IT_SCROLL };
    const int type = magic_item_type[rand_0n(4)];

    int level = p->pos.z;
    if (scroll->blessed)
    {
        if (type == IT_AMULET)
            level = max(10, p->pos.z);
        else
            level = rand_m_n(p->pos.z, MAP_MAX);
    }

    it = item_new_by_level(type, level);

    it->cursed = FALSE;
    if (it->bonus < 0)
        it->bonus = 0;

    // Blessed scrolls tend to hand out better items.
    if (scroll->blessed)
    {
        // Rings get some kind of bonus.
        if (it->bonus < 1 && item_is_optimizable(it->type))
            it->bonus = rand_1n(3);

        it->burnt    = 0;
        it->corroded = 0;
        it->rusty    = 0;

        if (it->type == IT_POTION || it->type == IT_SCROLL)
        {
            // The low-price, unobtainable items are more likely to be bad.
            while (item_base_price(it) < 200
                        && !item_obtainable(it->type, it->id))
            {
                it->id = rand_1n(item_max_id(it->type));
            }
            // holy water should always be blessed
            if (it->type == IT_POTION && it->id == PO_WATER && !it->blessed)
                it->blessed = TRUE;
        }
        else if (it->type == IT_BOOK)
        {
            // Roll again for an unknown, reasonably high-level book.
            while ((spell_known(p, it->id) && chance(80))
                    || (it->id < (item_max_id(IT_BOOK) * level) / (MAP_MAX - 1)
                            && chance(50)))
            {
                it->id = rand_1n(item_max_id(it->type));
            }
        }
    }
    inv_add(map_ilist_at(game_map(nlarn, p->pos.z), p->pos), it);

    item_describe(it, player_item_known(p, it), (it->count == 1),
                  FALSE, buf, 60);

    log_add_entry(nlarn->log, "You find %s below your feet.", buf);

    return TRUE;
}

/**
 * Scroll "enchant armour".
 *
 * @param the player
 * @param the scroll just read
 *
 */
static int scroll_enchant_armour(player *p, item *scroll)
{
    item **armour;

    assert(p != NULL && scroll != NULL);

    /* get a random piece of armour to enchant */
    /* try to get one not already fully enchanted */
    if ((armour = player_get_random_armour(p, TRUE)))
    {
        if (scroll->blessed)
        {
            log_add_entry(nlarn->log, "Your %s glows brightly for a moment.",
                          armour_name(*armour));

            (*armour)->rusty = FALSE;
            (*armour)->burnt = FALSE;
            (*armour)->corroded = FALSE;
            if ((*armour)->bonus < 0)
            {
                (*armour)->bonus = 0;
                if (chance(50))
                    return TRUE;
            }
            /* 50% chance of bonus enchantment */
            else if (chance(50) && (*armour)->bonus < 3)
                item_enchant(*armour);
        }
        else
        {
            log_add_entry(nlarn->log, "Your %s glows for a moment.",
                          armour_name(*armour));
        }

        /* blessed scroll never overenchants */
        if (!scroll->blessed || (*armour)->bonus < 3)
            item_enchant(*armour);

        return TRUE;
    }

    return FALSE;
}

static int scroll_enchant_weapon(player *p, item *scroll)
{
    assert(p != NULL && scroll != NULL);

    if (p->eq_weapon)
    {
        if (scroll->blessed)
        {
            log_add_entry(nlarn->log,
                          "Your %s glows brightly for a moment.",
                          weapon_name(p->eq_weapon));

            p->eq_weapon->rusty = FALSE;
            p->eq_weapon->burnt = FALSE;
            p->eq_weapon->corroded = FALSE;
            if (p->eq_weapon->bonus < 0)
            {
                p->eq_weapon->bonus = 0;
                return TRUE;
            }
        }
        else
        {
            log_add_entry(nlarn->log,
                          "Your %s glisters for a moment.",
                          weapon_name(p->eq_weapon));
        }

        /* blessed scroll never overenchants */
        if (!scroll->blessed || p->eq_weapon->bonus < 3)
            item_enchant(p->eq_weapon);

        return TRUE;
    }

    return FALSE;
}

static int scroll_gem_perfection(player *p, item *scroll)
{
    guint idx;
    item *it;

    assert(p != NULL && scroll != NULL);

    if (inv_length_filtered(p->inventory, item_filter_gems) == 0)
    {
        return FALSE;
    }

    log_add_entry(nlarn->log, "This is a scroll of gem perfection.");

    if (scroll->blessed)
    {
        for (idx = 0; idx < inv_length_filtered(p->inventory, item_filter_gems); idx++)
        {
            it = inv_get_filtered(p->inventory, idx, item_filter_gems);
            /* double gem value */
            it->bonus <<= 1;
        }
        log_add_entry(nlarn->log, "You bring all your gems to perfection.");
    }
    else
    {
        it = display_inventory("Choose a gem to make perfect", p, &p->inventory, NULL,
                               FALSE, FALSE, FALSE, item_filter_gems);

        if (it)
        {
            char desc[81];
            item_describe(it, TRUE, it->count == 1, TRUE, desc, 80);
            log_add_entry(nlarn->log, "You make %s perfect.", desc);

            /* double gem value */
            it->bonus <<= 1;
        }
    }

    return TRUE;
}

static int scroll_genocide_monster(player *p, item *scroll)
{
    char *in;
    int id;
    GString *msg;

    assert(p != NULL);

    msg = g_string_new(NULL);

    if (!p->identified_scrolls[ST_GENOCIDE_MONSTER])
    {
        g_string_append_printf(msg, "This is a scroll of %s. ",
                               scroll_name(scroll));
    }

    g_string_append(msg, "Which monster do you want to genocide (type letter)?");

    in = display_get_string(msg->str, NULL, 1);

    g_string_free(msg, TRUE);

    if (!in)
    {
        log_add_entry(nlarn->log, "You chose not to genocide any monster.");
        return FALSE;
    }

    guint32 candidates[10];
    int found = 0;
    for (id = 1; id < MT_MAX; id++)
    {
        if (monster_type_image(id) == in[0])
        {
            if (!monster_is_genocided(id))
            {
                candidates[found] = id;
                if (found++ == 10)
                    break;
            }
        }
    }
    g_free(in);

    if (found == 0)
    {
        log_add_entry(nlarn->log, "No such monster.");
        return FALSE;
    }

    /* blessed scrolls allow a choice of same-glyph monsters */
    int which = MT_NONE;
    if (!scroll->blessed || found == 1)
        which = candidates[0];
    else
    {
        GString *text = g_string_new("");

        for (id = 0; id < found; id++)
        {
            g_string_append_printf(text, "  %c) %-30s\n",
                                   id + 'a', monster_type_name(candidates[id]));
        }

        do
        {
            which = display_show_message("Genocide which monster?",
                                         text->str, 0);
        }
        while (which < 'a' || which >= found + 'a');

        g_string_free(text, TRUE);

        which -= 'a';
        assert(which >= 0 && which < found);
        which = candidates[which];
    }

    monster_genocide(which);
    log_add_entry(nlarn->log, "Wiped out all %s.",
                  monster_type_plural_name(which, 2));

    if (which == MT_TOWN_PERSON) // Oops!
        player_die(p, PD_GENOCIDE, 0);

    return TRUE;
}

static int scroll_heal_monster(player *p, item *scroll)
{
    GList *mlist;
    int count = 0;
    monster *m;

    assert(p != NULL && scroll != NULL);

    mlist = g_hash_table_get_values(nlarn->monsters);

    do
    {
        m = (monster *)mlist->data;
        position mpos = monster_pos(m);

        /* find monsters on the same level */
        if (mpos.z == p->pos.z)
        {
            if (monster_hp(m) < monster_hp_max(m))
            {
                monster_hp_inc(m, monster_hp_max(m));
                count++;
            }
        }
    }
    while ((mlist = mlist->next));

    if (count > 0)
    {
        log_add_entry(nlarn->log, "You feel uneasy.");
    }

    g_list_free(mlist);

    return count;
}

static int scroll_identify(player *p, item *scroll)
{
    item *it;

    assert(p != NULL && scroll != NULL);

    if (inv_length_filtered(p->inventory, item_filter_unid) == 0)
    {
        /* player has no unidentfied items */
        log_add_entry(nlarn->log, "Nothing happens.");
        return FALSE;
    }

    log_add_entry(nlarn->log, "This is a scroll of identify.");

    if (scroll->blessed)
    {
        /* identify all items */
        log_add_entry(nlarn->log, "You identify your possessions.");

        while (inv_length_filtered(p->inventory, item_filter_unid))
        {
            it = inv_get_filtered(p->inventory, 0, item_filter_unid);
            player_item_identify(p, NULL, it);
        }
    }
    else
    {
        /* identify the scroll being read, otherwise it would show up here */
        player_item_identify(p, NULL, scroll);

        /* may identify up to 3 items */
        int tries = rand_1n(4);
        while (tries-- > 0)
        {
            /* choose a single item to identify */
            it = display_inventory("Choose an item to identify", p, &p->inventory,
                                   NULL, FALSE, FALSE, FALSE, item_filter_unid);

            if (it == NULL)
                break;

            char desc[81];

            item_describe(it, FALSE, (it->count == 1), TRUE, desc, 80);
            log_add_entry(nlarn->log, "You identify %s.", desc);
            player_item_identify(p, NULL, it);

            item_describe(it, TRUE, (it->count == 1), FALSE, desc, 80);
            log_add_entry(nlarn->log, "%s %s.", (it->count > 1) ? "These are" :
                          "This is", desc);

            if (inv_length_filtered(p->inventory, item_filter_unid) == 0)
                break;
        }
    }

    return TRUE;
}

int scroll_mapping(player *p, item *scroll)
{
    position pos;
    map *m;

    /* scroll can be null as I use this to fake a known level */
    assert(p != NULL);

    m = game_map(nlarn, p->pos.z);
    pos.z = p->pos.z;

    const gboolean map_traps = (scroll != NULL && scroll->blessed);

    for (pos.y = 0; pos.y < MAP_MAX_Y; pos.y++)
    {
        for (pos.x = 0; pos.x < MAP_MAX_X; pos.x++)
        {
            map_tile_t tile = map_tiletype_at(m, pos);
            if (scroll == NULL || tile != LT_FLOOR)
                player_memory_of(p, pos).type = tile;
            player_memory_of(p, pos).sobject = map_sobject_at(m, pos);

            if (map_traps)
            {
                trap_t trap = map_trap_at(m, pos);
                if (trap)
                    player_memory_of(p, pos).trap = trap;
            }

        }
    }

    return TRUE;
}

static int scroll_remove_curse(player *p, item *scroll)
{
    char buf[61];
    item *it;

    assert(p != NULL && scroll != NULL);

    if (inv_length_filtered(p->inventory, item_filter_cursed) == 0)
    {
        /* player has no cursed items */
        log_add_entry(nlarn->log, "Nothing happens.");
        return FALSE;
    }

    log_add_entry(nlarn->log, "This is a scroll of remove curse.");

    if (scroll->blessed)
    {
        /* remove curses on all items */
        log_add_entry(nlarn->log, "You remove curses on your possessions.");

        while (inv_length_filtered(p->inventory, item_filter_cursed) > 0)
        {
            it = inv_get_filtered(p->inventory, 0, item_filter_cursed);

            // Get the description before uncursing the item.
            item_describe(it, player_item_known(p, it),
                          FALSE, TRUE, buf, 60);

            buf[0] = g_ascii_toupper(buf[0]);
            log_add_entry(nlarn->log, "%s glow%s in a white light.",
                          buf, it->count == 1 ? "s" : "");

            item_remove_curse(it);
        }
    }
    else
    {
        /* choose a single item to uncurse */
        it = display_inventory("Choose an item to uncurse", p, &p->inventory,
                               NULL, FALSE, FALSE, FALSE,
                               item_filter_cursed_or_unknown);

        if (it != NULL)
        {
            if (!it->cursed)
            {
                log_add_entry(nlarn->log, "Nothing happens.");
                return TRUE;
            }
            // Get the description before uncursing the item.
            item_describe(it, player_item_known(p, it),
                          FALSE, TRUE, buf, 60);

            buf[0] = g_ascii_toupper(buf[0]);

            log_add_entry(nlarn->log, "%s glow%s in a white light.",
                          buf, it->count == 1 ? "s" : "");

            item_remove_curse(it);
        }
    }

    return TRUE;
}

static int scroll_spell_extension(player *p, item *scroll)
{
    guint idx;
    spell *sp;

    assert(p != NULL && scroll != NULL);

    for (idx = 0; idx < p->known_spells->len; idx++)
    {
        sp = g_ptr_array_index(p->known_spells, idx);

        if (scroll->blessed)
        {
            /* double spell knowledge */
            sp->knowledge <<=1;
        }
        else
        {
            /* increase spell knowledge */
            sp->knowledge++;
        }

    }

    /* give a message if any spell has been extended */
    if (p->known_spells->len > 0)
    {
        log_add_entry(nlarn->log, "You feel your magic skills improve.");
        return TRUE;
    }

    return FALSE;
}

static int scroll_teleport(player *p, item *scroll)
{
    guint nlevel;

    assert(p != NULL);

    if (p->pos.z == 0)
        nlevel = 0;
    else if (p->pos.z < MAP_DMAX)
        nlevel = rand_0n(MAP_DMAX);
    else
        nlevel = rand_m_n(MAP_DMAX, MAP_MAX);

    if (nlevel != p->pos.z)
    {
        player_map_enter(p, game_map(nlarn, nlevel), TRUE);
        return TRUE;
    }

    return FALSE;
}

static int scroll_timewarp(player *p, item *scroll)
{
    gint32 mobuls = 0;  /* number of mobuls */
    gint32 turns;       /* number of turns */
    int idx = 0;        /* position inside player's effect list */

    assert(p != NULL && scroll != NULL);

    turns = (rand_1n(1000) - 850);

    // For blessed scrolls, use the minimum turn count of three tries.
    if (scroll->blessed)
    {
        turns = min(turns, (rand_1n(1000) - 850));
        turns = min(turns, (rand_1n(1000) - 850));
    }

    if (turns == 0)
        turns = 1;

    if ((gint32)(game_turn(nlarn) + turns) < 0)
    {
        turns = 1 - game_turn(nlarn);
    }

    mobuls = gtime2mobuls(turns);

    /* rare case that time has not been modified */
    if (!mobuls)
    {
        return FALSE;
    }

    game_turn(nlarn) += turns;
    log_add_entry(nlarn->log,
                  "You go %sward in time by %d mobul%s.",
                  (mobuls < 0) ? "back" : "for",
                  abs(mobuls),
                  (abs(mobuls) == 1) ? "" : "s");

    /* adjust effects for time warping */
    while (idx < p->effects->len)
    {
        /* loop over all effects which affect the player */
        effect *e = game_effect_get(nlarn, g_ptr_array_index(p->effects, idx));

        if (e->turns == 0)
        {
            /* leave permanent effects alone */
            idx++;
            continue;
        }

        if (turns > 0)
        {
            /* gone forward in time */
            if (e->turns < turns)
            {
                /* the effect's remaining turns are smaller
                   than the number of turns the player moved into the future,
                   thus the effect is no longer valid. */
                player_effect_del(p, e);
            }
            else
            {
                /* reduce the number of remaining turns for this effect */
                e->turns -= turns;

                /* proceed to next effect */
                idx++;
            }
        }
        else if (turns < 0)
        {
            /* gone backward in time */
            if (e->start > game_turn(nlarn))
            {
                /* the effect started after the new game turn,
                   thus it is no longer (in fact not yet) valid */
                player_effect_del(p, e);
            }
            else
            {
                /* increase the number of remaining turns */
                e->turns += abs(turns);

                /* proceed to next effect */
                idx++;
            }
        }
    }

    return TRUE;
}
