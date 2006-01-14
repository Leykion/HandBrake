/* $Id: Utils.c,v 1.6 2003/11/06 15:51:36 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#include "Utils.h"
#include "Fifo.h"

struct HBList
{
    void ** items;
    int     allocItems;
    int     nbItems;
};

void HBSnooze( int time )
{
#if defined( SYS_BEOS )
    snooze( time );
#elif defined( SYS_MACOSX ) || defined( SYS_LINUX )
    usleep( time );
#elif defined( SYS_CYGWIN )
    /* TODO */
#endif
}

void HBLog( char * log, ... )
{
    char string[81];
    time_t _now;
    struct tm * now;
    va_list args;
    int ret;

    if( !getenv( "HB_DEBUG" ) )
    {
        return;
    }

    /* Show the time */
    _now = time( NULL );
    now = localtime( &_now );
    sprintf( string, "[%02d:%02d:%02d] ",
             now->tm_hour, now->tm_min, now->tm_sec );

    /* Convert the message to a string */
    va_start( args, log );
    ret = vsnprintf( string + 11, 68, log, args );
    va_end( args );

    /* Add the end of line */
    string[ret+11] = '\n';
    string[ret+12] = '\0';

    /* Print it */
    fprintf( stderr, "%s", string );
}

uint64_t HBGetDate()
{
#ifndef SYS_CYGWIN
    struct timeval tv;
    gettimeofday( &tv, NULL );
    return( (uint64_t) tv.tv_sec * 1000000 + (uint64_t) tv.tv_usec );
#else
    return 0;
#endif
}

/* Basic MPEG demuxer - only works with DVDs ! (2048 bytes packets) */
int HBPStoES( HBBuffer ** _psBuffer, HBList * esBufferList )
{
    HBBuffer * psBuffer = *_psBuffer;
    HBBuffer * esBuffer;
    int        pos = 0;

#define d (psBuffer->data)

    /* pack_header */
    if( d[pos] != 0 || d[pos+1] != 0 ||
        d[pos+2] != 0x1 || d[pos+3] != 0xBA )
    {
        HBLog( "HBPStoES: not a PS packet (%02x%02x%02x%02x)",
             d[pos] << 24, d[pos+1] << 16,
             d[pos+2] << 8, d[pos+3] );
        HBBufferClose( _psBuffer );
        return 0;
    }
    pos += 4;                    /* pack_start_code */
    pos += 9;                    /* pack_header */
    pos += 1 + ( d[pos] & 0x7 ); /* stuffing bytes */

    /* system_header */
    if( d[pos] == 0 && d[pos+1] == 0 &&
        d[pos+2] == 0x1 && d[pos+3] == 0xBB )
    {
        int header_length;

        pos           += 4; /* system_header_start_code */
        header_length  = ( d[pos] << 8 ) + d[pos+1];
        pos           += 2 + header_length;
    }

    /* PES */
    while( pos + 6 < psBuffer->size &&
           d[pos] == 0 && d[pos+1] == 0 && d[pos+2] == 0x1 )
    {
        uint32_t streamId;
        uint32_t PES_packet_length;
        uint32_t PES_packet_end;
        uint32_t PES_header_d_length;
        uint32_t PES_header_end;
        int      hasPTS;
        uint64_t PTS = 0;

        pos               += 3;               /* packet_start_code_prefix */
        streamId           = d[pos];
        pos               += 1;

        PES_packet_length  = ( d[pos] << 8 ) + d[pos+1];
        pos               += 2;               /* PES_packet_length */
        PES_packet_end     = pos + PES_packet_length;

        if( streamId != 0xE0 && streamId != 0xBD )
        {
            /* Not interesting */
            pos = PES_packet_end;
            continue;
        }

        hasPTS             = ( ( d[pos+1] >> 6 ) & 0x2 ) ? 1 : 0;
        pos               += 2;               /* Required headers */

        PES_header_d_length  = d[pos];
        pos                    += 1;
        PES_header_end          = pos + PES_header_d_length;

        if( hasPTS )
        {
            PTS = ( ( ( (uint64_t) d[pos] >> 1 ) & 0x7 ) << 30 ) +
                  ( d[pos+1] << 22 ) +
                  ( ( d[pos+2] >> 1 ) << 15 ) +
                  ( d[pos+3] << 7 ) +
                  ( d[pos+4] >> 1 );
        }

        pos = PES_header_end;

        if( streamId == 0xBD )
        {
            /* A52: don't ask */
            streamId |= ( d[pos] << 8 );
            pos += 4;
        }

        /* Sanity check */
        if( pos >= PES_packet_end )
        {
            HBLog( "HBPStoES: pos >= PES_packet_end" );
            pos = PES_packet_end;
            continue;
        }
     
        /* Here we hit we ES payload */
        esBuffer = HBBufferInit( PES_packet_end - pos );

        esBuffer->position = psBuffer->position;
        esBuffer->pass     = psBuffer->pass;
        esBuffer->streamId = streamId;
        esBuffer->pts      = PTS;
        memcpy( esBuffer->data, d + pos,
                PES_packet_end - pos );

        HBListAdd( esBufferList, esBuffer );

        pos = PES_packet_end;
    }

    HBBufferClose( _psBuffer );

    return 1;
}

