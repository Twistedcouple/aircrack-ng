/*
 *  IV uniqueness detection method, as designed by Stanislaw Pusep:
 *
 *  Each IV byte is stored in corresponding "level". We have 3 levels with
 *  IV[2] as root index (level 0), IV[1] and IV[2] as level 2 and level 1
 *  indices respectively. Space required to allocate all data is at maximum
 *  2^24/8 (2 MB) and space required by filled index structures is 257 KB.
 */

#include <stdlib.h>
#include "uniqueiv.h"

/* allocate root structure */

unsigned char **uniqueiv_init( void )
{
    int i;

    /* allocate root bucket (level 0) as vector of pointers */

    unsigned char **uiv_root = (unsigned char **)
        malloc( 256 * sizeof( unsigned char * ) );

    if( uiv_root == NULL )
        return( NULL );

    /* setup initial state as empty */

    for( i = 0; i < 256; ++i )
        uiv_root[i] = NULL;

    return( uiv_root );
}

/* update records with new IV */

int uniqueiv_mark( unsigned char **uiv_root, unsigned char IV[3] )
{
    unsigned char **uiv_lvl1;
    unsigned char  *uiv_lvl2;
    short i;

    if( uiv_root == NULL )
        return( 0 );

    /* select bucket from level 1 */

    uiv_lvl1 = (unsigned char **) uiv_root[IV[2]];

    /* create if it doesn't exists */

    if( uiv_lvl1 == NULL )
    {
        /* allocate level 2 bucket being a vector of bits */

        uiv_lvl1 = (unsigned char **) malloc( 256 * sizeof( unsigned char * ) );

        if( uiv_lvl1 == NULL )
            return( 1 );

        /* setup initial state as empty */

        for( i = 0; i < 256; i++ )
            uiv_lvl1[i] = NULL;

        /* link to parent bucket */

        uiv_root[IV[2]] = (unsigned char *) uiv_lvl1;
    }

    /* select bucket from level 2 */

    uiv_lvl2 = (unsigned char *) uiv_lvl1[IV[1]];

    /* create if it doesn't exists */

    if( uiv_lvl2 == NULL )
    {
        /* allocate level 2 bucket as a vector of pointers */

        uiv_lvl2 = (unsigned char *) malloc( 32 * sizeof( unsigned char ) );

        if( uiv_lvl1 == NULL )
            return( 1 );

        /* setup initial state as empty */

        for( i = 0; i < 32; i++ )
            uiv_lvl2[i] = 0;

        /* link to parent bucket */

        uiv_lvl1[IV[1]] = uiv_lvl2;
    }

    /* place single bit into level 2 bucket */

    uiv_lvl2[BITWISE_OFFT( IV[0] )] |= BITWISE_MASK( IV[0] );

    return( 0 );
}

/* check if already seen IV */

int uniqueiv_check( unsigned char **uiv_root, unsigned char IV[3] )
{
    unsigned char **uiv_lvl1;
    unsigned char  *uiv_lvl2;

    if( uiv_root == NULL )
        return( IV_NOTHERE );

    /* select bucket from level 1 */

    uiv_lvl1 = (unsigned char **) uiv_root[IV[2]];

    /* stop here if not even allocated */

    if( uiv_lvl1 == NULL )
        return( IV_NOTHERE );

    /* select bucket from level 2 */

    uiv_lvl2 = (unsigned char *) uiv_lvl1[IV[1]];

    /* stop here if not even allocated */

    if( uiv_lvl2 == NULL )
        return( IV_NOTHERE );

    /* check single bit from level 2 bucket */

    if( ( uiv_lvl2[ BITWISE_OFFT( IV[0] ) ]
                  & BITWISE_MASK( IV[0] ) ) == 0 )
        return( IV_NOTHERE );
    else
        return( IV_PRESENT );
}

/* unallocate everything */

void uniqueiv_wipe( unsigned char **uiv_root )
{
    int i, j;
    unsigned char **uiv_lvl1;
    unsigned char  *uiv_lvl2;

    if( uiv_root == NULL )
        return;

    /* recursively wipe out allocated buckets */

    for( i = 0; i < 256; ++i )
    {
        uiv_lvl1 = (unsigned char **) uiv_root[i];

        if( uiv_lvl1 != NULL )
        {
            for( j = 0; j < 256; ++j )
            {
                uiv_lvl2 = (unsigned char *) uiv_lvl1[j];

                if( uiv_lvl2 != NULL )
                    free( uiv_lvl2 );
            }

            free( uiv_lvl1 );
        }
    }

    free( uiv_root );

    return;
}


unsigned char *data_init( void )
{
	// It could eat up to (256*256*256) * 3 bytes = 48Mb :/
	unsigned char * IVs = (unsigned char *) calloc(256*256*256 * 3, sizeof(unsigned char));
	return IVs;
}

/* Checking WEP packet:
 * The 2 first bytes of 2 different data packets having the same IV (for the same AP)
 * should be exactly the same due to the fact that unencrypted, they are always the same:
 * AA AA
 */

int data_check(unsigned char *data_root, unsigned char IV[3], unsigned char data[2])
{
	int IV_position, cloaking;

	// Init vars
	cloaking = NO_CLOAKING;

	// Make sure it is allocated
	if (data_root != NULL)
	{
		// Try to find IV
		IV_position = (((IV[0] * 256) + IV[1]) * 256) + IV[2];
		IV_position *= 3;

		// Check if existing
		if ( *(data_root + IV_position) == 0)
		{
			// Not existing
			*(data_root + IV_position) = 1;

			// Add it
			*(data_root + IV_position + 1) = data[0];
			*(data_root + IV_position + 2) = data[1];

		}
		else
		{
			// Good, we found it, so check it now
			if ( *(data_root + IV_position + 1) != data[0] ||
				*(data_root + IV_position + 2) != data[1])
			{
				cloaking = CLOAKING;
			}
		}

	}
	// else, cannot detect since it is not started

	return cloaking;
}

void data_wipe(unsigned char * data)
{
	if (data)
		free(data);
}
