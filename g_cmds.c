/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "g_local.h"
#include "m_player.h"

static void SelectNextItem (edict_t *ent, int itflags) {
	gclient_t	*cl;
	int			i, index;
	const gitem_t		*it;

	cl = ent->client;

    if( cl->menu ) {
        PMenu_Next( ent );
        return;
    }
	if (cl->chase_target) {
		ChaseNext(ent);
	    cl->chase_mode = CHASE_NONE;
		return;
	}

	// scan  for the next valid one
	for (i=1 ; i<=ITEM_TOTAL ; i++)
	{
		index = (cl->selected_item + i)%ITEM_TOTAL;
		if (!cl->inventory[index])
			continue;
		it = INDEX_ITEM(index);
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->selected_item = index;
		return;
	}

	cl->selected_item = -1;
}

static void SelectPrevItem (edict_t *ent, int itflags) {
	gclient_t	*cl;
	int			i, index;
	const gitem_t		*it;

	cl = ent->client;

    if( cl->menu ) {
        PMenu_Prev( ent );
        return;
    }
	if (cl->chase_target) {
		ChasePrev(ent);
	    cl->chase_mode = CHASE_NONE;
		return;
	}

	// scan  for the next valid one
	for (i=1 ; i<=ITEM_TOTAL ; i++)
	{
		index = (cl->selected_item + ITEM_TOTAL - i)%ITEM_TOTAL;
		if (!cl->inventory[index])
			continue;
		it = INDEX_ITEM(index);
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->selected_item = index;
		return;
	}

	cl->selected_item = -1;
}

void ValidateSelectedItem (edict_t *ent)
{
	gclient_t	*cl;

	cl = ent->client;

	if (cl->inventory[cl->selected_item])
		return;		// valid

	SelectNextItem (ent, -1);
}


//=================================================================================

static qboolean CheckCheats( edict_t *ent ) {
	if( !sv_cheats->value ) {
		gi.cprintf( ent, PRINT_HIGH, "Cheats are disabled on this server.\n" );
		return qfalse;
	}

    return qtrue;
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (edict_t *ent)
{
	char		*name;
	gitem_t		*it;
	int			index;
	int			i;
	qboolean	give_all;
	edict_t		*it_ent;

    if( !CheckCheats( ent ) ) {
        return;
    }

	name = gi.args();

	if (Q_stricmp(name, "all") == 0)
		give_all = qtrue;
	else
		give_all = qfalse;

	if (give_all || Q_stricmp(gi.argv(1), "health") == 0)
	{
		if (gi.argc() == 3)
			ent->health = atoi(gi.argv(2));
		else
			ent->health = ent->max_health;
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		for (i=0 ; i<ITEM_TOTAL ; i++)
		{
			it = INDEX_ITEM(i);
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_WEAPON))
				continue;
			ent->client->inventory[i] += 1;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		for (i=0 ; i<ITEM_TOTAL ; i++)
		{
			it = INDEX_ITEM(i);
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_AMMO))
				continue;
			Add_Ammo (ent, it, 1000);
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		gitem_armor_t	*info;

		ent->client->inventory[ITEM_ARMOR_JACKET] = 0;

		ent->client->inventory[ITEM_ARMOR_COMBAT] = 0;

		it = INDEX_ITEM( ITEM_ARMOR_BODY );
		info = (gitem_armor_t *)it->info;
		ent->client->inventory[ITEM_ARMOR_BODY] = info->max_count;

		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "Power Shield") == 0)
	{
		it = INDEX_ITEM( ITEM_POWER_SHIELD );
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
        if( it_ent->inuse ) {
    		Touch_Item (it_ent, ent, NULL, NULL);
		    if (it_ent->inuse)
			    G_FreeEdict(it_ent);
        }

		if (!give_all)
			return;
	}

	if (give_all)
	{
		for (i=0 ; i<ITEM_TOTAL ; i++)
		{
			it = INDEX_ITEM(i);
			if (!it->pickup)
				continue;
			if (it->flags & (IT_ARMOR|IT_WEAPON|IT_AMMO))
				continue;
			ent->client->inventory[i] = 1;
		}
		return;
	}

	it = FindItem (name);
	if (!it)
	{
		name = gi.argv(1);
		it = FindItem (name);
		if (!it)
		{
			gi.cprintf (ent, PRINT_HIGH, "unknown item\n");
			return;
		}
	}

	if (!it->pickup)
	{
		gi.cprintf (ent, PRINT_HIGH, "non-pickup item\n");
		return;
	}

	index = ITEM_INDEX(it);

	if (it->flags & IT_AMMO)
	{
		if (gi.argc() == 3)
			ent->client->inventory[index] = atoi(gi.argv(2));
		else
			ent->client->inventory[index] += it->quantity;
	}
	else
	{
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
        if( it_ent->inuse ) {
    		Touch_Item (it_ent, ent, NULL, NULL);
		    if (it_ent->inuse)
			    G_FreeEdict(it_ent);
        }
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (edict_t *ent)
{
    if( CheckCheats( ent ) ) {
        ent->flags ^= FL_GODMODE;
        gi.cprintf( ent, PRINT_HIGH, "godmode %s\n",
            ( ent->flags & FL_GODMODE ) ? "ON" : "OFF" );
    }
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f (edict_t *ent)
{
    if( CheckCheats( ent ) ) {
        ent->flags ^= FL_NOTARGET;
        gi.cprintf( ent, PRINT_HIGH, "notarget %s\n",
            ( ent->flags & FL_NOTARGET ) ? "ON" : "OFF" );
    }
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f (edict_t *ent)
{
    if( !CheckCheats( ent ) ) {
        return;
    }

	if( ent->movetype == MOVETYPE_NOCLIP ) {
		ent->movetype = MOVETYPE_WALK;
	} else {
		ent->movetype = MOVETYPE_NOCLIP;
	}

	gi.cprintf( ent, PRINT_HIGH, "noclip %s\n",
        ent->movetype == MOVETYPE_NOCLIP ? "ON" : "OFF" );
}


/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
void Cmd_Use_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "Unknown item: %s\n", s);
		return;
	}
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->use (ent, it);
}