#define HBLIST_DEFAULT_SIZE 20
HBList * HBListInit()
{
    HBList * l;
    if( !( l = malloc( sizeof( HBList ) ) ) )
    {
        HBLog( "HBListInit: malloc() failed, gonna crash" );
        return NULL;
    }

    if( !( l->items = malloc( HBLIST_DEFAULT_SIZE * sizeof( void* ) ) ) )
    {
        HBLog( "HBListInit: malloc() failed, gonna crash" );
        free( l );
        return NULL;
    }
    
    l->allocItems = HBLIST_DEFAULT_SIZE;
    l->nbItems    = 0;
    
    return l;
}

int HBListCountItems( HBList * l )
{
    return l->nbItems;
}

void HBListAdd( HBList * l, void * item )
{
    if( !item )
    {
        return;
    }

    if( l->nbItems == l->allocItems )
    {
        l->allocItems += HBLIST_DEFAULT_SIZE;
        l->items       = realloc( l->items,
                                  l->allocItems * sizeof( void* ) );
    }

    l->items[l->nbItems] = item;
    (l->nbItems)++;
}

void HBListRemove( HBList * l, void * item )
{
    int i;

    if( !item || !l->nbItems  )
    {
        return;
    }

    for( i = 0; i < l->nbItems; i++ )
    {
        if( l->items[i] == item )
        {
            break;
        }
    }

    if( l->items[i] != item )
    {
        HBLog( "HBListRemove: specified item is not in the list" );
        return;
    }

    for( ; i < l->nbItems - 1; i++ )
    {
        l->items[i] = l->items[i+1];
    }

    (l->nbItems)--;
}

void * HBListItemAt( HBList * l, int index )
{
    if( index < 0 || index >= l->nbItems )
    {
        return NULL;
    }

    return l->items[index];
}

void HBListClose( HBList ** _l )
{
    HBList * l = *_l;
    
    free( l->items );
    free( l );

    *_l = NULL;
}

HBTitle * HBTitleInit( char * device, int index )
{
    HBTitle * t = calloc( sizeof( HBTitle ), 1 );

    t->device    = strdup( device );
    t->index     = index;

    t->codec     = HB_CODEC_FFMPEG;
    t->bitrate   = 1024;
    
    t->audioList = HBListInit();
    
    return t;
}

void HBTitleClose( HBTitle ** _t )
{
    HBTitle * t = *_t;
    
    HBAudio * audio;
    while( ( audio = HBListItemAt( t->audioList, 0 ) ) )
    {
        HBListRemove( t->audioList, audio );
        HBAudioClose( &audio );
    }
    HBListClose( &t->audioList );
    
    if( t->file ) free( t->file );
    free( t->device );
    free( t );

    *_t = NULL;
}

HBAudio * HBAudioInit( int id, char * language )
{
    HBAudio * a;
    if( !( a = malloc( sizeof( HBAudio ) ) ) )
    {
        HBLog( "HBAudioInit: malloc() failed, gonna crash" );
        return NULL;
    }

    a->id            = id;
    a->language      = strdup( language );
    a->inSampleRate  = 0;
    a->outSampleRate = 44100;
    a->delay         = 0;
    a->ac3Fifo       = NULL;
    a->rawFifo       = NULL;
    a->mp3Fifo       = NULL;
    a->ac3Dec        = NULL;
    a->mp3Enc        = NULL;
    
    return a;
}

void HBAudioClose( HBAudio ** _a )
{
    HBAudio * a = *_a;
    
    free( a->language );
    free( a );

    *_a = NULL;
}

