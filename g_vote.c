/*
Copyright (C) 2008-2009 Andrey Nazarov

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

static int G_CalcVote( int *acc, int *rej ) {
    gclient_t *c;
    int total = 0, votes[2] = { 0, 0 };

    for( c = game.clients; c < game.clients + game.maxclients; c++ ) {
        if( c->pers.connected <= CONN_CONNECTED ) {
            continue;
        }
        if( c->pers.flags & CPF_MVDSPEC ) {
            continue;
        }
        if( (int)g_vote_spectators->value == 0 &&
            c->pers.connected != CONN_SPAWNED )
        {
            continue; // don't count spectators
        }

        total++;
        if( c->level.vote.index != level.vote.index ) {
            continue; // not voted yet
        }

        if( c->pers.flags & CPF_ADMIN ) {
            // admin vote decides immediately
            votes[c->level.vote.accepted    ] = game.maxclients;
            votes[c->level.vote.accepted ^ 1] = 0;
            break;
        }

        // count normal vote
        votes[c->level.vote.accepted]++;
    }

    if( !total ) {
        *acc = *rej = 0;
        return 0;
    }

    *rej = votes[0] * 100 / total;
    *acc = votes[1] * 100 / total;

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

    // are there any players?
    if( !G_CalcVote( &acc, &rej ) ) {
        gi.bprintf( PRINT_HIGH, "Vote aborted due to the absence of players.\n" );
        goto finish;
    }

    if( acc > treshold ) {
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
            gi.bprintf( PRINT_HIGH, "Vote passed. Next map is %s.\n", level.vote.map );
            strcpy( level.nextmap, level.vote.map );
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


static void G_BuildProposal( char *buffer ) {
    switch( level.vote.proposal ) {
    case VOTE_TIMELIMIT:
        sprintf( buffer, "change timelimit to %d", level.vote.value );
        break;
    case VOTE_FRAGLIMIT:
        sprintf( buffer, "change fraglimit to %d", level.vote.value );
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
        sprintf( buffer, "change map to %s", level.vote.map );
        break;
    default:
        strcpy( buffer, "unknown" );
        break;
    }
}

void Cmd_CastVote_f( edict_t *ent, qboolean accepted ) {
    if( (int)g_vote_spectators->value == 0 && !PLAYER_SPAWNED( ent ) ) {
        gi.cprintf( ent, PRINT_HIGH, "Spectators can't vote on this server.\n" );
        return;
    }
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

    if( (int)g_vote_announce->value ) {
        gi.bprintf( PRINT_HIGH, "%s voted %s.\n", 
            ent->client->pers.netname, accepted ? "YES" : "NO" );
    }

    if( G_CheckVote() ) {
        return;
    }
    
    if( (int)g_vote_announce->value == 0 ) {
        gi.cprintf( ent, PRINT_HIGH, "Vote cast.\n" );
    }
}


static qboolean vote_timelimit( edict_t *ent ) {
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

static qboolean vote_fraglimit( edict_t *ent ) {
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


static qboolean vote_items( edict_t *ent ) {
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
        } else if( !strcmp( s, "invu" ) || !strcmp( s, "pent" ) || !strcmp( s, "inv" ) ) {
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

static qboolean vote_victim( edict_t *ent ) {
    edict_t *other = G_SetPlayer( ent, 2 );
    if( !other ) {
        return qfalse;
    }

    if( other == ent ) {
        gi.cprintf( ent, PRINT_HIGH, "You can't %s yourself.\n", gi.argv( 1 ) );
        return qfalse;
    }

    if( other->client->pers.flags & (CPF_LOOPBACK|CPF_MVDSPEC) ) {
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

static qboolean vote_map( edict_t *ent ) {
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

    if( !strcmp( level.mapname, map->name ) ) {
        gi.cprintf( ent, PRINT_HIGH, "You are already playing '%s'.\n", map->name );
        return qfalse;
    }

    strcpy( level.vote.map, map->name );
    return qtrue;
}

typedef struct {
    const char *name;
    int bit;
    qboolean (*func)( edict_t * );
} vote_proposal_t;

static const vote_proposal_t vote_proposals[] = {
    { "timelimit",  VOTE_TIMELIMIT, vote_timelimit  },
    { "tl",         VOTE_TIMELIMIT, vote_timelimit  },
    { "fraglimit",  VOTE_FRAGLIMIT, vote_fraglimit  },
    { "fl",         VOTE_FRAGLIMIT, vote_fraglimit  },
    { "items",      VOTE_ITEMS,     vote_items      },
    { "kick",       VOTE_KICK,      vote_victim     },
    { "mute",       VOTE_MUTE,      vote_victim     },
    { "map",        VOTE_MAP,       vote_map        },
    { NULL }
};

void Cmd_Vote_f( edict_t *ent ) {
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
            gi.cprintf( ent, PRINT_HIGH, "No vote in progress. Type '%s help' for usage.\n", gi.argv( 0 ) );
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
            "Usage: %s [yes/no/help/proposal] [argument]\n"
            "Available proposals:\n", gi.argv( 0 ) );
        if( mask & VOTE_FRAGLIMIT ) {
            gi.cprintf( ent, PRINT_HIGH,
                " fraglimit/fl <frags>       Change frag limit\n" );
        }
        if( mask & VOTE_TIMELIMIT ) {
            gi.cprintf( ent, PRINT_HIGH,
                " timelimit/tl <minutes>     Change time limit\n" );
        }
        if( mask & VOTE_ITEMS ) {
            gi.cprintf( ent, PRINT_HIGH,
                " items [+|-]<quad/invu/bfg> Enable/disable items\n" );
        }
        if( mask & VOTE_KICK ) {
            gi.cprintf( ent, PRINT_HIGH,
                " kick <player_id>           Kick player from the server\n" );
        }
        if( mask & VOTE_MUTE ) {
            gi.cprintf( ent, PRINT_HIGH,
                " mute <player_id>           Disallow player to talk\n" );
        }
        if( mask & VOTE_MAP ) {
            gi.cprintf( ent, PRINT_HIGH,
                " map <name>                 Change current map\n" );
        }
        gi.cprintf( ent, PRINT_HIGH,
            "Available commands:\n"
                " yes                        Accept current vote\n"
                " no                         Deny current vote\n"
                " help                       Show this help\n" );
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
    if( !(int)g_vote_spectators->value &&
        !PLAYER_SPAWNED( ent ) &&
        !( ent->client->pers.flags & CPF_ADMIN ) )
    {
        gi.cprintf( ent, PRINT_HIGH, "Spectators can't vote on this server.\n" );
        return;
    }

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

    for( v = vote_proposals; v->name; v++ ) {
        if( !strcmp( s, v->name ) ) {
            break;
        }
    }
    if( !v->name ) {
        gi.cprintf( ent, PRINT_HIGH, "Unknown proposal '%s'. Type '%s help' for usage.\n", s, gi.argv( 0 ) );
        return;
    }

    if( !( mask & v->bit ) ) {
        gi.cprintf( ent, PRINT_HIGH, "Voting for '%s' is not allowed on this server.\n", v->name );
        return;
    }

    if( argc < 3 ) {
        if( v->bit == VOTE_MAP ) {
            map_entry_t *map;
            char buffer[MAX_STRING_CHARS];
            size_t total, len;

            total = 0;
            LIST_FOR_EACH( map_entry_t, map, &g_map_list, list ) {
                if( map->flags & MAP_NOVOTE ) {
                    continue;
                }
                len = strlen( map->name );
                if( total + len + 2 >= sizeof( buffer ) ) {
                    break;
                }
                memcpy( buffer + total, map->name, len );
                buffer[total + len    ] = ',';
                buffer[total + len + 1] = ' ';
                total += len + 2;
            }
            buffer[total] = 0;
            gi.cprintf( ent, PRINT_HIGH, "Available maplist: %s\n", buffer );
        } else {
            gi.cprintf( ent, PRINT_HIGH, "Argument required for '%s'. Type '%s help' for usage.\n", v->name, gi.argv( 0 ) );
        }
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
        gi.bprintf( PRINT_HIGH, "Proposal: %s\n"
            "Type 'yes' in the console to accept or 'no' to deny.\n", buffer );
    }
}