/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
void Cmd_Drop_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "Unknown item: %s\n", s);
		return;
	}
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->drop (ent, it);
}


/*
=================
Cmd_Inven_f
=================
*/
void Cmd_Inven_f (edict_t *ent) {
    extern void Cmd_Menu_f( edict_t *ent );

    // this one is left for backwards compatibility
    Cmd_Menu_f( ent );
}

/*
=================
Cmd_InvUse_f
=================
*/
void Cmd_InvUse_f (edict_t *ent)
{
	gitem_t		*it;

    if( ent->client->menu ) {
        PMenu_Select( ent );
        return;
    }

	ValidateSelectedItem (ent);

	if (ent->client->selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to use.\n");
		return;
	}

	it = INDEX_ITEM( ent->client->selected_item );
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	it->use (ent, it);
}

/*
=================
Cmd_WeapPrev_f
=================
*/
void Cmd_WeapPrev_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->weapon);

	// scan  for the next valid one
	for (i=1 ; i<=ITEM_TOTAL ; i++)
	{
		index = (selected_weapon + i)%ITEM_TOTAL;
		if (!cl->inventory[index])
			continue;
		it = INDEX_ITEM( index );
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapNext_f
=================
*/
void Cmd_WeapNext_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->weapon);

	// scan  for the next valid one
	for (i=1 ; i<=ITEM_TOTAL ; i++)
	{
        index = (selected_weapon + ITEM_TOTAL - i)%ITEM_TOTAL;
		if (!cl->inventory[index])
			continue;
		it = INDEX_ITEM( index );
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
void Cmd_WeapLast_f (edict_t *ent)
{
	gclient_t	*cl;
	int			index;
	gitem_t		*it;

	cl = ent->client;

	if (!cl->weapon || !cl->lastweapon)
		return;

	index = ITEM_INDEX(cl->lastweapon);
	if (!cl->inventory[index])
		return;
	it = INDEX_ITEM( index );
	if (!it->use)
		return;
	if (! (it->flags & IT_WEAPON) )
		return;
	it->use (ent, it);
}

/*
=================
Cmd_InvDrop_f
=================
*/
void Cmd_InvDrop_f (edict_t *ent)
{
	gitem_t		*it;

    if( ent->client->menu ) {
        PMenu_Select( ent );
        return;
    }

	ValidateSelectedItem (ent);

	if (ent->client->selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to drop.\n");
		return;
	}

	it = INDEX_ITEM( ent->client->selected_item );
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	it->drop (ent, it);
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f (edict_t *ent)
{
	if(level.framenum - ent->client->respawn_framenum < 5*HZ)
		return;
	ent->flags &= ~FL_GODMODE;
	ent->health = 0;
	meansOfDeath = MOD_SUICIDE;
	player_die (ent, ent, ent, 100000, vec3_origin);
}

/*
=================
Cmd_PutAway_f
=================
*/
void Cmd_PutAway_f (edict_t *ent) {
	ent->client->showscores = qfalse;
    PMenu_Close( ent );
}


/*
=================
Cmd_Wave_f
=================
*/
void Cmd_Wave_f (edict_t *ent)
{
	int		i;

	i = atoi (gi.argv(1));

	// can't wave when ducked
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		return;

	if (ent->client->anim_priority > ANIM_WAVE)
		return;

	ent->client->anim_priority = ANIM_WAVE;

	switch (i)
	{
	case 0:
		gi.cprintf (ent, PRINT_LOW, "flipoff\n");
		ent->s.frame = FRAME_flip01-1;
		ent->client->anim_end = FRAME_flip12;
		break;
	case 1:
		gi.cprintf (ent, PRINT_LOW, "salute\n");
		ent->s.frame = FRAME_salute01-1;
		ent->client->anim_end = FRAME_salute11;
		break;
	case 2:
		gi.cprintf (ent, PRINT_LOW, "taunt\n");
		ent->s.frame = FRAME_taunt01-1;
		ent->client->anim_end = FRAME_taunt17;
		break;
	case 3:
		gi.cprintf (ent, PRINT_LOW, "wave\n");
		ent->s.frame = FRAME_wave01-1;
		ent->client->anim_end = FRAME_wave11;
		break;
	case 4:
	default:
		gi.cprintf (ent, PRINT_LOW, "point\n");
		ent->s.frame = FRAME_point01-1;
		ent->client->anim_end = FRAME_point12;
		break;
	}
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f (edict_t *ent, qboolean team, qboolean arg0)
{
	int		i, j;
	edict_t	*other;
	char	text[150], *p;
	gclient_t *cl = ent->client;
    size_t len, total;

	if (gi.argc () < 2 && !arg0)
		return;

    if( cl->level.flags & CLF_MUTED ) {
		gi.cprintf(ent, PRINT_HIGH, "You have been muted by vote.\n" );
        return;
    }

	if (team)
		total = Q_scnprintf (text, sizeof(text), "(%s): ", cl->pers.netname);
	else
		total = Q_scnprintf (text, sizeof(text), "%s: ", cl->pers.netname);

    for (i = arg0 ? 0 : 1; i < gi.argc(); i++) {
        p = gi.argv (i);
        len = strlen (p);
        if (!len)
            continue;
        if (total + len + 1 >= sizeof (text))
            break;
        memcpy (text + total, p, len);
        text[total + len] = ' ';
        total += len + 1;
    }
    text[total] = 0;

    j = flood_msgs->value;
	if (j > 0) {
        if (level.framenum < cl->flood_locktill) {
			gi.cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n",
				( cl->flood_locktill - level.framenum ) / HZ );
            return;
        }
        i = cl->flood_whenhead - j + 1;
		if (i >= 0 && level.framenum - cl->flood_when[i % FLOOD_MSGS] < flood_persecond->value*HZ) {
            j = flood_waitdelay->value;
			cl->flood_locktill = level.framenum + j*HZ;
			gi.cprintf(ent, PRINT_CHAT,
                "Flood protection: You can't talk for %d seconds.\n", j);
            return;
        }
		cl->flood_when[++cl->flood_whenhead % FLOOD_MSGS] = level.framenum;
	}

	if (dedicated->value)
		gi.cprintf(NULL, PRINT_CHAT, "%s\n", text);

	for (j = 1; j <= game.maxclients; j++)
	{
		other = &g_edicts[j];
		if (!other->inuse)
			continue;
		if (!other->client)
			continue;
		if (team) {
            if( PLAYER_SPAWNED( ent ) != PLAYER_SPAWNED( other ) ) {
                continue;
            }
		}
		gi.cprintf(other, PRINT_CHAT, "%s\n", text);
	}
}

static void Cmd_Players_f( edict_t *ent ) {
    edict_t *other;
    gclient_t *c;
    int i, time, idle;

    gi.cprintf( ent, PRINT_HIGH,
        "id score ping time name            idle\n"
        "-- ----- ---- ---- --------------- ----\n" );

    for( i = 0; i < game.maxclients; i++ ) {
        other = &g_edicts[ i + 1 ];
		if( !( c = other->client ) ) {
            continue;
        }
        if( c->pers.connected <= CONN_CONNECTED ) {
            continue;
        }
        time = ( level.framenum - c->level.enter_framenum ) / HZ;
        idle = ( level.framenum - c->level.activity_framenum ) / HZ;
        gi.cprintf( ent, PRINT_HIGH, "%2d %5d %4d %4d %-15s %4d\n",
            i, c->resp.score, c->ping, time / 60, c->pers.netname, idle );
    }
}

edict_t *G_SetPlayer( edict_t *ent, int arg ) {
    edict_t     *other, *match;
	int			i, count;
	char		*s;

	s = gi.argv(arg);

	// numeric values are just slot numbers
	if( COM_IsUint( s ) ) {
		i = atoi(s);
		if (i < 0 || i >= game.maxclients) {
			gi.cprintf (ent, PRINT_HIGH, "Bad client slot number: %d\n", i);
			return NULL;
		}

        other = &g_edicts[ i + 1 ];
		if (!other->client || other->client->pers.connected <= CONN_CONNECTED) {
			gi.cprintf (ent, PRINT_HIGH, "Client %d is not active.\n", i);
			return NULL;
		}
		return other;
	}

	// check for a name match
    match = NULL;
    count = 0;
    for( i = 0; i < game.maxclients; i++ ) {
        other = &g_edicts[ i + 1 ];
		if (!other->client ) {
            continue;
        }
        if( other->client->pers.connected <= CONN_CONNECTED) {
            continue;
        }
		if (!Q_stricmp(other->client->pers.netname, s)) {
			return other; // exact match
		}
		if (Q_stristr(other->client->pers.netname, s)) {
            match = other; // partial match
            count++;
        }
	}

    if( !match ) {
    	gi.cprintf( ent, PRINT_HIGH, "No clients matching '%s' found.\n", s );
	    return NULL;
    }

    if( count > 1 ) {
    	gi.cprintf( ent, PRINT_HIGH, "'%s' matches multiple clients.\n", s );
	    return NULL;
    }

    return match;
}

static qboolean G_SpecRateLimited( edict_t *ent ) {
    if( level.framenum - ent->client->observer_framenum < 5*HZ ) {
		gi.cprintf( ent, PRINT_HIGH, "You may not change modes too soon.\n" );
        return qtrue;
    }
    return qfalse;
}

static void Cmd_Observe_f( edict_t *ent ) {
    if( ent->client->pers.connected == CONN_PREGAME ) {
        ent->client->pers.connected = CONN_SPECTATOR;
		gi.cprintf( ent, PRINT_HIGH, "Changed to spectator mode.\n" );
        return;
    }
    if( G_SpecRateLimited( ent ) ) {
        return;
    }
    if( ent->client->pers.connected == CONN_SPECTATOR ) {
        ent->client->pers.connected = CONN_SPAWNED;
    } else {
        ent->client->pers.connected = CONN_SPECTATOR;
    }
    spectator_respawn( ent );
}

static void Cmd_Chase_f( edict_t *ent ) {
    edict_t *target = NULL;
    chase_mode_t mode = CHASE_NONE;

    if( gi.argc() == 2 ) {
        char *who = gi.argv( 1 );

        if( !Q_stricmp( who, "quad" ) ) {
            mode = CHASE_QUAD;
        } else if( !Q_stricmp( who, "inv" ) ||
                   !Q_stricmp( who, "pent" ) )
        {
            mode = CHASE_INVU;
        } else if( !Q_stricmp( who, "top" ) || 
                   !Q_stricmp( who, "topfragger" ) ||
                   !Q_stricmp( who, "leader" ) )
        {
            mode = CHASE_LEADER;
        } else {
            target = G_SetPlayer( ent, 1 );
            if( !target ) {
                return;
            }
            if( !PLAYER_SPAWNED( target ) ) {
                gi.cprintf( ent, PRINT_HIGH,
                    "Player '%s' is not in the game.\n",
                    target->client->pers.netname);
                return;
            }
        }
    }

    // changing from pregame mode into spectator
    if( ent->client->pers.connected == CONN_PREGAME ) {
        ent->client->pers.connected = CONN_SPECTATOR;
		gi.cprintf( ent, PRINT_HIGH, "Changed to spectator mode.\n" );
        if( target ) {
            SetChaseTarget( ent, target );
        } else {
    		GetChaseTarget( ent, mode );
        }
        return;
    }

    // respawn the spectator
    if( ent->client->pers.connected != CONN_SPECTATOR ) {
        if( G_SpecRateLimited( ent ) ) {
            return;
        }
        ent->client->pers.connected = CONN_SPECTATOR;
        spectator_respawn( ent );
    }

    if( target ) {
        if( target == ent->client->chase_target ) {
	        gi.cprintf( ent, PRINT_HIGH,
                "You are already chasing this player.\n");
            return;
        }
        SetChaseTarget( ent, target );
    } else {
        if( !ent->client->chase_target || mode != CHASE_NONE ) {
            GetChaseTarget( ent, mode );
        } else {
            SetChaseTarget( ent, NULL );
        }
    }
}

static void Cmd_Join_f( edict_t *ent ) {
    switch( ent->client->pers.connected ) {
    case CONN_PREGAME:
    case CONN_SPECTATOR:
        if( G_SpecRateLimited( ent ) ) {
            return;
        }
        ent->client->pers.connected = CONN_SPAWNED;
        spectator_respawn( ent );
        break;
    case CONN_SPAWNED:
	    gi.cprintf( ent, PRINT_HIGH, "You are already in the game.\n" );
        break;
    default:
        break;
    }
}

static const char weapnames[WEAP_TOTAL][12] = {
    "None",         "Blaster",      "Shotgun",      "S.Shotgun",
    "Machinegun",   "Chaingun",     "Grenades",     "G.Launcher",
    "R.Launcher",   "H.Blaster",    "Railgun",      "BFG10K"
};

void Cmd_Stats_f( edict_t *ent, qboolean check_other ) {
    int i;
    weapstat_t *s;
    char acc[16];
    char hits[16];
    char frgs[16];
    char dths[16];
    edict_t *other;

    if( check_other && gi.argc() > 1 ) {
        other = G_SetPlayer( ent, 1 );
        if( !other ) {
            return;
        }
    } else if( ent->client->chase_target ) {
        other = ent->client->chase_target;
    } else {
        other = ent;
    }

    for( i = WEAP_SHOTGUN; i < WEAP_BFG; i++ ) {
        s = &other->client->resp.stats[i];
        if( s->atts || s->deaths ) {
            break;
        }
    }
    if( i == WEAP_BFG ) {
        gi.cprintf( ent, PRINT_HIGH, "No accuracy stats available for %s.\n",
            other->client->pers.netname );
        return;
    }

    gi.cprintf( ent, PRINT_HIGH,
        "Accuracy stats for %s:\n\n"
        "Weapon     Acc%% Hits/Atts Frgs Dths\n"
        "---------- ---- --------- ---- ----\n",
        other->client->pers.netname );

    for( i = WEAP_SHOTGUN; i < WEAP_BFG; i++ ) {
        s = &other->client->resp.stats[i];
        if( !s->atts && !s->deaths ) {
            continue;
        }
        if( s->atts ) {
            sprintf( acc, "%3i%%", s->hits * 100 / s->atts );
            sprintf( hits, "%4d/%-4d", s->hits, s->atts );
            if( s->frags ) {
                sprintf( frgs, "%4d", s->frags );
            } else {
                strcpy( frgs, "    " );
            }
        } else {
            strcpy( acc, "    " );
            strcpy( hits, "         " );
            strcpy( frgs, "    " );
        }
        if( s->deaths ) {
            sprintf( dths, "%4d", s->deaths );
        } else {
            strcpy( dths, "    " );
        }

        gi.cprintf( ent, PRINT_HIGH,
            "%-10s %s %s %s %s\n",
            weapnames[i], acc, hits, frgs, dths );
    }

    gi.cprintf( ent, PRINT_HIGH,
        "\nTotal damage given/recvd: %d/%d\n",
        other->client->resp.damage_given,
        other->client->resp.damage_recvd );
}

static void Cmd_Id_f( edict_t *ent ) {
    ent->client->pers.flags ^= CPF_NOVIEWID;

    gi.cprintf( ent, PRINT_HIGH,
        "Player identification display is now %sabled.\n",
        ( ent->client->pers.flags & CPF_NOVIEWID ) ? "dis" : "en" );
}

static void Cmd_CastVote_f( edict_t *ent, qboolean accepted ) {
    if( !level.vote.proposal ) {
        gi.cprintf( ent, PRINT_HIGH, "No vote in progress.\n" );
        return;
    }
    if( ent->client->level.vote.index == level.vote.index ) {
        gi.cprintf( ent, PRINT_HIGH, "You have already voted.\n" );
        return;
    }

    ent->client->level.vote.index = level.vote.index;
    ent->client->level.vote.accepted = accepted;

    if( G_CheckVote() ) {
        return;
    }
    
    gi.cprintf( ent, PRINT_HIGH, "Vote cast.\n" );
}

static int G_CalcVote( int *acc, int *rej ) {
    int i;
    gclient_t *client;
    int total = 0, accepted = 0, rejected = 0;

    for( i = 0, client = game.clients; i < game.maxclients; i++, client++ ) {
        if( client->pers.connected <= CONN_CONNECTED ) {
            continue;
        }
        if( client->pers.flags & CPF_MVDSPEC ) {
            continue;
        }
        total++;
        if( client->level.vote.index == level.vote.index ) {
            if( client->pers.flags & CPF_ADMIN ) {
                // admin vote decides immediately
                if( client->level.vote.accepted ) {
                    *acc = INT_MAX;
                    *rej = 0;
                } else {
                    *acc = 0;
                    *rej = INT_MAX;
                }
                return total;
            }
            if( client->level.vote.accepted ) {
                accepted++;
            } else {
                rejected++;
            }
        }
    }

    if( !total ) {
        *acc = *rej = 0;
        return 0;
    }

    *acc = accepted * 100 / total;
    *rej = rejected * 100 / total;

    return total;
}

qboolean G_CheckVote( void ) {
    int treshold = g_vote_treshold->value;
    int acc, rej;

	if( !level.vote.proposal ) {
		return qfalse;
	}

    // is vote initiator gone?
    if( !level.vote.initiator->pers.connected ) {
        gi.bprintf( PRINT_HIGH, "Vote aborted due to the initiator disconnect.\n" );
        goto finish;
    }
    
    // is vote victim gone?
    if( level.vote.victim && !level.vote.victim->pers.connected ) {
        gi.bprintf( PRINT_HIGH, "Vote aborted due to the victim disconnect.\n" );
        goto finish;
    }

    if( !G_CalcVote( &acc, &rej ) ) {
        goto finish;
    }

    if( acc > treshold || ( level.vote.initiator->pers.flags & CPF_ADMIN ) ) {
        switch( level.vote.proposal ) {
        case VOTE_TIMELIMIT:
            gi.bprintf( PRINT_HIGH, "Vote passed. Timelimit set to %d.\n", level.vote.value );
            gi.AddCommandString( va( "set timelimit %d\n", level.vote.value ) );
            break;
        case VOTE_FRAGLIMIT:
            gi.bprintf( PRINT_HIGH, "Vote passed. Fraglimit set to %d.\n", level.vote.value );
            gi.AddCommandString( va( "set fraglimit %d\n", level.vote.value ) );
            break;
        case VOTE_ITEMS:
            gi.bprintf( PRINT_HIGH, "Vote passed. New item config set.\n" );
            gi.AddCommandString( va( "set g_item_ban %d\n", level.vote.value ) );
            break;
        case VOTE_KICK:
            gi.bprintf( PRINT_HIGH, "Vote passed. Kicking %s...\n", level.vote.victim->pers.netname );
            gi.AddCommandString( va( "kick %d\n", ( int )( level.vote.victim - game.clients ) ) );
            break;
        case VOTE_MUTE:
            gi.bprintf( PRINT_HIGH, "Vote passed. Muting %s...\n", level.vote.victim->pers.netname );
            level.vote.victim->level.flags |= CLF_MUTED;
            break;
        case VOTE_MAP:
            gi.bprintf( PRINT_HIGH, "Vote passed. Next map is %s.\n", level.nextmap );
            BeginIntermission();
            break;
        default:
            break;
        }
        goto finish;
    }

    if( rej > treshold ) {
        gi.bprintf( PRINT_HIGH, "Vote failed.\n" );
        goto finish;
    }

    return qfalse;

finish:
    level.vote.proposal = 0;
    level.vote.framenum = level.framenum;
    return qtrue;
}


void G_BuildProposal( char *buffer ) {
    switch( level.vote.proposal ) {
    case VOTE_TIMELIMIT:
        sprintf( buffer, "set time limit to %d", level.vote.value );
        break;
    case VOTE_FRAGLIMIT:
        sprintf( buffer, "set frag limit to %d", level.vote.value );
        break;
    case VOTE_ITEMS: {
            int mask = ( int )g_item_ban->value ^ level.vote.value;

            //strcpy( buffer, "change item config: " );
            buffer[0] = 0;
            if( mask & ITB_QUAD ) {
                if( level.vote.value & ITB_QUAD ) {
                    strcat( buffer, "-quad " );
                } else {
                    strcat( buffer, "+quad " );
                }
            }
            if( mask & ITB_INVUL ) {
                if( level.vote.value & ITB_INVUL ) {
                    strcat( buffer, "-inv " );
                } else {
                    strcat( buffer, "+inv " );
                }
            }
            if( mask & ITB_BFG ) {
                if( level.vote.value & ITB_BFG ) {
                    strcat( buffer, "-bfg " );
                } else {
                    strcat( buffer, "+bfg " );
                }
            }
        }
        break;
    case VOTE_KICK:
        sprintf( buffer, "kick %s", level.vote.victim->pers.netname );
        break;
    case VOTE_MUTE:
        sprintf( buffer, "mute %s", level.vote.victim->pers.netname );
        break;
    case VOTE_MAP:
        sprintf( buffer, "change map to %s", level.nextmap );
        break;
    default:
        strcpy( buffer, "unknown" );
        break;
    }
}


typedef struct {
    char name[16];
    int bit;
    qboolean (*func)( edict_t * );
} vote_proposal_t;

static qboolean Vote_Timelimit( edict_t *ent ) {
    int num = atoi( gi.argv( 2 ) );

    if( num < 0 || num > 3600 ) {
        gi.cprintf( ent, PRINT_HIGH, "Timelimit %d is invalid.\n", num );
        return qfalse;
    }
    if( num == timelimit->value ) {
        gi.cprintf( ent, PRINT_HIGH, "Timelimit is already set to %d.\n", num );
        return qfalse;
    }
    level.vote.value = num;
    return qtrue;
}

static qboolean Vote_Fraglimit( edict_t *ent ) {
    int num = atoi( gi.argv( 2 ) );

    if( num < 0 || num > 999 ) {
        gi.cprintf( ent, PRINT_HIGH, "Fraglimit %d is invalid.\n", num );
        return qfalse;
    }
    if( num == fraglimit->value ) {
        gi.cprintf( ent, PRINT_HIGH, "Fraglimit is already set to %d.\n", num );
        return qfalse;
    }
    level.vote.value = num;
    return qtrue;
}


static qboolean Vote_Items( edict_t *ent ) {
    int i;
    char *s;
    int mask = g_item_ban->value;
    int bit, c;

    for( i = 2; i < gi.argc(); i++ ) {
        s = gi.argv( i );

        if( *s == '+' || *s == '-' ) {
            c = *s++;
        } else {
            c = '+';
        }

        if( !strcmp( s, "all" ) ) {
            bit = ITB_QUAD|ITB_INVUL|ITB_BFG;
        } else if( !strcmp( s, "quad" ) ) {
            bit = ITB_QUAD;
        } else if( !strcmp( s, "inv" ) || !strcmp( s, "pent" ) ) {
            bit = ITB_INVUL;
        } else if( !strcmp( s, "bfg" ) || !strcmp( s, "10k" ) ) {
            bit = ITB_BFG;
        } else {
            gi.cprintf( ent, PRINT_HIGH, "Item %s is not known.\n", s );
            return qfalse;
        }

        if( c == '-' ) {
            mask |= bit;
        } else {
            mask &= ~bit;
        }
    }

    if( mask == g_item_ban->value ) {
        gi.cprintf( ent, PRINT_HIGH, "This item config is already set.\n" );
        return qfalse;
    }

    level.vote.value = mask;
    return qtrue;
}

static qboolean Vote_Victim( edict_t *ent ) {
    edict_t *other = G_SetPlayer( ent, 2 );
    if( !other ) {
        return qfalse;
    }

    if( other == ent ) {
        gi.cprintf( ent, PRINT_HIGH, "You can't %s yourself.\n", gi.argv( 1 ) );
        return qfalse;
    }

    if( other->client->pers.flags & CPF_LOOPBACK ) {
        gi.cprintf( ent, PRINT_HIGH, "You can't %s local client.\n", gi.argv( 1 ) );
        return qfalse;
    }
    if( other->client->pers.flags & CPF_ADMIN ) {
        gi.cprintf( ent, PRINT_HIGH, "You can't %s an admin.\n", gi.argv( 1 ) );
        return qfalse;
    }

    level.vote.victim = other->client;
    return qtrue;
}

static qboolean Vote_Map( edict_t *ent ) {
    char *name = gi.argv( 2 );
    map_entry_t *map;

    map = G_FindMap( name );
    if( !map ) {
        gi.cprintf( ent, PRINT_HIGH, "Map '%s' is not available on this server.\n", name );
        return qfalse;
    }

    if( map->flags & MAP_NOVOTE ) {
        gi.cprintf( ent, PRINT_HIGH, "Map '%s' is not available for voting.\n", map->name );
        return qfalse;
    }

    strcpy( level.nextmap, map->name );
    return qtrue;
}

static const vote_proposal_t vote_proposals[] = {
    { "timelimit", VOTE_TIMELIMIT, Vote_Timelimit },
    { "tl", VOTE_TIMELIMIT, Vote_Timelimit },
    { "fraglimit", VOTE_FRAGLIMIT, Vote_Fraglimit },
    { "fl", VOTE_FRAGLIMIT, Vote_Fraglimit },
    { "items", VOTE_ITEMS, Vote_Items },
    { "kick", VOTE_KICK, Vote_Victim },
    { "mute", VOTE_MUTE, Vote_Victim },
    { "map", VOTE_MAP, Vote_Map },
    { "" }
};

static void Cmd_Vote_f( edict_t *ent ) {
    char buffer[MAX_STRING_CHARS];
    const vote_proposal_t *v;
    int mask = g_vote_mask->value;
    int limit = g_vote_limit->value;
    int treshold = g_vote_treshold->value;
    int argc = gi.argc();
    int acc, rej;
    char *s;

    if( !mask ) {
        gi.cprintf( ent, PRINT_HIGH, "Voting is disabled on this server.\n" );
        return;
    }

    if( argc < 2 ) {
        if( !level.vote.proposal ) {
            gi.cprintf( ent, PRINT_HIGH, "No vote in progress. Type 'vote help' for usage.\n" );
            return;
        }
        G_BuildProposal( buffer );
        G_CalcVote( &acc, &rej );
        gi.cprintf( ent, PRINT_HIGH,
            "Proposal   %s\n"
            "Accepted   %d%%\n"
            "Rejected   %d%%\n"
            "Treshold   %d%%\n"
            "Timeout    %d sec remaining\n"
            "Initiator  %s\n",
            buffer, acc, rej, treshold,
            ( level.vote.framenum - level.framenum ) / HZ,
            level.vote.initiator->pers.netname );
        return;
    }

    s = gi.argv( 1 );

//
// generic commands
//
    if( !strcmp( s, "help" ) || !strcmp( s, "h" ) ) {
        gi.cprintf( ent, PRINT_HIGH,
            "Usage: vote [yes/no/help/proposal] [argument]\n"
            "Available proposals:\n" );
        if( mask & VOTE_FRAGLIMIT ) {
            gi.cprintf( ent, PRINT_HIGH,
                " fraglimit/fl <frags>      Change frag limit\n" );
        }
        if( mask & VOTE_TIMELIMIT ) {
            gi.cprintf( ent, PRINT_HIGH,
                " timelimit/tl <minutes>    Change time limit\n" );
        }
        if( mask & VOTE_ITEMS ) {
            gi.cprintf( ent, PRINT_HIGH,
                " items [+|-]<quad/inv/bfg> Enable/disable items\n" );
        }
        if( mask & VOTE_KICK ) {
            gi.cprintf( ent, PRINT_HIGH,
                " kick <player_id>          Kick player from the server\n" );
        }
        if( mask & VOTE_MUTE ) {
            gi.cprintf( ent, PRINT_HIGH,
                " mute <player_id>          Disallow player to talk\n" );
        }
        if( mask & VOTE_MAP ) {
            gi.cprintf( ent, PRINT_HIGH,
                " map <name>                Change current map\n" );
        }
        gi.cprintf( ent, PRINT_HIGH,
            "Available commands:\n"
                " yes                       Accept current vote\n"
                " no                        Deny current vote\n"
                " help                      Show this help\n" );
        return;
    }
    if( !strcmp( s, "yes" ) || !strcmp( s, "y" ) ) {
        Cmd_CastVote_f( ent, qtrue );
        return;
    }
    if( !strcmp( s, "no" ) || !strcmp( s, "n" ) ) {
        Cmd_CastVote_f( ent, qfalse );
        return;
    }

//
// proposals
//
    if( level.vote.proposal ) {
        gi.cprintf( ent, PRINT_HIGH, "Vote is already in progress.\n" );
        return;
    }

    if( level.intermission_framenum ) {
        gi.cprintf( ent, PRINT_HIGH, "You may not initiate votes during intermission.\n" );
        return;
    }

    if( level.framenum - level.vote.framenum < 5*HZ ) {
        gi.cprintf( ent, PRINT_HIGH, "You may not initiate votes too soon.\n" );
        return;
    }

    if( limit > 0 && ent->client->level.vote.count >= limit ) {
        gi.cprintf( ent, PRINT_HIGH, "You may not initiate any more votes.\n" );
        return;
    }

    for( v = vote_proposals; v->bit; v++ ) {
        if( !strcmp( s, v->name ) ) {
            break;
        }
    }
    if( !v->bit ) {
        gi.cprintf( ent, PRINT_HIGH, "Unknown proposal. Type 'vote help' for usage.\n" );
        return;
    }

    if( !( mask & v->bit ) ) {
        gi.cprintf( ent, PRINT_HIGH, "Voting on '%s' is disabled.\n", v->name );
        return;
    }

    if( argc < 3 ) {
        gi.cprintf( ent, PRINT_HIGH, "Argument required for '%s'. Type 'vote help' for usage.\n", v->name );
        return;
    }

    if( !v->func( ent ) ) {
        return;
    }

    level.vote.initiator = ent->client;
    level.vote.proposal = v->bit;
    level.vote.framenum = level.framenum + g_vote_time->value * HZ;
    level.vote.index++;

    gi.bprintf( PRINT_CHAT, "%s has initiated a vote!\n",
        ent->client->pers.netname );
    ent->client->level.vote.index = level.vote.index;
    ent->client->level.vote.accepted = qtrue;
    ent->client->level.vote.count++;

    // decide vote immediately
    if( !G_CheckVote() ) {
        G_BuildProposal( buffer );
        gi.bprintf( PRINT_HIGH, "Proposal: %s\n", buffer );
    }
}

static void Cmd_Admin_f( edict_t *ent ) {
    char *p;

    if( ent->client->pers.flags & CPF_ADMIN ) {
        gi.bprintf( PRINT_HIGH, "%s is no longer an admin.\n",
            ent->client->pers.netname );
        ent->client->pers.flags &= ~CPF_ADMIN;
        return;
    }
    if( gi.argc() < 2 ) {
        gi.cprintf( ent, PRINT_HIGH, "Usage: %s <password>\n", gi.argv( 0 ) );
        return;
    }
    p = gi.argv( 1 );
    if( !g_admin_password->string[0] || strcmp( g_admin_password->string, p ) ) {
        gi.cprintf( ent, PRINT_HIGH, "Bad admin password.\n" );
        return;
    }

    ent->client->pers.flags |= CPF_ADMIN;
    gi.bprintf( PRINT_HIGH, "%s became an admin.\n",
        ent->client->pers.netname );

    G_CheckVote();
}

static qboolean become_spectator( edict_t *ent ) {
    switch( ent->client->pers.connected ) {
    case CONN_PREGAME:
        ent->client->pers.connected = CONN_SPECTATOR;
        return qtrue;
    case CONN_SPAWNED:
        if( G_SpecRateLimited( ent ) ) {
            return qfalse;
        }
        ent->client->pers.connected = CONN_SPECTATOR;
        spectator_respawn( ent );
        return qtrue;
    case CONN_SPECTATOR:
        return qtrue;
    default:
        return qfalse;
    }
}

static void select_test( edict_t *ent, pmenu_t *menu ) {
    switch( menu->cur ) {
    case 3:
        if( ent->client->pers.connected == CONN_SPAWNED ) {
            if( G_SpecRateLimited( ent ) ) {
                break;
            }
            ent->client->pers.connected = CONN_SPECTATOR;
            spectator_respawn( ent );
            break;
        }
        if( ent->client->pers.connected != CONN_PREGAME ) {
            if( G_SpecRateLimited( ent ) ) {
                break;
            }
        }
        ent->client->pers.connected = CONN_SPAWNED;
        spectator_respawn( ent );
        break;
    case 5:
        if( become_spectator( ent ) ) {
            if( ent->client->chase_target ) {
                SetChaseTarget( ent, NULL );
            }
            PMenu_Close( ent );
        }
        break;
    case 6:
        if( become_spectator( ent ) ) {
            if( !ent->client->chase_target ) {
                GetChaseTarget( ent, CHASE_NONE );
            }
            PMenu_Close( ent );
        }
        break;
    case 7:
        if( become_spectator( ent ) ) {
            GetChaseTarget( ent, CHASE_LEADER );
            PMenu_Close( ent );
        }
        break;
    case 8:
        if( become_spectator( ent ) ) {
            GetChaseTarget( ent, CHASE_QUAD );
            PMenu_Close( ent );
        }
        break;
    case 9:
        if( become_spectator( ent ) ) {
            GetChaseTarget( ent, CHASE_INVU );
            PMenu_Close( ent );
        }
        break;
    case 11:
        PMenu_Close( ent );
        break;
    }
}

static pmenu_entry_t main_menu[] = {
    { "OpenFFA - Main", PMENU_ALIGN_CENTER },
    { NULL },
    { NULL },
    { NULL, PMENU_ALIGN_LEFT, select_test },
    { NULL },
    { "*Enter freefloat mode", PMENU_ALIGN_LEFT, select_test },
    { "*Enter chasecam mode", PMENU_ALIGN_LEFT, select_test },
    { "*Autocam - Frag Leader", PMENU_ALIGN_LEFT, select_test },
    { "*Autocam - Quad Runner", PMENU_ALIGN_LEFT, select_test },
    { "*Autocam - Pent Runner", PMENU_ALIGN_LEFT, select_test },
    { NULL },
    { "*Exit menu", PMENU_ALIGN_LEFT, select_test },
//    { "*Voting menu", PMENU_ALIGN_LEFT, select_test },
    { NULL },
    { NULL },
    { NULL },
    { "Use [ and ] to move cursor", PMENU_ALIGN_CENTER },
    { "Press Enter to select", PMENU_ALIGN_CENTER },
    { "*"VERSION, PMENU_ALIGN_RIGHT }
};

void Cmd_Menu_f( edict_t *ent ) {
    if( ent->client->menu ) {
        PMenu_Close( ent );
        return;
    }

    switch( ent->client->pers.connected ) {
    case CONN_PREGAME:
    case CONN_SPECTATOR:
        main_menu[3].text = "*Enter the game";
        break;
    case CONN_SPAWNED:
        main_menu[3].text = "*Leave the game";
        break;
    default:
        return;
    }

    ent->client->showscores = qfalse;
    PMenu_Open( ent, main_menu, 0, 18, NULL );
}


/*
=================
ClientCommand
=================
*/
void ClientCommand (edict_t *ent)
{
	char	*cmd;

	if (!ent->client)
		return;		// not fully in game yet

    if( ent->client->pers.connected <= CONN_CONNECTED ) {
        return;
    }

    //ent->client->level.activity_framenum = level.framenum;

	cmd = gi.argv(0);

	if (Q_stricmp (cmd, "say") == 0) {
		Cmd_Say_f (ent, qfalse, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "say_team") == 0) {
		Cmd_Say_f (ent, qtrue, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "players") == 0 || Q_stricmp (cmd, "playerlist") == 0) {
		Cmd_Players_f (ent);
		return;
	}

	if (level.intermission_framenum)
		return;

	if (Q_stricmp (cmd, "score") == 0)
		Cmd_Score_f (ent);
	else if (Q_stricmp (cmd, "help") == 0)
		Cmd_Help_f (ent);
	else if (Q_stricmp (cmd, "use") == 0)
		Cmd_Use_f (ent);
	else if (Q_stricmp (cmd, "drop") == 0)
		Cmd_Drop_f (ent);
	else if (Q_stricmp (cmd, "give") == 0)
		Cmd_Give_f (ent);
	else if (Q_stricmp (cmd, "god") == 0)
		Cmd_God_f (ent);
	else if (Q_stricmp (cmd, "notarget") == 0)
		Cmd_Notarget_f (ent);
	else if (Q_stricmp (cmd, "noclip") == 0)
		Cmd_Noclip_f (ent);
	else if (Q_stricmp (cmd, "inven") == 0)
		Cmd_Inven_f (ent);
	else if (Q_stricmp (cmd, "invnext") == 0)
		SelectNextItem (ent, -1);
	else if (Q_stricmp (cmd, "invprev") == 0)
		SelectPrevItem (ent, -1);
	else if (Q_stricmp (cmd, "invnextw") == 0)
		SelectNextItem (ent, IT_WEAPON);
	else if (Q_stricmp (cmd, "invprevw") == 0)
		SelectPrevItem (ent, IT_WEAPON);
	else if (Q_stricmp (cmd, "invnextp") == 0)
		SelectNextItem (ent, IT_POWERUP);
	else if (Q_stricmp (cmd, "invprevp") == 0)
		SelectPrevItem (ent, IT_POWERUP);
	else if (Q_stricmp (cmd, "invuse") == 0)
		Cmd_InvUse_f (ent);
	else if (Q_stricmp (cmd, "invdrop") == 0)
		Cmd_InvDrop_f (ent);
	else if (Q_stricmp (cmd, "weapprev") == 0)
		Cmd_WeapPrev_f (ent);
	else if (Q_stricmp (cmd, "weapnext") == 0)
		Cmd_WeapNext_f (ent);
	else if (Q_stricmp (cmd, "weaplast") == 0)
		Cmd_WeapLast_f (ent);
	else if (Q_stricmp (cmd, "kill") == 0)
		Cmd_Kill_f (ent);
	else if (Q_stricmp (cmd, "putaway") == 0)
		Cmd_PutAway_f (ent);
	else if (Q_stricmp (cmd, "wave") == 0)
		Cmd_Wave_f (ent);
	else if (Q_stricmp(cmd, "observe") == 0 || Q_stricmp(cmd, "spectate") == 0 ||
             Q_stricmp(cmd, "spec") == 0 || Q_stricmp(cmd, "obs") == 0 ||
             Q_stricmp(cmd, "observer") == 0 || Q_stricmp(cmd, "spectator") == 0 )
		Cmd_Observe_f(ent);
	else if (Q_stricmp(cmd, "chase") == 0)
		Cmd_Chase_f(ent);
	else if (Q_stricmp(cmd, "join") == 0)
		Cmd_Join_f(ent);
	else if (Q_stricmp(cmd, "stats") == 0 || Q_stricmp(cmd, "accuracy") == 0)
		Cmd_Stats_f(ent, qtrue);
	else if (Q_stricmp(cmd, "id") == 0)
		Cmd_Id_f(ent);
	else if (Q_stricmp(cmd, "vote") == 0)
		Cmd_Vote_f(ent);
	else if (Q_stricmp(cmd, "yes") == 0)
		Cmd_CastVote_f(ent, qtrue);
	else if (Q_stricmp(cmd, "no") == 0)
		Cmd_CastVote_f(ent, qfalse);
	else if (Q_stricmp(cmd, "admin") == 0 || Q_stricmp(cmd, "referee") == 0)
		Cmd_Admin_f(ent);
	else if (Q_stricmp(cmd, "menu") == 0)
		Cmd_Menu_f(ent);
	else	// anything that doesn't match a command will be a chat
		Cmd_Say_f (ent, qfalse, qtrue);
}
