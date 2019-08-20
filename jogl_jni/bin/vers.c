/*
 * version number.
 *
 * Blackguard
 * Copyright (C) 2010 Tom Portegys
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#ifdef BLACKGUARD
#ifdef ANDROID
char version[] = "@(#) vers.c	1.5	 8/10/2012	dialectek.com/blackguard";
char *release = "1.5 8/10/2012 dialectek.com/blackguard";
#elif METRO
char version[] = "@(#) vers.c	1.0	 9/1/2012	dialectek.com/blackguard";
char *release = "1.0 9/1/2012 dialectek.com/blackguard";
#else
char version[] = "@(#) vers.c	1.0	 1/16/2011	dialectek.com/blackguard";
char *release = "1.0 1/16/2011 dialectek.com/blackguard";
#endif
#else
char version[] = "@(#)vers.c	9.0	(rdk)	 7/17/84";
char *release = "9.0 RDK 7/17/84";
#endif
